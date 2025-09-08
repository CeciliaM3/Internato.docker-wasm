#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

/opt/wasi-sdk/bin/wasm32-wasi-clang++ \
  -fno-exceptions \
  -I./include \
  -L./lib/wasm32-wasi \
  -lsqlite3 \
  ../statistics_calc_sqlite.cpp \
  -o statistics_calc_sqlite.wasm
