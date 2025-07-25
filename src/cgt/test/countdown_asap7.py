from openroad import Tech
import helpers

tech = Tech()

tech.readLiberty("asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz")
tech.readLiberty("asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz")
tech.readLiberty("asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz")
tech.readLiberty("asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib")
tech.readLiberty("asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz")
tech.readLef("asap7/asap7_tech_1x_201209.lef")
tech.readLef("asap7/asap7sc7p5t_28_R_1x_220121a.lef")

design = helpers.make_design(tech)
design.readVerilog("countdown_asap7.v")
design.link("countdown")

design.evalTclString("create_clock [get_ports clk] -name clock -period 0.5")

design.getClockGating().run()

design.evalTclString("write_verilog results/countdown_asap7_gated.v")
helpers.diff_files("countdown_asap7_gated.vok", "results/countdown_asap7_gated.v")
