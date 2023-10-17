from openroad import Tech, Design

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = Design(tech)
design.readDef("gcd_nangate45.def")
design.evalTclString("read_sdc timing_api_2.sdc")


for inst in design.getBlock().getInsts():
  print(inst.getName(), 
        inst.getMaster().getName(),
        design.isSequential(inst.getMaster()), 
        design.isInClock(inst),
        design.isBuffer(inst.getMaster()),
        design.isInverter(inst.getMaster()),
        )
  for corner in design.getCorners():
    print(design.staticPower(inst, corner),
          design.dynamicPower(inst, corner),
         )

for net in design.getBlock().getNets():
  print(net.getName(), design.getNetRoutedLength(net))
