---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Associative Arrays

## What Problem Does This Solve?

Associative arrays let Bash scripts store key-value data without parallel arrays, fragile delimiter parsing, or repeated `case` statements. They are useful for lookup tables, counters, configuration maps, and deduplication.

## Core Concepts

- Associative arrays require Bash 4 or newer.
- Declare them with `declare -A name`.
- Keys and values are strings.
- Quote keys and values during expansion.
- Iteration order is not a stable interface.
- `${!map[@]}` expands keys.
- `${map[$key]+set}` checks whether a key exists.

## Mental Model

Use associative arrays when the key is meaningful data. Do not encode key-value data into strings if Bash already gives you a map.

## Syntax / API / Mechanism

Basic use:

```bash
declare -A status_by_host
status_by_host[router]=up
status_by_host["dev board"]="down"

key="dev board"
printf '%s\n' "${status_by_host[$key]}"
```

Key existence:

```bash
if [[ ${status_by_host[$key]+present} ]]; then
    printf 'known key\n'
fi
```

Iteration:

```bash
for key in "${!status_by_host[@]}"; do
    printf '%s=%s\n' "$key" "${status_by_host[$key]}"
done
```

## Minimal Example

```bash
declare -A counts=()
counts[error]=3
counts[warning]=7
printf 'errors=%s\n' "${counts[error]}"
```

For a runnable demonstration, see `examples/bash/associative-arrays-demo.sh`.

## Real-World Example

Count log levels:

```bash
declare -A counts=()

while IFS= read -r level _message; do
    ((counts[$level] += 1))
done < log-levels.txt

for level in "${!counts[@]}"; do
    printf '%s %d\n' "$level" "${counts[$level]}"
done
```

## Common Mistakes

- Forgetting `declare -A`, causing numeric-index array behavior.
- Assuming iteration order is sorted.
- Failing under Bash 3.2, which is still common on old macOS installations.
- Testing key existence by checking whether the value is non-empty.
- Expanding keys unquoted.
- Using associative arrays when a real data format would be clearer.

## Debugging Checklist

- Use `declare -p map` to inspect the array.
- Test keys containing spaces.
- Check Bash version with `${BASH_VERSINFO[0]}`.
- Sort keys explicitly if output order matters.
- Distinguish missing keys from keys with empty values.
- Run ShellCheck for array warnings.

## Related Topics

- [Advanced Parameter Expansion](advanced-parameter-expansion.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)
- [Portability Matrix](portability-matrix.md)

## References

- [GNU Bash Reference Manual: Arrays](https://www.gnu.org/software/bash/manual/html_node/Arrays.html)
- [GNU Bash Reference Manual: Shell Parameter Expansion](https://www.gnu.org/software/bash/manual/html_node/Shell-Parameter-Expansion.html)
