from openroad import Tech
import helpers

tech = Tech()

tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = helpers.make_design(tech)
design.readVerilog("aes_nangate45.v")
design.link("aes_cipher_top")

design.evalTclString("create_clock [get_ports clk] -name core_clock -period 0.5")

design.getClockGating().run()

design.evalTclString("write_verilog results/aes_nangate45_gated.v")
helpers.diff_files("aes_nangate45_gated.vok", "results/aes_nangate45_gated.v")
