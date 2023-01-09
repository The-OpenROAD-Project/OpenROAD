from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("asap7_data/asap7.lef")
design = Design(tech)
design.readDef("asap7_data/aes_place.def")
tech.readLiberty("asap7_data/asap7.lib")

design.evalTclString("read_sdc asap7_data/aes_place.sdc")

pdnsim_aux.analyze_power_grid(design, net="VDD")
