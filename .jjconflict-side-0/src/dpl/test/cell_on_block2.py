from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130hd_std_cell.lef")
tech.readLef("cell_on_block2.lef")
design = helpers.make_design(tech)
design.readDef("cell_on_block2.def")

dpl_aux.set_placement_padding(design, globl=True, right=4)
dpl_aux.detailed_placement(design)

def_file = helpers.make_result_file("cell_on_block2.def")
design.writeDef(def_file)
helpers.diff_files("cell_on_block2.defok", def_file)
