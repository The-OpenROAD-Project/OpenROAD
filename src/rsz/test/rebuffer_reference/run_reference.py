#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Compare rsz buffering on buffer_varying_lengths with a reference.

The reference is the best solution per wire length found by a local search
timed with OpenSTA (see search.tcl). By default this runs the test's
tight-timing step with the given openroad binary and compares it with the
committed reference. With --search it regenerates the reference first.
"""

import argparse
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TEST_DIR = os.path.dirname(HERE)


def run_openroad(openroad, script, threads, env_vars):
    env = dict(os.environ, **env_vars)
    cmd = [openroad, "-exit", "-no_init", "-no_splash", "-threads", str(threads)]
    cmd.append(os.path.join(HERE, script))
    result = subprocess.run(
        cmd, cwd=TEST_DIR, env=env, capture_output=True, text=True, check=False
    )
    if result.returncode != 0:
        sys.stderr.write(result.stdout + result.stderr)
        sys.exit(f"{script} failed with exit code {result.returncode}")


def parse_tcl_list(text):
    """Split one solution line: index delay driver {{cell x} ...}."""
    head, _, chain = text.partition("{")
    index, delay, driver = head.split()
    cells = []
    for item in chain.replace("}", " ").replace("{", " ").split()[::2]:
        cells.append(item)
    return int(index), float(delay), driver, cells


def read_solutions(file_name):
    solutions = {}
    with open(file_name) as solution_file:
        for line in solution_file:
            if line.strip():
                index, delay, driver, cells = parse_tcl_list(line)
                solutions[index] = (delay, driver, cells)
    return solutions


def short(cell):
    return cell.replace("BUF_", "")


def describe(driver, cells):
    return f"{short(driver)}: " + (" ".join(short(cell) for cell in cells) or "-")


def compare(rsz, reference):
    print(
        " Length |   rsz (ps) solution          | ref (ps) solution          |  gap (ps)"
    )
    gaps = []
    for index in sorted(reference):
        rsz_delay, rsz_driver, rsz_cells = rsz[index]
        ref_delay, ref_driver, ref_cells = reference[index]
        gap = rsz_delay - ref_delay
        gaps.append(gap)
        print(
            f" {index * 25:6d} | {rsz_delay:7.1f} {describe(rsz_driver, rsz_cells):22s}"
            f" | {ref_delay:7.1f} {describe(ref_driver, ref_cells):18s} | {gap:6.1f}"
        )
    worst = max(range(len(gaps)), key=lambda k: gaps[k])
    print(
        f"gap vs reference: mean {sum(gaps) / len(gaps):.2f} ps,"
        f" max {gaps[worst]:.2f} ps at {(worst + 1) * 25} um,"
        f" {sum(gap > 1.0 for gap in gaps)} of {len(gaps)} lengths over 1 ps"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--openroad", required=True, help="openroad binary")
    parser.add_argument(
        "--size-driver",
        action="store_true",
        help="driver sizing (buffer_varying_lengths_size_driver)",
    )
    parser.add_argument(
        "--search", action="store_true", help="regenerate the reference first"
    )
    parser.add_argument("--threads", type=int, default=16)
    parser.add_argument(
        "--work-dir", help="directory for intermediate files (default: temporary)"
    )
    args = parser.parse_args()

    case = "size_driver" if args.size_driver else "fixed"
    reference_file = os.path.join(HERE, f"reference_{case}.txt")
    openroad = os.path.abspath(args.openroad)
    work_dir = args.work_dir or tempfile.mkdtemp(prefix="rebuffer_reference_")
    os.makedirs(work_dir, exist_ok=True)
    size_driver = "1" if args.size_driver else "0"

    rsz_file = os.path.join(work_dir, f"rsz_{case}.txt")
    run_openroad(
        openroad,
        "rsz_solution.tcl",
        args.threads,
        {"REF_SIZE_DRIVER": size_driver, "REF_OUT": rsz_file},
    )

    if args.search:
        sweep_file = os.path.join(work_dir, f"sweep_{case}.txt")
        run_openroad(
            openroad,
            "sweep.tcl",
            args.threads,
            {"REF_SIZE_DRIVER": size_driver, "REF_OUT": sweep_file},
        )
        run_openroad(
            openroad,
            "search.tcl",
            args.threads,
            {
                "REF_SIZE_DRIVER": size_driver,
                "REF_RSZ": rsz_file,
                "REF_SWEEP": sweep_file,
                "REF_OUT": reference_file,
            },
        )

    compare(read_solutions(rsz_file), read_solutions(reference_file))


if __name__ == "__main__":
    main()
