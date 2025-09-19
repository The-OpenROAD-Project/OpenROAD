import openroad as ord
from openroad import Tech, Design
import odb
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = Design(tech)
design.readVerilog("reg1.v")
design.link("top")

floorplan = design.getFloorplan()
floorplan.initFloorplan(
    helpers.make_rect(design, 0, 0, 1000, 1000),
    helpers.make_rect(design, 100, 100, 900, 900),
    floorplan.findSite("FreePDK45_38x28_10R_NP_162NW_34O"),
)

floorplan.makeTracks()

def_file = helpers.make_result_file("make_tracks6.def")
design.writeDef(def_file)
helpers.diff_files("make_tracks6.defok", def_file)
