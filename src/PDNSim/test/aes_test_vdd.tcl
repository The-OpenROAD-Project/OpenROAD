read_lef  Nangate45.lef
read_def aes.def
read_liberty NangateOpenCellLibrary_typical.lib
read_sdc aes.sdc

# The command below runs a check for connectivity of the power grid
# The analyze_power_grid command calls it by default
# check_power_grid -vsrc Vsrc_gcd.loc 
analyze_power_grid -vsrc Vsrc_aes_vdd.loc -net VDD
