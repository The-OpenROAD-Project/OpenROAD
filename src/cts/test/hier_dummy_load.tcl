# Dummy loads in a hierarchical design: when the clock tree is built inside a
# sub-module, CTS must create its dummy loads (clkload*) in that module and
# connect them through the hierarchy. A dummy load in the top module, with no
# port to the clock net's module, splits the net in two in write_verilog.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog hier_dummy_load.v
link_design top -hier

initialize_floorplan -die_area "0 0 200 200" -core_area "10 10 190 190" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
source Nangate45/Nangate45.tracks
place_pins -hor_layers metal3 -ver_layers metal2

# Uneven sink clusters: 32 flops on the left, 8 on the right, so leaf
# buffers drive different loads and CTS balances them with dummy loads.
set block [ord::get_db_block]
for { set i 0 } { $i < 40 } { incr i } {
  set inst [$block findInst u_sub/ff_$i]
  if { $i < 32 } {
    set x [expr { 20000 + ($i % 8) * 4000 }]
    set y [expr { 20000 + ($i / 8) * 2800 }]
  } else {
    set x [expr { 150000 + ($i % 4) * 4000 }]
    set y [expr { 150000 + (($i - 32) / 4) * 2800 }]
  }
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED
}
set root [$block findInst u_sub/u_root]
$root setLocation 100000 100000
$root setPlacementStatus PLACED
detailed_placement

create_clock -period 5 clk
set_wire_rc -clock -layer metal5

set_cts_config -wire_unit 20 \
  -sink_clustering_size 10 \
  -sink_clustering_max_diameter 60 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

clock_tree_synthesis -sink_clustering_enable

# Each dummy load must be in the same module as the buffer it loads.
set n_dummy 0
set n_bad 0
foreach inst [$block getInsts] {
  set name [$inst getName]
  if { ![string match clkload* $name] } {
    continue
  }
  incr n_dummy
  set net [[lindex [$inst getITerms] 0] getNet]
  foreach iterm [$net getITerms] {
    if { [$iterm isOutputSignal] } {
      set driver [$iterm getInst]
    }
  }
  set dmod [[$inst getModule] getName]
  set rmod [[$driver getModule] getName]
  if { $dmod ne $rmod } {
    incr n_bad
  }
  puts "$name module=$dmod driver=[$driver getName] driver_module=$rmod"
}
puts "dummy loads: $n_dummy, in a different module than their driver: $n_bad"

# In the netlist, every clkload is inside module sub.
set verilog_file [make_result_file hier_dummy_load_out.v]
write_verilog $verilog_file
set f [open $verilog_file]
set module ""
while { [gets $f line] >= 0 } {
  if { [regexp {^module (\S+)} $line -> module] } {
    continue
  }
  if { [regexp {\s(clkload\d+)\s} $line -> inst] } {
    puts "verilog: $inst in module $module"
  }
}
close $f
