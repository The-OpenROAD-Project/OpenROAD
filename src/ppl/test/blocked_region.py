# region blocked by macro in die boundary
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130_fd_sc_hd_merged.lef")
tech.readLef("blocked_region.lef")

design = helpers.make_design(tech)
design.readDef("blocked_region.def")

ppl_aux.place_pins(design, hor_layers="met3", ver_layers="met2", random=True)

def_file = helpers.make_result_file("blocked_region.def")
design.writeDef(def_file)
helpers.diff_files("blocked_region.defok", def_file)
