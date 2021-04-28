source ../script/user_env.tcl

set golden_spef ../../test/generate_pattern.spefok

read_lef $TECH_LEF

read_def EXT/patterns.def

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file $extRules \
      -cc_model 12 -max_res 0 -context_depth 10 \
      -coupling_threshold 0.1

write_spef ext_patterns.spef

diff_spef -file $golden_spef

exit
