from openroad import Tech, Design
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readVerilog("reg1.v")
design.link("top")

space = design.micronToDBU(2)

floorplan = design.getFloorplan()
site = floorplan.findSite("FreePDK45_38x28_10R_NP_162NW_34O")
floorplan.initFloorplan(30, 0.5, space, space, space, space, site)

def_file = helpers.make_result_file("init_floorplan3.def")
design.writeDef(def_file)
helpers.diff_files("init_floorplan3.defok", def_file)
