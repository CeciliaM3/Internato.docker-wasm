#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

# Funzione da attivare se si vuole usare emcc da container
if [ ! "$(which emcc)" ]; then
    echo "Comando emcc non trovato, lo eseguo con Docker"
    function emcc() {
        docker run --rm -v "$PWD":/src -u "$(id -u)":"$(id -g)" emscripten/emsdk emcc "$@"
    }
fi

# Compilazione con emscripten
emcc statistics_calc_fetch.cpp -o statistics_calc_fetch.js \
    -s WASM=1 \
    -s FETCH=1 \
    -s EXPORTED_FUNCTIONS="['_main', '_callback_timeout']" \
    -s ALLOW_MEMORY_GROWTH

# Aggiunge XMLHttpRequest al file .js mediante la libreria xhr2
# Le XMLHttpRequest vengono effettuate dal browser, non sono disponibili
# in runtime JavaScript come Node o Bun --> usiamo la libreria xhr2 per
# poterle effettuare anche da Node o Bun
#
# Aggiunge le istruzioni necessarie a misurare il tempo di esecuzione
# dell'intero processo javascript.

echo "// Inizio della misurazione del tempo di esecuzione
let start_time_js = performance.now();

// Callback per determinare il termine della misurazione del tempo di esecuzione
process.on('exit', () => {
    let end_time_js = performance.now();
    let execution_diration_js = (end_time_js - start_time_js).toFixed(3);
    console.log('Execution time for the full Javascript glue code, including the wasm module: ', execution_diration_js, ' milliseconds');
});

// Aggiunto dopo aver installato xhr2 con npm:
// npm i xhr2
// Permette di eseguire il file con node o bun --> Non serve un browser
var XMLHttpRequest = require('xhr2');

$(cat statistics_calc_fetch.js)" > statistics_calc_fetch.js


