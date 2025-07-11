#!/bin/bash
set -e
cd "$(dirname "$0")"

g++ statistics_calc_libcurl.cpp -o statistics_calc_libcurl -lcurl 
