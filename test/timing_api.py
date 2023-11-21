from openroad import Tech, Design

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = Design(tech)
design.readDef("get_core_die_areas.def")

for corner in design.getCorners():
    for net in design.getBlock().getNets():
        print(net.getName(),
              design.getNetCap(net, corner, Design.Max),
              design.getNetCap(net, corner, Design.Min))

for inst in design.getBlock().getInsts():
    print(inst.getName(), inst.getMaster().getName(),
          design.isSequential(inst.getMaster()))

for lib in tech.getDB().getLibs():
    for master in lib.getMasters():
        print(master.getName())
        for mterm in master.getMTerms():
            print('  ', mterm.getName())
            for m in design.getTimingFanoutFrom(mterm):
                print('    ', m.getName())
