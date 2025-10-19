from openroad import Design, Tech
import helpers
import cts_aux

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("16sinks.def")

design.evalTclString("create_clock -period 5 clk")
design.evalTclString("set_wire_rc -clock -layer metal3")

cts_aux.clock_tree_synthesis(
    design,
    root_buf="CLKBUF_X3",
    buf_list="CLKBUF_X3 CLKBUF_X2 BUF_X4 CLKBUF_X1",
    wire_unit=20,
)
