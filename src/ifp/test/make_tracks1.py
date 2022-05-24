from openroad import Tech, Design
import helpers

tech = Tech()
tech.readLEF("Nangate45/Nangate45_tech.lef")
tech.readLEF("Nangate45/Nangate45_stdcell.lef")

tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readVerilog("reg1.v")
design.link("top")

floorplan = design.getFloorplan()
floorplan.initFloorplan(helpers.make_rect(design, 0, 0, 1000, 1000),
                        helpers.make_rect(design, 100, 100, 900, 900),
                        "FreePDK45_38x28_10R_NP_162NW_34O")

floorplan.makeTracks()

design.writeDef("results/make_tracks1.py.def")
