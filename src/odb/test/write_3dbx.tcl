source "helpers.tcl"

set db [ord::get_db]
set tech [odb::dbTech_create $db "tech"]

read_3dbx "data/example.3dbx"
set out_3dbx [make_result_file "write_3dbx/write_3dbx.3dbx"]
set 3dbx_write_result [write_3dbx $out_3dbx]

diff_files $out_3dbx "write_3dbx.3dbxok"

puts "pass"
exit 0
