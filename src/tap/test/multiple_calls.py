from openroad import Design, Tech
import helpers
import tap

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")
design = Design(tech)
design.readDef("gcd_ripup.def")

options = tap.Options()
options.dist   = design.micronToDBU(20)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
options.tapcell_master = tech.getDB().findMaster("TAPCELL_X1")
options.endcap_master  = tech.getDB().findMaster("TAPCELL_X1")

design.getTapcell().run(options)

def_file1 = helpers.make_result_file("mc1.def")
design.writeDef(def_file1)

design.getTapcell().clear()

options.dist   = design.micronToDBU(10)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
design.getTapcell().run(options)
def_file2 = helpers.make_result_file("mc2.def")
design.writeDef(def_file2)

helpers.diff_files(def_file1, def_file2)
