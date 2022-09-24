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
