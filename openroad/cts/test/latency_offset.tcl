# set_cts_latency_offset clocks a macro later than CTS's latency balance:
# the macro's clock pin arrival plus its insertion delay lands the offset
# after the registers' mean clock arrival, within a delay buffer. The
# result is printed as a boolean so the golden survives STA changes.
source "helpers.tcl"

source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_liberty array_tile_ins_delay.lib
read_lef Nangate45/Nangate45.lef
read_lef array_tile_ins_delay.lef
read_def insertion_delay.def

source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

create_clock -name core -period 5 clk

set_cts_config -wire_unit 20 \
  -distance_between_buffers 100 \
  -sink_clustering_size 10 \
  -sink_clustering_max_diameter 60 \
  -num_static_layers 1 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

set offset 0.05
set_cts_latency_offset -offset $offset inst_0
clock_tree_synthesis -sink_clustering_enable

set_propagated_clock [all_clocks]
estimate_parasitics -placement

# array_tile_ins_delay.lib: max_clock_tree_path rise 0.003, fall 0.004
set macro [expr { [get_property [get_pins inst_0/clk] arrival_max_rise] + 0.0035 }]
set sum 0.0
set n 0
foreach pin [all_registers -clock_pins] {
  if { [get_full_name $pin] eq "inst_0/clk" } {
    continue
  }
  set sum [expr { $sum + [get_property $pin arrival_max_rise] }]
  incr n
}
set skew [expr { $macro - $sum / $n }]
puts "Macro clocked the offset after the registers within 0.015 ns:\
  [expr { abs($skew - $offset) < 0.015 }]"

foreach bad {{-offset 0.01 no_such_inst} {inst_0}} {
  catch { set_cts_latency_offset {*}$bad } err
  puts $err
}
