# init_floorplan called twice for some stupid reason

from openroad import Tech, Design
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readVerilog("reg1.v")
design.link("top")

die1 = helpers.make_rect(design, 0, 0, 1000, 1000)
core1 = helpers.make_rect(design, 100, 100, 900, 900)
die2 = helpers.make_rect(design, 100, 100, 1100, 1100)
core2 = helpers.make_rect(design, 200, 200, 800, 800)

floorplan = design.getFloorplan()
site = floorplan.findSite("FreePDK45_38x28_10R_NP_162NW_34O")
floorplan.initFloorplan(die1, core1, site)
floorplan.initFloorplan(die2, core2, site)

def_file = helpers.make_result_file("init_floorplan7.def")
design.writeDef(def_file)
helpers.diff_files("init_floorplan7.defok", def_file)
