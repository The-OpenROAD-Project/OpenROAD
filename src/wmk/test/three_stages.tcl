# The flow the README describes, end to end: all three marks embedded on one
# design, the design routed, and ownership decided at the default two of three
# stages.  Nothing else in the suite runs the stages together, and nothing
# else measures what routing does to the placement mark -- which is the
# tolerance the extraction threshold exists for.
#
# Keys are generated into a file and every keyed command reads them from it,
# so the path a flow is expected to use is the path that is tested.
source "helpers.tcl"

foreach id { 5 6 7 8 9 392 393 1102 1103 1104 } {
  suppress_message DPL $id
}

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

# Fixed inputs, so the run is reproducible; a real flow draws both.
set key_file [make_result_file three_stages.key]
file delete -force $key_file
set public [generate_watermark_key -design_id gcd_three_stages \
  -key_hex 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee \
  -nonce_hex 00112233445566778899aabbccddeeff -file $key_file]
check "with -file the command returns only the public parameters" \
  { lsort [dict keys $public] } {design_id nonce_hex}

set place_claims [make_result_file three_stages_place.csv]
set committed [place_watermark -key_file $key_file -claims_file $place_claims \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 64]
check "the placement stage marks the design" { expr { $committed > 0 } } 1

clock_tree_synthesis -buf_list CLKBUF_X3 -root_buf CLKBUF_X3 -sink_clustering_enable
set_propagated_clock [all_clocks]
estimate_parasitics -placement
set cts_claims [make_result_file three_stages_cts.csv]
set cts_pairs [cts_watermark -key_file $key_file -claims_file $cts_claims]
check "the clock tree stage marks the design" { expr { $cts_pairs > 0 } } 1

# What a flow does next: legalize the buffers CTS added, then route.
detailed_placement
set_routing_watermark -key_file $key_file -fraction 0.5
set_routing_watermark_strength 100
global_route
detailed_route -verbose 0

# The placement mark after everything downstream of it has run.
set survived [wmk::verify_placement_watermark_cmd \
  [derive_watermark_key -key_file $key_file -stage placement] $place_claims]
puts "placement extraction rate after CTS, legalization and routing: $survived"
check "the placement mark survives the rest of the flow" \
  { expr { $survived >= 0.75 } } 1

check "ownership holds at the default two of three stages" {
  verify_watermark -key_file $key_file -placement_claims $place_claims \
    -cts_claims $cts_claims -routing -routing_fraction 0.5
} 1

exit_summary
