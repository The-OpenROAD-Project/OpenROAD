#!/usr/bin/env python3

# This runs the ISPD 18 & 19 routing benchmarks.  GNU Parallel is used for
# parallelism over many designs.  This is used by Jenkins for nightly
# OpenROAD testing.

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
    description="Run the ISPD routing benchamrks",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
)
parser.add_argument(
    "-d",
    "--dir",
    default=os.path.expanduser('~/ispd'),
    help="Root directory to run under (must have a tests subdir with benchmarks)"
)
parser.add_argument(
    "-t",
    "--tests",
    nargs="*",
    default="*",
    help="The tests to run. Matched to designs by glob."
)
parser.add_argument(
    "-j",
    "--jobs",
    default=4,
    type=int,
    help="Number of jobs to run concurrently"
)
parser.add_argument(
    "-p",
    "--program",
    default=shutil.which('openroad'),
    help="Path to openroad to test"
)
args = parser.parse_args()

if args.program is None or not os.path.isfile(args.program):
    raise FileNotFoundError(f"openroad binary not found at '{args.program}'")

if not os.path.isdir(args.dir):
    raise FileNotFoundError(f"Benchmark root folder not found at '{args.dir}'")


def genFiles(run_dir, ispd_year, design, drv):
    # host setup
    bench_dir = os.path.join(args.dir, "tests")
    if not os.path.exists(os.path.join(bench_dir, design)):
        raise Exception("Missing test {}".format(design))

    # TritonRoute setup
    verbose = 1
    threads = multiprocessing.cpu_count()

    design_dir = os.path.join(run_dir, design)
    os.makedirs(design_dir, exist_ok=True)

    print(f"Create openroad tcl script for {design}")
    script = f"""
            set_thread_count {threads}
            read_lef {bench_dir}/{design}/{design}.input.lef
            read_def {bench_dir}/{design}/{design}.input.def
            detailed_route -guide {bench_dir}/{design}/{design}.input.guide \\
                           -output_guide {design}.output.guide.mod \\
                           -output_maze {design}.output.maze.log \\
                           -output_drc {design}.output.drc.rpt \\
                           -verbose {verbose}
            write_def {run_dir}/{design}/{design}.output.def
            set drv_count [detailed_route_num_drvs]
            if {{ $drv_count > {drv} }} {{
              puts \"ERROR: Increase in number of violations from {drv} to $drv_count\"
              exit 1
            }} elseif {{ $drv_count < {drv} }} {{
              puts \"NOTICE: Decrease in number of violations from {drv} to $drv_count\"
              exit 2
            }}
    """
    with open(os.path.join(design_dir, "run.tcl"), 'w') as f:
        print(textwrap.dedent(script), file=f)

    print(f"Create run shell script for {design}")
    run_sh = os.path.join(design_dir, "run.sh")
    script = f"""
            set -e
            cd {design_dir}
            echo Running {design}
            {args.program} -exit run.tcl | tee {design_dir}/run_{design}.log
            cd '{bench_dir}/ispd{ispd_year}eval'
            ./ispd{ispd_year}eval \\
                -lef {bench_dir}/{design}/{design}.input.lef \\
                -def {design_dir}/{design}.output.def \\
                -guide {bench_dir}/{design}/{design}.input.guide \\
              | grep -v WARNING | grep -v ERROR
            echo
    """
    with open(run_sh, 'w') as f:
        print(textwrap.dedent(script), file=f)
    st = os.stat(run_sh)
    os.chmod(run_sh, st.st_mode | stat.S_IXUSR | stat.S_IXGRP)


def test_enabled(design, patterns):
    for pattern in patterns:
        if fnmatch.fnmatch(design, pattern):
            return True
    return False


design_list_ispd18 = [
    ("ispd18_test1",   0),
    ("ispd18_test2",   0),
    ("ispd18_test3",  14),
    ("ispd18_test4",  13),
    ("ispd18_test5",   0),
    ("ispd18_test6",   0),
    ("ispd18_test7",   0),
    ("ispd18_test8",   0),
    ("ispd18_test9",   0),
    ("ispd18_test10",  0),
]
design_list_ispd19 = [
    ("ispd19_test1",   0),
    ("ispd19_test2",   0),
    ("ispd19_test3",   0),
    ("ispd19_test4",   0),
    ("ispd19_test5",   0),
    ("ispd19_test6",   3),
    ("ispd19_test7",   0),
    ("ispd19_test8",   1),
    ("ispd19_test9",   0),
    ("ispd19_test10", 19),
]

run_dir = os.path.join(args.dir, "runs")
running_tests = set()
for (design, drv) in design_list_ispd18:
    if test_enabled(design, args.tests):
        genFiles(run_dir, 18, design, drv)
        running_tests.add(design)


for (design, drv) in design_list_ispd19:
    if test_enabled(design, args.tests):
        genFiles(run_dir, 19, design, drv)
        running_tests.add(design)

os.chdir(run_dir)

status = subprocess.run(['parallel',
                         '-j', str(args.jobs),
                         '--halt', 'never',
                         '--joblog', f"{run_dir}/log",
                         './{}/run.sh', ':::', *list(running_tests)])
for design in running_tests:
    subprocess.run(['tar', 'czvf', f"{design}.tar.gz", f"{design}"],
                   check=True)

print("=======================")
if status.returncode:
    print("Fail")
else:
    print("Success")

print("=======================")
sys.exit(status.returncode)
