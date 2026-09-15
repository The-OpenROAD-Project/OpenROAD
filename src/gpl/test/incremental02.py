import os
import subprocess
import sys

import helpers

if os.environ.get("TEST_SRCDIR", ""):
    openroad_exe = os.environ.get("OPENROAD_EXE")
    if not openroad_exe:
        raise RuntimeError("OPENROAD_EXE is required for Bazel GPL regression tests")

    subprocess.run(
        [openroad_exe, "-no_splash", "-no_init", "-exit", "incremental02.tcl"],
        check=True,
    )
    sys.exit(0)

from openroad import Design, Tech, set_thread_count
import gpl

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./incremental02.def")

set_thread_count(4)
options = gpl.PlaceOptions()
options.padLeft = 2
options.padRight = 2
options.density = 0.3
design.getReplace().doIncrementalPlace(1, options)

def_file = helpers.make_result_file("incremental02.def")
design.writeDef(def_file)

unplaced = sum(
    1 for inst in design.getBlock().getInsts() if inst.getPlacementStatus() == "NONE"
)

if unplaced != 0:
    raise Exception(f"Expected all instances to be placed, found {unplaced}")

print("pass")
