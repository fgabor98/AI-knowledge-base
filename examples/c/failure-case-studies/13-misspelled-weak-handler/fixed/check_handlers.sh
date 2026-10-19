#!/usr/bin/env bash
# Fail if any interrupt handler in an image still resolves to default_handler.
# usage: check_handlers.sh IMAGE HANDLER...
set -euo pipefail

image=$1
shift
default_address=$(nm "$image" | awk '$3 == "default_handler" { print $1 }')
status=0
for handler in "$@"; do
    address=$(nm "$image" | awk -v name="$handler" '$3 == name { print $1 }')
    if [[ -z $address || $address == "$default_address" ]]; then
        printf 'error: %s is not implemented (resolves to default_handler)\n' "$handler"
        status=1
    fi
done
exit "$status"
