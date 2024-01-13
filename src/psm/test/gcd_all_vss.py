from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readDef("Nangate45_data/gcd.def")
design.evalTclString("read_sdc Nangate45_data/gcd.sdc")

voltage_file = helpers.make_result_file("gcd_all_vss-voltage.rpt")
em_file = helpers.make_result_file("gcd_all_vss-em.rpt")
spice_file = helpers.make_result_file("gcd_all_vss-spice.sp")

pdnsim_aux.check_power_grid(design, net="VSS")
pdnsim_aux.analyze_power_grid(
    design,
    vsrc="Vsrc_gcd_vss.loc",
    outfile=voltage_file,
    net="VSS",
    enable_em=True,
    em_outfile=em_file)

helpers.diff_files(voltage_file, "gcd_all_vss-voltage.rptok")
helpers.diff_files(em_file, "gcd_all_vss-em.rptok")

pdnsim_aux.write_pg_spice(
    design,
    vsrc="Vsrc_gcd_vss.loc",
    outfile=spice_file,
    net="VSS")

helpers.diff_files(spice_file, "gcd_all_vss-spice.spok")
