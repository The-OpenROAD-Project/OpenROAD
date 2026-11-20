# Destroying a dbNet must delete its parasitic network annotation.
#
# sta::Net* is the dbNet pointer (dbNetwork::dbToSta is a reinterpret_cast) and
# ConcreteParasitics keys parasitic_network_map_ by it.  dbNet::destroy() only
# disconnects the network's pin nodes, leaving a driver-less network annotated
# on the dead net.  odb recycles the dbNet slot, so the next net created lands
# on the same sta::Net* and inherits it.  PrimaDelayCalc then finds no node for
# its driver pin, builds a zero-node circuit, and indexes an empty
# threshold_times_ -- before the fix this aborted here.
#
# prima is required: the default calculator reduces each network to pi-Elmore
# and deletes it, so there is nothing left to inherit.  It must also be
# selected before estimate_parasitics for the same reason.
read_lef ../../../test/asap7/asap7_tech_1x_201209.lef
read_lef ../../../test/asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_liberty ../../../test/asap7/asap7_small_ccs.lib.gz
read_def prima_net_recycle.def

create_clock -name clk -period 500 clk
set_input_delay -clock clk 0 in
set_input_transition 10 {clk in}

set_layer_rc -layer M3 -resistance 3.63251E-02 -capacitance 1.53955E-01
set_wire_rc -layer M3
sta::set_delay_calculator prima
estimate_parasitics -placement

# Rebuild n1 with the identical pin set.  The netlist is unchanged afterwards;
# only the recycled sta::Net* differs.
set block [ord::get_db_block]
set net [$block findNet n1]
set pins {}
foreach iterm [$net getITerms] {
  lappend pins [list [[$iterm getInst] getName] [[$iterm getMTerm] getName]]
}
odb::dbNet_destroy $net

set net [odb::dbNet_create $block n1]
foreach pin $pins {
  odb::dbITerm_connect [[$block findInst [lindex $pin 0]] findITerm [lindex $pin 1]] $net
}

report_worst_slack -max
