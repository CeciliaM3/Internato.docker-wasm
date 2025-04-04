#!/bin/bash

set -e

docker build -t sample_dynamic_copy -f docker/dynamic/copy/Dockerfile .
docker build -t sample_dynamic_mount -f docker/dynamic/mount/Dockerfile .
docker build -t sample_static_copy -f docker/static/copy/Dockerfile .
docker build -t sample_static_mount -f docker/static/mount/Dockerfile .
