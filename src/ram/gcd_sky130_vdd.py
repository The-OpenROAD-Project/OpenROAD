from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130hd_std_cell.lef")
tech.readLiberty("sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib")

design = helpers.make_design(tech)
design.readDef("sky130hd_data/gcd_sky130hd_floorplan.def")
design.evalTclString("read_sdc sky130hd_data/gcd_sky130hd_floorplan.sdc")

pdnsim_aux.check_power_grid(design, net="VDD")
