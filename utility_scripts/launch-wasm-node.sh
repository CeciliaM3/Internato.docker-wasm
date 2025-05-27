#!/bin/bash
set -e
cd "$(dirname "$0")"/..

clear

# Compilazione con emscripten
emcc statistics_calc_fetch.cpp -o statistics_calc_fetch.js -s FETCH=1 -s WASM=1 -s EXIT_RUNTIME=1

# Aggiunge XMLHttpRequest al file .js mediante la libreria xhr2
# Le XMLHttpRequest vengono effettuate dal browser, non sono disponibili
# in runtime JavaScript come Node o Bun --> usiamo la libreria xhr2 per
# poterle effettuare anche da Node o Bun
echo "// Aggiunto dopo aver installato xhr2 con npm:
// npm i xhr2
// Permette di eseguire il file con node o bun --> Non serve un browser
var XMLHttpRequest = require('xhr2');

$(cat statistics_calc_fetch.js)" > statistics_calc_fetch.js

# esecuzione nativa su wsl2 con Node.js
node statistics_calc_fetch.js 10
