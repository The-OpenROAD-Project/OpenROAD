# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "clock_route.def"

set segs_file [make_result_file write_segments2.segs]

read_global_route_segments write_segments1.segsok

write_global_route_segments $segs_file
diff_file write_segments2.segsok $segs_file
