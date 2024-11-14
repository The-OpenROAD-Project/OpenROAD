# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

ppl_aux.place_pins(
    design,
    hor_layers="metal3",
    ver_layers="metal2",
    random=True,
    group_pins=["req_msg[14] req_msg[15] req_msg[16] req_msg[17]"],
)

def_file = helpers.make_result_file("random7.def")
design.writeDef(def_file)
helpers.diff_files("random7.defok", def_file)


# # gcd_nangate45 IO placement
# source "helpers.tcl"
# read_lef Nangate45/Nangate45.lef
# read_def gcd.def

# place_pins -hor_layers metal3 -ver_layers metal2 -random -group_pins {req_msg[14] req_msg[15] req_msg[16] req_msg[17]}

# set def_file [make_result_file random7.def]

# write_def $def_file

# diff_file random7.defok $def_file
