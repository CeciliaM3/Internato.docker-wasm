#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

g++ ../statistics_calc_sqlite.cpp -o statistics_calc_sqlite -lsqlite3 -static 