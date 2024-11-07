from openroad import Design, Tech
import helpers
import cts_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = helpers.make_design(tech)
design.readDef("16sinks.def")

design.evalTclString("create_clock -period 5 clk")
design.evalTclString("set_wire_rc -clock -layer metal3")

cts_aux.clock_tree_synthesis(
    design,
    root_buf="CLKBUF_X3",
    buf_list="CLKBUF_X3",
    wire_unit=20,
    obstruction_aware=True,
    apply_ndr=True,
)

def_file = helpers.make_result_file("simple_test_out.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple_test_out.defok")
