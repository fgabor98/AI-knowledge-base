#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

mkdir -p "$tmpdir/logs"
cat > "$tmpdir/logs/app.log" <<'LOG'
INFO start
WARN disk
INFO ready
ERROR failed
WARN retry
LOG

cat > "$tmpdir/users.csv" <<'CSV'
name,role
alice,admin
bob,user
carol,user
CSV

printf 'warning and error lines:\n'
grep -E 'WARN|ERROR' "$tmpdir/logs/app.log"

printf 'roles:\n'
tail -n +2 "$tmpdir/users.csv" | cut -d, -f2 | sort | uniq -c

report=$tmpdir/report.txt
awk -F, 'NR > 1 { count[$2]++ } END { for (role in count) print role, count[role] }' "$tmpdir/users.csv" |
sort |
tee "$report" >/dev/null

printf 'report file:\n'
cat "$report"

printf 'log files found with find:\n'
find "$tmpdir" -type f -name '*.log' -print
