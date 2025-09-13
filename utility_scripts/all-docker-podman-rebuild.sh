#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

##########################################################################
# compilazione dei sorgenti (versione con richieste HTTP a server fastAPI)
# da C++ a eseguibile nativo (control)
printf "Compiling statistics_calc_libcurl.cpp to executable using g++.\n\n"
../src_libcurl/build.sh

# da C++ a wasm + glue code JS (prospettiva d'uso: fetch API)
printf "Compiling statistics_calc_fetch.cpp to wasm + glue code JS using emcc.\n\n"
../src_fetch/build.sh

##########################################################################
# compilazione dei sorgenti (versione con database sqlite)
# da C++ a eseguibile nativo (control)
printf "Compiling statistics_calc_sqlite.cpp to executable using g++.\n\n"
../src_rawsqlite/build.sh

# da C++ a wasm standalone (prospettiva d'uso: WASI)
printf "Compiling statistics_calc_sqlite.cpp to standalone wasm using wasm32-wasi-clang++.\n\n"
../src_wasi/build.sh

# da C++ a wasm + glue code JS (prospettiva d'uso: NODEFS(NODERAWFS))
printf "Compiling statistics_calc_sqlite.cpp to wasm + glue code JS using emcc (NODEFS and NODERAFS versions).\n\n"
../src_fs/build.sh

##########################################################################
# build delle immagini (versione con richieste HTTP a server fastAPI)
# determinazione dell'argomento da passare in fase di build per settare (necessario per indirizzare correttamente le richieste HTTP)
# la variabile d'ambiente interna ai container HOST_ADDR
if [ -r /proc/sys/kernel/osrelease ] && grep -qi microsoft /proc/sys/kernel/osrelease; then
  echo "Detected WSL2"
  host_address_forbuild="host.docker.internal"
else
  echo "Detected real Linux"
  host_address_forbuild="172.17.0.1"
fi

# Eseguibile
pushd ../src_libcurl
printf "\nBuilding image with tag <statistics_calc_executable> (Docker).\n\n"
time docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_executable .
printf "\nBuilding image with tag <statistics_calc_executable> (Podman).\n\n"
time podman build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_executable .
popd

# Node
pushd ../src_fetch
printf "\nBuilding image with tag <statistics_calc_wasm_node> (Docker).\n\n"
time docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_node -f Dockerfile-node .
printf "\nBuilding image with tag <statistics_calc_wasm_node> (Podman).\n\n"
time podman build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_node -f Dockerfile-node .

# Bun
printf "\nBuilding image with tag <statistics_calc_wasm_bun> (Docker).\n\n"
time docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_bun -f Dockerfile-bun .
printf "\nBuilding image with tag <statistics_calc_wasm_bun> (Podman).\n\n"
time podman build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_bun -f Dockerfile-bun .
popd

##########################################################################
# build delle immagini (versione con database sqlite)
# Eseguibile (control)
pushd ../src_rawsqlite
printf "\nBuilding image with tag <statistics_calc_sqlite_executable> (Docker).\n\n"
time docker build -t statistics_calc_sqlite_executable .
printf "\nBuilding image with tag <statistics_calc_sqlite_executable> (Podman).\n\n"
time podman build -t statistics_calc_sqlite_executable .
popd

# WASI, wasmtime
pushd ../src_wasi
printf "\nBuilding image with tag <statistics_calc_sqlite_wasmtime> (Docker).\n\n"
time docker build -t statistics_calc_sqlite_wasmtime -f Dockerfile-wasmtime .
printf "\nBuilding image with tag <statistics_calc_sqlite_wasmtime> (Podman).\n\n"
time podman build -t statistics_calc_sqlite_wasmtime -f Dockerfile-wasmtime .

# WASI, wasmedge
printf "\nBuilding image with tag <statistics_calc_sqlite_wasmedge> (Docker).\n\n"
time docker build -t statistics_calc_sqlite_wasmedge -f Dockerfile-wasmedge .
printf "\nBuilding image with tag <statistics_calc_sqlite_wasmedge> (Podman).\n\n"
time podman build -t statistics_calc_sqlite_wasmedge -f Dockerfile-wasmedge .

# WASI, direct workload (preview)
printf "\nBuilding image with tag <statistics_calc_sqlite_wasmworkload> (Docker).\n\n"
time docker build --platform=wasi/wasm -t statistics_calc_sqlite_wasmworkload . -f Dockerfile-wasmworkload
popd

# NODEFS, node
pushd ../src_fs
printf "\nBuilding image with tag <statistics_calc_sqlite_node_nodefs> (Docker).\n\n"
time docker build -t statistics_calc_sqlite_node_nodefs -f Dockerfile-node-nodefs .
printf "\nBuilding image with tag <statistics_calc_sqlite_node_nodefs> (Podman).\n\n"
time podman build -t statistics_calc_sqlite_node_nodefs -f Dockerfile-node-nodefs .

# NODEFS, bun
printf "\nBuilding image with tag <statistics_calc_sqlite_bun_nodefs> (Docker).\n\n"
time docker build -t statistics_calc_sqlite_bun_nodefs -f Dockerfile-bun-nodefs .
printf "\nBuilding image with tag <statistics_calc_sqlite_bun_nodefs> (Podman).\n\n"
time podman build -t statistics_calc_sqlite_bun_nodefs -f Dockerfile-bun-nodefs .

# NODERAWFS, node
printf "\nBuilding image with tag <statistics_calc_sqlite_node_noderawfs> (Docker).\n\n"
time docker build -t statistics_calc_sqlite_node_noderawfs -f Dockerfile-node-noderawfs .
printf "\nBuilding image with tag <statistics_calc_sqlite_node_noderawfs> (Podman).\n\n"
time podman build -t statistics_calc_sqlite_node_noderawfs -f Dockerfile-node-noderawfs .

# NODERAWFS, bun
printf "\nBuilding image with tag <statistics_calc_sqlite_bun_noderawfs> (Docker).\n\n"
time docker build -t statistics_calc_sqlite_bun_noderawfs -f Dockerfile-bun-noderawfs .
printf "\nBuilding image with tag <statistics_calc_sqlite_bun_noderawfs> (Podman).\n\n"
time podman build -t statistics_calc_sqlite_bun_noderawfs -f Dockerfile-bun-noderawfs .
popd

printf "\nAll Docker and Podman images and containers have been rebuilt.\n"
