from openroad import Design, Tech
import helpers
import cts_aux
import cts_helpers

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.evalTclString(cts_helpers.make_array)
design.evalTclString("make_array 300")
design.evalTclString("sta::db_network_defined")
design.evalTclString("create_clock -period 5 clk")
design.evalTclString("set_wire_rc -signal -layer metal3")
design.evalTclString("set_wire_rc -clock -layer metal5")

cts_aux.clock_tree_synthesis(
    design,
    root_buf="CLKBUF_X3",
    buf_list="CLKBUF_X3",
    wire_unit=20,
    sink_clustering_enable=True,
    distance_between_buffers=100.0,
    num_static_layers=1,
    obstruction_aware=True,
)
