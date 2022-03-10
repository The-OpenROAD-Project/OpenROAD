# test inst pin out of die area
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "inst_pin_out_of_die.def"

catch {global_route -verbose} error
puts $error
