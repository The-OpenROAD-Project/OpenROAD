# test for define_pdn_grid -cells
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

# Create an instance with the same prefix as the macro
odb::dbInst_create \
  [ord::get_db_block] \
  [[ord::get_db] findMaster FILLCELL_X1] \
  {dcache.data.data_arrays_0.data_arrays_0_ext.mem_10}

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

add_pdn_stripe -layer metal4 -width 0.48 -spacing 4.0 -pitch 49.0 -offset 2.5
add_pdn_stripe -layer metal7 -width 1.4 -pitch 40.0 -offset 2.5

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "sram" -cells "fakeram45_64x55"

define_pdn_grid -macro -name "sram" -instances "dcache.data.data_arrays_0.data_arrays_0_ext.mem"

pdngen -report_only
