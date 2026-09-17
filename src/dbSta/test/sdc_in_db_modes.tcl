# Second process of the multi-mode round trip: the design, its modes and
# their constraints come from the .odb alone. Reports what sta answers in
# each mode, as key=value lines the parent checks.
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_db $::env(SDC_IN_DB_ODB)
puts "after read_db, stored form: [ord::sdc_in_db_kind]"

proc report { key value } {
  puts "$key=$value"
}

proc sdc_text { mode } {
  set path [file rootname $::env(SDC_IN_DB_ODB)]_$mode.sdc
  write_sdc -no_timestamp $path
  set fh [open $path r]
  set text [read $fh]
  close $fh
  return $text
}

report modes [lsort [lmap mode [get_modes *] { get_name $mode }]]
foreach mode {func test} {
  set_mode $mode
  report ${mode}_clocks [lsort [lmap clk [all_clocks] { get_name $clk }]]
  report ${mode}_period [get_property [lindex [get_clocks clk] 0] period]
  set text [sdc_text $mode]
  report ${mode}_false_paths [regexp -all {set_false_path} $text]
  report ${mode}_input_delays [regexp -all {set_input_delay} $text]
  report ${mode}_derates [regexp -all {set_timing_derate} $text]
}
