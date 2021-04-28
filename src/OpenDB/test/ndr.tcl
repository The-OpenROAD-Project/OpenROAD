source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/ndr.def"
set tech [$db getTech]
set block [[$db getChip] getBlock]
set clk [$block findNet clk]
create_ndr -name NDR -spacing { metal1:metal5 *3 } -width {metal2:metal6 0.5} -via { M2_M1_via }
create_ndr -name mult -spacing { *2 } -width {metal1:metal3 *3}
assign_ndr -ndr NDR -net clk
foreach ndr [$block getNonDefaultRules] {
  puts "[$ndr getName]:"
  foreach layer [$tech getLayers] {
    set rule [$ndr getLayerRule $layer]
    if { $rule == "NULL" } {
      continue
    }
    puts "layer [$layer getName] spacing [$rule getSpacing] width [$rule getWidth]"
  }
}
puts "clk nondefault rule: [[$clk getNonDefaultRule] getName]"
set def_file [make_result_file "ndr.def"]
write_def $def_file
diff_file $def_file ndr.defok