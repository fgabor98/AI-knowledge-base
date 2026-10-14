#!/usr/bin/env python3
"""Compile every C code block in the documentation.

Each ```c / ~~~c block is compiled as its own translation unit with
-std=c17 -fsyntax-only -Wall -Wextra -Werror. Blocks can opt into other
behavior with an HTML comment on the nearest non-blank line above the fence
(invisible in the rendered site):

    <!-- c-check: skip -- statement fragment, not a translation unit -->
    <!-- c-check: expect-warning -->   must compile and must warn: deliberately
                                       bad code, or a construct GCC does not
                                       support (e.g. #pragma STDC FENV_ACCESS)
    <!-- c-check: expect-error -->     must be rejected (e.g. #error demos)

A block whose first line is a file-name comment such as /* sensor.h */ is
also written into a per-page directory under that name, so later blocks on
the same page can #include it. A later block with the same name replaces it. Headers are checked by compiling them alone,
which also enforces that they are self-contained.

usage: check-c-snippets.py [PATH...]   (default: docs/c)
"""

import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

CC = os.environ.get("CC", "gcc")
# Excerpts often define static helpers that the excerpt itself never calls.
BASE_FLAGS = ["-std=c17", "-fsyntax-only", "-Wall", "-Wextra", "-Wformat=2",
              "-Wno-unused-function"]

FENCE_OPEN = re.compile(r"^(?P<indent>[ \t]*)(?P<fence>```|~~~)c[ \t]*$")
DIRECTIVE = re.compile(r"^\s*<!--\s*c-check:\s*(?P<body>.*?)\s*-->\s*$")
FILE_NAME = re.compile(r"^/\*\s*(?P<name>[\w./-]+\.[ch])\s*\*/\s*$")


@dataclass
class Block:
    page: Path
    line: int           # 1-based line of the opening fence
    code: str
    mode: str           # "compile", "skip", "expect-warning", "expect-error"
    file_name: str | None


class SnippetError(Exception):
    pass


def parse_directive(page: Path, line: int, body: str) -> str:
    if body in ("expect-warning", "expect-error"):
        return body
    if body.startswith("skip"):
        reason = body[len("skip"):].strip().lstrip("-").strip()
        if not reason:
            raise SnippetError(f"{page}:{line}: 'skip' needs a reason: skip -- <reason>")
        return "skip"
    raise SnippetError(f"{page}:{line}: unknown c-check directive '{body}'")


def extract_blocks(page: Path) -> list[Block]:
    lines = page.read_text().splitlines()
    blocks = []
    i = 0
    while i < len(lines):
        match = FENCE_OPEN.match(lines[i])
        if not match:
            i += 1
            continue
        indent, fence = match["indent"], match["fence"]
        start = i
        body = []
        i += 1
        while i < len(lines) and lines[i].strip() != fence:
            line = lines[i]
            body.append(line[len(indent):] if line.startswith(indent) else line.lstrip())
            i += 1
        if i == len(lines):
            raise SnippetError(f"{page}:{start + 1}: unterminated code fence")
        i += 1

        code = "\n".join(body) + "\n"
        if "--8<--" in code:            # snippet include: checked at its source
            continue

        mode = "compile"
        previous = start - 1
        while previous >= 0 and not lines[previous].strip():
            previous -= 1
        if previous >= 0:
            directive = DIRECTIVE.match(lines[previous])
            if directive:
                mode = parse_directive(page, previous + 1, directive["body"])

        first = next((line for line in body if line.strip()), "")
        name_match = FILE_NAME.match(first.strip())
        blocks.append(Block(page, start + 1, code, mode,
                            name_match["name"] if name_match else None))
    return blocks


def compile_block(block: Block, workspace: Path, index: int) -> tuple[int, str]:
    if block.file_name:
        source = workspace / block.file_name
    else:
        source = workspace / f"block_{index}.c"
        source.write_text(block.code)
    flags = BASE_FLAGS + ["-I", str(workspace)]
    if block.mode == "compile":
        flags.append("-Werror")
    result = subprocess.run([CC, *flags, "-x", "c", str(source)],
                            capture_output=True, text=True)
    output = result.stdout + result.stderr
    return result.returncode, output.replace(str(workspace) + "/", "")


def check_page(page: Path) -> tuple[int, int, int, list[str]]:
    blocks = extract_blocks(page)
    failures = []
    checked = skipped = 0
    with tempfile.TemporaryDirectory() as directory:
        workspace = Path(directory)
        for index, block in enumerate(blocks):
            # A named block defines that file from this point in the page on,
            # so a page can show a header evolving across several blocks.
            if block.file_name:
                target = workspace / block.file_name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(block.code)
            if block.mode == "skip":
                skipped += 1
                continue
            checked += 1
            status, output = compile_block(block, workspace, index)
            where = f"{block.page}:{block.line}"
            if block.mode == "compile" and status != 0:
                failures.append(f"{where}: does not compile cleanly\n{output}")
            elif block.mode == "expect-warning":
                if status != 0:
                    failures.append(f"{where}: expect-warning block fails to compile\n{output}")
                elif "warning:" not in output:
                    failures.append(f"{where}: expect-warning block produced no warning")
            elif block.mode == "expect-error" and status == 0:
                failures.append(f"{where}: expect-error block compiled successfully")
    return len(blocks), checked, skipped, failures


def main() -> int:
    roots = [Path(argument) for argument in sys.argv[1:]] or [Path("docs/c")]
    pages = sorted(page for root in roots
                   for page in ([root] if root.is_file() else root.rglob("*.md")))
    total = checked = skipped = 0
    failures = []
    try:
        for page in pages:
            page_total, page_checked, page_skipped, page_failures = check_page(page)
            total += page_total
            checked += page_checked
            skipped += page_skipped
            failures += page_failures
    except SnippetError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    for failure in failures:
        print(failure.rstrip() + "\n", file=sys.stderr)
    print(f"C snippets: {total} found, {checked} compiled, {skipped} skipped, "
          f"{len(failures)} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
