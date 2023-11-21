# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

catch {place_pins -hor_layers metal3 \
                  -ver_layers metal2 \
                  -corner_avoidance 0 \
                  -min_distance 0.12 \
                  -group_pins {resp_msg[10] resp_msg[11] resp_msg[12] resp_msg[13]} \
                  -group_pins {req_msg[10] req_msg[11] req_msg[12] resp_msg[13]} \
                  -group_pins {req_msg[14] req_msg[15] req_msg[2] resp_msg[11]}
} error

puts $error
