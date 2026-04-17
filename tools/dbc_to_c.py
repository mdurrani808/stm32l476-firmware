#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path


def c_escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def build_filtered_lines(text: str) -> tuple[list[str], int]:
    filtered: list[str] = []
    in_message = False
    motorola_warnings = 0

    for raw in text.splitlines():
        line = raw.rstrip("\r\n")
        stripped = line.lstrip(" \t")

        if stripped.startswith("BO_ "):
            in_message = True
            filtered.append(stripped)
            continue

        if stripped.startswith("SG_ "):
            if not in_message:
                continue
            if "@0" in stripped:
                motorola_warnings += 1
            filtered.append("\t" + stripped)
            continue

    return filtered, motorola_warnings


def emit_c_file(inp: Path, out: Path, filtered: list[str]) -> None:
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
        lines.append(f'"{c_escape(l)}\\n"\n')
    lines.append(";\n\n")
    lines.append(f"const size_t g_can_dbc_text_len = {len(filtered)}U;\n")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("".join(lines), encoding="utf-8", newline="\n")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert a DBC into the on-target BO_/SG_ text blob used by the firmware.",
    )
    parser.add_argument("input_dbc", help="Path to the source .dbc file")
    parser.add_argument(
        "output_c",
        nargs="?",
        default="App/dbc/can_dbc_text.c",
        help="Destination C file (default: App/dbc/can_dbc_text.c)",
    )
    parser.add_argument(
        "--install-dbc",
        default="App/dbc/file.dbc",
        help="Optional destination to copy the raw .dbc into the repo (default: App/dbc/file.dbc)",
    )
    parser.add_argument(
        "--no-install-dbc",
        action="store_true",
        help="Do not copy the raw .dbc into the repo before generating the C blob",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)

    inp = Path(args.input_dbc).expanduser().resolve()
    out = Path(args.output_c).expanduser()
    install_dbc = Path(args.install_dbc).expanduser()

    if not inp.exists():
        print(f"Error: input file does not exist: {inp}")
        return 2
    if inp.is_dir():
        print(f"Error: input path is a directory, not a file: {inp}")
        return 2

    try:
        text = inp.read_text(encoding="utf-8", errors="strict")
    except UnicodeDecodeError as e:
        print(f"Error: failed to read as UTF-8: {inp}")
        print(f"  {e}")
        return 2

    if not args.no_install_dbc:
        install_dbc.parent.mkdir(parents=True, exist_ok=True)
        if inp.resolve() != install_dbc.resolve():
            shutil.copyfile(inp, install_dbc)
            print(f"Installed raw DBC to: {install_dbc}")
        else:
            print(f"Raw DBC already in place: {install_dbc}")
        source_for_comment = install_dbc
    else:
        source_for_comment = inp

    filtered, motorola_warnings = build_filtered_lines(text)
    emit_c_file(source_for_comment, out, filtered)

    if motorola_warnings:
        print(f"Warning: found {motorola_warnings} SG_ lines containing @0 (Motorola/big-endian).")
        print("         The current firmware bit helpers are still little-endian only (@1).")

    print(f"Wrote: {out}")
    print(f"Kept {len(filtered)} BO_/SG_ lines.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
