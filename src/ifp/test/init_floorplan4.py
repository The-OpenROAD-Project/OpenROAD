from openroad import Tech, Design
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readVerilog("reg1.v")
design.link("top")

die_area = helpers.make_rect(design, 0, 0, 1000, 1000)
core_area = helpers.make_rect(design, 110, 110, 900, 900)

floorplan = design.getFloorplan()
site = floorplan.findSite("FreePDK45_38x28_10R_NP_162NW_34O")
floorplan.initFloorplan(die_area, core_area, site)

def_file = helpers.make_result_file("init_floorplan4.def")
design.writeDef(def_file)
helpers.diff_files("init_floorplan4.defok", def_file)
