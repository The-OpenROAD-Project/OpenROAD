source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc

set voltage_file [make_result_file gcd_all_vss-voltage.rpt]
set em_file [make_result_file gcd_all_vss-em.rpt]
set spice_file [make_result_file gcd_all_vss-spice.sp]

set_pdnsim_source_settings -external_resistance 100
analyze_power_grid -vsrc Vsrc_gcd_vss.loc -net VSS
