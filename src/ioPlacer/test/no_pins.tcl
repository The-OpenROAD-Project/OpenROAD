# design without IO pins
read_lef Nangate45/Nangate45.lef
read_def no_pins.def

catch {io_placer -hor_layer 3 -ver_layer 2} error
puts $error
