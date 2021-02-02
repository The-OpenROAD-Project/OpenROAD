# design with invalid pin placement
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "invalid_pin_placement.def"

catch {global_route} error
puts $error
