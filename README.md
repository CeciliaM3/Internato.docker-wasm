# Internato.docker-wasm
Progetto di tirocinio interno LT Informatica su containerizzazione e Webassembly.

La presente architettura è stata realizzata con lo scopo di consentire un confronto prestazionale tra diverse modalità di deployment di uno stesso programma 
sfruttando in varie combinazioni Docker, Podman e Wasm.

Le potenzialità di queste tecnologie che possono essere esplorate con le configurazioni attualmente disponibili sono:
-capacità di accesso a filesystem
-conettività HTTP

La struttura è principalmente composta dai seguenti elementi:

########### Elementi per i test relativi all'accesso a filesysyem ###########
Un database SQLite come file aircompressor.db, caricato coi dati in db-api/database/aircompressor_cleaned.csv
(Si noti che poiché nelle configurazioni presenti i vari eseguibili utilizzano un path relativo per accedere al db, ma si trovano all'interno di cartelle 
diverse per via delle diverse dipendenze e risorse associate, per far funzionare i test come sono al momento impostati è necessario replicare il database 
in tutte le seguenti locazioni:
src_fs/db
src_rawsqlite/db
src_wasi/db
Si consiglia perciò di creare dapprima il database in uno di questi folder e di realizzare nei due rimanenti degli hard link a quello già esistente.)

Un programma C++ che utilizza la libreria sqlite3.c per contattare il database sqlite, effettuando una query
per prelevare una quantità configurabile di dati ed esegue poi alcuni calcoli statistici su di essi, stampando su standard output il risultato.
Il programma è statistics_calc_sqlite.cpp e può essere compilato a 3 diversi target:
- eseguibile nativo, compilato con g++, 
- modulo Webassembly da eseguire con il supporto del glue code Javascript e con accesso a filesystem garantito da NODEFS o NODERAWFS, compilato con em++
(Tale file Javascript viene generato automaticamente dal compilatore Emscripten e può essere eseguito in runtime Node.js o Bun), 
- modulo Webassembly da eseguire senza supporto di alcun glue code Javascript e con accesso a filesystem garantito da WASI, compilato con clang++ wasi-compatible (Tale modulo Wasm può essere eseguito in runtime Wasmtime o Wasmedge). 
Ognuna di queste versioni (eseguibile, wasm-js-nodefs su node, wasm-js-nodefs bun, wasm-js-noderawfs su node, wasm-js-noderawfs su bun, wasm-wasi su wasmtime e wasm-wasi su wasmedge) può poi essere lanciata direttamente sull'host, all'interno di un container Docker o all'interno di un contaianer Podman, per un totale attuale di 21 possibili configurazioni.

########### Elementi per i test relativi a connettività Http ############
Un container Docker contenente un'istanza del db server MySQL, in cui un database sempre caricato coi dati in db-api/database/aircompressor_cleaned.csv attende richieste di query.
Un container Docker contenente un'istanza di un web server realizzato con la libreria fastAPI di Python, 
che attende richieste http per eseguire sul database le queries contenute nei payloads.
(Questi due servizi sono definiti nello stesso file docker-compose.yml e i relativi containers comunicano quindi tra loro tramite la rete bridge interna)

Un programma C++ che invia al server fastAPI una query sotto forma di richiesta http
per prelevare una quantità configurabile di dati e li elabora in modo analogo a statistics_calc_sqlite.cpp.
Esistono due versioni di questo programma:
- statistics_calc_libcurl.cpp, che per effettuare le richieste http utilizza la libreria libcurl ed è realizzata per essere compilata con ++ ad eseguibile nativo,
- statistics_calc_fetch.cpp, che per effettuare le richieste http utilizza la fetch API messa a disposizione da Emscripten ed è realizzata per essere compilata con emcc a modulo Webassembly, da eseguire con il supporto del glue code Javascript. (Glue code sempre eseguibile in runtime Node.js o Bun, sfrutta xhr2 per sopperire alla mancanza di XMLHttpRequest negli ambienti non-browser e riuscire comunque ad inviare le richieste http al server fastAPI)
Ognuna di queste versioni (eseguibile, wasm-js su node e wasm-js su bun) può poi essere lanciata direttamente sull'host, 
all'interno di un container Docker o all'interno di un contaianer Podman, per un totale attuale di 9 possibili configurazioni.

