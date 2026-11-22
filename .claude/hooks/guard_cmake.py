#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""PreToolUse(Bash): refuse CMake configures whose build tree would land inside the repo.

ADR 0010: all build output lives in /tmp/qkrellm. Presets already do that, so
`cmake --preset X` is always fine; `cmake -B build`, `cmake .`, `cmake ..` from inside the
repo are not.
"""
import json
import os
import pathlib
import re
import shlex
import sys

data = json.load(sys.stdin)
command = (data.get("tool_input") or {}).get("command") or ""
if "cmake" not in command:
    sys.exit(0)

project = pathlib.Path(
    os.environ.get("CLAUDE_PROJECT_DIR") or pathlib.Path(__file__).resolve().parents[2]
).resolve()
cwd = pathlib.Path(data.get("cwd") or os.getcwd())

NON_CONFIGURE = {"--build", "--install", "--workflow", "-P", "-E", "--version", "--help",
                 "--list-presets", "--find-package", "--open"}


def inside_repo(p: pathlib.Path) -> bool:
    try:
        p.resolve().relative_to(project)
        return True
    except ValueError:
        return False


def deny(reason: str) -> None:
    print(json.dumps({
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": "deny",
            "permissionDecisionReason": reason,
        }
    }))
    sys.exit(0)


# Track `cd` within the command line so `cd repo && cmake -B build` is judged correctly.
for segment in re.split(r"&&|\|\||;|\|", command):
    try:
        tokens = shlex.split(segment)
    except ValueError:
        tokens = segment.split()
    if not tokens:
        continue
    if tokens[0] == "cd" and len(tokens) > 1:
        target = pathlib.Path(os.path.expanduser(tokens[1]))
        cwd = target if target.is_absolute() else cwd / target
        continue
    if pathlib.Path(tokens[0]).name != "cmake":
        continue
    args = tokens[1:]
    if any(a in NON_CONFIGURE for a in args):
        continue
    binary_dir = None
    for i, a in enumerate(args):
        if a == "-B" and i + 1 < len(args):
            binary_dir = args[i + 1]
        elif a.startswith("-B") and len(a) > 2:
            binary_dir = a[2:]
    has_preset = any(a == "--preset" or a.startswith("--preset=") for a in args)
    if binary_dir is None:
        if has_preset:
            continue  # presets put the tree in /tmp/qkrellm/build/<preset>
        build_path = cwd  # classic `cmake <src>` configures into the current directory
    else:
        p = pathlib.Path(os.path.expanduser(binary_dir))
        build_path = p if p.is_absolute() else cwd / p
    if inside_repo(build_path):
        deny(
            f"Blocked: this cmake call would create a build tree inside the repository "
            f"({build_path}). ADR 0010: use the presets (e.g. `cmake --preset dev`), which "
            f"build in /tmp/qkrellm/build/<preset>, or pass -B /tmp/qkrellm/<dir>."
        )
