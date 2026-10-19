#!/usr/bin/env python3
"""Check that command transcripts quoted in the build-systems docs are real.

A transcript is a fenced ``text`` block whose first line starts with "$ ".
Every line of such a block must appear, in order, in the output of one of the
build-systems tool sessions:

- sessions under examples/build-systems/tool-sessions/*/run.sh are run now;
- heavyweight sessions (a Yocto build) keep a recorded transcript in
  examples/build-systems/**/transcripts/*.txt, refreshed by their own script.

A line "..." inside a quoted block skips any number of output lines, so a page
may shorten a long listing. Usage:

    check-build-transcripts.py [DOCS_DIR]   (default: docs/build-systems)
"""

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SESSIONS = ROOT / "examples" / "build-systems" / "tool-sessions"
RECORDED = ROOT / "examples" / "build-systems"

FENCE = re.compile(r"^(```|~~~)text\s*$")


def session_outputs():
    """Return a list of (name, lines) for every live and recorded session."""
    outputs = []
    for script in sorted(SESSIONS.glob("*/run.sh")):
        result = subprocess.run(
            ["bash", str(script)], capture_output=True, text=True, check=False
        )
        if result.returncode != 0:
            sys.exit(f"session {script.parent.name} failed:\n{result.stderr}")
        outputs.append((script.parent.name, result.stdout.splitlines()))
    for recorded in sorted(RECORDED.glob("**/transcripts/*.txt")):
        outputs.append((str(recorded.relative_to(ROOT)), recorded.read_text().splitlines()))
    return outputs


def quoted_transcripts(docs_dir):
    """Yield (path, line number, lines) for each transcript block."""
    for path in sorted(docs_dir.rglob("*.md")):
        lines = path.read_text().splitlines()
        i = 0
        while i < len(lines):
            match = FENCE.match(lines[i])
            if not match:
                i += 1
                continue
            fence = match.group(1)
            start = i + 1
            end = start
            while end < len(lines) and lines[end].strip() != fence:
                end += 1
            block = lines[start:end]
            if block and block[0].startswith("$ "):
                yield path, start + 1, block
            i = end + 1


def contains_in_order(haystack, needle):
    """True if needle's lines occur consecutively in haystack; '...' skips lines."""
    first = needle[0].rstrip()
    return any(
        _match_from(haystack, k + 1, needle[1:])
        for k, line in enumerate(haystack)
        if line.rstrip() == first
    )


def _match_from(haystack, position, needle):
    pending_skip = False
    for line in needle:
        line = line.rstrip()
        if line == "...":
            pending_skip = True
            continue
        if pending_skip:
            found = None
            for k in range(position, len(haystack)):
                if haystack[k].rstrip() == line:
                    found = k
                    break
            if found is None:
                return False
            position = found + 1
            pending_skip = False
            continue
        if position >= len(haystack) or haystack[position].rstrip() != line:
            return False
        position += 1
    return True


def main():
    docs_dir = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "docs" / "build-systems"
    outputs = session_outputs()
    failures = 0
    checked = 0
    for path, line_no, block in quoted_transcripts(docs_dir):
        checked += 1
        if not any(contains_in_order(lines, block) for _, lines in outputs):
            failures += 1
            rel = path.relative_to(ROOT) if path.is_absolute() else path
            print(f"{rel}:{line_no}: transcript not found in any session output:")
            for line in block[:6]:
                print(f"    {line}")
    print(f"checked {checked} quoted transcripts, {failures} not found")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
