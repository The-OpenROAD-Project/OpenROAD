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

design.evalTclString(
    "create_clock -name core_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk}]"
)
design.evalTclString("set_propagated_clock [get_clocks {core_clock}]")

clk = design.getBlock().findNet("clk")

design.evalTclString(
    "create_ndr -name NDR "
    + "-spacing { li1 0.51 met1 0.42 met2 0.42 met3 0.9 met4 0.9 met5 4.8 } "
    + "-width { li1 0.17 met1 0.14 met2 0.14 met3 0.3 met4 0.3 met5 1.6 }"
)

grt_aux.assign_ndr(design, ndrName="NDR", netName="clk")
grt_aux.assign_ndr(design, ndrName="NDR", netName="clknet_0_clk")
grt_aux.assign_ndr(design, ndrName="NDR", netName="clknet_2_0__leaf_clk")
grt_aux.assign_ndr(design, ndrName="NDR", netName="clknet_2_1__leaf_clk")
grt_aux.assign_ndr(design, ndrName="NDR", netName="clknet_2_2__leaf_clk")
grt_aux.assign_ndr(design, ndrName="NDR", netName="clknet_2_3__leaf_clk")

guide_file = helpers.make_result_file("ndr_1w_3s.guide")

grt_aux.set_global_routing_layer_adjustment(design, "met1", 0.8)
grt_aux.set_global_routing_layer_adjustment(design, "met2", 0.7)
grt_aux.set_global_routing_layer_adjustment(design, "*", 0.5)

grt_aux.set_routing_layers(design, signal="met1-met5", clock="met3-met5")

gr.setVerbose(True)
gr.globalRoute(True)

design.getBlock().writeGuides(guide_file)
helpers.diff_files("ndr_1w_3s.guideok", guide_file)
