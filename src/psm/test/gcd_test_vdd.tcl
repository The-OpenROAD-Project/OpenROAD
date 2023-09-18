source helpers.tcl

read_lef  Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc

set voltage_file [make_result_file gcd_voltage_vdd.rpt]
set error_file [make_result_file gcd_error_vdd.rpt]
check_power_grid -net VDD
analyze_power_grid -vsrc Vsrc_gcd_vdd.loc -outfile $voltage_file -net VDD \
    -error_file $error_file
diff_files $voltage_file gcd_voltage_vdd.rptok
diff_files $error_file gcd_error_vdd.rptok
