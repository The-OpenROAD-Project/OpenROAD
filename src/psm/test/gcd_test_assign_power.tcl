source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc

set voltage_file [make_result_file gcd_test_vdd-voltage.rpt]

set_pdnsim_inst_power -inst _440_ -power 1e-5
set_pdnsim_inst_power -inst _829_ -power 1e-5

check_power_grid -net VDD
analyze_power_grid -vsrc Vsrc_gcd_vdd.loc -net VDD
