from openroad import Design, Tech
import helpers
import mpl_aux
import gpl_aux

# 3 levels of registers between mem0 and mem1
tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLiberty("Nangate45/fakeram45_64x7.lib")
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")

design = helpers.make_design(tech)
design.readDef("level3.def")
design.evalTclString('read_sdc "gcd.sdc"')

gpl_aux.global_placement(design)
mpl_aux.macro_placement(design, style="corner_min_wl", halo=[0.5, 0.5])

def_file = helpers.make_result_file("level3_02.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "level3_02.defok")
