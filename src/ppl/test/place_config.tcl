# set_place_config keeps the pin settings in block properties: each call
# changes only the options it is given, and place_pins switches last for their
# own run only.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_place_config -io_pin_hor_layers metal3
catch { place_pins } error
puts $error

catch { set_place_config -io_pin_min_distance_in_tracks } error
puts $error
catch { place_pins -min_distance_in_tracks } error
puts $error

set_place_config -io_pin_ver_layers metal2 -io_pin_min_distance 1
report_place_config
place_pins

place_pins -min_distance 5 -min_distance_in_tracks
place_pins

# Without the min distance setting the default spacing applies again.
reset_place_config -io_pin_min_distance
report_place_config
place_pins

reset_place_config
report_place_config
catch { place_pins } error
puts $error
