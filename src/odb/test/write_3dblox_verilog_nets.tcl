source "helpers.tcl"

read_3dbx "data/example_nets.3dbx"
set out_v [make_result_file "write_3dblox_verilog_nets/out.v"]
write_3dblox_verilog $out_v

diff_files $out_v "write_3dblox_verilog_nets.vok"

puts "pass"
exit 0
