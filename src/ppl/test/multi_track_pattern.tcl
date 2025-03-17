# design with multiple track patterns
source "helpers.tcl"
read_lef multi_track_pattern1.lef
read_lef multi_track_pattern2.lef
read_def multi_track_pattern.def

place_pins -hor_layers M2 -ver_layers M5 -min_distance 0.036 -exclude top:* -exclude bottom:* -annealing

set def_file [make_result_file multi_track_pattern.def]

write_def $def_file

diff_file multi_track_pattern.defok $def_file
