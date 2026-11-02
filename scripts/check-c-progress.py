#!/usr/bin/env python3
"""Check that docs/c/progress.md lists every C page exactly once.

docs/c/progress.md is edited by hand: readers tick `- [x]` and commit it. This
script never rewrites it. It reports pages that are missing (with the line to
add), listed twice, or no longer in the navigation, and prints the reader's
progress.

usage: check-c-progress.py
"""

import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
CHECKLIST = ROOT / "docs" / "c" / "progress.md"
SELF = "c/progress.md"
ITEM = re.compile(r"^- \[([ xX])\] \[[^\]]+\]\(([^)#\s]+)\)\s*$")


class NavLoader(yaml.SafeLoader):
    """SafeLoader that ignores mkdocs-specific tags such as !!python/name."""


NavLoader.add_multi_constructor("", lambda loader, suffix, node: None)
NavLoader.add_multi_constructor("tag:yaml.org,2002:python/", lambda loader, suffix, node: None)


def nav_pages(entries, chapter=None):
    """Yield (chapter, title, path) for every page in a nav subtree, in order."""
    for entry in entries:
        for title, value in entry.items():
            if isinstance(value, str):
                yield chapter or "Start Here", title, value
            else:
                yield from nav_pages(value, chapter or title)


def main() -> int:
    nav = yaml.load((ROOT / "mkdocs.yml").read_text(), Loader=NavLoader)["nav"]
    c_nav = next(entry["C"] for entry in nav if isinstance(entry, dict) and "C" in entry)
    expected = [(chapter, title, path) for chapter, title, path in nav_pages(c_nav)
                if path != SELF]

    listed = {}
    done = 0
    for number, line in enumerate(CHECKLIST.read_text().splitlines(), 1):
        match = ITEM.match(line)
        if not match:
            continue
        path = "c/" + match.group(2)
        listed.setdefault(path, []).append(number)
        done += match.group(1) in "xX"

    problems = []
    for chapter, title, path in expected:
        if path not in listed:
            problems.append(f"missing ({chapter}): - [ ] [{title}]({path[len('c/'):]})")
    for path, lines in listed.items():
        if len(lines) > 1:
            problems.append(f"listed more than once: {path} (lines {lines})")
    known = {path for _, _, path in expected}
    for path, lines in listed.items():
        if path not in known:
            problems.append(f"not in the navigation: {path} (line {lines[0]})")

    for problem in problems:
        print(f"docs/c/progress.md: {problem}", file=sys.stderr)
    print(f"C progress checklist: {len(expected)} pages, {done} complete, "
          f"{len(problems)} problems")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
