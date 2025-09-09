from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("block2.lef")
design = helpers.make_design(tech)
design.readDef("cell_on_block1.def")

dpl_aux.detailed_placement(design)

def_file = helpers.make_result_file("cell_on_block1.def")
design.writeDef(def_file)
helpers.diff_files("cell_on_block1.defok", def_file)
