# repair_design shorted outputs (synthesized RAM)
source "helpers.tcl"
read_lef sky130hd/sky130hd_vt.tlef
read_lef sky130hd/sky130hd_std_cell_vt.lef
read_liberty sky130hd/sky130hd_tt.lib
read_def repair_design2.def

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

report_equiv_cells sky130_fd_sc_hd__buf_1

report_equiv_cells sky130_fd_sc_hd__buf_1 -match_cell_footprint

set_opt_config -sizing_area_limit 3.0

report_equiv_cells sky130_fd_sc_hd__buf_1 -match_cell_footprint

reset_opt_config

report_equiv_cells sky130_fd_sc_hd__buf_1 -match_cell_footprint

report_equiv_cells sky130_fd_sc_hd__clkinv_2

set_opt_config -keep_sizing_vt true -limit_sizing_area 2.0 \
  -limit_sizing_leakage 2.0 -keep_sizing_site true

report_equiv_cells sky130_fd_sc_hd__clkinv_2

report_equiv_cells -all sky130_fd_sc_hd__clkinv_2

report_equiv_cells sky130_fd_sc_hd__clkinv_2

# o211ai_1 has unusually low leakage, so every larger o211ai exceeds a 4X
# leakage limit.  The smallest larger cell (o211ai_2) remains swappable.
reset_opt_config
set_opt_config -limit_sizing_leakage 4.0

report_equiv_cells -all -match_cell_footprint sky130_fd_sc_hd__o211ai_1
