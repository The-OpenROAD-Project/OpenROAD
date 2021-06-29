# repair_antennas. def file generated using the openroad-flow
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "gcd_sky130.def"

global_route

catch {repair_antennas sky130_fd_sc_hs__diode_2/DIOD} error
puts $error
