---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Advanced Parameter Expansion

## What Problem Does This Solve?

Advanced parameter expansion can replace small external commands for string manipulation, defaults, slicing, transformations, and indirect lookup. Used carefully, it makes scripts faster and more precise. Used carelessly, it becomes unreadable.

## Core Concepts

- Prefix and suffix removal use shell patterns, not regular expressions.
- Replacement forms can substitute matched patterns.
- Slicing works on strings and arrays.
- Indirect expansion can refer to variables by name.
- Namerefs with `declare -n` create references to variables.
- Case modification operators are Bash-specific.
- Complex expansion should be documented or replaced with clearer code.

## Mental Model

Parameter expansion operates before the command runs. It is good for local string and array transformations, not for parsing complex formats.

## Syntax / API / Mechanism

Common advanced forms:

```bash
name=${path##*/}          # longest prefix removal
stem=${name%.*}           # shortest suffix removal
dir=${path%/*}
new=${value/pat/repl}     # replace first match
all=${value//pat/repl}    # replace all matches
slice=${value:2:5}
upper=${value^^}
```

Indirect expansion:

```bash
var_name=HOME
printf '%s\n' "${!var_name}"
```

Nameref:

```bash
set_default() {
    local -n ref=$1
    ref=${ref:-default}
}
```

## Minimal Example

```bash
path=/opt/app/archive.tar.gz
file=${path##*/}
printf 'file=%s stem=%s\n' "$file" "${file%%.*}"
```

For a runnable demonstration, see `examples/bash/advanced-parameter-expansion-demo.sh`.

## Real-World Example

Normalize option names:

```bash
option=--enable-feature-x
name=${option#--}
name=${name//-/_}
printf 'CONFIG_%s=1\n' "${name^^}"
```

This produces `CONFIG_ENABLE_FEATURE_X=1` without external processes.

## Common Mistakes

- Treating shell patterns as regular expressions.
- Using indirect expansion where an associative array would be clearer.
- Hiding too much logic in one expansion.
- Forgetting to quote expansions after transforming them.
- Assuming `${var^^}` works in old Bash versions.
- Using namerefs without considering caller variable names and scope.

## Debugging Checklist

- Print each intermediate value.
- Test empty values and values without the expected delimiter.
- Check whether the pattern is a shell pattern or regex.
- Use `declare -p` for arrays and referenced variables.
- Prefer multiple simple expansions over one dense expression.
- Verify target Bash version for case conversion and namerefs.

## Related Topics

- [Variables And Expansion](variables-and-expansion.md)
- [Associative Arrays](associative-arrays.md)
- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)
- [Performance Boundaries](performance-boundaries.md)

## References

- [GNU Bash Reference Manual: Shell Parameter Expansion](https://www.gnu.org/software/bash/manual/html_node/Shell-Parameter-Expansion.html)
- [GNU Bash Reference Manual: Arrays](https://www.gnu.org/software/bash/manual/html_node/Arrays.html)
- [GNU Bash Reference Manual: Bash Builtin Commands](https://www.gnu.org/software/bash/manual/html_node/Bash-Builtins.html)
