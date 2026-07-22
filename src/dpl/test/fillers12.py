from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("fillers12.lef")
design = helpers.make_design(tech)
design.readDef("fillers12.def")

dpl_aux.filler_placement(design, masters=["FILLCELL_RVT_X1"])
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("fillers12.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "fillers12.defok")
