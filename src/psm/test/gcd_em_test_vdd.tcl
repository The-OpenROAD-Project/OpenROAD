source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc

set em_file [make_result_file gcd_em_test_vdd-em.rpt]
analyze_power_grid -vsrc Vsrc_gcd_vdd.loc -enable_em -em_outfile $em_file -net VDD

diff_files $em_file gcd_em_test_vdd-em.rptok
