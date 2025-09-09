from openroad import Tech
import helpers

tech = Tech()

tech.readLiberty("sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib")
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130hd_std_cell.lef")

design = helpers.make_design(tech)
design.readVerilog("ibex_sky130hd.v")
design.link("ibex_core")

design.evalTclString("create_clock [get_ports clk_i] -name core_clock -period 10")

cgt = design.getClockGating()
cgt.setMaxCover(50)
cgt.run()

verilog_file = helpers.make_result_file("ibex_sky130hd_gated.v")
design.evalTclString(f"write_verilog {verilog_file}")
helpers.diff_files("ibex_sky130hd_gated.vok", verilog_file)
