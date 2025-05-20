source "$(dirname "$0")/all-docker-destroy.sh"

# ricompilazione del file da C++ ad eseguibile
printf "Compiling statistics_calc_libcurl.cpp to executable using g++.\n\n"
g++ -g -o statistics_calc_libcurl statistics_calc_libcurl.cpp -lcurl

# build delle immagini 
printf "\nBuilding image with tag <statistics_calc_executable>.\n\n"
docker build -t statistics_calc_executable -f docker/executable/Dockerfile .
