# The option checks are in C++, so Python reports the same messages as Tcl
# (see options_errors.tcl).
from openroad import Design, Tech
import helpers
import gpl

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple01.def")

for opts in [
    {"densty": 0.5},
    {"timing": {"nets_percentage": "abc"}},
    {"density": 1.5},
    {"routability.rc_coefficients": [1, 2, 3]},
    {"place_ios": True, "skip_io": True},
    {"timing": {"repair_design": {"slew_margin": 120}}},
    {"routability.global_route.grid_origin": [1, 2, 3]},
    {"initial": 5},
]:
    try:
        gpl.PlaceOptions(opts)
    except RuntimeError as e:
        print(e)

# A valid struct is still checked when it is used.
options = gpl.PlaceOptions()
options.density = 1.5
try:
    design.getReplace().doPlace(1, options)
except RuntimeError as e:
    print(e)

# A misspelled attribute is an error, not a silently ignored new attribute.
try:
    options.densty = 0.5
except AttributeError as e:
    print(e)
