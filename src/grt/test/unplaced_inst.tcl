# design with unplaced instance
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "unplaced_inst.def"

catch {global_route -verbose} error
puts $error