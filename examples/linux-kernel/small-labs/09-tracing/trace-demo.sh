#!/usr/bin/env bash
set -euo pipefail

duration="${1:-5}"

if [[ ! "$duration" =~ ^[0-9]+$ ]] || (( duration < 1 )); then
	echo "usage: sudo $0 [seconds]" >&2
	exit 2
fi

if (( EUID != 0 )); then
	echo "run this script as root so tracefs can be configured" >&2
	exit 1
fi

if ! command -v trace-cmd >/dev/null 2>&1; then
	echo "trace-cmd is required; install it with the target distribution's package manager" >&2
	exit 1
fi

echo "Recording IRQ, workqueue, and timer events for ${duration}s..."
trace-cmd record -e irq -e workqueue -e timer sleep "$duration"
echo
echo "Trace report:"
trace-cmd report

