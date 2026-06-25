#!/usr/bin/env bash
set -euo pipefail

path=/opt/app/archive.tar.gz
file=${path##*/}
dir=${path%/*}

printf 'path=%s\n' "$path"
printf 'dir=%s\n' "$dir"
printf 'file=%s\n' "$file"
printf 'stem shortest=%s\n' "${file%.*}"
printf 'stem longest=%s\n' "${file%%.*}"

option=--enable-feature-x
name=${option#--}
name=${name//-/_}
printf 'config key=CONFIG_%s\n' "${name^^}"

target_var=HOME
printf 'indirect %s=%s\n' "$target_var" "${!target_var}"

set_default() {
    local -n ref=$1
    ref=${ref:-default}
}

value=
set_default value
printf 'nameref result=%s\n' "$value"
