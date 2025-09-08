#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

source ./source-structs.sh

output_file="commands_benchmark_results.csv"
echo "configuration,tuples,mean,stddev,min,max" > "$output_file"

for conf in "${!configurations[@]}"
do
    workdir="${configurations[$conf]}"
    for tuples in "${tuple_quantities[@]}"
    do
        # Vista l'estrema lentezza osservata per Wasmegde, si evita di testare configurazioni che lo usano per quantità eccessive di tuple
        if [[ "$conf" == *"wasmedge"* && "$tuples" -gt 70000 ]]
        then
            continue   
        fi

        echo ">>> Running <$conf> with $tuples tuples..."
        
        json_tmpfile=$(mktemp)
        (cd "$workdir" && hyperfine --ignore-failure --warmup 2 --runs 10 "$conf $tuples" --export-json "$json_tmpfile" > /dev/null )

        mean=$(jq '.results[0].mean' "$json_tmpfile")
        stddev=$(jq '.results[0].stddev' "$json_tmpfile")
        min=$(jq '.results[0].min' "$json_tmpfile")
        max=$(jq '.results[0].max' "$json_tmpfile")

        echo "$conf,$tuples,$mean,$stddev,$min,$max" >> "$output_file"

        rm "$json_tmpfile"
    done
    echo
done

echo "All tests have been run. Results are shown in test_results.csv"