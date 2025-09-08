#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

if [ $# -ne 1 ]
then
    echo "Required parameter missing. Usage: $0 <tuples>"
    exit 1
fi

source ./source-structs.sh

echo ">>> Running al possible configurations with $1 tuples (through Flask server)..."
echo
for endpoint in "${endpoints[@]}"
do
    full_command="curl http://127.0.0.1:5000$endpoint?rows=$1"
    echo $full_command
    hyperfine --ignore-failure --warmup 2 --runs 10 "$full_command"
done