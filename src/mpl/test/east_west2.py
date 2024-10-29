# mem0 with west connctions, mem1 with east connections
from openroad import Design, Tech
import helpers
import mpl_aux

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLiberty("Nangate45/fakeram45_64x7.lib")
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")

design = helpers.make_design(tech)
design.readDef("east_west1.def")

# This appears to be the only difference from east_west1, ie, we do not
# read in the sdc file
# design.evalTclString('read_sdc "gcd.sdc"')

mpl_aux.macro_placement(design, style="corner_min_wl", halo=[0.5, 0.5])

def_file = helpers.make_result_file("east_west2.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "east_west2.defok")
