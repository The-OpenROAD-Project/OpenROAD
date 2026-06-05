# Inverter-pair insertion in the Rebuffer DP (enable_inverter_pair on).
#
#   - repair_timing -setup (rebufferPin / "rsz" path) genuinely inserts inverter
#     pairs on this fanout net -- verified by a positive inverter-count delta.
#   - rsz::fully_rebuffer (placement / "gpl" path) shares the same DP; it is
#     exercised here to confirm it runs cleanly with the inverter machinery
#     active. It does not commit inverters on a single net (inverter pairs are
#     slack-neutral against buffers, so the DP only prefers them across larger
#     designs with many unconstrained nets -- that QoR coverage lives in flow
#     CI, see PR Future-work).
#
# rebuffer1 separately checks the flag-off path stays byte-identical to baseline.
source "helpers.tcl"
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
foreach lf {AO INVBUF OA SIMPLE} {
  foreach f [glob asap7/asap7sc7p5t_${lf}_RVT_FF_nldm_*.lib.gz] { read_liberty $f }
}
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_verilog inv_pair1.v
link_design inv_pair1
read_def -floorplan_initialize inv_pair1.def
create_clock -name clk -period 0.08 [get_ports clk]
source asap7/setRC.tcl
set_wire_rc -signal -layer M3
estimate_parasitics -placement

proc inv_count { } {
  set n 0
  set blk [[[ord::get_db] getChip] getBlock]
  foreach inst [$blk getInsts] {
    if { [string match {INVx*} [[$inst getMaster] getName]] } { incr n }
  }
  return $n
}

# Exercise the public command, including the -disable flag round-trip.
enable_inverter_pair -disable
enable_inverter_pair

# rsz repair path: genuinely inserts inverter pairs (delta from zero).
repair_design
repair_timing -setup -repair_tns 100
set rsz_inv [inv_count]

# gpl placement path: exercise the shared DP via fully_rebuffer and confirm it
# runs cleanly with the inverter machinery active (no crash / no error).
set gpl_errors 0
foreach p [get_pins -of_objects [get_cells src_*] -filter {direction == output}] {
  if { [catch { rsz::fully_rebuffer $p } msg] } {
    incr gpl_errors
    puts "fully_rebuffer error: $msg"
  }
}

puts "rsz path (repair_timing) inserted inverters: [expr { $rsz_inv > 0 ? {yes} : {no} }]"
puts "gpl path (fully_rebuffer) ran without error: [expr { $gpl_errors == 0 ? {yes} : {no} }]"
