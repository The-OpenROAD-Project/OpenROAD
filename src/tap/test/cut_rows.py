from openroad import Design, Tech
import tap
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")
design = helpers.make_design(tech)
design.readDef("boundary_macros.def")

options = tap.Options()
options.endcap_master = tech.getDB().findMaster("TAPCELL_X1")

design.getTapcell().cutRows(options)

design.getOpendp().checkPlacement(True)

def_file = helpers.make_result_file("cut_rows.def")
design.writeDef(def_file)
helpers.diff_files("cut_rows.defok", def_file)
