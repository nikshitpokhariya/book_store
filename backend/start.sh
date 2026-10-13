#!/bin/sh
set -eu

cd /app/backend

# Ensure upload directory exists for local fallback
mkdir -p ./public/uploads

# Execute backend binary
exec ./BookExchangeBackend
