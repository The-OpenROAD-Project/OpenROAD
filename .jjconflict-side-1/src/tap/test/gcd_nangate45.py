from openroad import Design, Tech
import helpers
import tap

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")
design = helpers.make_design(tech)
design.readDef("gcd_nangate45.def")

options = tap.Options()
options.dist = design.micronToDBU(20)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
options.tapcell_master = tech.getDB().findMaster("TAPCELL_X1")
options.endcap_master = tech.getDB().findMaster("TAPCELL_X1")

design.getTapcell().run(options)

def_file = helpers.make_result_file("gcd_nangate45.def")
design.writeDef(def_file)
helpers.diff_files("gcd_nangate45.defok", def_file)
