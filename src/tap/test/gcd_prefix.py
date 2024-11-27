from openroad import Design, Tech
import helpers
import tap
import odb

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")
design = helpers.make_design(tech)
design.readDef("gcd_prefix.def")

options = tap.Options()
options.dist = design.micronToDBU(20)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
options.tapcell_master = tech.getDB().findMaster("TAPCELL_X1")
options.endcap_master = tech.getDB().findMaster("TAPCELL_X1")

design.getTapcell().setTapPrefix("CHECK_TAPCELL_")
design.getTapcell().setEndcapPrefix("CHECK_END_")

design.getTapcell().run(options)

def_file = helpers.make_result_file("gcd_prefix.def")
design.writeDef(def_file)
helpers.diff_files("gcd_prefix.defok", def_file)
