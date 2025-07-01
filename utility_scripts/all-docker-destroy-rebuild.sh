source "$(dirname "$0")/all-docker-destroy.sh"

# ricompilazione del file da C++ ad eseguibile
printf "Compiling statistics_calc_libcurl.cpp to executable using g++.\n\n"
g++ -I. -g -o src_libcurl/statistics_calc_libcurl src_libcurl/statistics_calc_libcurl.cpp -lcurl

# ricompilazione del file da C++ a wasm + glue code JS
printf "Compiling statistics_calc_fetch.cpp to wasm + glue code JS using emcc.\n\n"
./utility_scripts/compile-wasm.sh

# determinazione dell'argometno da passare in fase di build per settare la variabile d'ambiente interna ai container HOST_ADDR
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
  printf "\nBuilding image with tag <statistics_calc_executable>.\n\n"
  docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_executable -f docker/executable/Dockerfile .
fi

if [ "$SELECTED_WASM_NODE" == "true" ];
then
  printf "\nBuilding image with tag <statistics_calc_wasm_node>.\n\n"
  docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_node -f docker/wasm/node/Dockerfile .
fi

if [ "$SELECTED_WASM_BUN" == "true" ];
then
  printf "\nBuilding image with tag <statistics_calc_wasm_bun>.\n\n"
  docker build --build-arg host_address_forenv="$host_address_forbuild" -t statistics_calc_wasm_bun -f docker/wasm/bun/Dockerfile .
fi 

printf "\nAll requested docker images and containers have been rebuilt.\n"