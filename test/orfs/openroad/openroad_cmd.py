#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Generic CLI for OpenROAD commands.
#
# Routes to native cc_binary when available (fast rebuild, direct gdb).
# Falls back to monolithic openroad + generated TCL for other commands.
#
# Usage:
#   openroad_cmd detailed_placement --read_db in.odb --write_db out.odb -max_displacement 10
#   openroad_cmd global_route --read_db in.odb --write_db out.odb -congestion_iterations 50

import os
import subprocess
import sys
import tempfile

# Commands that have native cc_binary implementations.
# As more commands get broken out, add them here.
NATIVE_COMMANDS = {
    "detailed_placement",
    "check_placement",
    "filler_placement",
    "optimize_mirroring",
    "detailed_route",
    "global_route",
    "check_antennas",
    "extract_parasitics",
    "tapcell",
    "repair_design",
    "global_placement",
    "clock_tree_synthesis",
    "density_fill",
    "place_pins",
    "macro_placement",
    "init_floorplan",
    "analyze_power_grid",
}

# I/O flags (double-dash) that we extract from the argument list.
# Everything else is passed verbatim as TCL/command flags.
IO_FLAGS_WITH_VALUE = {
    "--read_db",
    "--write_db",
    "--read_lef",
    "--read_def",
    "--write_def",
}


def resolve_path(path):
    """Resolve a path relative to the user's working directory.

    When invoked via 'bazelisk run', cwd is the execroot, not where
    the user typed the command. BUILD_WORKING_DIRECTORY is set by
    Bazel to the original directory.
    """
    if os.path.isabs(path):
        return path
    base = os.environ.get("BUILD_WORKING_DIRECTORY", os.getcwd())
    return os.path.join(base, path)


def find_binary(name):
    """Find a binary in the runfiles tree."""
    # Try runfiles location first (bazelisk run)
    runfiles = os.environ.get("RUNFILES_DIR", "")
    if runfiles:
        # Try common patterns
        for prefix in ["_main/test/orfs/openroad/", "test/orfs/openroad/"]:
            candidate = os.path.join(runfiles, prefix + name)
            if os.path.isfile(candidate):
                return candidate

    # Try relative to this script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidate = os.path.join(script_dir, name)
    if os.path.isfile(candidate):
        return candidate

    return None


def parse_io_args(args):
    """Separate I/O flags from command flags.

    Returns (io_dict, command_flags) where io_dict maps flag names to
    lists of values (--read_lef can be repeated).
    """
    io = {}
    cmd_flags = []
    i = 0
    while i < len(args):
        if args[i] in IO_FLAGS_WITH_VALUE:
            flag = args[i]
            if i + 1 >= len(args):
                raise ValueError(f"{flag} requires a value")
            value = resolve_path(args[i + 1])
            io.setdefault(flag, []).append(value)
            i += 2
        else:
            cmd_flags.append(args[i])
            i += 1
    return io, cmd_flags


def generate_tcl(command, io, cmd_flags):
    """Generate a TCL script for the given command."""
    lines = []

    # Read input
    if "--read_db" in io:
        for path in io["--read_db"]:
            lines.append(f"read_db {path}")
    else:
        for path in io.get("--read_lef", []):
            lines.append(f"read_lef {path}")
        if "--read_def" in io:
            for path in io["--read_def"]:
                lines.append(f"read_def {path}")

    # Run command with passthrough flags
    flags_str = " ".join(cmd_flags)
    lines.append(f"{command} {flags_str}".strip())

    # Write output
    if "--write_db" in io:
        for path in io["--write_db"]:
            lines.append(f"write_db {path}")
    if "--write_def" in io:
        for path in io["--write_def"]:
            lines.append(f"write_def {path}")

    return "\n".join(lines) + "\n"


def run_native(command, io, cmd_flags):
    """Run a native cc_binary for the command."""
    binary = find_binary(f"openroad-{command}")
    if binary is None:
        return None  # Fall back to TCL

    argv = [binary]
    for flag, values in io.items():
        for v in values:
            argv.extend([flag, v])
    argv.extend(cmd_flags)

    result = subprocess.run(argv)
    return result.returncode


def run_tcl_fallback(command, io, cmd_flags):
    """Generate TCL and run via monolithic openroad."""
    tcl = generate_tcl(command, io, cmd_flags)

    # Write TCL to temp file (preserved for debugging)
    fd, tcl_path = tempfile.mkstemp(prefix=f"openroad_{command}_", suffix=".tcl")
    with os.fdopen(fd, "w") as f:
        f.write(tcl)
    print(f"Generated TCL: {tcl_path}", file=sys.stderr)

    # Find openroad binary
    openroad = find_binary("openroad")
    if openroad is None:
        # Try PATH
        openroad = "openroad"

    result = subprocess.run([openroad, "-exit", tcl_path])
    return result.returncode


def main():
    if len(sys.argv) < 2:
        print(
            "Usage: openroad_cmd COMMAND [--read_db FILE] [--write_db FILE] [flags...]",
            file=sys.stderr,
        )
        print(
            "\nExamples:",
            file=sys.stderr,
        )
        print(
            "  openroad_cmd detailed_placement --read_db in.odb --write_db out.odb",
            file=sys.stderr,
        )
        print(
            "  openroad_cmd global_route --read_db in.odb --write_db out.odb",
            file=sys.stderr,
        )
        sys.exit(1)

    command = sys.argv[1]
    io, cmd_flags = parse_io_args(sys.argv[2:])

    # Validate: must have some input
    if not io.get("--read_db") and not io.get("--read_lef"):
        print(
            f"Error: specify --read_db or --read_lef/--read_def",
            file=sys.stderr,
        )
        sys.exit(1)

    # Try native binary first
    if command in NATIVE_COMMANDS:
        rc = run_native(command, io, cmd_flags)
        if rc is not None:
            sys.exit(rc)

    # Fall back to monolithic openroad + TCL
    sys.exit(run_tcl_fallback(command, io, cmd_flags))


if __name__ == "__main__":
    main()
