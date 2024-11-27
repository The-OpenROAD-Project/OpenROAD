source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc

set voltage_file [make_result_file gcd_test_vdd-voltage.rpt]

check_power_grid -net VDD
analyze_power_grid -vsrc Vsrc_gcd_vdd.loc -voltage_file $voltage_file -net VDD

diff_files $voltage_file gcd_test_vdd-voltage.rptok
