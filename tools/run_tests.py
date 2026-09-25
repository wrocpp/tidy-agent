#!/usr/bin/env python3
"""Run every check against its test cases and compare with the expectations.

Each check has a directory tests/<check>/. Every .cpp in it is a translation
unit to run clang-tidy on; .hpp files next to it are included by those units.
A test file's first line is `// spec: <check>`, naming the spec in
docs/checks/<check>.md that it satisfies. A line that must be flagged ends
with `expect: <diagnostic-name>` (in a comment); every other line in the
directory must not be flagged. The runner fails when:

- a flagged line is missing, or an unexpected line is flagged;
- a spec has no tests, or a test names a spec that does not exist.

Query checks run with --experimental-custom-checks and a config generated from
checks/query/<check>.yaml; config recipes use checks/config/<check>.yaml;
plugin checks are loaded with -load when --plugin is given.

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
EXPECT_RE = re.compile(r"expect:\s*(\S+)\s*$")
DIAG_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):\d+: warning: .* \[(?P<name>[\w.,-]+)\]$")
CXX_STD = "-std=c++23"

Finding = tuple[str, int, str]  # file name within the test directory, line, check


def specs() -> set[str]:
    """Specs that must have tests: all but README and those marked blocked."""
    return {p.stem for p in SPEC_DIR.glob("*.md")
            if p.stem != "README" and "\nStatus: blocked" not in p.read_text()}


def read_expectations(directory: Path) -> tuple[set[str], set[Finding]]:
    named_specs, expected = set(), set()
    for path in sorted([*directory.glob("*.cpp"), *directory.glob("*.hpp")]):
        for number, line in enumerate(path.read_text().splitlines(), start=1):
            if number == 1 and (m := SPEC_RE.match(line)):
                named_specs.add(m.group(1))
            if "//" in line and (m := EXPECT_RE.search(line)):
                expected.add((path.name, number, m.group(1)))
    return named_specs, expected


def run_tidy(tidy: str, source: Path, check: str, plugin: str | None) -> set[Finding]:
    cmd = [tidy, "--quiet", f"--header-filter={re.escape(str(source.parent))}/.*"]
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
    cmd += [str(source), "--", CXX_STD, f"-I{source.parent}"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if config_file:
        config_file.unlink()
    found = set()
    for line in result.stdout.splitlines():
        m = DIAG_RE.match(line)
        if m and Path(m.group("file")).resolve().parent == source.parent.resolve():
            # Compiler warnings (clang-diagnostic-*) are not the check's output.
            for name in (n for n in m.group("name").split(",") if not n.startswith("clang-diagnostic-")):
                found.add((Path(m.group("file")).name, int(m.group("line")), name))
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
    for directory in sorted(d for d in TEST_DIR.iterdir() if d.is_dir()):
        check = directory.name
        if args.checks and check not in args.checks:
            continue
        named_specs, expected = read_expectations(directory)
        unknown = named_specs - known
        if unknown or not named_specs:
            failures.append(f"{directory}: names spec(s) {sorted(unknown) or 'none'} without docs/checks/<spec>.md")
            continue
        tested |= named_specs
        runs_without_plugin = (QUERY_DIR / f"{check}.yaml").exists() or (CONFIG_DIR / f"{check}.yaml").exists()
        if not runs_without_plugin and not args.plugin:
            print(f"SKIP  tests/{check} (plugin check, no --plugin)")
            continue
        found = set()
        for source in sorted(directory.glob("*.cpp")):
            found |= run_tidy(args.clang_tidy, source, check, args.plugin)
        # The query form reports custom-wrocpp-X; the plugin form wrocpp-X.
        norm = lambda s: {(f, n, name.removeprefix("custom-")) for f, n, name in s}  # noqa: E731
        missing = norm(expected) - norm(found)
        extra = norm(found) - norm(expected)
        print(f"{'PASS' if not missing and not extra else 'FAIL'}  tests/{check}")
        failures += [f"tests/{check}/{f}:{n}: expected {name}, not reported" for f, n, name in sorted(missing)]
        failures += [f"tests/{check}/{f}:{n}: unexpected {name}" for f, n, name in sorted(extra)]

    if not args.checks:
        failures += [f"docs/checks/{spec}.md has no tests under tests/{spec}/" for spec in sorted(known - tested)]

    for f in failures:
        print(f"  {f}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
