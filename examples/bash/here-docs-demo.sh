#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

name='demo-service'
port=8080
config=$tmpdir/service.conf
literal=$tmpdir/literal.sh

cat > "$config" <<EOF
[service]
name=$name
port=$port
EOF

cat > "$literal" <<'EOF'
printf '%s\n' "$PATH"
EOF

printf 'expanded here document:\n'
cat "$config"

printf 'literal here document:\n'
cat "$literal"

read -r first second <<< "alpha beta"
printf 'here string read: first=%s second=%s\n' "$first" "$second"
