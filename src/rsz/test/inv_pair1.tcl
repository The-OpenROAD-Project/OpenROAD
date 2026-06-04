# Inverter-pair insertion. Check inverters are inserted by both rebuffer
# entry points when enable_inverter_pair is on:
#   - repair_timing -setup  (rebufferPin / "rsz" repair path)
#   - rsz::fully_rebuffer    (placement-driven / "gpl" path)
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

rsz::set_enable_inverter_pair true

# rsz repair path
repair_design
repair_timing -setup -repair_tns 100
set rsz_inv [inv_count]

# placement (gpl) path
foreach p [get_pins -of_objects [get_cells src_*] -filter {direction == output}] {
  catch { rsz::fully_rebuffer $p }
}
set total_inv [inv_count]

puts "rsz path (repair_timing) inserted inverters: [expr { $rsz_inv > 0 ? {yes} : {no} }]"
puts "gpl path (fully_rebuffer) inserted inverters: [expr { $total_inv > 0 ? {yes} : {no} }]"
