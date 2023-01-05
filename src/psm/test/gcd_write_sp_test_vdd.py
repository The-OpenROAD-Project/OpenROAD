from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("Nangate45.lef")
tech.readLiberty("NangateOpenCellLibrary_typical.lib")

design = Design(tech)
design.readDef("gcd.def")
design.evalTclString("read_sdc gcd.sdc")

spice_file = helpers.make_result_file("gcd_spice_vdd.sp")

pdnsim_aux.write_pg_spice(design, vsrc="Vsrc_gcd_vdd.loc",
                          outfile=spice_file, net="VDD")

helpers.diff_files(spice_file, "gcd_spice_vdd.spok")
