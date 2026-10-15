# gpl.PlaceOptions built from keyword arguments and nested dicts gives the
# same placement as simple01 (see options01.tcl).
from openroad import Design, Tech
import helpers
import gpl

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple01.def")

options = gpl.PlaceOptions(
    initial=dict(max_iter=0), nesterov=dict(init_density_penalty=0.01)
)
print(options)
print(options.to_dict(only_changed=True))
print(options.timing.repair_design)
design.getReplace().doPlace(1, options)

def_file = helpers.make_result_file("options_py01.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01.defok")

# Attributes, update() with a dotted key and a sub-struct edited in place
# build the same options.
same = gpl.PlaceOptions()
same.nesterov.init_density_penalty = 0.01
same.update({"initial.max_iter": 0})
assert same.to_dict() == options.to_dict()
timing = same.timing
timing.repair_design.slew_margin = 5
assert same.timing.repair_design.slew_margin == 5
print(same.to_dict(only_changed=True))
