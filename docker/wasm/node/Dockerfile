FROM node:latest

WORKDIR /app
COPY src_fetch/statistics_calc_fetch.js .
COPY src_fetch/statistics_calc_fetch.wasm .
COPY src_fetch/package.json .
COPY src_fetch/package-lock.json .
COPY external/ external/

RUN npm install

ARG host_address_forenv=172.17.0.1
ENV HOST_ADDR=${host_address_forenv}

COPY src_fetch/wrapperstart.sh .

RUN chmod +x wrapperstart.sh
ENTRYPOINT ["./wrapperstart.sh", "node"]

# external/nlohmann/json.hpp: versione header-only della libreria precedentemente scaricata sull'host con:
# wget https://github.com/nlohmann/json/releases/latest/download/json.hpp -O json.hpp