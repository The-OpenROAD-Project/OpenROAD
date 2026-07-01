source "helpers.tcl"
read_db aes_nangate45_reroute.odb
detailed_route -no_pin_access \
  -verbose 0
set def_file [make_result_file aes_nangate45_reroute.def]
write_def $def_file
diff_files aes_nangate45_reroute.defok $def_file
