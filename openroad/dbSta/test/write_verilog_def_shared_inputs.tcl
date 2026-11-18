# Reject shared DEF input nets without replacing an existing output file.
source "helpers.tcl"
read_lef ../../drt/test/sky130hs/sky130hs.tlef
read_lef ../../drt/test/sky130hs/sky130hs_std_cell.lef
read_def ../../drt/test/top_level_term2.def

set verilog_file [make_result_file write_verilog_def_shared_inputs.v]
set previous "module preserved; endmodule\n"
set stream [open $verilog_file w]
puts -nonewline $stream $previous
close $stream
if { ![catch { write_verilog $verilog_file } message] } {
  error "Expected STA-2749 for distinct input ports sharing one net"
}
if { ![string match {*STA-2749*} $message] } {
  error "Expected STA-2749, got: $message"
}
set stream [open $verilog_file r]
set text [read $stream]
close $stream
if { $text ne $previous } {
  error "Rejected write changed the existing output file"
}
puts "pass"
