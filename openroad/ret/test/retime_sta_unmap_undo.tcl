# A trial that unmaps registers and that STA rejects is undone.  r4 and r5
# (read through QN) sit far from g10 and g11, which feed them, and the
# inputs are timed.  The first trial unmaps r4 and r5 and slides across
# the unmap gates; the search times those gates before they are placed, so
# without their wires, and STA rejects the placed result.  The trial is
# undone, which restores r4 and r5; the first half of the ranked slides is
# the other cone and is kept.  Round 2 proposes the slides behind r4 and r5
# again: each trial unmaps them, STA rejects it and it is undone, so r4 and
# r5 stay as they were, r4 still in the path group that lists every
# register (-move_sdc_registers lets the listed registers move).  The
# result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_existing_names.v
link_design top
initialize_floorplan -die_area "0 0 3500 20" \
  -core_area "0 0 3500 20" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
create_clock -period 0.16 clk
set_input_delay -0.02 -clock clk [delete_from_list [all_inputs] [get_ports clk]]

# Placed locations in microns: cone 1 120 microns apart, cone 2 1900
# microns past g10 and g11.
set x 10
foreach name {g0 g1 r1 r2 g2 g3 g4 g5 g6 g7 ret_ff_0 g8} {
  place_inst -name $name -location "$x 2.8"
  incr x 120
}
place_inst -name g10 -location "1450 2.8"
place_inst -name g11 -location "1455 2.8"
set x 3360
foreach name {r4 r5 g12 g13 g14 g15 g16 g17 unmap_ff_0 unmap_inv_2} {
  place_inst -name $name -location "$x 2.8"
  incr x 5
}
set_wire_rc -resistance 4.0e-02 -capacitance 1.0e-05
estimate_parasitics -placement
group_path -name registers -to [all_registers]

write_verilog_for_dsec retime_sta_unmap_undo before
report_worst_slack -max -digits 4
report_tns -digits 4

retime -move_sdc_registers

estimate_parasitics -placement
report_worst_slack -max -digits 4
report_tns -digits 4
set block [ord::get_db_block]
foreach name {r4 r5} {
  set inst [$block findInst $name]
  if { $inst == "NULL" } {
    puts "$name: removed"
  } else {
    puts "$name: [[$inst getMaster] getName]"
  }
}
set sdc_file [make_result_file retime_sta_unmap_undo.sdc]
write_sdc $sdc_file
set stream [open $sdc_file r]
set sdc [read $stream]
close $stream
regexp {group_path[^#]*} $sdc path_group
puts "path group lists r4: [regexp {{r4}} $path_group]"
run_dsec_test retime_sta_unmap_undo \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n

report_new_cells
