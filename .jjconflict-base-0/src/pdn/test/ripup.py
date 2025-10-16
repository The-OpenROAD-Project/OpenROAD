# test for pdngen -reset
from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("nangate_gcd/floorplan_with_grid.def")

pdngen = design.getPdnGen()
pdngen.ripUp(None)

def_file = helpers.make_result_file("ripup.def")
design.writeDef(def_file)
helpers.diff_files("ripup.defok", def_file)
