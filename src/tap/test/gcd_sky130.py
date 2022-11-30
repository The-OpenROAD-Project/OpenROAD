from openroad import Design, Tech
import helpers
import tap

tech = Tech()
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")
design = Design(tech)
design.readDef("gcd_sky130hs_floorplan.def")

options = tap.Options()
options.dist   = design.micronToDBU(15)
options.halo_x = design.micronToDBU(2)
options.halo_y = design.micronToDBU(2)
options.tapcell_master = tech.getDB().findMaster("sky130_fd_sc_hs__tap_1")
options.endcap_master  = tech.getDB().findMaster("sky130_fd_sc_hs__decap_4")

design.getTapcell().run(options)

def_file = helpers.make_result_file("gcd_sky130.def")
design.writeDef(def_file)
helpers.diff_files("gcd_sky130.defok", def_file)
