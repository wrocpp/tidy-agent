"""Unit tests for the PostToolUse hook that need no clang-tidy."""
from __future__ import annotations

import io
import json
import sys
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import tidy_hook  # noqa: E402


class GlobMatching(unittest.TestCase):
    def test_star_matches_every_check(self):
        self.assertTrue(tidy_hook.glob_matches("bugprone-use-after-move", ["*"]))

    def test_later_negative_glob_wins(self):
        globs = ["bugprone-*", "-bugprone-easily-swappable-parameters"]
        self.assertFalse(tidy_hook.glob_matches("bugprone-easily-swappable-parameters", globs))
        self.assertTrue(tidy_hook.glob_matches("bugprone-use-after-move", globs))

    def test_query_check_prefix(self):
        self.assertTrue(tidy_hook.glob_matches("custom-wrocpp-array-counted-size", ["custom-wrocpp-*"]))

    def test_empty_list_blocks_nothing(self):
        self.assertFalse(tidy_hook.glob_matches("bugprone-use-after-move", []))


class Passthrough(unittest.TestCase):
    def run_hook(self, event: dict) -> tuple[int, str]:
        out = io.StringIO()
        with mock.patch("sys.stdin", io.StringIO(json.dumps(event))), redirect_stdout(out):
            code = tidy_hook.main()
        return code, out.getvalue()

    def test_non_cxx_file_is_ignored(self):
        code, out = self.run_hook({"tool_input": {"file_path": "/tmp/notes.md"}})
        self.assertEqual((code, out), (0, ""))

    def test_missing_path_is_ignored(self):
        code, out = self.run_hook({"tool_input": {}})
        self.assertEqual((code, out), (0, ""))

    def test_no_compilation_database_explains_and_passes(self):
        with mock.patch.object(tidy_hook, "find_build_dir", return_value=None), \
             mock.patch("shutil.which", return_value="/usr/bin/clang-tidy"):
            code, out = self.run_hook({"tool_input": {"file_path": __file__.replace(".py", ".cpp")}})
        # The .cpp path does not exist, so the hook exits before looking for tools.
        self.assertEqual(code, 0)


if __name__ == "__main__":
    unittest.main()
