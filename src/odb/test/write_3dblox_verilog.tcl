source "helpers.tcl"

read_3dbx "data/example.3dbx"
set out_v [make_result_file "write_3dblox_verilog/out.v"]
write_3dblox_verilog $out_v

diff_files $out_v "write_3dblox_verilog.vok"

puts "pass"
exit 0
