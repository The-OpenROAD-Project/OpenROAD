import openroad as ord
import odb
from openroad import Tech, Design
import helpers, ifp_helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readVerilog("tiecells.v")
design.link("top")

floorplan = design.getFloorplan()
floorplan.initFloorplan(
    helpers.make_rect(design, 0, 0, 1000, 1000),
    helpers.make_rect(design, 100, 100, 900, 900),
    floorplan.findSite("FreePDK45_38x28_10R_NP_162NW_34O"),
)


ifp_helpers.insert_tiecells(tech, floorplan, "LOGIC0_X1/Z", "TIE_ZERO_")
ifp_helpers.insert_tiecells(tech, floorplan, "LOGIC1_X1/Z")

def_file = helpers.make_result_file("tiecells.def")
design.writeDef(def_file)
helpers.diff_files("tiecells.defok", def_file)
