from openroad import Design, Tech
import helpers
import pdnsim_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readDef("Nangate45_data/aes.def")
design.evalTclString("read_sdc Nangate45_data/aes.sdc")

pdnsim_aux.analyze_power_grid(design, vsrc="Vsrc_aes_vdd.loc", net="VDD",
                              node_density_factor=3)
