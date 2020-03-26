# repair_max_fanout for hi fanout def
source "helpers.tcl"
source "hi_fanout.tcl"

set def_filename [file join $result_dir "hi_fanout.def"]
write_hi_fanout_def $def_filename 35

read_liberty liberty1.lib
read_lef liberty1.lef
read_def $def_filename
create_clock -period 10 clk1

repair_max_fanout -max_fanout 10 -buffer_cell liberty1/snl_bufx1

foreach drvr {r1/Q buffer1/Z buffer2/Z buffer3/Z buffer4/Z} {
  set fanout [expr [llength [get_pins -of [get_net -of [get_pin $drvr]]]] - 1]
  puts "$drvr fanout $fanout"
  #set dist [sta::max_load_manhatten_distance [get_pin $drvr]]
  #puts "$drvr fanout $fanout dist [format %.0f [expr $dist * 1e6]]"
  #report_object_names [get_pins -of [get_net $ent]]
}

# set repaired_filename [file join $result_dir "repair_max_fanout1.def"]
# write_def $repaired_filename
# diff_file $repaired_filename repair_max_fanout1.defok
