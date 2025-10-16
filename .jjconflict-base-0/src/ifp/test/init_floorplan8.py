from openroad import Tech, Design
import odb
import helpers
import ifp_helpers as ifph

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readVerilog("reg1.v")
design.link("top")

die = helpers.make_rect(design, 0, 0, 150, 150)
core = helpers.make_rect(design, 20, 20, 130, 130)

l = design.micronToDBU(27)
u = design.micronToDBU(60)

ifph.create_voltage_domain(design, "TEMP_ANALOG", (l, l, u, u))

floorplan = design.getFloorplan()
site = floorplan.findSite("FreePDK45_38x28_10R_NP_162NW_34O")
floorplan.initFloorplan(die, core, site)

def_file = helpers.make_result_file("init_floorplan8.def")
design.writeDef(def_file)
helpers.diff_files("init_floorplan8.defok", def_file)
