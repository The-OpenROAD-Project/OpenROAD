from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("asap7/asap7_tech_1x_201209.lef")
tech.readLef("asap7/asap7sc7p5t_28_R_1x_220121a.lef")
design = helpers.make_design(tech)
design.readDef("./nograd01.def")

gpl_aux.global_placement(design, skip_io=True)

def_file = helpers.make_result_file("nograd01.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "nograd01.defok")
