from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("gcd_replace.def")

dpl_aux.detailed_placement(design)
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("gcd.def")
design.writeDef(def_file)
helpers.diff_files("gcd.defok", def_file)
