source "helpers.tcl"

read_lef "data/gscl45nm.lef"
read_def "data/virtual_route.def"

set block [ord::get_db_block]
set net [$block findNet "net1"]
set wire [$net getWire]

set decoder [odb::dbWireDecoder]
$decoder begin $wire

set ops {}
set points {}
while { 1 } {
  set op [$decoder next]
  if { $op == $odb::dbWireDecoder_END_DECODE } {
    break
  }
  if { $op == $odb::dbWireDecoder_PATH } {
    lappend ops "PATH"
  } elseif { $op == $odb::dbWireDecoder_POINT } {
    lappend ops "POINT"
    lappend points [$decoder getPoint]
  } elseif { $op == $odb::dbWireDecoder_VWIRE } {
    lappend ops "VWIRE"
  } elseif { $op == $odb::dbWireDecoder_RECT } {
    lappend ops "RECT"
  }
}

check "wire opcode sequence" {join $ops " "} "PATH POINT POINT VWIRE POINT RECT POINT"
check "wire points" {join $points " "} "0 0 2000 0 2000 1000 2000 2000"

set out_def [make_result_file "read_def_virtual.def"]
write_def $out_def
set stream [open $out_def r]
set def_text [read $stream]
close $stream
check "write_def preserves virtual point" {
  expr {[string first "VIRTUAL ( 2000 1000 )" $def_text] >= 0}
} 1

exit_summary
