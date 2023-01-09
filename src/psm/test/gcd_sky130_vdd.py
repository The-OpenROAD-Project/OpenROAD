from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("sky130_data/sky130hd.tlef")
tech.readLef("sky130_data/sky130hd_std_cell.lef")
tech.readLiberty("sky130_data/sky130hd_tt.lib")

design = Design(tech)
design.readDef("sky130_data/gcd_sky130hd_floorplan.def")
design.evalTclString("read_sdc sky130_data/gcd_sky130hd_floorplan.sdc")

pdnsim_aux.check_power_grid(design, net="VDD")
