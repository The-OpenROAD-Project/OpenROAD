source "helpers.tcl"

read_lef "data/sky130hd/sky130hd_multi_patterned.tlef"
read_def "data/rect_mask.def"

set out_def [make_result_file "write_def_rect_mask.def"]
write_def $out_def

# Verify the written DEF contains MASK 1 RECT (not RECT MASK \x01)
set fp [open $out_def r]
set def_data [read $fp]
close $fp

if { ![string match "*MASK 1 RECT*" $def_data] } {
  puts "FAIL: MASK 1 RECT not found in $out_def"
  exit 1
}

# Verify read_def parses the written DEF into a fresh chip without error
odb::dbChip_destroy [[ord::get_db] getChip]
read_def $out_def

set block [ord::get_db_block]
set net [$block findNet "n1"]
set wire [$net getWire]
set decoder [odb::dbWireDecoder]
$decoder begin $wire
while { [$decoder next] != $odb::dbWireDecoder_END_DECODE } {
  if { [$decoder getColor] != "NULL" } {
    set rect_color [$decoder getColor]
  }
}
check "round-tripped rect color" {set rect_color} 1

puts "pass"
exit 0
