source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design.def"
set tech [$db getTech]
set block [[$db getChip] getBlock]
create_ndr -name NDR -spacing { metal1:metal5 0.4 } -width {metal2:metal6 0.5} -via { M2_M1_via }
assign_ndr -ndr NDR -net clk
foreach ndr [$tech getNonDefaultRules] {
  puts "[$ndr getName]:"
  foreach layer [$tech getLayers] {
    set rule [$ndr getLayerRule $layer]
    if { $rule == "NULL" } {
      continue
    }
    puts "layer [$layer getName] spacing [$rule getSpacing] width [$rule getWidth]"
  }
}