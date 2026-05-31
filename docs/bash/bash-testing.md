---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Bash Testing

## What Problem Does This Solve?

Bash scripts often automate risky operations, yet they are frequently tested only by manual runs. Testing gives confidence that argument parsing, failure paths, quoting, cleanup, and command invocation remain correct.

## Core Concepts

- Test success and failure paths.
- Test paths with spaces and leading dashes.
- Keep core logic in functions when practical.
- Use temporary directories as fixtures.
- Capture stdout, stderr, and exit status.
- Use Bats when you want a Bash-oriented test framework.
- ShellCheck complements tests but does not replace them.

## Mental Model

Treat scripts as command-line programs with contracts: input arguments, environment variables, stdout, stderr, filesystem effects, and exit status.

## Syntax / API / Mechanism

Plain Bash smoke test:

```bash
output=$(script.sh arg)
status=$?
```

Bats-style test:

```bash
@test "prints help" {
    run ./script.sh --help
    [ "$status" -eq 0 ]
    [[ "$output" == *Usage:* ]]
}
```

## Minimal Example

```bash
bash -n script.sh
shellcheck script.sh
script.sh --help
```

For a small testable script and Bats example, see:

- `examples/bash/testable-calculator.sh`
- `examples/bash/testable-calculator.bats`

## Real-World Example

Test a script that creates files:

```bash
tmpdir=$(mktemp -d)
trap 'rm -rf -- "$tmpdir"' EXIT

run ./generate.sh "$tmpdir/output file.txt"
[ "$status" -eq 0 ]
[ -f "$tmpdir/output file.txt" ]
```

This tests behavior without touching real project state.

## Common Mistakes

- Testing only the successful path.
- Not checking exit status.
- Ignoring stderr.
- Using fixed `/tmp` paths in tests.
- Failing to test whitespace and leading-dash paths.
- Mocking so much that the test no longer covers real command behavior.
- Assuming Bats is installed on every target system.

## Debugging Checklist

- Add one test for each bug fixed.
- Keep fixtures temporary and isolated.
- Assert stdout, stderr, status, and filesystem effects separately.
- Run ShellCheck before tests.
- Test scripts from a different current directory.
- Include invalid arguments and missing dependency cases.

## Related Topics

- [Robust Scripts](robust-scripts.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)
- [Advanced Debugging And Tracing](advanced-debugging-and-tracing.md)
- [ShellCheck Configuration](shellcheck-configuration.md)

## References

- [Bats-core documentation](https://bats-core.readthedocs.io/)
- [ShellCheck wiki](https://www.shellcheck.net/wiki/Home)
- [GNU Bash Reference Manual: Shell Functions](https://www.gnu.org/software/bash/manual/html_node/Shell-Functions.html)
