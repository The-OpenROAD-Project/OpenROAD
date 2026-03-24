source "helpers.tcl"

set db [ord::get_db]
set tech [odb::dbTech_create $db "tech"]

read_3dbx "data/example.3dbx"
set out_3dbv [make_result_file "write_3dbv/write_3dbv.3dbv"]
set 3dbv_write_result [write_3dbv $out_3dbv]

diff_files $out_3dbv "write_3dbv.3dbvok"

puts "pass"
exit 0
