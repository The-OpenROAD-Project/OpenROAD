#!/usr/bin/env python3
"""
This runs the ISPD 18 & 19 routing benchmarks.  GNU Parallel is used for
parallelism over many designs.  This is used by Jenkins for nightly
OpenROAD testing.
"""

import argparse
import fnmatch
import multiprocessing
import os
import shutil
import stat
import subprocess
import sys
import textwrap

parser = argparse.ArgumentParser(
    prog="run-ispd",
    description="Run the ISPD routing benchamrks.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
parser.add_argument(
    "-d",
    "--dir",
    default=os.path.expanduser("~/ispd"),
    help="Root directory to run under " "(must have a tests subdir with benchmarks)",
)
parser.add_argument(
    "-t",
    "--tests",
    nargs="*",
    default="*",
    help="The tests to run. Matched to designs by glob",
)
parser.add_argument(
    "-j", "--jobs", default=4, type=int, help="Number of jobs to run concurrently"
)
parser.add_argument(
    "-p", "--program", default=shutil.which("openroad"), help="Path to openroad to test"
)
parser.add_argument(
    "-w",
    "--workspace",
    default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "results"),
    help="Workspace directory to create the run scripts and save output files",
)
args = parser.parse_args()

args.program = os.path.abspath(args.program)
args.dir = os.path.abspath(args.dir)

if args.program is None or not os.path.isfile(args.program):
    raise FileNotFoundError(f"openroad binary not found at '{args.program}'")

if not os.path.isdir(args.dir):
    raise FileNotFoundError(f"Benchmark root folder not found at '{args.dir}'")


def gen_files(work_dir, ispd_year, design, drv_min, drv_max):
    """host setup"""
    bench_dir = os.path.join(args.dir, "tests")
    if not os.path.exists(os.path.join(bench_dir, design)):
        raise Exception("Missing test {}".format(design))

    # TritonRoute setup
    verbose = 1
    threads = multiprocessing.cpu_count()

    design_dir = os.path.join(work_dir, design)
    os.makedirs(design_dir, exist_ok=True)

    print(f"Create openroad tcl script for {design}")
    script = f"""
            set_thread_count {threads}
            read_lef {bench_dir}/{design}/{design}.input.lef
            read_def {bench_dir}/{design}/{design}.input.def
            read_guides {bench_dir}/{design}/{design}.input.guide 
            detailed_route -output_maze {design_dir}/{design}.output.maze.log \\
                           -output_drc {design_dir}/{design}.output.drc.rpt \\
                           -verbose {verbose}
            write_def {design_dir}/{design}.output.def
            set drv_count [detailed_route_num_drvs]
            if {{ $drv_count > {drv_max} }} {{
              puts \"ERROR: Increase in number of violations from {drv_max} to $drv_count\"
              exit 1
            }} elseif {{ $drv_count < {drv_min} }} {{
              puts \"NOTICE: Decrease in number of violations from {drv_min} to $drv_count\"
              exit 2
            }}
    """
    with open(os.path.join(design_dir, "run.tcl"), "w") as tcl_file:
        print(textwrap.dedent(script), file=tcl_file)

    print(f"Create run shell script for {design}")
    run_sh = os.path.join(design_dir, "run.sh")
    script = f"""
            set -e -o pipefail
            echo Running {design}
            {args.program} -exit {design_dir}/run.tcl 2>&1 \\
                | tee {design_dir}/{design}.log
            cd '{bench_dir}/ispd{ispd_year}eval'
            ./ispd{ispd_year}eval \\
                -lef {bench_dir}/{design}/{design}.input.lef \\
                -def {design_dir}/{design}.output.def \\
                -guide {bench_dir}/{design}/{design}.input.guide \\
              | grep -v WARNING | grep -v ERROR
            echo
            """
    with open(run_sh, "w") as script_file:
        print(textwrap.dedent(script), file=script_file)
    file_st = os.stat(run_sh)
    os.chmod(run_sh, file_st.st_mode | stat.S_IXUSR | stat.S_IXGRP)


def test_enabled(design, patterns):
    """check if test is enabled"""
    for pattern in patterns:
        if fnmatch.fnmatch(design, pattern):
            return True
    return False


design_list_ispd18 = [
    ("ispd18_test1", 0, 0),
    ("ispd18_test2", 0, 0),
    ("ispd18_test3", 0, 0),
    ("ispd18_test4", 0, 0),
    ("ispd18_test5", 0, 0),
    ("ispd18_test6", 0, 0),
    ("ispd18_test7", 0, 0),
    ("ispd18_test8", 0, 0),
    ("ispd18_test9", 0, 0),
    ("ispd18_test10", 0, 0),
]
design_list_ispd19 = [
    ("ispd19_test1", 0, 0),
    ("ispd19_test2", 0, 0),
    ("ispd19_test3", 0, 0),
    ("ispd19_test4", 0, 0),
    ("ispd19_test5", 0, 0),
    ("ispd19_test6", 0, 0),
    ("ispd19_test7", 0, 0),
    ("ispd19_test8", 0, 0),
    ("ispd19_test9", 0, 0),
    ("ispd19_test10", 12, 25),
]

os.makedirs(args.workspace, exist_ok=True)
running_tests = set()
for design_name, drv_min, drv_max in design_list_ispd18:
    if test_enabled(design_name, args.tests):
        gen_files(args.workspace, 18, design_name, drv_min, drv_max)
        running_tests.add(design_name)


for design_name, drv_min, drv_max in design_list_ispd19:
    if test_enabled(design_name, args.tests):
        gen_files(args.workspace, 19, design_name, drv_min, drv_max)
        running_tests.add(design_name)

status = subprocess.run(
    [
        "parallel",
        "-j",
        str(args.jobs),
        "--halt",
        "never",
        "--joblog",
        f"{args.workspace}/ispd-parallel.log",
        "bash",
        os.path.join(args.workspace, "{}/run.sh"),
        ":::",
        *list(running_tests),
    ],
    check=True,
)

for design_name in running_tests:
    subprocess.run(
        [
            "tar",
            "czvf",
            f"{args.workspace}/{design_name}.tar.gz",
            f"{args.workspace}/{design_name}",
        ],
        check=True,
    )

print("=======================")
if status.returncode:
    print("Fail")
else:
    print("Success")

print("=======================")
sys.exit(status.returncode)
