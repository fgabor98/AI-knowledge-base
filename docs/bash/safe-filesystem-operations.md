---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Safe Filesystem Operations

## What Problem Does This Solve?

Many Bash scripts create, copy, move, or delete files. Filesystem mistakes can destroy data, follow unexpected paths, or fail on normal filenames. Safe patterns make these operations explicit and reviewable.

## Core Concepts

- Quote every path expansion.
- Use `--` before paths when a command supports it.
- Validate paths before destructive operations.
- Prefer `mktemp` for temporary files and directories.
- Prefer atomic replace patterns for generated files.
- Avoid parsing `ls`.
- Use NUL-delimited records for filenames from `find`.
- Keep destructive commands narrow and visible.
- Avoid relying on the current directory unless it is deliberate.

## Mental Model

Treat paths as untrusted data. Even paths created by your own script can contain spaces, newlines, glob characters, or leading dashes.

Prefer:

```bash
rm -- "$path"
```

over:

```bash
rm $path
```

The first passes exactly one path. The second may pass many words or glob-expanded filenames.

## Syntax / API / Mechanism

Temporary directory:

```bash
tmpdir=$(mktemp -d)
trap 'rm -rf -- "$tmpdir"' EXIT
```

Copy with path safety:

```bash
cp -- "$src" "$dst"
```

Atomic file update:

```bash
tmp=$(mktemp "${target}.tmp.XXXXXX")
generate_content > "$tmp"
mv -- "$tmp" "$target"
```

Find with NUL delimiters:

```bash
find "$root" -type f -print0 |
while IFS= read -r -d '' path; do
    printf '%s\n' "$path"
done
```

## Minimal Example

```bash
src=$1
dst=$2

[[ -f "$src" ]] || {
    printf 'source is not a file: %s\n' "$src" >&2
    exit 1
}

cp -- "$src" "$dst"
```

For a runnable demonstration, see `examples/bash/safe-filesystem-operations.sh`.

## Real-World Example

Write a generated config file atomically:

```bash
target=/etc/my-service/config.generated
tmp=$(mktemp "${target}.XXXXXX")

cleanup() {
    [[ -e "${tmp:-}" ]] && rm -f -- "$tmp"
}
trap cleanup EXIT

generate_config > "$tmp"
chmod 0644 "$tmp"
mv -- "$tmp" "$target"
tmp=
```

If generation fails, the old target remains unchanged.

## Common Mistakes

- Running `rm -rf "$dir"` without validating that `dir` is non-empty and expected.
- Forgetting `--` before paths that may begin with `-`.
- Writing temporary files with predictable names.
- Parsing `ls` output.
- Using `for path in $(find ...)`.
- Assuming `mv` across filesystems is always atomic.
- Relying on a current working directory that may change.
- Expanding globs without considering the no-match case.

## Debugging Checklist

- Test paths with spaces and leading dashes.
- Test empty variables under `set -u`.
- Print planned destructive operations before enabling them.
- Use `find ... -print0` for filename streams.
- Inspect commands with `set -x` before destructive changes.
- Add guard checks before `rm`, `mv`, or `cp`.
- Test failure during generation before `mv`.

## Related Topics

- [Quoting](quoting.md)
- [Traps And Cleanup](traps-and-cleanup.md)
- [Read Lines Safely](read-lines-safely.md)
- [Robust Scripts](robust-scripts.md)

## References

- `mktemp(1)`
- `cp(1)`, `mv(1)`, `rm(1)`, and `find(1)`
- `man bash`, sections `EXPANSION` and `REDIRECTION`
