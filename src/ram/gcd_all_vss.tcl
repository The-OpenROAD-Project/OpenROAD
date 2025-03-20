source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc

set voltage_file [make_result_file gcd_all_vss-voltage.rpt]
set em_file [make_result_file gcd_all_vss-em.rpt]
set spice_file [make_result_file gcd_all_vss-spice.sp]

check_power_grid -net VSS
analyze_power_grid -vsrc Vsrc_gcd_vss.loc -voltage_file $voltage_file -net VSS \
  -enable_em -em_outfile $em_file

diff_files $voltage_file gcd_all_vss-voltage.rptok
diff_files $em_file gcd_all_vss-em.rptok

write_pg_spice -vsrc Vsrc_gcd_vss.loc -net VSS $spice_file

diff_files $spice_file gcd_all_vss-spice.spok
