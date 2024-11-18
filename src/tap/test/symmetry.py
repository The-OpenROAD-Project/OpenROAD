from openroad import Design, Tech
import helpers
import tap

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("symmetry.lef")
design = helpers.make_design(tech)
design.readDef("symmetry.def")

options = tap.Options()
options.dist = design.micronToDBU(120)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
options.tapcell_master = tech.getDB().findMaster("TAPCELL")
options.endcap_master = tech.getDB().findMaster("TAPCELL")

design.getTapcell().run(options)

def_file = helpers.make_result_file("symmetry.def")
design.writeDef(def_file)
helpers.diff_files("symmetry.defok", def_file)
