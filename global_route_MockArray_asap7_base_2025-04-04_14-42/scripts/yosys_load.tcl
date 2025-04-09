# Load synthesis result
yosys -import

source $::env(SCRIPTS_DIR)/synth_stdcells.tcl

read_verilog $::env(RESULTS_DIR)/1_synth.v
