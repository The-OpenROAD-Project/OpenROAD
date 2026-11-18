#
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top

set block [ord::get_db_block]
set top [$block getTopModule]
puts [$top getName]
foreach i [$top getChildren] {
  set master [$i getMaster]
  puts "[$i getName] [$master getName]"
  foreach ii [$master getInsts] {
    puts "[$ii getName] [[$ii getMaster] getName]"
  }
}

# Verify that the prop exists on the block
set found_prop 0
set block [ord::get_db_block]
foreach prop [odb::dbProperty_getProperties $block] {
  if { [string first [$prop getName] "src_file_"] } {
    set found_prop $prop
  }
}
if { $found_prop != 0 } {
  puts [format "Found filename prop %s on block" [$prop getName]]
} else {
  error "Didn't find filename prop on block"
}
foreach module [$block getModules] {
  set prop [odb::dbStringProperty_find $module "original_name"]
  if { $prop != "NULL" } {
    puts "[$module getName] [$prop getValue]"
  }
}
