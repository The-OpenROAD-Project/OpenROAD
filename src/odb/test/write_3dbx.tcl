source "helpers.tcl"

set db [ord::get_db]
set tech [odb::dbTech_create $db "tech"]

read_3dbx "data/example.3dbx"
set out_3dbx [make_result_file "write_3dbx.dbx"]
set 3dbx_write_result [odb::write_dbx $lib $out_3dbx]
if { $3dbx_write_result != 1 } {
  puts "FAIL: dbv write error"
  exit 1
}

diff_files $out_3dbx "write_3dbx.dbvok"

puts "pass"
exit 0
