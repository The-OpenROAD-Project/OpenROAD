# Exercise enclosure validity through ordinary pdngen on a fixed macro.
# The generated_via_default_{edge,positive,valid}.tcl scripts select each case.
source "helpers.tcl"
set here generated_via_enclosure
set audit_file [file normalize [file join $here audit.tcl]]
set case $enclosure_case
read_lef Nangate45/Nangate45.lef
if {$case eq "positive"} {
  # Synthetic 10-nm generated-rule minimum; fixed-via geometry is unchanged.
  set tech [[ord::get_db] getTech]
  set rule [$tech findViaGenerateRule Via4Array-0]
  set minimum [expr {[$tech getDbUnitsPerMicron] / 100}]
  for {set i 0} {$i < [$rule getViaLayerRuleCount]} {incr i} {
    set layer_rule [$rule getViaLayerRule $i]
    if {[$layer_rule hasEnclosure]} { $layer_rule setEnclosure $minimum $minimum }
  }
}
read_lef [file join $here ${case}_macro.lef]
read_def [file join $here floorplan.def]
cd [file normalize [make_result_test_dir generated_via_default_${case}]]
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
global_connect
set_voltage_domain -power VDD -ground VSS
define_pdn_grid -name core -pins {metal7} -starts_with POWER
add_pdn_stripe -grid core -layer metal6 -width 0.93 -pitch 10 -offset 2
add_pdn_stripe -grid core -layer metal7 -width 1.4 -pitch 10 -offset 2
add_pdn_connect -grid core -layers {metal6 metal7}
define_pdn_grid -name macro_grid -macro -cells {enclosure_macro} -grid_over_boundary -starts_with POWER
add_pdn_stripe -grid macro_grid -layer metal5 -width 0.93 -pitch 10 -offset 2
add_pdn_stripe -grid macro_grid -layer metal6 -width 0.93 -pitch 10 -offset 2
add_pdn_connect -grid macro_grid -layers {metal4 metal5}
add_pdn_connect -grid macro_grid -layers {metal5 metal6}
add_pdn_connect -grid macro_grid -layers {metal6 metal7}
pdngen -failed_via_report failed_vias.rpt
write_def result.def
write_db result.odb
set block [ord::get_db_block]
set inst [$block findInst macro]
if {[$inst getOrient] ne "R0" || [$inst getLocation] ne "40000 40000"} {
  error "Macro pose changed"
}
source $audit_file
lassign [audit_generated_vias $block] count negative invalid
puts "GENERATED_VIAS $count NEGATIVE_ENCLOSURE_INSTANCES $negative INVALID_ENCLOSURE_INSTANCES $invalid"
set vdd_status [catch {check_power_grid -net VDD -error_file vdd.rpt} vdd_message]
set vss_status [catch {check_power_grid -net VSS -error_file vss.rpt} vss_message]
puts "POWER_GRID_VDD $vdd_status $vdd_message"
puts "POWER_GRID_VSS $vss_status $vss_message"
set out [open outcome.txt w]
puts $out "generated_vias=$count\nnegative_enclosure_instances=$negative\ninvalid_enclosure_instances=$invalid\nvdd_status=$vdd_status\nvss_status=$vss_status"
close $out
if {$invalid > 0} { error "Generated via violates its zero-or-positive enclosure rule" }
if {$vdd_status || $vss_status} { error "Required supply connectivity failed" }
puts "ENCLOSURE_REGRESSION_PASS"
