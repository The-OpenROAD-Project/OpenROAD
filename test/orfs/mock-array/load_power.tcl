source $::env(SCRIPTS_DIR)/util.tcl

foreach libFile $::env(LIB_FILES) {
  if { [lsearch -exact $::env(ADDITIONAL_LIBS) $libFile] == -1 } {
    read_liberty $libFile
  }
}

log_cmd read_verilog $::env(RESULTS_DIR)/$::env(POWER_STAGE_STEM).v
log_cmd read_verilog $::env(RESULTS_DIR)/../../Element/base/$::env(POWER_STAGE_STEM).v
log_cmd read_verilog $::env(PLATFORM_DIR)/verilog/stdcell/empty.v
log_cmd link_design MockArray

log_cmd read_sdc $::env(RESULTS_DIR)/$::env(POWER_STAGE_STEM).sdc
log_cmd read_spef $::env(RESULTS_DIR)/$::env(POWER_STAGE_STEM).spef
puts "read_spef for ces_*_* macros"
for { set x 0 } { $x < 8 } { incr x } {
  for { set y 0 } { $y < 8 } { incr y } {
    log_cmd read_spef -path ces_${x}_${y} \
      $::env(RESULTS_DIR)/../../Element/base/$::env(POWER_STAGE_STEM).spef
  }
}

set vcd_file $::env(VCD_STIMULI)
log_cmd read_vcd -scope TOP/MockArray $vcd_file
