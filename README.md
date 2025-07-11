# Internato.docker-wasm
Progetto di tirocinio interno LT Informatica su containerizzazione e Webassembly.

La presente architettura è stata realizzata con lo scopo di consentire un confronto prestazionale tra diverse modalità di deployment di uno stesso programma 
sfruttando in varie combinazioni Docker, Podman e Wasm.

La struttura è principalmente composta dai seguenti elementi:
# un container Docker contenente un'istanza del db server MySQL, in cui un database caricato coi dati in mysql/aircompressor_cleaned.csv attende richieste di query.
# un container Docker contenente un'istanza di un web server realizzato con la libreria fastAPI di Python, 
che attende richieste http per eseguire sul database le queries contenute nei payloads.
(Questi due servizi sono definiti nello stesso file docker-compose.yml e i relativi containers comunicano quindi tra loro tramite la rete bridge interna)

# un programma C++ che invia al server fastAPI una serie predefinita di queries sotto forma di richieste http  
per prelevare una quantità configurabile di dati ed esegue poi alcuni calcoli statistici sui risultati, stampando su standard output il risultato.
Esistono due versioni di questo programma:
-statistics_calc_libcurl.cpp, che per effettuare le richieste http utilizza la libreria libcurl ed è realizzata per essere compilata con g++ ad eseguibile nativo,
-statistics_calc_fetch.cpp, che per effettuare le richieste http utilizza la fetch API messa a disposizione da emscripten ed è realizzata per essere compilata con emcc
a modulo Webassembly, da eseguire con il supporto del glue code Javascript generato automaticamente dal compilatore.
(Tale file Javascript può essere eseguito in runtime node.js o bun e sfrutta xhr2 per sopperire alla mancanza di XMLHttpRequest negli ambienti non-browser e riuscire
comunque ad inviare le richieste http al server fastAPI)
Ognuna di queste versioni (eseguibile, wasm-js su node e wasm-js su bun) può poi essere lanciata direttamente sull'host, 
all'interno di un container Docker o all'interno di un contaianer Podman, per un totale attuale di 9 possibili configurazioni.

# un web server realizzato con la libreria Flask di Python che consente di lanciare una specifica versione del programma C++ tra quelle disponibili
e intercettarne l'output restituendolo al client.
Esso può essere contattato specificando uno dei seguenti url:
- /run_query/containerized/docker/wasm/node
- /run_query/containerized/docker/wasm/bun
- /run_query/containerized/docker/executable/-
- /run_query/containerized/podman/wasm/node
- /run_query/containerized/podman/wasm/bun
- /run_query/containerized/podman/executable/-
- /run_query/noncontainerized/-/wasm/node
- /run_query/noncontainerized/-/wasm/bun
- /run_query/noncontainerized/-/executable/-
(è possibile indicare anche il numero di tuple da prelevare dal database, e considerare per i calcoli, tramite il query parameter "rows")

# un'interfaccia web che consente di interagire con il server Flask in modo intuitivo e mostra su browser i dettagli sui risultati.


###################################################################


Per approntare l'architettura per i test occorre quindi:
# compilare statistics_calc_libcurl.cpp e statistics_calc_fetch.cpp con i seguenti comandi:
--- g++ statistics_calc_libcurl.cpp -o statistics_calc_libcurl -lcurl 
--- emcc statistics_calc_fetch.cpp -o statistics_calc_fetch.js \
    -s WASM=1 -s FETCH=1 -s EXPORTED_FUNCTIONS="['_main', '_callback_timeout']" \
    -sALLOW_MEMORY_GROWTH \

Definire la seguente funzione nella shell prima di compilare statistics_calc_fetch.cpp 
se si intende utilizzare il compilatore emscripten in versione containerizzata invece di installarlo in locale:
function emcc() {
    docker run --rm -v "$PWD":/src -u "$(id -u)":"$(id -g)" emscripten/emsdk emcc "$@"
}

# eseguire il build delle immagini con i seguenti tag, sia con Docker che con Podman: 
<statistics_calc_wasm_executable> dal dockerfile src_libcurl/Dockerfile
<statistics_calc_wasm_node> dal dockerfile src_fetch/Dockerfile-node
<statistics_calc_wasm_bun> dal dockerfile src_fetch/Dockerfile-bun
(Attenzione: se si è su wsl è necessario passare ad ogni comando l'opzione --build-arg host_address_forenv="host.docker.internal")

# avviare i servizi di backend con "docker compose up" dalla directory db-api

# avviare il server di frontend Flask con "python3 app.py" dalla directory app

# visitare da browser l'url indicato nel terminale su cui è stato lanciato il server Flask per accedere all'interfaccia web

-------------------------------------------------------------------
E' possibile eseguire in modo automatico buona parte di questi passaggi lanciando gli scripts:
> all-docker-podman-destroy-rebuild.sh
> docker-compose-shutdown-restart.sh
presenti nella directory utility_scripts, tenendo conto del fatto che la loro efficacia potrebbe 
variare a seconda della piattaforma su cui ci si trova. 




