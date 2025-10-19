from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = helpers.make_design(tech)
design.readDef("Nangate45_data/gcd.def")
design.evalTclString("read_sdc Nangate45_data/gcd.sdc")

pdnsim_aux.set_pdnsim_net_voltage(design, net="VDD", voltage=1.5)
pdnsim_aux.analyze_power_grid(design, net="VDD")
pdnsim_aux.analyze_power_grid(design, net="VSS")
