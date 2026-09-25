#!/usr/bin/env python3
"""Collect evidence of recurring agent mistakes, as JSON on stdout.

Signals:
  fixes      C++ hunks from commits that look like review fixes
  findings   counts from .tidy-agent/findings.jsonl (written by the hook)
  corrections  (--transcripts only) user turns that correct the agent in local
               Claude Code transcripts for this project; stays on this machine

Usage: collect_signals.py [--since REV] [--max-commits N] [--transcripts]
"""
from __future__ import annotations

import argparse
import collections
import json
import re
import subprocess
import sys
from pathlib import Path

FIX_RE = re.compile(r"\b(fix|fixes|fixed|review|address|per review|follow-?up|nit|revert)\b", re.I)
CORRECTION_RE = re.compile(r"\b(no[,.]|don't|do not|instead|wrong|why did you|stop|never)\b", re.I)
CXX_GLOBS = ["*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.hh", "*.ipp"]
MAX_HUNK_LINES = 60


def git(*args: str) -> str:
    return subprocess.run(["git", *args], capture_output=True, text=True, check=True).stdout


def fix_commits(since: str | None, limit: int) -> list[dict]:
    rev = f"{since}..HEAD" if since else "HEAD"
    log = git("log", f"-{limit}", "--format=%H%x1f%s%x1f%b%x1e", rev)
    out = []
    for record in filter(None, (r.strip() for r in log.split("\x1e"))):
        sha, subject, body = (record.split("\x1f") + ["", ""])[:3]
        if not FIX_RE.search(subject + " " + body):
            continue
        diff = git("show", "--format=", "--unified=2", sha, "--", *CXX_GLOBS)
        if not diff.strip():
            continue
        lines = diff.splitlines()
        out.append({
            "sha": sha[:12],
            "subject": subject,
            "diff": "\n".join(lines[:MAX_HUNK_LINES]),
            "truncated": len(lines) > MAX_HUNK_LINES,
        })
    return out


def findings(root: Path) -> dict:
    log = root / ".tidy-agent" / "findings.jsonl"
    if not log.exists():
        return {}
    counts = collections.Counter()
    for line in log.read_text().splitlines():
        try:
            counts[json.loads(line)["check"]] += 1
        except (ValueError, KeyError):
            continue
    return dict(counts.most_common())


def transcript_dir(root: Path) -> Path:
    # Claude Code stores sessions under <config>/projects/<cwd with / replaced by ->.
    key = str(root).replace("/", "-")
    for config in (Path.home() / ".claude", Path.home() / ".claude-scudo"):
        candidate = config / "projects" / key
        if candidate.exists():
            return candidate
    return Path("/nonexistent")


def corrections(root: Path, limit: int = 200) -> list[dict]:
    out = []
    for session in sorted(transcript_dir(root).glob("*.jsonl")):
        last_edit = None
        for raw in session.read_text(errors="replace").splitlines():
            try:
                event = json.loads(raw)
            except ValueError:
                continue
            message = event.get("message") or {}
            content = message.get("content")
            if event.get("type") == "assistant" and isinstance(content, list):
                for block in content:
                    if block.get("type") == "tool_use" and block.get("name") in ("Edit", "Write"):
                        last_edit = (block.get("input") or {}).get("file_path")
            if event.get("type") == "user" and isinstance(content, str) and CORRECTION_RE.search(content):
                out.append({"session": session.stem, "after_edit_of": last_edit, "text": content[:500]})
                if len(out) >= limit:
                    return out
    return out


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--since")
    p.add_argument("--max-commits", type=int, default=400)
    p.add_argument("--transcripts", action="store_true",
                   help="also read local session transcripts (ADR-0004: local only)")
    args = p.parse_args()
    root = Path(git("rev-parse", "--show-toplevel").strip())
    result = {"fixes": fix_commits(args.since, args.max_commits), "findings": findings(root)}
    if args.transcripts:
        result["corrections"] = corrections(root)
    json.dump(result, sys.stdout, indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())
