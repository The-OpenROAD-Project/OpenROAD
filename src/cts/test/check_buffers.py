from openroad import Design, Tech
import helpers
import cts_aux
import utl

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("check_buffers.def")

design.evalTclString("create_clock -period 5 clk")
design.evalTclString("set_wire_rc -clock -layer metal5")

cts_aux.clock_tree_synthesis(
    design,
    root_buf="CLKBUF_X3",
    buf_list="CLKBUF_X3",
    wire_unit=20,
    sink_clustering_enable=True,
    distance_between_buffers=100.0,
    sink_clustering_size=10,
    sink_clustering_max_diameter=60.0,
    num_static_layers=1,
)

# This is only for checking clock tree results and not testing per se
cmd_block = """
set unconnected_buffers 0
foreach buf [get_cells clkbuf_*_clk] {
  set buf_name [get_name $buf]
  set input_pin [get_pin $buf_name/A]
  set input_net [get_net -of $input_pin]
  if { $input_net == "NULL" } {
    incr unconnected_buffers
  }
}
puts "Found $unconnected_buffers unconnected buffers."
"""
design.evalTclString(cmd_block)
