# Regression for issue #10622: buffer_ports must use normal DATA buffers
# (*_buf_*) for data nets, not clock-delay buffers (*clkdlybuf*) or dedicated
# delay cells (*dlygate*, *dlymetal*).
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_def repair_design2.def

buffer_ports -inputs -outputs

# Fail loudly if any inserted buffer is a clock-delay or dedicated delay cell.
set bad 0
foreach inst [[ord::get_db_block] getInsts] {
  set master [[$inst getMaster] getName]
  if {
    [string match "*clkdlybuf*" $master]
    || [string match "*dlygate*" $master]
    || [string match "*dlymetal*" $master]
  } {
    puts "ERROR: delay/clock-delay cell used for data buffering: [$inst getName] $master"
    incr bad
  }
}
if { $bad == 0 } {
  puts "pass"
} else {
  puts "FAIL: $bad delay/clock-delay cells used for data buffering"
}
