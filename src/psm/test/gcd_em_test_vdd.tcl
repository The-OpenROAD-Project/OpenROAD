source helpers.tcl

read_lef  Nangate45.lef
read_def gcd.def
read_liberty NangateOpenCellLibrary_typical.lib
read_sdc gcd.sdc

# The command below runs a check for connectivity of the power grid
# The analyze_power_grid command calls it by default
# check_power_grid -vsrc Vsrc_gcd.loc 
set em_file [make_result_file gcd_em_vdd.rpt]
check_power_grid -net VDD
#analyze_power_grid -vsrc Vsrc_gcd.loc -enable_em
analyze_power_grid -vsrc Vsrc_gcd_vdd.loc -enable_em -em_outfile $em_file -net VDD
#diff_files $em_file gcd_em_vdd.rptok
