# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Drive OpenROAD's integrated synthesis tool (sv_elaborate + synthesize)
# on gcd.v and dump the mapped netlist via write_verilog.
#
# Required environment variables:
#   LEF_FILES    space-separated list of .lef paths (tech + std-cell);
#                needed by syn::export_to_odb at the end of synthesize.
#   LIB_FILES    space-separated list of .lib / .lib.gz paths
#   VERILOG_FILE input Verilog/SystemVerilog source
#   OUT_NETLIST  destination .v path for the mapped netlist

foreach lef_file $::env(LEF_FILES) {
  read_lef $lef_file
}

foreach lib_file $::env(LIB_FILES) {
  read_liberty $lib_file
}

sv_elaborate $::env(VERILOG_FILE)
synthesize
write_verilog $::env(OUT_NETLIST)
