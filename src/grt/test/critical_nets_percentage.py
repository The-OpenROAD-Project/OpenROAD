from openroad import Tech, Design
import helpers
import grt_aux

tech = Tech()
tech.readLiberty("sky130hs/sky130hs_tt.lib")
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")

design = helpers.make_design(tech)
design.readDef("critical_nets_percentage.def")

gr = design.getGlobalRouter()

guideFile = helpers.make_result_file("congestion1.guide")

oblk = design.getBlock()
oblk.writeGuides(guideFile)

# FIXME: update when we have Python versions of the following
design.evalTclString('read_sdc "critical_nets_percentage.sdc"')
design.evalTclString('source "sky130hs/sky130hs.rc"')
design.evalTclString('set_wire_rc -signal -layer "met2"')
design.evalTclString('set_wire_rc -clock  -layer "met5"')
design.evalTclString("set_propagated_clock [all_clocks]")
design.evalTclString("estimate_parasitics -placement")

guideFile = helpers.make_result_file("critical_nets_percentage.guide")

grt_aux.set_routing_layers(design, signal="met1-met5", clock="met3-met5")
grt_aux.set_global_routing_layer_adjustment(design, "met1-met5", 0.8)

gr.setVerbose(True)
gr.setCriticalNetsPercentage(30.0)
gr.globalRoute(True)

design.getBlock().writeGuides(guideFile)

diff = helpers.diff_files("critical_nets_percentage.guideok", guideFile)
