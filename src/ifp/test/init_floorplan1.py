from openroad import *
import odb

def make_rect(design, xl, yl, xh, yh):
    xl = design.micronToDBU(xl)
    yl = design.micronToDBU(yl)
    xh = design.micronToDBU(xh)
    yh = design.micronToDBU(yh)
    return odb.Rect(xl, yl, xh, yh)

tech = Tech()
tech.readLEF("Nangate45/Nangate45_tech.lef")
tech.readLEF("Nangate45/Nangate45_stdcell.lef")

tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readVerilog("reg1.v")
design.link("top")

floorplan = design.getFloorplan()
floorplan.initialize(make_rect(design, 0, 0, 1000, 1000),
                     make_rect(design, 100, 100, 900, 900),
                     "FreePDK45_38x28_10R_NP_162NW_34O")

design.writeDb("results/init_floorplan1.py.odb")
design.writeDef("results/init_floorplan1.py.def")
