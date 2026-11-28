# The unmapping of retime_dsec_enable with the cells it picks marked
# set_dont_use: the unmap gates use other cells.  The cell of the enable
# registers is marked too, so no new register keeps it: they are unmapped
# where retime_dsec_enable moves them as they are.  The result is proven
# sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_enable.v
link_design top
create_clock -period 0.75 clk
set_dont_use {sky130_fd_sc_hd__and2_0 sky130_fd_sc_hd__clkinv_1 \
                sky130_fd_sc_hd__or2_0 sky130_fd_sc_hd__edfxtp_1}

write_verilog_for_dsec retime_dsec_dont_use before
retime
run_dsec_test retime_dsec_dont_use -liberty_files sky130hd/sky130hd_tt.lib \
  -clock clk -resetn rst_n

report_new_cells
