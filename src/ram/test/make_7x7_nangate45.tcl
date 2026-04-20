# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024-2025, The OpenROAD Authors
#
# Test: ram/make_7x7_nangate45
# Verifies that generate_ram works correctly with the Nangate45 technology
# node using an intentionally odd 7-word x 1-byte (49-bit) configuration
# to exercise the decoder logic on a non-power-of-2 word count.

source "helpers.tcl"

set_thread_count [expr [cpu_count] / 4]

read_liberty Nangate45/Nangate45_typ.lib

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45.lef

set behavioral_file [make_result_file make_7x7_nangate45_behavioral.v]

generate_ram \
  -mask_size 7 \
  -word_size 7 \
  -num_words 7 \
  -read_ports 1 \
  -storage_cell DFF_X1 \
  -power_pin VDD \
  -ground_pin VSS \
  -routing_layer {metal1 0.08} \
  -ver_layer {metal4 0.14 9} \
  -hor_layer {metal3 0.08 8} \
  -filler_cells {FILLCELL_X1 FILLCELL_X2 FILLCELL_X4 FILLCELL_X8} \
  -tapcell TAPCELL_X1 \
  -max_tap_dist 10 \
  -write_behavioral_verilog $behavioral_file

set lef_file [make_result_file make_7x7_nangate45.lef]
write_abstract_lef -bloat_occupied_layers $lef_file
diff_files make_7x7_nangate45.lefok $lef_file

set def_file [make_result_file make_7x7_nangate45.def]
write_def $def_file
diff_files make_7x7_nangate45.defok $def_file

diff_files make_7x7_nangate45_behavioral.vok $behavioral_file
