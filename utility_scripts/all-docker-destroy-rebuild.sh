source "$(dirname "$0")/all-docker-destroy.sh"

# ricompilazione del file da C++ ad eseguibile
printf "Compiling statistics_calc_libcurl.cpp to executable using g++.\n\n"
g++ -g -o statistics_calc_libcurl statistics_calc_libcurl.cpp -lcurl

# determinazione dell'argometno da passare in fase di build per settare la variabile d'ambiente interna ai container HOST_ADDR
if [ -r /proc/sys/kernel/osrelease ] && grep -qi microsoft /proc/sys/kernel/osrelease; then
  echo "Detected WSL2"
  host_address_forbuild="host.docker.internal"
else
  echo "Detected real Linux"
  host_address_forbuild="172.17.0.1"
fi

# build delle immagini 
printf "\nBuilding image with tag <statistics_calc_executable>.\n\n"
docker build --build-arg host_address_forenv=$host_address_forbuild -t statistics_calc_executable -f docker/executable/Dockerfile .
