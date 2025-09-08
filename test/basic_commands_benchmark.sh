#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

if [ $# -ne 1 ]
then
    echo "Required parameter missing. Usage: $0 <tuples>"
    exit 1
fi

source ./source-structs.sh

echo ">>> Running al possible configurations with $1 tuples (through direct commands launch)..."
echo
for conf in "${!configurations[@]}"
do
    workdir="${configurations[$conf]}"
    (cd "$workdir" && hyperfine --ignore-failure --warmup 2 --runs 10 "$conf $1")
done