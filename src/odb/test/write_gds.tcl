source "helpers.tcl"

read_lef "data/gscl45nm.lef"
read_def "data/design.def"

set gds_file [make_result_file write_gds_out.gds]
write_gds $gds_file

puts "pass"
