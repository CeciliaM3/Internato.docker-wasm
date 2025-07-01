#!/bin/bash

runtime_name="$1"
shift 

exec $runtime_name statistics_calc_fetch.js "$HOST_ADDR" "$@"