#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")/../db-api"

docker compose down --volumes

docker compose up --build --force-recreate
