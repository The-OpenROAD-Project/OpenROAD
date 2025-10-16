from openroad import Tech, Design, Timing
import openroad

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = Design(tech)
design.readDef("get_core_die_areas.def")
timing = Timing(design)

db = tech.getDB()

for corner in timing.getCorners():
    for net in design.getBlock().getNets():
        print(
            "{} {:.3g} {:.3g}".format(
                net.getName(),
                timing.getNetCap(net, corner, Timing.Max),
                timing.getNetCap(net, corner, Timing.Min),
            )
        )

for inst in design.getBlock().getInsts():
    print(
        inst.getName(),
        inst.getMaster().getName(),
        design.isSequential(inst.getMaster()),
    )

for lib in tech.getDB().getLibs():
    for master in lib.getMasters():
        print(master.getName())
        for mterm in master.getMTerms():
            print(
                "{}  {:.3g} {:.3g}".format(
                    mterm.getName(),
                    timing.getMaxCapLimit(mterm),
                    timing.getMaxSlewLimit(mterm),
                )
            )
            for m in timing.getTimingFanoutFrom(mterm):
                print("    ", m.getName())

timing.makeEquivCells()
for lib in tech.getDB().getLibs():
    for master in lib.getMasters():
        print(master.getName())
        dbmaster = db.findMaster(master.getName())
        equicCells = timing.equivCells(dbmaster)
        for equicCell in equicCells:
            print("  ", equicCell.getName())
