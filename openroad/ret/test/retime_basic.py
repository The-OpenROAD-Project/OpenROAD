from openroad import Design, Tech
import ret
import helpers

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readVerilog("retime_basic.v")
design.link("top")
design.evalTclString("create_clock -period 0.13 clk")

design.evalTclString("report_worst_slack -max -digits 4")
design.getRetimer().retime(ret.RetimeOptions())
design.evalTclString("report_worst_slack -max -digits 4")

v_file = helpers.make_result_file("retime_basic.v")
design.evalTclString(f"write_verilog {v_file}")
helpers.diff_files("retime_basic.vok", v_file)
