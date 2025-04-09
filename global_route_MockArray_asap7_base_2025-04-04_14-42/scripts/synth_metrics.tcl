utl::set_metrics_stage "synth__{}"
source $::env(SCRIPTS_DIR)/load.tcl
load_design 1_1_yosys.v 1_synth.sdc

report_metrics 1 "Post synthesis" false false
