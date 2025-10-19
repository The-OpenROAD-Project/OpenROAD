from openroad import Design, Tech
import helpers
import tap
import odb

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")
design = helpers.make_design(tech)
design.readDef("boundary_macros.def")

options = tap.Options()
options.dist = design.micronToDBU(20)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
options.tapcell_master = tech.getDB().findMaster("TAPCELL_X1")
options.endcap_master = tech.getDB().findMaster("TAPCELL_X1")
options.tap_nwin2_master = tech.getDB().findMaster("TAPCELL_X1")
options.tap_nwin3_master = tech.getDB().findMaster("TAPCELL_X1")
options.tap_nwout2_master = tech.getDB().findMaster("TAPCELL_X1")
options.tap_nwout3_master = tech.getDB().findMaster("TAPCELL_X1")
options.tap_nwintie_master = tech.getDB().findMaster("TAPCELL_X1")
options.tap_nwouttie_master = tech.getDB().findMaster("TAPCELL_X1")
options.cnrcap_nwin_master = tech.getDB().findMaster("TAPCELL_X1")
options.cnrcap_nwout_master = tech.getDB().findMaster("TAPCELL_X1")
options.incnrcap_nwin_master = tech.getDB().findMaster("TAPCELL_X1")
options.incnrcap_nwout_master = tech.getDB().findMaster("TAPCELL_X1")

design.getTapcell().run(options)
design.getOpendp().checkPlacement(True)

def_file = helpers.make_result_file("boundary_macros.def")
design.writeDef(def_file)

helpers.diff_files("boundary_macros.defok", def_file)
