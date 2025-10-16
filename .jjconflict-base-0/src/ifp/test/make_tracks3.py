import openroad as ord
from openroad import Tech, Design
import odb
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readVerilog("reg1.v")
design.link("top")

floorplan = design.getFloorplan()
floorplan.initFloorplan(
    helpers.make_rect(design, 10, 20, 1010, 1020),
    helpers.make_rect(design, 110, 120, 910, 920),
    floorplan.findSite("FreePDK45_38x28_10R_NP_162NW_34O"),
)

db_tech = tech.getTech()
m1 = db_tech.findLayer("metal1")
m2 = db_tech.findLayer("metal2")

x_offset = design.micronToDBU(0.1)
x_pitch = design.micronToDBU(0.2)
y_offset = design.micronToDBU(0.1)
y_pitch = design.micronToDBU(0.2)

floorplan.makeTracks(m1, x_offset, x_pitch, y_offset, y_pitch)
floorplan.makeTracks(m2, x_offset, x_pitch, y_offset, y_pitch)

def_file = helpers.make_result_file("make_tracks3.def")
design.writeDef(def_file)
helpers.diff_files("make_tracks3.defok", def_file)
