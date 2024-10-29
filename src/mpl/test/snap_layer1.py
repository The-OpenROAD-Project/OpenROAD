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

mpl_aux.macro_placement(design, snap_layer=3, halo=[1.0, 1.0])

def_file = helpers.make_result_file("snap_layer1.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "snap_layer1.defok")
