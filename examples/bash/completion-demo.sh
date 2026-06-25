#!/usr/bin/env bash

_mytool_complete() {
    local cur
    cur=${COMP_WORDS[COMP_CWORD]:-}
    COMPREPLY=()

    case "${COMP_WORDS[1]-}" in
        start|stop|status)
            return 0
            ;;
    esac

    mapfile -t COMPREPLY < <(compgen -W 'start stop status --help --verbose' -- "$cur")
}

if [[ ${BASH_SOURCE[0]} != "$0" ]]; then
    complete -F _mytool_complete mytool
else
    printf 'Source this file from an interactive Bash shell to register completion.\n'
fi
