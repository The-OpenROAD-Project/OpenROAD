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
    corner_avoidance=0,
    min_distance=0.12,
)

def_file1 = helpers.make_result_file("mc1_iop.def")
design.writeDef(def_file1)

ppl_aux.place_pins(
    design,
    hor_layers="metal3",
    ver_layers="metal2",
    corner_avoidance=0,
    min_distance=0.12,
)

def_file2 = helpers.make_result_file("mc2_iop.def")
design.writeDef(def_file2)

helpers.diff_files(def_file1, def_file2)
