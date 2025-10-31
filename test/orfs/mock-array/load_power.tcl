source $::env(SCRIPTS_DIR)/util.tcl

foreach libFile $::env(LIB_FILES) {
  if { [lsearch -exact $::env(ADDITIONAL_LIBS) $libFile] == -1 } {
    read_liberty $libFile
  }
}

proc check_log_for_warning {logfile} {
  set f [open $logfile r]
  set log_contents [read $f]
  close $f
  puts $log_contents
  if { [regexp -nocase "Warning" $log_contents] } {
    puts "ERROR: Warning found in $logfile"
    exit 1
  }
}

# $::env(VARIANT) contains 4x4_foo, fish out what comes before _
set name [lindex [split $::env(FLOW_VARIANT) "_"] 0]

log_cmd read_verilog $::env(RESULTS_DIR)/$::env(POWER_STAGE_STEM).v
log_cmd read_verilog $::env(RESULTS_DIR)/../../Element/${name}_base/$::env(POWER_STAGE_STEM).v
log_cmd read_verilog $::env(PLATFORM_DIR)/verilog/stdcell/empty.v
log_cmd link_design MockArray

log_cmd read_sdc $::env(RESULTS_DIR)/$::env(POWER_STAGE_STEM).sdc
log_cmd read_spef $::env(RESULTS_DIR)/$::env(POWER_STAGE_STEM).spef > log.txt
check_log_for_warning log.txt

puts "read_spef for ces_*_* macros"
for { set x 0 } { $x < $::env(ARRAY_COLS) } { incr x } {
  for { set y 0 } { $y < $::env(ARRAY_ROWS) } { incr y } {
    log_cmd read_spef -path ces_${x}_${y} \
      $::env(RESULTS_DIR)/../../Element/${name}_base/$::env(POWER_STAGE_STEM).spef > log.txt
    check_log_for_warning log.txt
  }
}

set vcd_file $::env(VCD_STIMULI)
log_cmd read_vcd -scope TOP/MockArray $vcd_file
