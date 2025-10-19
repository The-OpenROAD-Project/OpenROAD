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
    helpers.make_rect(design, 0, 0, 200, 200),
    helpers.make_rect(design, 10, 10, 190, 190),
    floorplan.findSite("FreePDK45_38x28_10R_NP_162NW_34O"),
)

db_tech = tech.getTech()
m2 = db_tech.findLayer("metal2")

x_offset = design.micronToDBU(300)
x_pitch = design.micronToDBU(0.2)
y_offset = design.micronToDBU(0.1)
y_pitch = design.micronToDBU(0.2)
floorplan.makeTracks(m2, x_offset, x_pitch, y_offset, y_pitch)

x_offset = design.micronToDBU(0.1)
x_pitch = design.micronToDBU(0.2)
y_offset = design.micronToDBU(300)
y_pitch = design.micronToDBU(0.2)
floorplan.makeTracks(m2, x_offset, x_pitch, y_offset, y_pitch)
