# A dont_touch register on a clock net that CTS skips (one sink, CTS-41)
# is no reason to stop: the net is never changed. The same design with the
# dont_touch register on a net CTS builds is refused (dont_touch_sink,
# dont_touch_timing_buffer).
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

set db [ord::get_db]
set block [ord::get_db_block]
set clk_net [$block findNet clk]

# tbuf: a clock buffer on clk driving ff16 alone, on a net CTS skips.
set tbuf [odb::dbInst_create $block [$db findMaster CLKBUF_X1] tbuf]
$tbuf setLocation 10000 10000
$tbuf setPlacementStatus PLACED
set clk_tbuf [odb::dbNet_create $block clk_tbuf]
$clk_tbuf setSigType CLOCK
[$tbuf findITerm A] connect $clk_net
[$tbuf findITerm Z] connect $clk_tbuf
set ff16_ck [[$block findInst ff16] findITerm CK]
$ff16_ck disconnect
$ff16_ck connect $clk_tbuf

create_clock -period 5 clk
set_wire_rc -clock -layer metal3
set_cts_config -wire_unit 20 \
  -apply_ndr root_only \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

set_dont_touch [get_cells ff16]
set insts_before [llength [$block getInsts]]


set failed [catch { tee -quiet -variable log { clock_tree_synthesis } } message]
if { $failed } {
  puts $log
  puts $message
}
check "CTS completes" { set failed } 0
check "the one-sink net is skipped" \
  { string match {*CTS-0041*clk_tbuf*} $log } 1
check "the skipped net is not refused" { string match {*CTS-0137*} $log } 0
check "the dont_touch register stays on its net" \
  { [$ff16_ck getNet] getName } clk_tbuf
check "the tree on clk was built" \
  { expr { [llength [$block getInsts]] > $insts_before } } 1

exit_summary
