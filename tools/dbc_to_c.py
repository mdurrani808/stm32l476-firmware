#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


def c_escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace("\"", "\\\"")


def usage() -> int:
    print("Usage: dbc_to_c.py <input.dbc> <output_can_dbc_text.c>")
    print("Example: python tools/dbc_to_c.py app/dbc/file.dbc app/dbc/can_dbc_text.c")
    return 2


def keep_line(line: str) -> bool:
    # Your firmware only recognizes BO_ and SG_ lines.
    # It trims leading spaces/tabs before matching.
    stripped = line.lstrip(" \t")
    return stripped.startswith("BO_ ") or stripped.startswith("SG_ ")


def main() -> int:
    if len(sys.argv) != 3:
        return usage()

    inp = Path(sys.argv[1]).expanduser()
    out = Path(sys.argv[2]).expanduser()

    if not inp.exists():
        print(f"Error: input file does not exist: {inp}")
        return 2
    if inp.is_dir():
        print(f"Error: input path is a directory, not a file: {inp}")
        return 2

    try:
        text_lines = inp.read_text(encoding="utf-8", errors="strict").splitlines()
    except UnicodeDecodeError as e:
        print(f"Error: failed to read as UTF-8: {inp}")
        print(f"  {e}")
        return 2

    # Filter down to only BO_/SG_ that your firmware uses.
    # Also: only keep SG_ lines after a BO_ (otherwise they are meaningless).
    filtered: list[str] = []
    in_message = False
    motorola_warnings = 0

    for raw in text_lines:
        line = raw.rstrip("\r\n")

        stripped = line.lstrip(" \t")
        if stripped.startswith("BO_ "):
            in_message = True
            filtered.append(stripped)  # normalize indentation
            continue

        if stripped.startswith("SG_ "):
            if not in_message:
                # Ignore stray SG_ before any BO_
                continue

            # Optional sanity warning: your firmware only implements little-endian bit order.
            # DBC uses @0 for Motorola/big-endian, @1 for Intel/little-endian.
            if "@0" in stripped:
                motorola_warnings += 1

            filtered.append("\t" + stripped)  # pretty-print under BO_
            continue

        # Ignore everything else

    if motorola_warnings:
        print(f"Warning: found {motorola_warnings} SG_ lines containing '@0' (Motorola/big-endian).")
        print("         Your firmware's extract/insert helpers are little-endian only (@1).")

    # Emit C file
    lines: list[str] = []
    lines.append("#include <stddef.h>\n\n")
    lines.append("/*\n")
    lines.append(" * AUTO-GENERATED FILE — DO NOT EDIT BY HAND.\n")
    lines.append(" *\n")
    lines.append(f" * Source: {inp.as_posix()}\n")
    lines.append(" * Generator: tools/dbc_to_c.py\n")
    lines.append(" *\n")
    lines.append(" * NOTE: This file intentionally contains only BO_ and SG_ lines,\n")
    lines.append(" * because the firmware DBC parser ignores all other DBC constructs.\n")
    lines.append(" */\n\n")

    lines.append("const char* g_can_dbc_text =\n")
    for l in filtered:
        lines.append(f"\"{c_escape(l)}\\n\"\n")
    lines.append(";\n\n")

    # If you ever want this, you can compute it here and keep it nonzero.
    lines.append("const size_t g_can_dbc_text_len = 0;\n")

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("".join(lines), encoding="utf-8", newline="\n")

    print(f"Wrote: {out}")
    print(f"Kept {len(filtered)} / {len(text_lines)} lines (BO_/SG_ only).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
