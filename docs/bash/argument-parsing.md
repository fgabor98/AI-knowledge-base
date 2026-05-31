---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Argument Parsing

## What Problem Does This Solve?

Argument parsing turns command-line input into validated script configuration. Good parsing makes scripts predictable, gives useful errors, and separates options from positional arguments safely.

## Core Concepts

- Always provide a usage message.
- Validate argument counts.
- Use `--` to stop option parsing.
- Use `getopts` for portable short options.
- Bash has no built-in long-option parser.
- Manual parsing with `case` is common for long options.
- Shift only after consuming the right number of arguments.
- Keep parsed values in clearly named variables.
- Treat positional arguments as data and quote them.

## Mental Model

Parsing should transform raw argv into a small set of validated variables. After parsing, the rest of the script should not care about option spelling.

## Syntax / API / Mechanism

Short options with `getopts`:

```bash
verbose=0
output=

while getopts ':vo:' opt; do
    case "$opt" in
        v) verbose=1 ;;
        o) output=$OPTARG ;;
        :) printf 'option -%s requires an argument\n' "$OPTARG" >&2; exit 2 ;;
        \?) printf 'unknown option: -%s\n' "$OPTARG" >&2; exit 2 ;;
    esac
done
shift "$((OPTIND - 1))"
```

Manual long options:

```bash
while (($#)); do
    case "$1" in
        --verbose) verbose=1; shift ;;
        --output) output=$2; shift 2 ;;
        --) shift; break ;;
        -*) printf 'unknown option: %s\n' "$1" >&2; exit 2 ;;
        *) break ;;
    esac
done
```

## Minimal Example

```bash
usage() {
    printf 'Usage: %s [-v] [-o FILE] INPUT\n' "${0##*/}"
}
```

For runnable demonstrations, see:

- `examples/bash/getopts-demo.sh`
- `examples/bash/manual-args-demo.sh`

## Real-World Example

Parse options, then validate positional arguments:

```bash
verbose=0
output=-

while getopts ':vo:' opt; do
    case "$opt" in
        v) verbose=1 ;;
        o) output=$OPTARG ;;
        :) usage >&2; exit 2 ;;
        \?) usage >&2; exit 2 ;;
    esac
done
shift "$((OPTIND - 1))"

(($# == 1)) || {
    usage >&2
    exit 2
}

input=$1
```

## Common Mistakes

- Forgetting `shift "$((OPTIND - 1))"` after `getopts`.
- Forgetting to handle `--`.
- Shifting twice for an option that takes one value.
- Reading `$2` without first checking that it exists.
- Treating long options as supported by `getopts`.
- Leaving parsed values unvalidated.
- Printing usage to stdout on error instead of stderr.

## Debugging Checklist

- Test no arguments.
- Test `--help`.
- Test unknown options.
- Test missing option arguments.
- Test `--` before a filename beginning with `-`.
- Test paths containing spaces.
- Print parsed variables before doing destructive work.

## Related Topics

- [Conditionals](conditionals.md)
- [Functions](functions.md)
- [Robust Scripts](robust-scripts.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)

## References

- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
- [POSIX.1-2024: Utility Conventions](https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/basedefs/V1_chap12.html)
- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
