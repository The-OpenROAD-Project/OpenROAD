source helpers.tcl

read_lef  Nangate45.lef
read_def gcd.def
read_liberty NangateOpenCellLibrary_typical.lib
read_sdc gcd.sdc

set voltage_file [make_result_file gcd_voltage_vss.rpt]
set em_file [make_result_file gcd_em_vss.rpt]
set spice_file [make_result_file gcd_spice_vss.sp]

check_power_grid -net VSS
analyze_power_grid -vsrc Vsrc_gcd_vss.loc -outfile $voltage_file -net VSS -enable_em -em_outfile $em_file
write_pg_spice -vsrc Vsrc_gcd_vss.loc -outfile $spice_file -net VSS

diff_files $voltage_file gcd_voltage_vss.rptok
#diff_files $em_file gcd_em_vss.rptok
diff_files $spice_file gcd_spice_vss.spok
