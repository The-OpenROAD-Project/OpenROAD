source "helpers.tcl"

set db [ord::get_db]
set tech [odb::dbTech_create $db "tech"]

read_3dbv "data/example.3dbv"
set out_3dbv [make_result_file "write_3dbv.dbv"]
set 3dbv_write_result [odb::write_dbv $lib $out_3dbv]
if { $3dbv_write_result != 1 } {
  puts "FAIL: dbv write error"
  exit 1
}

diff_files $out_3dbv "write_3dbv.dbvok"

puts "pass"
exit 0
