#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""PostToolUse(Edit|Write): clang-format C/C++ files; remind about ABI rules for include/qkrellm."""
import json
import os
import pathlib
import shutil
import subprocess
import sys

data = json.load(sys.stdin)
path_str = (data.get("tool_input") or {}).get("file_path") or ""
if not path_str:
    sys.exit(0)

project = pathlib.Path(os.environ.get("CLAUDE_PROJECT_DIR") or pathlib.Path(__file__).resolve().parents[2])
path = pathlib.Path(path_str).resolve()
try:
    rel = path.relative_to(project.resolve())
except ValueError:
    sys.exit(0)  # outside the repo: not ours

if path.suffix in {".c", ".h", ".cpp", ".hpp"} and path.is_file() and shutil.which("clang-format"):
    subprocess.run(["clang-format", "-i", str(path)], check=False)

if rel.parts[:2] == ("include", "qkrellm"):
    print(json.dumps({
        "hookSpecificOutput": {
            "hookEventName": "PostToolUse",
            "additionalContext": (
                f"You edited the PUBLIC plugin ABI ({rel}). Follow include/qkrellm/AGENTS.md: "
                "append-only structs, bump QK_ABI_VERSION_MINOR/MAJOR, update tests/abi layout "
                "asserts and docs/plugin-api.md (skill: abi-change; reviewer: abi-guardian)."
            ),
        }
    }))
