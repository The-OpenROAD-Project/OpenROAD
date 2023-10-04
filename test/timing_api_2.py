from openroad import Tech, Design

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = Design(tech)
design.readDef("gcd_nangate45.def")

for inst in design.getBlock().getInsts():
    print(inst.getName(), 
          inst.getMaster().getName(),
          design.isSequential(inst.getMaster()), 
          design.isInClock(inst))

for net in design.getBlock().getNets():
    print(net.getName(), design.getNetRoute(net))
