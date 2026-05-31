---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# ShellCheck Configuration

## What Problem Does This Solve?

ShellCheck is most useful when warnings are treated deliberately. Configuration and targeted suppressions let a project keep the linter strict without fighting warnings that are intentional and documented.

## Core Concepts

- Prefer fixing warnings over suppressing them.
- Suppress warnings as narrowly as possible.
- Explain suppressions when the reason is not obvious.
- Match ShellCheck's shell dialect to the script shebang.
- Use project-level configuration sparingly.
- Keep generated or fixture scripts separate when needed.

## Mental Model

A suppression is a small code review note. It should say, "This warning was considered, and this specific occurrence is acceptable."

## Syntax / API / Mechanism

Inline suppression:

```bash
# shellcheck disable=SC2086
printf '<%s>\n' $value
```

File-level directive:

```bash
# shellcheck shell=bash
```

External source directive:

```bash
# shellcheck source=lib/common.sh
source "$script_dir/lib/common.sh"
```

Command-line shell selection:

```sh
shellcheck --shell=bash script
```

## Minimal Example

```bash
# Intentionally unquoted to demonstrate word splitting in this example.
# shellcheck disable=SC2086
print_args $value
```

This kind of suppression is acceptable in teaching code because the warning is the point of the example.

## Real-World Example

Use ShellCheck in a project validation script:

```bash
shopt -s nullglob
scripts=(scripts/*.sh examples/bash/*.sh)
shellcheck "${scripts[@]}"
```

This is the pattern used by `scripts/validate-bash.sh`.

## Common Mistakes

- Disabling warnings globally because one example is intentional.
- Suppressing without explaining why.
- Running ShellCheck with the wrong shell dialect.
- Ignoring warnings about unquoted variables in production scripts.
- Treating ShellCheck as a replacement for tests.
- Forgetting to lint helper scripts under `scripts/`.

## Debugging Checklist

- Read the ShellCheck wiki page for the warning.
- Decide whether the warning indicates a real bug.
- Prefer a code change over a suppression.
- If suppressing, use the smallest scope.
- Add a short comment for intentional teaching examples.
- Keep validation scripts linted too.

## Related Topics

- [Quoting](quoting.md)
- [Bash Testing](bash-testing.md)
- [Robust Scripts](robust-scripts.md)
- [Eval, Injection, And Shell Security](eval-injection-and-shell-security.md)

## References

- [ShellCheck wiki](https://www.shellcheck.net/wiki/Home)
- [ShellCheck SC2086](https://www.shellcheck.net/wiki/SC2086)
- [ShellCheck SC2046](https://www.shellcheck.net/wiki/SC2046)
- [ShellCheck SC2154](https://www.shellcheck.net/wiki/SC2154)
