FROM node:latest

RUN apt-get update && \
    apt-get install -y curl &&\
    curl -fsSL https://bun.sh/install | bash &&\
    rm -rf /var/lib/apt/lists/*

ENV PATH="/root/.bun/bin:$PATH"

WORKDIR /app
COPY src_fetch/statistics_calc_fetch.js .
COPY src_fetch/statistics_calc_fetch.wasm .
COPY src_fetch/package.json .
COPY src_fetch/package-lock.json .
COPY external/ external/

RUN bun install

ARG host_address_forenv=172.17.0.1
ENV HOST_ADDR=${host_address_forenv}

COPY src_fetch/wrapperstart.sh .

RUN chmod +x wrapperstart.sh
ENTRYPOINT ["./wrapperstart.sh", "bun"]

# external/nlohmann/json.hpp: versione header-only della libreria precedentemente scaricata sull'host con:
# wget https://github.com/nlohmann/json/releases/latest/download/json.hpp -O json.hpp