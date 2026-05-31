#!/usr/bin/env bash
set -euo pipefail

message="shell variable"
export exported_message="environment variable"

printf 'Command lookup examples:\n'
type printf
type cd

printf '\nChild process visibility:\n'
printf 'parent message=<%s>\n' "$message"
bash -c 'printf "message=<%s>\n" "${message-}"'
bash -c 'printf "exported_message=<%s>\n" "$exported_message"'

printf '\nCurrent shell IDs:\n'
printf 'parent: $$=%s BASHPID=%s\n' "$$" "$BASHPID"
(
    printf 'subshell: $$=%s BASHPID=%s\n' "$$" "$BASHPID"
)
