FROM ubuntu:latest

RUN apt-get update && \
    apt-get install -y libcurl4-openssl-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY statistics_calc_libcurl .

ARG host_address_forenv=172.17.0.1
ENV HOST_ADDR=${host_address_forenv}

ENTRYPOINT ["./statistics_calc_libcurl"]

# external/nlohmann/json.hpp: versione header-only della libreria precedentemente scaricata sull'host con:
# wget https://github.com/nlohmann/json/releases/latest/download/json.hpp -O json.hpp