########### Elementi comuni ###########
Un web server realizzato con la libreria Flask di Python che consente di lanciare una specifica versione del programma C++ tra quelle disponibili
e intercettarne l'output restituendolo al client.
Esso può essere contattato specificando uno dei seguenti url, che coprono tutte le conifigurazioni disponibili
(30 funzionanti, 2 attualmente inagibili in attesa di sviluppi futuri):
> /run_query/filesystem/noncontainerized/execsqlite/-/-/-
> /run_query/filesystem/noncontainerized/wasmwasi/-/wasmtime/-
> /run_query/filesystem/noncontainerized/wasmwasi/-/wasmedge/-
> /run_query/filesystem/noncontainerized/wasmfs/nodefs/node/-
> /run_query/filesystem/noncontainerized/wasmfs/nodefs/bun/-
> /run_query/filesystem/noncontainerized/wasmfs/noderawfs/node/-
> /run_query/filesystem/noncontainerized/wasmfs/noderawfs/bun/-
> /run_query/filesystem/containerized/execsqlite/-/-/docker
> /run_query/filesystem/containerized/execsqlite/-/-/podman
> /run_query/filesystem/containerized/wasmwasi/-/wasmtime/docker
> /run_query/filesystem/containerized/wasmwasi/-/wasmtime/podman
> /run_query/filesystem/containerized/wasmwasi/-/wasmtime/wasmworkload -> non funzionante
> /run_query/filesystem/containerized/wasmwasi/-/wasmedge/docker
> /run_query/filesystem/containerized/wasmwasi/-/wasmedge/podman
> /run_query/filesystem/containerized/wasmwasi/-/wasmedge/wasmworkload -> non funzionante
> /run_query/filesystem/containerized/wasmfs/nodefs/node/docker
> /run_query/filesystem/containerized/wasmfs/nodefs/node/podman
> /run_query/filesystem/containerized/wasmfs/nodefs/bun/docker
> /run_query/filesystem/containerized/wasmfs/nodefs/bun/podman
> /run_query/filesystem/containerized/wasmfs/noderawfs/node/docker
> /run_query/filesystem/containerized/wasmfs/noderawfs/node/podman
> /run_query/filesystem/containerized/wasmfs/noderawfs/bun/docker
> /run_query/filesystem/containerized/wasmfs/noderawfs/bun/podman
> /run_query/httpconn/noncontainerized/execlibcurl/-/-/-
> /run_query/httpconn/noncontainerized/wasmfetch/-/node/-
> /run_query/httpconn/noncontainerized/wasmfetch/-/bun/-
> /run_query/httpconn/containerized/execlibcurl/-/-/docker
> /run_query/httpconn/containerized/execlibcurl/-/-/podman
> /run_query/httpconn/containerized/wasmfetch/-/node/docker
> /run_query/httpconn/containerized/wasmfetch/-/node/podman
> /run_query/httpconn/containerized/wasmfetch/-/bun/docker
> /run_query/httpconn/containerized/wasmfetch/-/bun/podman
(è possibile indicare anche il numero di tuple da prelevare dal database, e considerare per i calcoli, tramite il query parameter "rows")

Un'interfaccia web che consente di interagire con il server Flask in modo intuitivo e mostra su browser i dettagli sui risultati.

-------------------------------------------------------------------

Per approntare l'architettura per i test occorre quindi:
Compilare statistics_calc_libcurl.cpp e statistics_calc_fetch.cpp con i seguenti comandi:
--- g++ statistics_calc_libcurl.cpp -o statistics_calc_libcurl -lcurl 
--- emcc statistics_calc_fetch.cpp -o statistics_calc_fetch.js \
    -s WASM=1 -s FETCH=1 -s EXPORTED_FUNCTIONS="['_main', '_callback_timeout']" \
    -sALLOW_MEMORY_GROWTH \

Definire la seguente funzione nella shell prima di compilare statistics_calc_fetch.cpp 
se si intende utilizzare il compilatore Emscripten in versione containerizzata invece di installarlo in locale:
function emcc() {
    docker run --rm -v "$PWD":/src -u "$(id -u)":"$(id -g)" emscripten/emsdk emcc "$@"
}

