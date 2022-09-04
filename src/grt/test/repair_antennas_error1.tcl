# repair_antennas arg checking
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

# no global routes
catch {repair_antennas sky130_fd_sc_hs__diode_2/DIODE} error
puts $error

global_route

# port name not found
catch {repair_antennas sky130_fd_sc_hs__diode_} error
puts $error
