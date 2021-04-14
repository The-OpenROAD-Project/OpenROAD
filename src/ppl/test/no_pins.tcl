# design without IO pins
read_lef Nangate45/Nangate45.lef
read_def no_pins.def

catch {place_pins -hor_layers metal3 -ver_layers metal2} error
puts $error
