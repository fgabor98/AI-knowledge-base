#!/usr/bin/env bash
set -euo pipefail

name=${1:-world}
printf 'hello, %s\n' "$name"

required=${REQUIRED_VALUE:?Set REQUIRED_VALUE before running the rest of this demo}
printf 'required value: %s\n' "$required"

path="/tmp/archive.tar.gz"
printf 'path=<%s>\n' "$path"
printf 'basename=<%s>\n' "${path##*/}"
printf 'without shortest suffix=<%s>\n' "${path%.*}"
printf 'without longest suffix=<%s>\n' "${path%%.*}"

items=("alpha" "two words" "")
printf 'array elements:\n'
printf '<%s>\n' "${items[@]}"

count=0
((count += 1))
printf 'count=%d\n' "$count"
