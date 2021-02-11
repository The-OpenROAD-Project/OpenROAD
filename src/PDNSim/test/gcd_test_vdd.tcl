source helpers.tcl

read_lef  Nangate45.lef
read_def gcd.def
read_liberty NangateOpenCellLibrary_typical.lib
read_sdc gcd.sdc

# The command below runs a check for connectivity of the power grid
# The analyze_power_grid command calls it by default
# check_power_grid -vsrc Vsrc_gcd.loc 
set voltage_file [make_result_file gcd_voltage_vdd.rpt]
check_power_grid -net VDD
analyze_power_grid -vsrc Vsrc_gcd_vdd.loc -outfile $voltage_file -net VDD
diff_files $voltage_file gcd_voltage_vdd.rptok
