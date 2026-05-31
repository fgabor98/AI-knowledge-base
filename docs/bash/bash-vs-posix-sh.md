---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Bash Vs POSIX Sh

## What Problem Does This Solve?

Many systems provide `/bin/sh`, but `/bin/sh` is not necessarily Bash. Scripts are easier to deploy and debug when they are explicit about whether they require Bash features or only POSIX shell features.

## Core Concepts

- Use `#!/usr/bin/env bash` or a known Bash path when the script requires Bash.
- Use `#!/bin/sh` only when the script is written for POSIX shell.
- Bash arrays are not POSIX shell.
- `[[ ... ]]` is Bash syntax, not POSIX shell.
- Process substitution `<( ... )` is Bash syntax.
- `mapfile` and `readarray` are Bash builtins.
- `set -o pipefail` is not POSIX shell.
- `function name { ... }` is not POSIX shell.
- POSIX shell scripts should use portable `test`, `case`, and simple parameter expansion.

## Mental Model

The shebang is a contract. If the script uses Bash features, declare Bash. If it declares `/bin/sh`, assume it may run under `dash`, BusyBox `ash`, or another POSIX-like shell.

Do not write Bash and hope `/bin/sh` is Bash on the target system.

## Syntax / API / Mechanism

Bash-specific:

```bash
#!/usr/bin/env bash
set -euo pipefail

items=("one" "two")
[[ ${items[0]} == one ]]
printf '%s\n' "${items[@]}"
```

POSIX-style:

```sh
#!/bin/sh
set -eu

case "$1" in
    start|stop)
        printf '%s\n' "$1"
        ;;
    *)
        printf 'usage: %s start|stop\n' "$0" >&2
        exit 2
        ;;
esac
```

Portable function form:

```sh
name() {
    printf '%s\n' "$1"
}
```

## Minimal Example

```bash
if [ -n "${BASH_VERSION:-}" ]; then
    printf 'running under Bash %s\n' "$BASH_VERSION"
else
    printf 'not running under Bash\n'
fi
```

For a runnable demonstration, see `examples/bash/bash-vs-posix-demo.sh`.

## Real-World Example

Embedded systems often use BusyBox `ash` as `/bin/sh`. A script using arrays should therefore declare Bash:

```bash
#!/usr/bin/env bash
set -euo pipefail

args=(ip link set dev "$iface" up)
"${args[@]}"
```

If Bash is not guaranteed on the target root filesystem, rewrite the script in POSIX shell or ensure Bash is installed as a dependency.

## Common Mistakes

- Using Bash arrays in a `#!/bin/sh` script.
- Using `[[ ... ]]` in a `#!/bin/sh` script.
- Assuming `/bin/sh` points to Bash because it does on one development machine.
- Using `echo -e` portably. Prefer `printf`.
- Using `source` instead of POSIX `.` in portable scripts.
- Assuming `pipefail` exists in POSIX shell.
- Using Bash-specific `read -d` in a portable script.

## Debugging Checklist

- Run the script with `bash script.sh`.
- Run intended POSIX scripts with `sh script.sh`.
- On Debian-like systems, test with `dash script.sh`.
- On embedded systems, test with the target `/bin/sh`.
- Run ShellCheck with the right shebang.
- Check whether every feature used is supported by the declared shell.

## Related Topics

- [Functions](functions.md)
- [Conditionals](conditionals.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Robust Scripts](robust-scripts.md)

## References

- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
- [GNU Bash Reference Manual: Invoking Bash](https://www.gnu.org/software/bash/manual/html_node/Invoking-Bash.html)
- [GNU Bash Reference Manual: Bash POSIX Mode](https://www.gnu.org/software/bash/manual/html_node/Bash-POSIX-Mode.html)
- [GNU Bash Reference Manual: Bash Conditional Expressions](https://www.gnu.org/software/bash/manual/html_node/Bash-Conditional-Expressions.html)
- [GNU Bash Reference Manual: Arrays](https://www.gnu.org/software/bash/manual/html_node/Arrays.html)
