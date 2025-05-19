# Same as read_verilog10.tcl, but we pass -no_filename_prop to link_design
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design -omit_filename_prop top

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

# Verify that the prop doesn't exist on the block
set found_prop 0
set block [ord::get_db_block]
foreach prop [odb::dbProperty_getProperties $block] {
    if {[string first [$prop getName] "src_file_"]} {
        error [format "Found filename prop %s on block" [$prop getName]]
    }
}
