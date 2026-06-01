---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Here Documents And Here Strings

## What Problem Does This Solve?

Here documents and here strings feed inline text to commands. They are useful for templates, generated configuration, tests, SQL fragments, and short input values, but delimiter quoting controls whether Bash expands the content.

## Core Concepts

- A here document redirects multiple following lines to a command's stdin.
- `<<EOF` allows parameter, command, and arithmetic expansion inside the body.
- `<<'EOF'` disables shell expansion inside the body.
- `<<-EOF` strips leading tab characters from the body.
- A here string, `<<< "$value"`, sends one string to stdin.
- Here strings append a newline.
- Here documents are redirections, not string literals.
- Delimiter names should be clear and unique.

## Mental Model

A here document is inline stdin:

```bash
cat <<EOF
hello
EOF
```

The command reads the text as if it came from a file or pipe.

Delimiter quoting decides whether Bash treats the body as a template or as literal text.

## Syntax / API / Mechanism

Expanded here document:

```bash
name=service-a
cat <<EOF
name=$name
EOF
```

Literal here document:

```bash
cat <<'EOF'
The string $name is not expanded here.
EOF
```

Indented with tabs:

```bash
cat <<-EOF
	leading tab characters are stripped
EOF
```

Here string:

```bash
read -r first second <<< "$line"
```

## Minimal Example

```bash
user=${USER:-unknown}

cat <<EOF
generated_for=$user
EOF
```

For a runnable demonstration, see `examples/bash/here-docs-demo.sh`.

## Real-World Example

Generate a small config file safely:

```bash
tmp=$(mktemp)
trap 'rm -f -- "$tmp"' EXIT

service_name=my-service
port=8080

cat > "$tmp" <<EOF
[service]
name=$service_name
port=$port
EOF

install -m 0644 "$tmp" "$target"
```

If the body contains literal shell syntax, quote the delimiter:

```bash
cat > "$script" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf '%s\n' "$PATH"
EOF
```

## Common Mistakes

- Forgetting to quote the delimiter when literal `$`, backticks, or `$(...)` should remain unchanged.
- Quoting the delimiter when a template is supposed to expand.
- Using a delimiter string that also appears in the body.
- Indenting `<<EOF` bodies with spaces and expecting `<<-EOF` to strip them.
- Feeding secrets through expanded here documents without reviewing trace output.
- Using here strings for large data streams.
- Forgetting that a here string adds a trailing newline.

## Debugging Checklist

- Start with `cat <<EOF` to inspect exactly what stdin will be.
- Use a quoted delimiter for literal examples and generated scripts.
- Use `set -x` carefully because expanded here-doc commands may reveal data.
- Test values containing spaces, quotes, and dollar signs.
- Prefer temporary files when debugging complex generated input.
- Check whether indentation uses tabs or spaces when using `<<-`.

## Related Topics

- [Redirection And Pipes](redirection-and-pipes.md)
- [Command And Process Substitution](command-and-process-substitution.md)
- [Quoting](quoting.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)

## References

- [GNU Bash Reference Manual: Redirections](https://www.gnu.org/software/bash/manual/html_node/Redirections.html)
- [GNU Bash Reference Manual: Here Documents](https://www.gnu.org/software/bash/manual/html_node/Here-Documents.html)
- [GNU Bash Reference Manual: Shell Expansions](https://www.gnu.org/software/bash/manual/html_node/Shell-Expansions.html)