Compilare statistics_calc_sqlite.cpp a nativo, modulo wasm+js e modulo wasm+wasi con i seguenti comandi:
--- g++ ../statistics_calc_sqlite.cpp -o statistics_calc_sqlite -lsqlite3 -static 

--- em++ ../statistics_calc_sqlite.cpp sqlite3.o \
    -Isqlite-amalgamation-3470000 \
    -lnodefs.js \
    -lnoderawfs.js \
    -o statistics_calc_sqlite_noderawfs.js \
    -s ALLOW_MEMORY_GROWTH

--- /opt/wasi-sdk/bin/wasm32-wasi-clang++ \
  -fno-exceptions \
  -I./include \
  -L./lib/wasm32-wasi \
  -lsqlite3 \
  ../statistics_calc_sqlite.cpp \
  -o statistics_calc_sqlite.wasm

(Si noti che a seconda della piattaforma potrebbe essere necessario modificare il path del compilatore clang++)

Eseguire il build delle immagini con i seguenti tag, sia con Docker che con Podman: 

<statistics_calc_wasm_executable> dal dockerfile src_libcurl/Dockerfile
<statistics_calc_wasm_node> dal dockerfile src_fetch/Dockerfile-node
<statistics_calc_wasm_bun> dal dockerfile src_fetch/Dockerfile-bun
(Attenzione: se si è su wsl è necessario passare ad ognuno di questi primi comandi l'opzione --build-arg host_address_forenv="host.docker.internal")
<statistics_calc_sqlite_executable> dal dockerfile src_rawsqlite/Dockerfile
<statistics_calc_sqlite_wasmtime> dal dockerfile src_wasi/Dockerfile-wasmtime
<statistics_calc_sqlite_wasmedge> dal dockerfile src_wasi/Dockerfile-wasmedge
<statistics_calc_sqlite_node_nodefs> dal dockerfile src_fs/Dockerfile-node-nodefs
<statistics_calc_sqlite_bun_nodefs> dal dockerfile src_fs/Dockerfile-bun-nodefs
<statistics_calc_sqlite_node_noderawfs> dal dockerfile src_fs/Dockerfile-node-noderawfs
<statistics_calc_sqlite_bun_noderawfs> dal dockerfile src_fs/Dockerfile-bun-noderawfs

   > <statistics_calc_sqlite_wasmworkload> dal dockerfile src_wasi/Dockerfile-wasmworkload
   > E' l'immagine relativa alle 2 configurazioni attualmente non funzionanti. Per quanto l'esecuzione dei relativi container non possa ancora avvenire, 
   > la costruzione dell'immagine va a buon fine. L'obiettivo è di riuscire a sfruttare la preview Docker+Wasm per lanciare e gestire workload Webassembly 
   > con Docker non all'interno di container Linux, bensì all'interno di shim basati su Wasmtime o Wasmedge che vengono invocati direttamente da Containerd, senza 
   > coinvolgere Runc. NB: trattandosi di una preview esclusiva di Docker, il build di questa immagine non può avvenire anche in Podman.

Avviare i servizi di backend con "docker compose up" dalla directory db-api

Avviare il server di frontend Flask con "python3 app.py" dalla directory app

Visitare da browser l'url indicato nel terminale su cui è stato lanciato il server Flask per accedere all'interfaccia web

-------------------------------------------------------------------
E' possibile eseguire in modo automatico buona parte di questi passaggi lanciando gli scripts:
> all-docker-podman-rebuild.sh
> docker-compose-shutdown-restart.sh
presenti nella directory utility_scripts, tenendo conto del fatto che la loro efficacia potrebbe 
variare a seconda della piattaforma su cui ci si trova. 

-------------------------------------------------------------------

Se si intendono automatizzare i test prestazionali si possono sfruttare gli script bash presenti nella cartella test per condurre una
serie di misurazioni e statistiche con hyperfine. Sono disponibili programmi per misurare il tempo di esecuzione:
- dell'intera chiamata ad un endpoint del server Flask
- del lancio diretto del comando relativo ad una configurazione
a seconda delle preferenze.

Per ognuno dei due casi è presente uno script che consente di effettuare statistiche più immediate e che stampa l'output a terminale 
e uno che esegue un benchmarking completo esportando i dati su di un file csv.
I contenuti nei csv possono poi essere elaborati da graphs_maker.py per generare automaticamente alcuni grafici.