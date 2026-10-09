# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Test: ram/generate_regfile_modes_asap7
# mode macro: the register file is a macro to its parent, every instance
# placed by the generator, none left to the parent, and nothing to say
# about it. (mode netlist, which leaves the periphery to the parent, is
# checked by generate_regfile_asap7.)

source "helpers.tcl"
source "regfile_checks.tcl"

rf_read_libraries

set spec [rf_spec generate_regfile_asap7.regfile \
  generate_regfile_modes_asap7.regfile {"mode macro"}]
tee -quiet -variable gen_log [list generate_regfile \
  -spec $spec -check_ports generate_regfile_asap7_rtl.v]

set placement [rf_placement]
check "mode macro places every instance" \
  { dict get $placement unplaced } 0
check "mode macro fixes every storage flop" \
  { dict get $placement flops_not_fixed } 0
check "mode macro reports nothing left unplaced" \
  { string match "*RAM-0051*" $gen_log } 0

exit_summary
