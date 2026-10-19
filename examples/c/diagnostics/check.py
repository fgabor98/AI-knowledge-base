#!/usr/bin/env python3
"""Verify the answer key of the test-out diagnostics against the compiler.

Each question qNN-*.c has a qNN-*.expected file made of directives:

    run FLAGS...     build with -std=c17 FLAGS, run; the lines that follow,
                     up to the next directive, are the exact expected stdout
    compile          -std=c17 -Wall -Wextra -c succeeds without diagnostics
    warn REGEX       compiles, and a warning matches REGEX
    reject REGEX     fails to compile, and an error matches REGEX
"""

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

CC = os.environ.get("CC", "gcc")
DIRECTIVES = ("run", "compile", "warn", "reject")


def parse(expected: Path):
    checks = []
    for line in expected.read_text().splitlines():
        word, _, rest = line.partition(" ")
        if word in DIRECTIVES:
            checks.append([word, rest, []])
        elif checks and checks[-1][0] == "run":
            checks[-1][2].append(line)
        else:
            raise ValueError(f"{expected}: unexpected line {line!r}")
    return checks


def compile_only(source: Path):
    result = subprocess.run([CC, "-std=c17", "-Wall", "-Wextra", "-c", str(source),
                             "-o", os.devnull], capture_output=True, text=True)
    return result.returncode, result.stderr


def check(source: Path, workdir: Path) -> list[str]:
    errors = []
    expected = source.with_suffix(".expected")
    if not expected.exists():
        return [f"{source}: missing {expected.name}"]
    for kind, argument, lines in parse(expected):
        if kind == "run":
            program = workdir / source.stem
            build = subprocess.run([CC, "-std=c17", *argument.split(), "-w", str(source),
                                    "-o", str(program)], capture_output=True, text=True)
            if build.returncode != 0:
                errors.append(f"{source}: run {argument}: build failed\n{build.stderr}")
                continue
            output = subprocess.run([str(program)], capture_output=True, text=True,
                                    timeout=10).stdout.splitlines()
            if output != lines:
                errors.append(f"{source}: run {argument}: expected {lines}, got {output}")
        else:
            status, stderr = compile_only(source)
            if kind == "compile" and (status != 0 or stderr):
                errors.append(f"{source}: expected a clean compile\n{stderr}")
            elif kind == "warn" and (status != 0 or not re.search(argument, stderr)):
                errors.append(f"{source}: expected a warning matching /{argument}/\n{stderr}")
            elif kind == "reject" and (status == 0 or not re.search(argument, stderr)):
                errors.append(f"{source}: expected rejection matching /{argument}/\n{stderr}")
    return errors


def main() -> int:
    root = Path(__file__).resolve().parent
    sources = sorted(root.glob("*/q*.c"))
    errors = []
    with tempfile.TemporaryDirectory() as directory:
        for source in sources:
            errors += check(source, Path(directory))
    for error in errors:
        print(error, file=sys.stderr)
    print(f"Diagnostic answer key: {len(sources)} questions checked, {len(errors)} failed")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
