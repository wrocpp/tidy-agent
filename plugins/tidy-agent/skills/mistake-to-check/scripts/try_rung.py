#!/usr/bin/env python3
"""Check a candidate clang-tidy config against positive and negative cases.

Every line in the positive file that ends with `// expect: <diagnostic>` must
be flagged with that diagnostic, and nothing else in either file may be
flagged. Exit 0 when the rung is exact, 1 otherwise, with a line per mismatch.

Usage: try_rung.py --config CANDIDATE --pos POS.cpp --neg NEG.cpp
                   [--clang-tidy PATH] [--load PLUGIN] [-- EXTRA COMPILER ARGS]
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

EXPECT_RE = re.compile(r"//\s*expect:\s*(\S+)\s*$")
DIAG_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):\d+: (?:warning|error): .* \[(?P<names>[\w.,-]+)\]$")


def expected(path: Path) -> set[tuple[int, str]]:
    return {(n, m.group(1)) for n, line in enumerate(path.read_text().splitlines(), 1)
            if (m := EXPECT_RE.search(line))}


def flagged(tidy: str, config: Path, source: Path, load: str | None, extra: list[str]) -> set[tuple[int, str]]:
    cmd = [tidy, "--quiet", "--experimental-custom-checks", f"--config-file={config}"]
    if load:
        cmd.append(f"-load={load}")
    cmd += [str(source), "--", *(extra or ["-std=c++23"])]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    found = set()
    for line in out.splitlines():
        if (m := DIAG_RE.match(line)) and Path(m.group("file")).resolve() == source.resolve():
            found |= {(int(m.group("line")), n) for n in m.group("names").split(",")}
    return found


def main() -> int:
    argv = sys.argv[1:]
    extra = argv[argv.index("--") + 1:] if "--" in argv else []
    argv = argv[: argv.index("--")] if "--" in argv else argv
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--config", type=Path, required=True)
    p.add_argument("--pos", type=Path, required=True)
    p.add_argument("--neg", type=Path, required=True)
    p.add_argument("--clang-tidy", default="clang-tidy")
    p.add_argument("--load")
    args = p.parse_args(argv)

    problems = []
    want = expected(args.pos)
    got = flagged(args.clang_tidy, args.config, args.pos, args.load, extra)
    problems += [f"{args.pos}:{n}: expected {d}, not flagged" for n, d in sorted(want - got)]
    problems += [f"{args.pos}:{n}: flagged {d}, not expected" for n, d in sorted(got - want)]
    for n, d in sorted(flagged(args.clang_tidy, args.config, args.neg, args.load, extra)):
        problems.append(f"{args.neg}:{n}: negative case flagged by {d}")
    for problem in problems:
        print(problem)
    print("EXACT" if not problems else f"{len(problems)} mismatch(es)")
    return 0 if not problems else 1


if __name__ == "__main__":
    sys.exit(main())
