# repair high fanout tie hi/low net
source helpers.tcl
read_liberty Nangate_typ.lib
read_lef Nangate.lef
# def produced by writing results/repair_tie_fanout1.v
read_def repair_tie_fanout2.def

repair_tie_fanout -max_fanout 10 Nangate_typ/LOGIC1_X1/Z

set tie_insts [get_cells -filter "ref_name == LOGIC1_X1"]
foreach tie_inst $tie_insts {
  set tie_pin [get_pins -of $tie_inst -filter "direction == output"]
  set net [get_nets -of $tie_inst]
  set fanout [llength [get_pins -of $net -filter "direction == input"]]
  puts "[get_full_name $tie_inst] $fanout"
}
