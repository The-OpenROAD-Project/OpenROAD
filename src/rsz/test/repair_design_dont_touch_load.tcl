# repair_design on a net over max_fanout with dont_touch loads. A buffer
# cannot be inserted in front of a dont_touch load (ODB-1211); repair_design
# leaves that group of loads on the net, says so, and repairs the rest
# instead of failing. An explicit insert_buffer in front of a dont_touch
# load is still an error.
source "helpers.tcl"
source "hi_fanout.tcl"

set def_filename [make_result_file "repair_design_dont_touch_load.def"]
write_hi_fanout_def $def_filename 35

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename
create_clock -period 10 clk1
set_max_fanout 10 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

set dont_touch_loads {load16 load17}
foreach name $dont_touch_loads {
  set_dont_touch [get_cells $name]
}

proc net_of { pin_name } {
  return [get_full_name [get_nets -of_objects [get_pins $pin_name]]]
}

proc net0_load_count { } {
  return [llength [get_pins -of_objects [get_nets net0] -filter "direction == input"]]
}

set loads_before [net0_load_count]

set failed [catch { tee -quiet -variable log { repair_design } } message]
if { $failed } {
  puts $log
  puts $message
}
check "repair_design returns" { set failed } 0
check "the dont_touch load is reported" \
  { regexp {ODB-1211[^\n]*load1[67]/D} $log } 1
check "the group left unbuffered is reported" \
  { string match {*WARNING RSZ-3006*net0*} $log } 1
foreach name $dont_touch_loads {
  check "$name stays on net0" { net_of $name/D } net0
}
check "the other loads are buffered" \
  { expr { [net0_load_count] < $loads_before } } 1

# An explicit request to buffer a dont_touch load is refused, with the
# resizer's error on top of odb's warning.
set failed [catch {
  tee -quiet -variable log {
    insert_buffer -buffer_cell BUF_X1 -net net0 -load_pins {load16/D load17/D}
  }
} message]
check "insert_buffer before dont_touch loads fails" { set failed } 1
check "insert_buffer names the reason" \
  { string match {*ODB-1211*load*dont_touch*} $log } 1
check "insert_buffer fails with RSZ-3018" \
  { string match {*RSZ-3018*} $message } 1
check "the dont_touch loads are untouched" { net_of load16/D } net0

exit_summary
