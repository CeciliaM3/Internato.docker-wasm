#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

emcc -c sqlite-amalgamation-3470000/sqlite3.c -o sqlite3.o

#############################################################################################
# versione con NODEFS
em++ ../statistics_calc_sqlite.cpp sqlite3.o \
    -Isqlite-amalgamation-3470000 \
    -sEXPORTED_RUNTIME_METHODS='["FS"]' \
    -lnodefs.js \
    -o statistics_calc_sqlite_nodefs.js \
    -s ALLOW_MEMORY_GROWTH

# aggiunta al glue code generato delle istruzioni per la misurazione del tempo
echo "// Inizio della misurazione del tempo di esecuzione
let start_time_js = performance.now();

// Callback per determinare il termine della misurazione del tempo di esecuzione
process.on('exit', () => {
    let end_time_js = performance.now();
    let execution_diration_js = (end_time_js - start_time_js).toFixed(3);
    console.log('Execution time for the full Javascript glue code, including the wasm module: ', execution_diration_js, ' milliseconds');
});

$(cat statistics_calc_sqlite_nodefs.js)" > statistics_calc_sqlite_nodefs.js

# aggiunta al glue code generato dell'istruzione per il caricamento del filesystem NODEFS
sed -i "/var Module = typeof Module != 'undefined' ? Module : {};/a\\
\n\
// Caricamento del filesystem NODEFS\n\
Module.onRuntimeInitialized = () => {\n\
    Module.FS.mkdir('/nodefs');\n\
    Module.FS.mount(Module.FS.filesystems.NODEFS, { root: '.' }, '/nodefs');\n\
    Module.FS.chdir('/nodefs');\n\
};" statistics_calc_sqlite_nodefs.js

#############################################################################################
# versione con NODERAWFS
em++ ../statistics_calc_sqlite.cpp sqlite3.o \
    -Isqlite-amalgamation-3470000 \
    -lnodefs.js \
    -lnoderawfs.js \
    -o statistics_calc_sqlite_noderawfs.js \
    -s ALLOW_MEMORY_GROWTH

# aggiunta al glue code generato delle istruzioni per la misurazione del tempo 
echo "// Inizio della misurazione del tempo di esecuzione
let start_time_js = performance.now();

// Callback per determinare il termine della misurazione del tempo di esecuzione
process.on('exit', () => {
    let end_time_js = performance.now();
    let execution_diration_js = (end_time_js - start_time_js).toFixed(3);
    console.log('Execution time for the full Javascript glue code, including the wasm module: ', execution_diration_js, ' milliseconds');
});

$(cat statistics_calc_sqlite_noderawfs.js)" > statistics_calc_sqlite_noderawfs.js
