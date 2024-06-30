source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design58.def"

set out_def [make_result_file "write_def58.def"]
write_def $out_def

diff_files $out_def "write_def58.defok"

puts "pass"
exit 0
