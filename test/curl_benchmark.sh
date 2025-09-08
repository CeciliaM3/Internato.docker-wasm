#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

source ./source-structs.sh

output_file="curl_benchmark_results.csv"
echo "configuration,tuples,mean,stddev,min,max" > "$output_file"

for endpoint in "${endpoints[@]}"
do
    for tuples in "${tuple_quantities[@]}"
    do
        # Vista l'estrema lentezza osservata per Wasmegde, si evita di testare configurazioni che lo usano per quantità eccessive di tuple
        if [[ "$endpoint" == *"wasmedge"* && "$tuples" -gt 70000 ]]
        then
            continue   
        fi

        echo ">>> Hitting <$endpoint> with $tuples tuples..."
        
        json_tmpfile=$(mktemp)
        hyperfine --ignore-failure --warmup 2 --runs 10 "curl http://127.0.0.1:5000$endpoint?rows=$tuples" --export-json "$json_tmpfile" > /dev/null 

        mean=$(jq '.results[0].mean' "$json_tmpfile")
        stddev=$(jq '.results[0].stddev' "$json_tmpfile")
        min=$(jq '.results[0].min' "$json_tmpfile")
        max=$(jq '.results[0].max' "$json_tmpfile")

        echo "$endpoint,$tuples,$mean,$stddev,$min,$max" >> "$output_file"

        rm "$json_tmpfile"
    done
    echo
done

echo "All tests have been run. Results are shown in test_results.csv"