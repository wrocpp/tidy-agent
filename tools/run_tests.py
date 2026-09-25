#!/usr/bin/env python3
"""Run every check against its test cases and compare with the expectations.

A test file lives in tests/<check>/ and starts with `// spec: <check>`, naming
the spec in docs/checks/<check>.md that it satisfies. A line that must be
flagged ends with `// expect: <diagnostic-name>`; every other line must not be
flagged. The runner fails when:

- a flagged line is missing, or an unexpected line is flagged;
- a spec has no tests, or a test names a spec that does not exist.

Query checks run with --experimental-custom-checks and a config generated from
checks/query/<check>.yaml. Plugin checks (when built) are loaded with -load.

Usage: tools/run_tests.py [--clang-tidy PATH] [--plugin PATH] [CHECK ...]
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_config  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
SPEC_DIR = ROOT / "docs" / "checks"
TEST_DIR = ROOT / "tests"
QUERY_DIR = ROOT / "checks" / "query"
CONFIG_DIR = ROOT / "checks" / "config"

SPEC_RE = re.compile(r"^//\s*spec:\s*(\S+)")
EXPECT_RE = re.compile(r"//\s*expect:\s*(\S+)\s*$")
DIAG_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):\d+: warning: .* \[(?P<name>[\w.,-]+)\]$")
CXX_STD = "-std=c++23"


def specs() -> set[str]:
    """Specs that must have tests: all but README and those marked blocked."""
    return {p.stem for p in SPEC_DIR.glob("*.md")
            if p.stem != "README" and "\nStatus: blocked" not in p.read_text()}


def expectations(path: Path) -> tuple[str | None, set[tuple[int, str]]]:
    spec = None
    expected = set()
    for number, line in enumerate(path.read_text().splitlines(), start=1):
        if number == 1 and (m := SPEC_RE.match(line)):
            spec = m.group(1)
        if m := EXPECT_RE.search(line):
            expected.add((number, m.group(1)))
    return spec, expected


def run_tidy(tidy: str, source: Path, check: str, plugin: str | None) -> set[tuple[int, str]]:
    cmd = [tidy, "--quiet"]
    config_file = None
    if (QUERY_DIR / f"{check}.yaml").exists():
        config_file = Path(tempfile.mkstemp(suffix=".clang-tidy")[1])
        config_file.write_text(build_config.render(build_config.load_entries([check])))
        cmd += ["--experimental-custom-checks", f"--config-file={config_file}"]
    elif (CONFIG_DIR / f"{check}.yaml").exists():
        # Rung 1-2 of the ladder: an existing check, enabled or configured.
        cmd += [f"--config-file={CONFIG_DIR / f'{check}.yaml'}"]
    if plugin:
        cmd += [f"-load={plugin}", f"--checks=wrocpp-{check}"]
    cmd += [str(source), "--", CXX_STD]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if config_file:
        config_file.unlink()
    found = set()
    for line in result.stdout.splitlines():
        if (m := DIAG_RE.match(line)) and Path(m.group("file")).resolve() == source.resolve():
            for name in m.group("name").split(","):
                found.add((int(m.group("line")), name))
    if result.returncode not in (0, 1) or ("error:" in result.stderr and not found):
        sys.stderr.write(result.stderr)
    return found


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--clang-tidy", default="clang-tidy")
    p.add_argument("--plugin", help="path to the built wrocpp clang-tidy plugin")
    p.add_argument("checks", nargs="*")
    args = p.parse_args()

    failures = []
    known = specs()
    tested = set()
    for source in sorted(TEST_DIR.glob("*/*.cpp")):
        check = source.parent.name
        if args.checks and check not in args.checks:
            continue
        spec, expected = expectations(source)
        if spec not in known:
            failures.append(f"{source}: names spec {spec!r}, which has no docs/checks/{spec}.md")
            continue
        tested.add(spec)
        runs_without_plugin = (QUERY_DIR / f"{check}.yaml").exists() or (CONFIG_DIR / f"{check}.yaml").exists()
        if not runs_without_plugin and not args.plugin:
            print(f"SKIP  {source.relative_to(ROOT)} (plugin check, no --plugin)")
            continue
        found = run_tidy(args.clang_tidy, source, check, args.plugin)
        # The query form reports custom-wrocpp-X; the plugin form wrocpp-X.
        norm = lambda s: {(n, name.removeprefix("custom-")) for n, name in s}  # noqa: E731
        missing = norm(expected) - norm(found)
        extra = norm(found) - norm(expected)
        status = "PASS" if not missing and not extra else "FAIL"
        print(f"{status}  {source.relative_to(ROOT)}")
        for n, name in sorted(missing):
            failures.append(f"{source}:{n}: expected {name}, not reported")
        for n, name in sorted(extra):
            failures.append(f"{source}:{n}: unexpected {name}")

    if not args.checks:
        for spec in sorted(known - tested):
            failures.append(f"docs/checks/{spec}.md has no tests under tests/{spec}/")

    for f in failures:
        print(f"  {f}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
