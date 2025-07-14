#!/bin/bash

set -euo pipefail

source "$(dirname "$0")/all-docker-podman-destroy.sh"

# ricompilazione del file da C++ ad eseguibile
printf "Compiling statistics_calc_libcurl.cpp to executable using g++.\n\n"
./src_libcurl/build.sh

# ricompilazione del file da C++ a wasm + glue code JS
printf "Compiling statistics_calc_fetch.cpp to wasm + glue code JS using emcc.\n\n"
./src_fetch/build.sh

# determinazione dell'argomento da passare in fase di build per settare
# la variabile d'ambiente interna ai container HOST_ADDR
if [ -r /proc/sys/kernel/osrelease ] && grep -qi microsoft /proc/sys/kernel/osrelease; then
  echo "Detected WSL2"
  host_address_forbuild="host.docker.internal"
else
  echo "Detected real Linux"
  host_address_forbuild="172.17.0.1"
fi

# build delle immagini 
if [ "$SELECTED_EXECUTABLE" == "true" ];
then
  pushd ./src_libcurl
  printf "\nBuilding image with tag <statistics_calc_executable> (Docker).\n\n"
  docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_executable .
  printf "\nBuilding image with tag <statistics_calc_executable> (Podman).\n\n"
  podman build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_executable .
  popd
fi

if [ "$SELECTED_WASM_NODE" == "true" ];
then
  pushd ./src_fetch
  printf "\nBuilding image with tag <statistics_calc_wasm_node> (Docker).\n\n"
  docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_node -f Dockerfile-node .
  printf "\nBuilding image with tag <statistics_calc_wasm_node> (Podman).\n\n"
  podman build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_node -f Dockerfile-node .
  popd
fi

if [ "$SELECTED_WASM_BUN" == "true" ];
then
  pushd ./src_fetch
  printf "\nBuilding image with tag <statistics_calc_wasm_bun> (Docker).\n\n"
  docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_bun -f Dockerfile-bun .
  printf "\nBuilding image with tag <statistics_calc_wasm_bun> (Podman).\n\n"
  podman build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_bun -f Dockerfile-bun .
  popd
fi 

printf "\nAll requested Docker and Podman images and containers have been rebuilt.\n"
