from openroad import Tech, Design
import helpers
import ifp_helpers as ifph

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130_fd_sc_hd_merged.lef")
tech.readLiberty("sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib")

design = Design(tech)
design.readVerilog("reg2.v")
design.link("top")

die = helpers.make_rect(design, 0, 0, 155.48, 146.88)
core = helpers.make_rect(design, 18.4, 16.32, 137.08, 130.56)

floorplan = design.getFloorplan()

lx = design.micronToDBU(33.58)
ly = design.micronToDBU(32.64)
ux = design.micronToDBU(64.86)
uy = design.micronToDBU(62.56)

ifph.create_voltage_domain(design, "TEMP_ANALOG", (lx, ly, ux, uy))

floorplan.initFloorplan(die, core, floorplan.findSite("unithd"))

def_file = helpers.make_result_file("init_floorplan9.def")
design.writeDef(def_file)
helpers.diff_files("init_floorplan9.defok", def_file)
