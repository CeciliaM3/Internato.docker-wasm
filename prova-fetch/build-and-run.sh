#!/bin/bash

# Build del codice con emscripten, io lo uso tramite Docker
docker run --rm -v "$PWD":/src -u "$(id -u)":"$(id -g)" emscripten/emsdk \
    emcc fetch2.cpp -o fetch2.js -s FETCH=1 -s WASM=1

# Aggiunge XMLHttpRequest al file .js mediante la libreria xhr2
# Le XMLHttpRequest vengono effettuate dal browser, non sono disponibili
# in runtime JavaScript come Node o Bun --> usiamo la libreria xhr2 per
# poterle effettuare anche da Node o Bun
echo "// Aggiunto dopo aver installato xhr2 con npm:
// npm i xhr2
// Permette di eseguire il file con node o bun --> Non serve un browser
var XMLHttpRequest = require('xhr2');

$(cat fetch2.js)" > fetch2.js

echo -e "Running...\n\n\n"
# Run:
# Node:
docker run --rm -it -v "$PWD":/usr/src/app -w /usr/src/app node fetch2.js
# Bun:
#docker run --rm -it -v "$PWD":/usr/src/app -w /usr/src/app oven/bun fetch2.js
