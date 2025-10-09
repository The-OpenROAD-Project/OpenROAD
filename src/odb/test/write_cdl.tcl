source "helpers.tcl"

read_lef write_cdl.lef
read_liberty write_cdl.lib

read_verilog write_cdl.v

link_design top

set cdl_file [make_result_file write_cdl_out.cdl]
write_cdl -masters {write_cdl.cdl} $cdl_file

set cdl_escaped_file [make_result_file write_cdl_escaped_out.cdl]
write_cdl -masters {write_cdl_escaped.cdl} $cdl_escaped_file

diff_files write_cdl.cdlok $cdl_file

diff_files write_cdl.cdlok $cdl_escaped_file
