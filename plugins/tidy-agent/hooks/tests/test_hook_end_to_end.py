"""End-to-end: the hook runs a real clang-tidy on a scratch project.

Skipped unless TIDY_AGENT_CLANG_TIDY points at clang-tidy 22 or later.
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HOOK = Path(__file__).resolve().parent.parent / "tidy_hook.py"
ROOT = Path(__file__).resolve().parents[4]
TIDY = os.environ.get("TIDY_AGENT_CLANG_TIDY")

SOURCE = """#include <array>
const std::array<int, 3> table{1, 2, 3};
int first() { return table[0]; }
"""


def project(warnings_as_errors: str) -> Path:
    root = Path(tempfile.mkdtemp(prefix="tidy-agent-e2e-"))
    subprocess.run(["git", "init", "-q"], cwd=root, check=True)
    sys.path.insert(0, str(ROOT / "tools"))
    import build_config

    config = build_config.render(build_config.load_entries(["array-counted-size"]))
    config += f"WarningsAsErrors: '{warnings_as_errors}'\n"
    (root / ".clang-tidy").write_text(config)
    (root / "table.cpp").write_text(SOURCE)
    (root / "build").mkdir()
    # The compiler next to clang-tidy carries the platform's include setup
    # (Homebrew's clang++ knows the macOS SDK; a bare `c++` driver does not).
    sibling = Path(TIDY).parent / "clang++"
    compiler = str(sibling) if sibling.exists() else "clang++"
    (root / "build" / "compile_commands.json").write_text(json.dumps([{
        "directory": str(root),
        "file": str(root / "table.cpp"),
        "arguments": [compiler, "-std=c++20", "-c", "table.cpp"],
    }]))
    return root


def run_hook(root: Path) -> subprocess.CompletedProcess:
    event = {"tool_name": "Write", "tool_input": {"file_path": str(root / "table.cpp")}}
    env = {**os.environ, "TIDY_AGENT_CLANG_TIDY": TIDY or ""}
    return subprocess.run([sys.executable, str(HOOK)], input=json.dumps(event),
                          capture_output=True, text=True, env=env)


@unittest.skipUnless(TIDY, "set TIDY_AGENT_CLANG_TIDY to run the end-to-end hook tests")
class EndToEnd(unittest.TestCase):
    def test_warnings_as_errors_blocks_and_logs(self):
        root = project("custom-wrocpp-*")
        result = run_hook(root)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("custom-wrocpp-array-counted-size", result.stderr)
        log = (root / ".tidy-agent" / "findings.jsonl").read_text().splitlines()
        self.assertEqual(json.loads(log[0])["check"], "custom-wrocpp-array-counted-size")

    def test_other_findings_go_back_as_context(self):
        root = project("")
        result = run_hook(root)
        self.assertEqual(result.returncode, 0, result.stderr)
        context = json.loads(result.stdout)["hookSpecificOutput"]["additionalContext"]
        self.assertIn("custom-wrocpp-array-counted-size", context)


if __name__ == "__main__":
    unittest.main()
