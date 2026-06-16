# Test for the swapPins() infinite loop when exactly one lone pin
# is constrained and one is unconstrained

source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

# Mirror ALL pins except clk and reset — making them non-lone.
# This leaves exactly 2 lone pins: clk (constrained) and reset (unconstrained).
set_io_pin_constraint -mirrored_pins {resp_msg[0] req_msg[0] resp_msg[1] \
  req_msg[1] resp_msg[2] req_msg[2] resp_msg[3] req_msg[3] resp_msg[4] \
  req_msg[4] resp_msg[5] req_msg[5] resp_msg[6] req_msg[6] resp_msg[7] \
  req_msg[7] resp_msg[8] req_msg[8] resp_msg[9] req_msg[9] resp_msg[10] \
  req_msg[10] resp_msg[11] req_msg[11] resp_msg[12] req_msg[12] \
  resp_msg[13] req_msg[13] resp_msg[14] req_msg[14] resp_msg[15] \
  req_msg[15]}

set_io_pin_constraint -mirrored_pins {req_msg[16] req_rdy req_msg[17] \
  resp_rdy req_msg[18] resp_val req_msg[19] req_val req_msg[20] \
  req_msg[21] req_msg[22] req_msg[23] req_msg[24] req_msg[25] \
  req_msg[26] req_msg[27] req_msg[28] req_msg[29] req_msg[30] \
  req_msg[31]}

# Constrain clk to top edge — this makes clk a "constrained lone pin"
set_io_pin_constraint -region top:* -pin_names clk

# reset is left unconstrained and unmirrored — the only other lone pin.
# Bug trigger: swapPins() picks pin1=reset, then searches for pin2 that
# is not mirrored, not grouped, not constrained, and not pin1. Only clk
# qualifies but it IS constrained, so no valid pin2 exists → infinite loop.

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12 -annealing

set def_file [make_result_file annealing_mirrored_hang.def]

write_def $def_file

diff_file annealing_mirrored_hang.defok $def_file
