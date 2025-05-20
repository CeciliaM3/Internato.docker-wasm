#!/bin/bash
set -e
cd "$(dirname "$0")/../db-api"

docker compose down -v

# eliminazione di eventuali container ancora associati (anche se dovrebbe essere fatto automaticamente da docker compose down)
printf "\nDeleting associated containers.\n\n"
docker compose ps -q | xargs -r docker rm -f

# eliminazione di eventuali immagini ancora associate (tranne mysql poichè esterna e quindi invariata nel progetto)
printf "\nDeleting associated images.\n\n"
docker image prune -f
docker rmi db-api-fastapi 2>/dev/null || true

# rebuild da zero delle risorse necessarie 
printf "\nBuilding resources from scratch.\n\n"
docker builder prune --all -f
docker compose build --no-cache

printf "\nDocker compose completely reset for db-api. Ready to restart.\n\n"

