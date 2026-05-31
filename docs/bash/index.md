---
status: draft
reviewed: false
domain: bash
difficulty: beginner
last_reviewed: null
---

# Bash Programming

Shell scripting topics focused on robust automation, predictable error handling, and maintainable command-line tooling.

This section builds practical Bash knowledge from the execution model upward. The goal is not to memorize shell syntax, but to understand how Bash expands commands, handles errors, composes processes, and fails in real automation scripts.

## Scope

The Bash section covers:

- writing maintainable Bash scripts
- understanding expansion, quoting, and word splitting
- handling exit status and errors deliberately
- using functions, arrays, traps, pipes, and redirection correctly
- knowing when Bash is appropriate and when another language is a better fit
- recognizing portability differences between Bash and POSIX shell
- handling advanced failure, process, tracing, security, and portability concerns

## Learning Path

Follow the pages in this order. Each group builds on the previous one.

### Beginner

These pages establish the core mental model and the habits that prevent most shell bugs.

1. [Execution Model](execution-model.md)
2. [Quoting](quoting.md)
3. [Variables And Expansion](variables-and-expansion.md)
4. [Exit Codes](exit-codes.md)
5. [Conditionals](conditionals.md)
6. [Loops](loops.md)
7. [Functions](functions.md)
8. [Robust Scripts](robust-scripts.md)
9. [Bash Vs POSIX Sh](bash-vs-posix-sh.md)

### Intermediate

These pages cover process composition, input handling, cleanup, and filesystem safety.

1. [Redirection And Pipes](redirection-and-pipes.md)
2. [Subshells](subshells.md)
3. [Read Lines Safely](read-lines-safely.md)
4. [Argument Parsing](argument-parsing.md)
5. [Logging](logging.md)
6. [Traps And Cleanup](traps-and-cleanup.md)
7. [Retries And Timeouts](retries-and-timeouts.md)
8. [Safe Filesystem Operations](safe-filesystem-operations.md)

### Advanced

These pages are for scripts that manage complex failure behavior, child processes, file descriptors, portability constraints, shell security, or developer tooling.

1. [Errexit And ERR Traps](errexit-and-err-traps.md)
2. [Advanced File Descriptors](advanced-file-descriptors.md)
3. [Signals, Process Groups, And Child Processes](signals-process-groups-and-child-processes.md)
4. [Parallelism And Background Jobs](parallelism-and-background-jobs.md)
5. [Advanced Parameter Expansion](advanced-parameter-expansion.md)
6. [Associative Arrays](associative-arrays.md)
7. [Eval, Injection, And Shell Security](eval-injection-and-shell-security.md)
8. [Advanced Debugging And Tracing](advanced-debugging-and-tracing.md)
9. [Programmable Completion](programmable-completion.md)
10. [Bash Testing](bash-testing.md)
11. [Portability Matrix](portability-matrix.md)
12. [Performance Boundaries](performance-boundaries.md)
13. [ShellCheck Configuration](shellcheck-configuration.md)

## Example Inventory

Runnable examples live under `examples/bash/` and are referenced from the relevant topic pages.

Beginner examples:

- `execution-model-demo.sh`
- `quoting-demo.sh`
- `expansion-demo.sh`
- `exit-code-demo.sh`
- `pipefail-demo.sh`
- `conditionals-demo.sh`
- `loops-demo.sh`
- `functions-demo.sh`
- `robust-script-template.sh`
- `bash-vs-posix-demo.sh`

Intermediate examples:

- `redirection-demo.sh`
- `subshell-demo.sh`
- `read-lines-safely.sh`
- `getopts-demo.sh`
- `manual-args-demo.sh`
- `logging-demo.sh`
- `trap-cleanup.sh`
- `retry-demo.sh`
- `safe-filesystem-operations.sh`

Advanced examples:

- `errexit-err-trap-demo.sh`
- `advanced-fd-demo.sh`
- `process-management-demo.sh`
- `bounded-parallelism-demo.sh`
- `advanced-parameter-expansion-demo.sh`
- `associative-arrays-demo.sh`
- `eval-injection-demo.sh`
- `tracing-demo.sh`
- `completion-demo.sh`
- `testable-calculator.sh`
- `testable-calculator.bats`
- `portability-matrix-demo.sh`
- `performance-boundaries-demo.sh`

## Page Rules

Every Bash topic page should use the [Topic Page Template](../topic-page-template.md) and stay in draft status until reviewed.

Good Bash pages should include:

- a minimal example
- a realistic script fragment
- at least one common failure mode
- commands or tests the reader can run locally
- notes about Bash-specific behavior when relevant
- direct links to primary references such as the GNU Bash manual, POSIX shell documentation, GNU userland manuals, or ShellCheck rule notes

## Review Priorities

Before marking a Bash page reviewed, check especially for:

- unsafe unquoted variables
- misleading advice around `set -e`
- incorrect assumptions about pipelines
- Bash-specific syntax presented as portable shell
- examples that fail on paths with spaces
- loops that mishandle empty lines or backslashes
- traps that do not clean up reliably
- process cleanup for background jobs
- advanced tracing that may expose secrets
- portability claims across Bash versions and non-GNU userlands
