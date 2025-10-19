from openroad import Design, Tech
import helpers
import odb

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130_fd_sc_hd_merged.lef")
design = helpers.make_design(tech)
design.readDef("gcd_prefill.def")

dfl = design.getFinale()
dfl.densityFill("fill.json", design.getBlock().getCoreArea())

def_file = helpers.make_result_file("gcd_fill.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "gcd_fill.defok")
