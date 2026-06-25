---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Performance Boundaries

## What Problem Does This Solve?

Bash is excellent glue, but it is not a general-purpose data processing engine. Performance boundaries help decide when to keep Bash, when to use shell tools such as `awk`, and when to switch to Python, C, or a domain-specific tool.

## Core Concepts

- Starting external processes is relatively expensive.
- Loops in Bash are slow for large data sets.
- Text processing tools can process streams more efficiently.
- Arrays keep data in memory.
- Command substitution captures all output in memory.
- Correctness and maintainability matter more than micro-optimization.
- Bash is best at orchestration, not heavy computation.

## Mental Model

Use Bash to connect tools. Use the right tool to process the data. If most of the script is parsing, transforming, or calculating, Bash may be the wrong primary language.

## Syntax / API / Mechanism

Avoid per-line subprocesses:

```bash
while IFS= read -r line; do
    printf '%s\n' "$line" | tr a-z A-Z
done < input.txt
```

Prefer one streaming tool:

```bash
tr a-z A-Z < input.txt
```

Avoid loading huge output:

```bash
output=$(find "$root" -type f)
```

Prefer streaming:

```bash
find "$root" -type f -print0 |
while IFS= read -r -d '' path; do
    process "$path"
done
```

## Minimal Example

```bash
find "$root" -type f -name '*.log' -print0 |
while IFS= read -r -d '' path; do
    gzip -- "$path"
done
```

For a runnable demonstration, see `examples/bash/performance-boundaries-demo.sh`.

## Real-World Example

Use `awk` for record processing:

```bash
awk -F, '$3 == "error" { count += 1 } END { print count + 0 }' events.csv
```

This is simpler and faster than splitting every CSV-like line manually in Bash. For real CSV with quoting rules, use a proper parser instead.

## Common Mistakes

- Running `grep`, `sed`, or `awk` once per line in a Bash loop.
- Capturing large command output into a variable.
- Using Bash arrays for large datasets.
- Reimplementing parsers in shell.
- Optimizing before measuring.
- Keeping a script in Bash after it has become a real application.

## Debugging Checklist

- Count how many external processes run in the hot path.
- Measure with `time` before optimizing.
- Test with realistic input sizes.
- Look for command substitutions that capture large data.
- Replace per-line subprocesses with streaming commands.
- Move complex parsing to Python, awk, or a purpose-built tool.

## Related Topics

- [Read Lines Safely](read-lines-safely.md)
- [Parallelism And Background Jobs](parallelism-and-background-jobs.md)
- [Advanced Parameter Expansion](advanced-parameter-expansion.md)
- [Portability Matrix](portability-matrix.md)

## References

- [GNU Bash Reference Manual: Command Execution Environment](https://www.gnu.org/software/bash/manual/html_node/Command-Execution-Environment.html)
- [GNU Bash Reference Manual: Command Substitution](https://www.gnu.org/software/bash/manual/html_node/Command-Substitution.html)
- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
