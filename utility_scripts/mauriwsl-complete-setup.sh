#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

gnome-terminal -- bash -c "./docker-compose-shutdown-restart.sh; exec bash" 
./all-docker-podman-destroy-rebuild.sh
cd ../app
printf "\n############################################################\n\nNow starting Flask server.\n\n"
python3 app.py