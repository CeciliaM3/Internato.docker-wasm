#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

if [ ! "$(which tmux)" ]; then
    echo "Installa tmux e riprova."
    exit 1
fi

VENV_PATH="../venv"

./all-docker-podman-rebuild.sh

TMUX_SESSION="progetto"

tmux new-session -d -s "$TMUX_SESSION" -n main

tmux send-keys -t "$TMUX_SESSION" "./docker-compose-shutdown-restart.sh" C-m

tmux split-window -h -t "$TMUX_SESSION"
tmux send-keys -t "$TMUX_SESSION" "cd ../app; $VENV_PATH/bin/python app.py" C-m

tmux attach-session -t "$TMUX_SESSION"
