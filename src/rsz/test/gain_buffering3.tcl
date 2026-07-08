# Gain buffering sink-required ordering vs incremental STA update discipline.
#
# performGainBuffering() sorts the sinks of each net by required time and
# buffers the least-critical prefix (bounded here by set_max_fanout 8), so
# the sink at the group boundary is decided by the required times seen at
# that moment.  Net "nb" has nine sinks: s_thru, whose required path runs
# through GA and the buffer tree inserted on net "na" earlier in the same
# early-sizing pass, and d0-d7, whose paths run through private inverter
# chains.  The required times of s_thru and d7 are close enough that the
# choice of which one joins the buffered group depends on how fresh the
# delays behind them are (bounded cone refresh vs full findAllArrivals
# refresh between nets).
#
# Expected behavior (no golden files yet -- inspect the output):
#
#   current code (bounded incremental STA: upfront findAllArrivals seed,
#   per-driver findRequireds(level+1), end-of-net cone-bounded
#   findDelays/findArrivals -- the fdedc4be STA-discipline changes were
#   reverted; see rebuffer_sta_findings.md sections 9-10):
#     - buffered group on nb = {d0..d6, s_thru}: s_thru is buffered,
#       d7 stays directly on nb
#     - report_checks: worst slack 0.07 (endpoint fa0; the nb gain buffer
#       stacks onto the s_thru -> GA -> na path)
#
#   with a per-net full refresh instead (sta_->findRequired(drvr), as in
#   commit fdedc4be):
#     - buffered group on nb = {d0..d7}: d7 is buffered,
#       s_thru stays directly on nb
#     - report_checks: worst slack 0.16 (endpoint fd7)
#
# The 90 ps WNS delta between the two disciplines is the QoR deviation.
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib

read_verilog gain_buffering3.v
link_design top

create_clock -name clk -period 1.0 [get_ports clk]

set_dont_use {sky130_fd_sc_hd__probe_*
    sky130_fd_sc_hd__lpflow_*
    sky130_fd_sc_hd__clkdly*
    sky130_fd_sc_hd__dlygate*
    sky130_fd_sc_hd__dlymetal*
    sky130_fd_sc_hd__clkbuf_*
    sky130_fd_sc_hd__bufbuf_*
    sky130_fd_sc_hd__buf_2
    sky130_fd_sc_hd__buf_4
    sky130_fd_sc_hd__buf_6
    sky130_fd_sc_hd__buf_8
    sky130_fd_sc_hd__buf_12
    sky130_fd_sc_hd__buf_16
    sky130_fd_sc_hd__inv_2
    sky130_fd_sc_hd__inv_4
    sky130_fd_sc_hd__inv_6
    sky130_fd_sc_hd__inv_8
    sky130_fd_sc_hd__inv_12
    sky130_fd_sc_hd__inv_16}

set_max_fanout 8 [current_design]

repair_design -pre_placement
report_checks -fields {fanout}

# Show which sink ended up behind the gain buffer (d7 with current code,
# s_thru with the legacy bounded-refresh code).
puts "s_thru/A net: [get_full_name [get_nets -of_objects [get_pins s_thru/A]]]"
puts "d7/A net:     [get_full_name [get_nets -of_objects [get_pins d7/A]]]"

set test_name "gain_buffering3"
set verilog_file [make_result_file "${test_name}.v"]
write_verilog $verilog_file
