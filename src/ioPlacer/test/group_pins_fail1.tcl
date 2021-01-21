# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

catch {place_pins -hor_layers 2 -ver_layers 3 -boundaries_offset 0 -min_distance 1 -group_pins {resp_val resp_rdy req_rdy} -group_pins {req_msg[15] req_msg[14] resp_msg[15] resp_msg[141]}} error

puts "$error"
