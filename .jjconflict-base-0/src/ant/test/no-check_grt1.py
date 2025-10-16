from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLiberty("sky130hs/sky130hs_tt.lib")
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")

design = helpers.make_design(tech)
design.readDef("gcd_sky130.def")

# TODO: grt not yet wrapped and we need that here
# groute = design.getGlobalRouter()

# Need Python translation of the 2 following tcl cmds
# set_routing_layers -signal met1-met5
# global_route

# ack = design.getAntennaChecker()
# ack.check_antennas("", False)
