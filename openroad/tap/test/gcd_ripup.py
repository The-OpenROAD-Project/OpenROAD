from openroad import Design, Tech
import helpers
import tap
import odb

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")
design = helpers.make_design(tech)
design.readDef("gcd_ripup.def")

options = tap.Options()
options.dist = design.micronToDBU(20)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
options.tapcell_master = tech.getDB().findMaster("TAPCELL_X1")
options.endcap_master = tech.getDB().findMaster("TAPCELL_X1")

design.getTapcell().run(options)

design.getTapcell().setTapPrefix("TAP_")
design.getTapcell().removeCells("TAP_")
design.getTapcell().setEndcapPrefix("PHY_")
design.getTapcell().removeCells("PHY_")
design.getTapcell().reset()


def_file = helpers.make_result_file("gcd_ripup.def")
design.writeDef(def_file)

helpers.diff_files("gcd_ripup.defok", def_file)
