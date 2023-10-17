from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readDef("Nangate45_data/gcd.def")
design.evalTclString("read_sdc Nangate45_data/gcd.sdc")

voltage_file = helpers.make_result_file("gcd_voltage_vss.rpt")
em_file = helpers.make_result_file("gcd_em_vss.rpt")
spice_file = helpers.make_result_file("gcd_spice_vss.sp")

pdnsim_aux.check_power_grid(design, net="VSS")
pdnsim_aux.analyze_power_grid(design, vsrc="Vsrc_gcd_vss.loc", outfile=voltage_file,
                              net="VSS", enable_em=True, em_outfile= em_file)

pdnsim_aux.write_pg_spice(design, vsrc="Vsrc_gcd_vss.loc", outfile=spice_file, net="VSS")

helpers.diff_files(voltage_file, "gcd_voltage_vss.rptok")
#helpers.diff_files(em_file, "gcd_em_vss.rptok")
helpers.diff_files(spice_file, "gcd_spice_vss.spok")
