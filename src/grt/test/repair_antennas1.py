from openroad import Tech, Design
import helpers
import grt_aux

tech = Tech()
tech.readLiberty("sky130hs/sky130hs_tt.lib")
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")

design = helpers.make_design(tech)
design.readDef("gcd_sky130.def")
gr = design.getGlobalRouter()

design.evalTclString("set_placement_padding -global -left 2 -right 2")

grt_aux.set_global_routing_layer_adjustment(design, "met2-met5", 0.15)
grt_aux.set_routing_layers(design, signal="met1-met5")

gr.globalRoute(True)

ant = design.getAntennaChecker()

ant.checkAntennas()
gr.repairAntennas(None, 1, 0)
ant.checkAntennas()

design.evalTclString("check_placement")

gfile = helpers.make_result_file("repair_antennas1.guide")

design.getBlock().writeGuides(gfile)

helpers.diff_files("repair_antennas1.guideok", gfile)

defFile = helpers.make_result_file("repair_antennas1.def")

design.writeDef(defFile)

helpers.diff_files("repair_antennas1.defok", defFile)
