from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = helpers.make_design(tech)
design.readDef("Nangate45_data/gcd.def")
design.evalTclString("read_sdc Nangate45_data/gcd.sdc")

em_file = helpers.make_result_file("gcd_em_test_vdd-em.rpt")
pdnsim_aux.analyze_power_grid(
    design, vsrc="Vsrc_gcd_vdd.loc", enable_em=True, em_outfile=em_file, net="VDD"
)

helpers.diff_files(em_file, "gcd_em_test_vdd-em.rptok")
