# Test case for via access layer functionality
source "helpers.tcl"
read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_lef "via_access_layer.lef"
read_def "via_access_layer.def"
read_guides "via_access_layer.guides"

detailed_route -bottom_routing_layer met3 \
  -top_routing_layer met5 \
  -via_access_layer met2 \
  -verbose 0

set def_file [make_result_file via_access_layer.def]

write_def $def_file
diff_files via_access_layer.defok $def_file
