#!/usr/bin/env bash
set -Eeuo pipefail

tmpdir=

usage() {
    cat <<'USAGE'
Usage: robust-script-template.sh [--dry-run] SOURCE_DIR

List regular files from SOURCE_DIR using a structure suitable for larger scripts.
USAGE
}

log() {
    printf '%s\n' "$*" >&2
}

die() {
    log "error: $*"
    exit 1
}

need_command() {
    command -v "$1" >/dev/null 2>&1 || die "missing required command: $1"
}

cleanup() {
    if [[ -n "${tmpdir:-}" && -d "$tmpdir" ]]; then
        rm -rf -- "$tmpdir"
    fi
}
trap cleanup EXIT

main() {
    local dry_run=0

    while (($#)); do
        case "$1" in
            -n|--dry-run)
                dry_run=1
                shift
                ;;
            -h|--help)
                usage
                return 0
                ;;
            --)
                shift
                break
                ;;
            -*)
                die "unknown option: $1"
                ;;
            *)
                break
                ;;
        esac
    done

    (($# == 1)) || {
        usage >&2
        exit 2
    }

    local source_dir=$1
    [[ -d "$source_dir" ]] || die "not a directory: $source_dir"

    need_command find
    need_command mktemp

    tmpdir=$(mktemp -d)
    local manifest=$tmpdir/files.bin

    find "$source_dir" -maxdepth 1 -type f -print0 > "$manifest"

    if ((dry_run)); then
        log "would process files from: $source_dir"
    else
        log "processing files from: $source_dir"
    fi

    local path
    while IFS= read -r -d '' path; do
        printf 'file: %s\n' "$path"
    done < "$manifest"
}

main "$@"
