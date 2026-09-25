#!/usr/bin/env python3
"""PostToolUse hook: run clang-tidy on the C or C++ file the agent just edited.

Reads the hook event from stdin. For a C/C++ source or header it runs
clang-tidy with the project's own .clang-tidy and compile_commands.json, then:

- findings from checks the project lists in WarningsAsErrors: exit 2 with the
  diagnostics on stderr, so the agent must fix them before continuing;
- any other findings: returned as additionalContext for the agent's next turn;
- every finding: appended to .tidy-agent/findings.jsonl for the
  mistake-to-check skill.

It never fails the tool call for its own problems (no clang-tidy, no
compilation database): it exits 0 and says why in additionalContext once.

Environment:
  TIDY_AGENT_CLANG_TIDY   clang-tidy binary (default: clang-tidy on PATH)
  TIDY_AGENT_PLUGIN       path to the wrocpp plugin library, loaded with -load
  TIDY_AGENT_BUILD_DIR    directory holding compile_commands.json (default: search)
"""
from __future__ import annotations

import datetime as _dt
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

CXX_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".c++", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".tpp"}
BUILD_DIR_CANDIDATES = ("build", "build-host", "out/build", "cmake-build-debug", ".")
DIAG_RE = re.compile(
    r"^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+): (?P<level>warning|error): (?P<msg>.*?) \[(?P<checks>[\w.,-]+)\]$"
)
TIDY_TIMEOUT_S = 110  # below the hook timeout in hooks.json


def emit_context(text: str) -> None:
    print(json.dumps({"hookSpecificOutput": {"hookEventName": "PostToolUse", "additionalContext": text}}))


def find_project_root(start: Path) -> Path:
    for parent in [start, *start.parents]:
        if (parent / ".clang-tidy").exists() or (parent / ".git").exists():
            return parent
    return start


def find_build_dir(root: Path) -> Path | None:
    env = os.environ.get("TIDY_AGENT_BUILD_DIR")
    if env:
        return Path(env) if (Path(env) / "compile_commands.json").exists() else None
    for candidate in BUILD_DIR_CANDIDATES:
        if (root / candidate / "compile_commands.json").exists():
            return root / candidate
    matches = sorted(root.glob("build*/**/compile_commands.json"))
    return matches[0].parent if matches else None


def warnings_as_errors(tidy: str, source: Path) -> list[str]:
    """The project's WarningsAsErrors globs, as clang-tidy resolves them."""
    result = subprocess.run([tidy, "--dump-config", str(source)], capture_output=True, text=True)
    m = re.search(r"^WarningsAsErrors:\s*'?([^'\n]*)'?", result.stdout, re.MULTILINE)
    return [g.strip() for g in m.group(1).split(",") if g.strip()] if m else []


def glob_matches(check: str, globs: list[str]) -> bool:
    verdict = False
    for g in globs:
        negative = g.startswith("-")
        pattern = "^" + re.escape(g.lstrip("-")).replace(r"\*", ".*") + "$"
        if re.match(pattern, check):
            verdict = not negative
    return verdict


def log_findings(root: Path, findings: list[dict]) -> None:
    log_dir = root / ".tidy-agent"
    log_dir.mkdir(exist_ok=True)
    stamp = _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds")
    with (log_dir / "findings.jsonl").open("a") as f:
        for finding in findings:
            f.write(json.dumps({"time": stamp, **finding}) + "\n")


def main() -> int:
    event = json.load(sys.stdin)
    path = (event.get("tool_input") or {}).get("file_path")
    if not path or Path(path).suffix.lower() not in CXX_SUFFIXES:
        return 0
    source = Path(path).resolve()
    if not source.exists():
        return 0

    tidy = os.environ.get("TIDY_AGENT_CLANG_TIDY") or shutil.which("clang-tidy")
    root = find_project_root(source.parent)
    build_dir = find_build_dir(root)
    if not tidy or not build_dir:
        missing = "clang-tidy" if not tidy else "compile_commands.json"
        emit_context(f"tidy-agent: skipped clang-tidy on {source.name}, no {missing} found.")
        return 0

    cmd = [tidy, "--quiet", f"-p={build_dir}", "--experimental-custom-checks"]
    plugin = os.environ.get("TIDY_AGENT_PLUGIN")
    if plugin:
        cmd.append(f"-load={plugin}")
    cmd.append(str(source))
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=TIDY_TIMEOUT_S, cwd=root)
    except subprocess.TimeoutExpired:
        emit_context(f"tidy-agent: clang-tidy timed out on {source.name}.")
        return 0

    hard_globs = warnings_as_errors(tidy, source)
    findings, blocking, advisory = [], [], []
    for line in result.stdout.splitlines():
        m = DIAG_RE.match(line)
        if not m or Path(m.group("file")).resolve() != source:
            continue
        for check in m.group("checks").split(","):
            finding = {
                "file": str(source.relative_to(root)),
                "line": int(m.group("line")),
                "check": check,
                "message": m.group("msg"),
            }
            findings.append(finding)
            text = f"{finding['file']}:{finding['line']}: {finding['message']} [{check}]"
            (blocking if glob_matches(check, hard_globs) else advisory).append(text)

    if findings:
        log_findings(root, findings)
    if blocking:
        sys.stderr.write("clang-tidy findings that this project treats as errors; fix them before continuing:\n")
        sys.stderr.write("\n".join(blocking) + "\n")
        return 2
    if advisory:
        emit_context("clang-tidy findings in the file you just edited:\n" + "\n".join(advisory))
    return 0


if __name__ == "__main__":
    sys.exit(main())
