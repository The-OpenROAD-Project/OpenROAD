# Regression for PR #3194: dbNetwork::highestConnectedNet on a hier
# dbModNet must resolve to the associated flat dbNet so that
# Parasitics::findParasiticNet hits the parasitic_network_map_ entry
# inserted under the flat dbNet key.
#
# Why this test uses read_spef rather than estimate_parasitics:
# estimate_parasitics keys parasitic_network_map_ via findParasiticNet
# for BOTH insert and lookup, so the bug is self-consistent and
# invisible (insert key == lookup key, both buggy or both fixed).
# SPEF read keys insertion by flat dbNet (parser -> findNet -> flat
# at top scope) while lookup still uses findParasiticNet ->
# dbNetwork::highestConnectedNet. Pre-fix override echoed the modnet
# wrapper -> modnet key != flat key -> cache miss -> driver
# classified unannotated. Fix returns the related flat dbNet -> hit.
#
# Design (hier_highest_connected_net.v): 2-level hier with reg-to-reg
# data path crossing sub_inst. Top clk is buffered by ck_buf so the
# clock-tree driver iterated is a leaf iterm, not the top BTerm; this
# avoids the top-port bypass branch in Parasitics::findParasiticNet
# (which returns network_->net(term) directly with no
# highestConnectedNet call and is independent of PR #3194) that would
# otherwise leave clk unannotated on the fixed binary too.
#
# Hier-crossing nets (carry both flat dbNet + dbModNet on their leaf
# iterms after link_design -hier):
#   clk_int      : ck_buf/Z -> ff_top/CK, ff_out/CK, sub_inst/ff_sub/CK
#   midnet       : ff_top/Q -> sub_inst/ff_sub/D
#   intermediate : sub_inst/buf_sub/Z -> ff_out/D
#
# Expected outputs:
#
# Fixed binary:
#   Found 3 unannotated drivers.
#    ff_out/QN
#    ff_top/QN
#    sub_inst/ff_sub/QN
#   Found 0 partially unannotated drivers.
#
# Bug binary:
#   Found 6 unannotated drivers.
#    ck_buf/Z
#    ff_out/QN
#    ff_top/Q
#    ff_top/QN
#    sub_inst/buf_sub/Z
#    sub_inst/ff_sub/QN
#   Found 0 partially unannotated drivers.
#
# The 3 baseline entries (ff_top/QN, ff_out/QN, sub_inst/ff_sub/QN)
# are dangling DFF QN outputs -- no loads, no wire, no parasitic
# possible. The 3 bug-extras (ck_buf/Z, ff_top/Q, sub_inst/buf_sub/Z)
# are the modnet-wrapped drivers on the hier-crossing nets above:
# findParasiticNet returns their dbModNet wrapper, SPEF-inserted
# entries are keyed by flat dbNet -> map miss on bug binary, hit on
# fixed binary.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog hier_highest_connected_net.v
link_design top -hier

create_clock -name clk -period 5 [get_ports clk]
set_input_delay -clock clk 0 [get_ports in]
set_output_delay -clock clk 0 [get_ports out]

read_spef hier_highest_connected_net.spef

report_parasitic_annotation -report_unannotated
