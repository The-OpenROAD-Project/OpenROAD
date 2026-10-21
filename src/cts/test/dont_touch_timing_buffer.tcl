# A dont_touch clock buffer from an earlier timing repair (source TIMING)
# on a clock net. CTS traverses such a buffer rather than treating it as a
# sink, but writing the tree still disconnects every input pin on the net,
# its input included, so a dont_touch one is refused before the net is
# changed (CTS-0137), not in odb (ODB-0370).
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

set db [ord::get_db]
set block [ord::get_db_block]
set clk_net [$block findNet clk]

# tbuf: a TIMING clock buffer on clk driving ff16 on a net of its own.
set tbuf [odb::dbInst_create $block [$db findMaster CLKBUF_X3] tbuf]
$tbuf setSourceType TIMING
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

set_dont_touch [get_cells tbuf]
set insts_before [llength [$block getInsts]]

set failed [catch { tee -quiet -variable log { clock_tree_synthesis } } message]
if { ![string match {*CTS-0137*} $message] } {
  puts $log
  puts $message
}
check "a dont_touch timing buffer stops CTS" { set failed } 1
check "the error is CTS-0137" { string match {*CTS-0137*} $message } 1
check "the error names the buffer's input pin" \
  { string match {*CTS-0137*tbuf/A*} $log } 1
check "odb is not the one refusing" { string match {*ODB-0370*} $log } 0
check "the buffer is still on clk" \
  { [[$tbuf findITerm A] getNet] getName } clk
check "no buffer was inserted" { llength [$block getInsts] } $insts_before

exit_summary
