from openroad import Tech, Design
import helpers
import grt_aux

tech = Tech()
tech.readLiberty("sky130hs/sky130hs_tt.lib")
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")

design = helpers.make_design(tech)
design.readDef("clock_route.def")
gr = design.getGlobalRouter()

# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
# source "helpers.tcl"

design.evalTclString(
    "create_clock -name core_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk}]"
)
design.evalTclString("set_propagated_clock [get_clocks {core_clock}]")

gfile = helpers.make_result_file("clock_route.guide")

grt_aux.set_global_routing_layer_adjustment(design, "met1", 0.8)
grt_aux.set_global_routing_layer_adjustment(design, "met2", 0.7)
grt_aux.set_global_routing_layer_adjustment(design, "*", 0.5)

grt_aux.set_routing_layers(design, signal="met1-met5", clock="met3-met5")

gr.setVerbose(True)
gr.globalRoute(True)

design.getBlock().writeGuides(gfile)

helpers.diff_files("clock_route.guideok", gfile)
