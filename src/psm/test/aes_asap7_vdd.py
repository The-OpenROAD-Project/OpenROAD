from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("asap7/asap7_tech_1x_201209.lef")
tech.readLef("asap7/asap7sc7p5t_28_R_1x_220121a.lef")
design = helpers.make_design(tech)
design.readDef("asap7_data/aes_place.def")
tech.readLiberty("asap7_data/asap7.lib")

design.evalTclString("read_sdc asap7_data/aes_place.sdc")

design.evalTclString("source asap7/setRC.tcl")

pdnsim_aux.analyze_power_grid(design, net="VDD")
