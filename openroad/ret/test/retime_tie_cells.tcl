# The tie cells that only a moving register reads are removed with it, as in
# retime_dsec_async_tied, unless something holds them in place.  Each case
# holds some of them and leaves the others free: first the supply pin of t4
# is on a don't-touch net, which deleting t4 would disconnect; then t1 is
# fixed in place and the net of t2 has a constraint.  The registers move as
# in retime_dsec_async_tied both times; the held tie cells stay and the free
# ones are removed.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

proc read_design { } {
  read_verilog retime_dsec_async_tied.v
  link_design top
  create_clock -period 0.15 clk
}

proc report_cells { } {
  set block [ord::get_db_block]
  foreach name {r1 r2 r4 t1 t2 t4} {
    set inst [$block findInst $name]
    puts "$name: [expr { $inst == "NULL" ? "removed" : "stays" }]"
  }
}

read_design
set block [ord::get_db_block]
set supply [odb::dbNet_create $block VDD]
$supply setSigType POWER
[[$block findInst t4] findITerm VDD] connect $supply
$supply setDoNotTouch 1
retime
report_cells
$supply setDoNotTouch 0

read_design
[[ord::get_db_block] findInst t1] setPlacementStatus FIRM
set_timing_derate -net_delay -late 1.0 [get_nets hi2]
retime
report_cells
