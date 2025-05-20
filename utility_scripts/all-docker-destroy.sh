#!/bin/bash
set -e
cd "$(dirname "$0")"/..

# eliminazione di eventuali container ancora associati
printf "\nDeleting associated containers.\n\n"
docker ps -a -q --filter ancestor=statistics_calc_executable | xargs -r docker rm -f

# eliminazione di eventuali immagini ancora associate 
printf "\nDeleting associated images.\n\n"
docker image prune -f
docker rmi statistics_calc_executable 2>/dev/null || true

printf "\nAll docker containers for this project have been eliminated. Ready to rebuild.\n\n"
