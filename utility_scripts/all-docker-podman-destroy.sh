#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"/..

# determinazione di quali container/immagini devono effettivamente essere eliminati
if [ $# -eq 0 ]; 
then
  SELECTED_EXECUTABLE=true
  SELECTED_WASM_NODE=true
  SELECTED_WASM_BUN=true
else
  SELECTED_EXECUTABLE=false
  SELECTED_WASM_NODE=false
  SELECTED_WASM_BUN=false

  for arg in "$@"; 
  do
    case "$arg" in
      executable)
        SELECTED_EXECUTABLE=true
        ;;
      wasm_node)
        SELECTED_WASM_NODE=true
        ;;
      wasm_bun)
        SELECTED_WASM_BUN=true
        ;;
      *)
        echo "Unrecognized argument: $arg"
        ;;
    esac
  done
fi

# eliminazione di eventuali container ancora associati alle immagini Docker o Podman di progetto
printf "\nDeleting associated containers.\n\n"
$SELECTED_EXECUTABLE && docker ps -a -q --filter ancestor=statistics_calc_executable | xargs -r docker rm -f && \
                        podman ps -a -q --filter ancestor=statistics_calc_executable | xargs -r podman rm -f
$SELECTED_WASM_NODE && docker ps -a -q --filter ancestor=statistics_calc_wasm_node | xargs -r docker rm -f && \
                       podman ps -a -q --filter ancestor=statistics_calc_wasm_node | xargs -r podman rm -f
$SELECTED_WASM_BUN && docker ps -a -q --filter ancestor=statistics_calc_wasm_bun | xargs -r docker rm -f && \
                      podman ps -a -q --filter ancestor=statistics_calc_wasm_bun | xargs -r podman rm -f

# eliminazione di eventuali immagini di progetto ancora presenti
printf "\nDeleting associated images.\n\n"
if [ $# -eq 0 ]; 
then
  docker image prune -f
  podman image prune -f
fi
$SELECTED_EXECUTABLE && (docker rmi statistics_calc_executable 2>/dev/null || true) && \
                        (podman rmi statistics_calc_executable 2>/dev/null || true) 
$SELECTED_WASM_NODE && (docker rmi statistics_calc_wasm_node 2>/dev/null || true) && \
                       (podman rmi statistics_calc_wasm_node 2>/dev/null || true)
$SELECTED_WASM_BUN && (docker rmi statistics_calc_wasm_bun 2>/dev/null || true) && \
                      (podman rmi statistics_calc_wasm_bun 2>/dev/null || true)

printf "\nAll requested docker and podman images and containers for this project have been eliminated. Ready for rebuild.\n\n"
