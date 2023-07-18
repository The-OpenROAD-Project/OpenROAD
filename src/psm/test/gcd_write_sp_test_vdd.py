from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readDef("Nangate45_data/gcd.def")
design.evalTclString("read_sdc Nangate45_data/gcd.sdc")

spice_file = helpers.make_result_file("gcd_spice_vdd.sp")

pdnsim_aux.write_pg_spice(design, vsrc="Vsrc_gcd_vdd.loc",
                          outfile=spice_file, net="VDD")

helpers.diff_files(spice_file, "gcd_spice_vdd.spok")
