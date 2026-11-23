# set_placer_config keeps the pin settings in block properties: each call
# changes only the options it is given, and place_pins switches last for their
# own run only.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_placer_config -io_pin_hor_layers metal3
catch { place_pins } error
puts $error

catch { set_placer_config -io_pin_min_distance_in_tracks } error
puts $error
catch { place_pins -min_distance_in_tracks } error
puts $error

set_placer_config -io_pin_ver_layers metal2 -io_pin_min_distance 1
report_placer_config
place_pins

place_pins -min_distance 5 -min_distance_in_tracks
place_pins
# The omitted direction keeps its saved layers.
place_pins -hor_layers metal5

# Without the min distance setting the default spacing applies again.
reset_placer_config -io_pin_min_distance
report_placer_config
place_pins

# The per-layer min distance overrides the other spacing on its layers. Like
# the other switches, place_pins -layer_min_distance replaces the whole saved
# list for its own run, and -min_distance leaves the saved list in place. The
# runs report the min distance of each layer.
set_placer_config -io_pin_layer_min_distance {metal3 4 metal2 3} \
  -io_pin_min_distance_in_tracks
report_placer_config
place_pins
place_pins -min_distance_in_tracks -layer_min_distance {metal3 1}
place_pins -min_distance 1 -min_distance_in_tracks

# A failed run leaves nothing for the next one.
catch {
  place_pins -min_distance_in_tracks \
    -layer_min_distance {metal3 1000 metal2 1000}
} error
puts $error
place_pins
# Neither does one that fails before it places the pins.
catch {
  place_pins -hor_layers metal5 -min_distance 9 -min_distance_in_tracks \
    -group_pins {no_such_pin}
} error
puts $error
place_pins

# In microns, with a min distance for the other layers.
set_placer_config -io_pin_min_distance 0.38 \
  -io_pin_layer_min_distance {metal3 0.28}
report_placer_config
place_pins

# A layer that is not an IO pin layer.
set_placer_config -io_pin_layer_min_distance {metal4 1} \
  -io_pin_min_distance_in_tracks
report_placer_config
place_pins

reset_placer_config -io_pin_layer_min_distance -io_pin_min_distance
report_placer_config

foreach bad_args {
  {-io_pin_layer_min_distance {metal3}}
  {-io_pin_layer_min_distance {metal3 0}}
  {-io_pin_layer_min_distance {metal3 1 metal3 2}}
  {-io_pin_layer_min_distance {metal3 0.0001}}
  {-io_pin_layer_min_distance {metal3 0x4} -io_pin_min_distance_in_tracks}
  {-io_pin_layer_min_distance {via1 2}}
  {-io_pin_min_distance -1}
  {-io_pin_min_distance 0x4}
} {
  catch { set_placer_config {*}$bad_args } error
  puts $error
}
foreach bad_args {
  {-layer_min_distance {metal3 1.5}}
  {-min_distance 2.5}
  {-min_distance 0}
} {
  catch { place_pins -min_distance_in_tracks {*}$bad_args } error
  puts $error
}
# Read as decimal.
set_placer_config -io_pin_layer_min_distance {metal3 010} \
  -io_pin_min_distance_in_tracks
report_placer_config
reset_placer_config -io_pin_layer_min_distance

reset_placer_config
report_placer_config
catch { place_pins } error
puts $error
