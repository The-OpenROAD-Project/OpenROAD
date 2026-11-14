source "helpers.tcl"

set db [ord::get_db]
read_lef "data/write_def_mask.lef"
read_def "data/write_def_mask.def"

set out_def [make_result_file "write_def_mask.def"]
write_def $out_def

diff_files $out_def "write_def_mask.defok"

puts "pass"
exit 0
