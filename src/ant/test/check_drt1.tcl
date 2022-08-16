source "helpers.tcl"
# check_antennas detail routes
read_lef merged_spacing.lef
read_def sw130_random.def

check_antennas
check_antennas -verbose

check_antennas -net net50
catch { check_antennas -verbose -net xxx } msg
puts $msg

