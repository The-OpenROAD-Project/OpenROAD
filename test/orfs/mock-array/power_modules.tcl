source $::env(SCRIPTS_DIR)/load.tcl
load_design $::env(POWER_STAGE_STEM).odb $::env(POWER_STAGE_STEM).sdc

log_cmd report_power

# FIXME add check that all pins are annotated and that parasitics are
# estimated or read from SPEF

set instances {io_outs_down_mult io_outs_left_mult io_outs_up_mult io_outs_right_mult}
report_power -instances $instances

# FIXME
# log_cmd report_parasitic_annotation
#log_cmd report_activity_annotation -report_unannotated

set f [open $::env(OUTPUT) w]
puts $f OK
close $f
