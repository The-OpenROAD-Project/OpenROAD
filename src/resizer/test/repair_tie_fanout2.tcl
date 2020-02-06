# repair high fanout tie hi/low net
source helpers.tcl
source tie_fanout.tcl

read_liberty Nangate_typ.lib
read_lef Nangate.lef

set def_filename [file join $result_dir "tie_hi2.def"]
write_tie_hi_fanout_def $def_filename \
  Nangate_typ/LOGIC1_X1/Z Nangate_typ/BUF_X1/A 35

read_def $def_filename

repair_tie_fanout -max_fanout 10 Nangate_typ/LOGIC1_X1/Z

set tie_insts [get_cells -filter "ref_name == LOGIC1_X1"]
foreach tie_inst $tie_insts {
  set tie_pin [get_pins -of $tie_inst -filter "direction == output"]
  set net [get_nets -of $tie_inst]
  set fanout [llength [get_pins -of $net -filter "direction == input"]]
  puts "[get_full_name $tie_inst] $fanout"
  #set dist [sta::max_load_manhatten_distance $tie_pin]
  #puts "[get_full_name $tie_inst] $fanout dist [format %.0f [expr $dist * 1e6]]"
}

# set repaired_filename [file join $result_dir "repair_tie_fanout2.def"]
# write_def $repaired_filename
# diff_file $repaired_filename repair_tie_fanout2.defok
