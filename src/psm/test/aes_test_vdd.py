from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("Nangate45.lef")
tech.readLiberty("NangateOpenCellLibrary_typical.lib")

design = Design(tech)
design.readDef("aes.def")
design.evalTclString("read_sdc aes.sdc")

pdnsim_aux.analyze_power_grid(design, vsrc="Vsrc_aes_vdd.loc", net="VDD")
