---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Standard Unix Tools

## What Problem Does This Solve?

Bash is most useful when it orchestrates existing Unix tools. Knowing which tool should handle searching, filtering, field extraction, sorting, counting, formatting, and filesystem traversal keeps shell scripts smaller and less error-prone.

## Core Concepts

- Bash should coordinate tools, not reimplement them.
- Use `find` for filesystem traversal.
- Use `grep` for line filtering.
- Use `awk` for field-oriented text processing and simple reports.
- Use `sed` for stream editing, especially simple substitutions.
- Use `sort` and `uniq` for ordering and counting repeated lines.
- Use `cut` for simple delimiter-based field extraction.
- Use `wc` for counting bytes, words, and lines.
- Use `tee` to split output to a file and stdout.
- Use NUL-delimited modes when filenames are data.
- GNU, BSD/macOS, and BusyBox versions differ.

## Mental Model

A good Bash script often looks like a small pipeline of specialized programs:

```bash
find "$root" -type f -name '*.log' -print0 |
while IFS= read -r -d '' path; do
    grep -H -- "$pattern" "$path"
done
```

Bash owns control flow, quoting, arguments, redirection, and exit status. External tools own the domain work.

## Syntax / API / Mechanism

Common tool choices:

| Need | Tool | Notes |
| --- | --- | --- |
| Find files by name, type, age, size | `find` | Prefer `-print0` for filenames. |
| Filter lines by pattern | `grep` | Use `grep -E` for extended regex. |
| Extract simple fields | `cut` | Good for simple delimiter formats. |
| Process records and fields | `awk` | Better than Bash loops for many text reports. |
| Substitute text in streams | `sed` | Be careful with in-place editing portability. |
| Sort records | `sort` | Use locale deliberately when order matters. |
| Count duplicates | `sort` + `uniq -c` | Input must be sorted for `uniq`. |
| Count lines/bytes | `wc` | Redirect input to avoid filenames in output. |
| Duplicate output | `tee` | Useful for logs and generated reports. |
| Build command invocations from input | `xargs` | Prefer `-0` with NUL-delimited input. |

## Minimal Example

```bash
grep -R --include='*.md' -n -- 'ShellCheck' docs |
sort |
tee shellcheck-references.txt
```

For a runnable demonstration, see `examples/bash/unix-tools-demo.sh`.

## Real-World Example

Count file extensions under a tree:

```bash
find "$root" -type f -print |
awk '
    {
        name = $0
        sub(/^.*\//, "", name)
        if (name ~ /\./) {
            sub(/^.*\./, "", name)
            count[name]++
        }
    }
    END {
        for (ext in count) {
            print count[ext], ext
        }
    }
' |
sort -nr
```

This is more maintainable than trying to do all field processing with Bash parameter expansion once the report logic grows.

For arbitrary filenames, prefer NUL-delimited pipelines where every tool in the chain supports them:

```bash
find "$root" -type f -print0 |
xargs -0 grep -H -- "$pattern"
```

## Common Mistakes

- Parsing `ls` output.
- Using Bash loops for large text processing that `awk`, `sort`, or `grep` can handle directly.
- Forgetting to quote patterns and filenames passed to tools.
- Assuming GNU-only options exist on macOS or BusyBox.
- Using `xargs` without `-0` for filenames.
- Forgetting that `grep` returns `1` when no lines match.
- Relying on locale-dependent sort order without setting `LC_ALL`.
- Using `sed -i` without considering GNU vs BSD behavior.

## Debugging Checklist

- First run each pipeline stage separately.
- Add `set -o pipefail` when upstream failure should fail the script.
- Print intermediate output with `tee` while debugging.
- Test filenames with spaces, newlines, and leading dashes.
- Check command availability with `command -v`.
- Check option portability on the target system.
- Use `LC_ALL=C sort` when bytewise stable ordering matters.

## Related Topics

- [Redirection And Pipes](redirection-and-pipes.md)
- [Command And Process Substitution](command-and-process-substitution.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)
- [Portability Matrix](portability-matrix.md)
- [Performance Boundaries](performance-boundaries.md)

## References

- [GNU Findutils manual](https://www.gnu.org/software/findutils/manual/find.html)
- [GNU grep manual](https://www.gnu.org/software/grep/manual/grep.html)
- [GNU sed manual](https://www.gnu.org/software/sed/manual/sed.html)
- [GNU Awk User's Guide](https://www.gnu.org/software/gawk/manual/gawk.html)
- [GNU Coreutils manual](https://www.gnu.org/software/coreutils/manual/coreutils.html)
- [POSIX.1-2024: Utilities](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/contents.html)
