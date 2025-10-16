from openroad import Design, Tech
import helpers
import tap

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")
design = helpers.make_design(tech)
design.readDef("gcd_fakeram.def")

options = tap.Options()
options.dist = design.micronToDBU(20)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
options.tapcell_master = tech.getDB().findMaster("TAPCELL_X1")
options.endcap_master = tech.getDB().findMaster("TAPCELL_X1")

design.getTapcell().run(options)

def_file = helpers.make_result_file("gcd_fakeram.def")
design.writeDef(def_file)
helpers.diff_files("gcd_fakeram.defok", def_file)
