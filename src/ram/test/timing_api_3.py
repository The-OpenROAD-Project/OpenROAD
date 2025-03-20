from openroad import Tech, Design, Timing

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = Design(tech)
design.readDef("gcd_nangate45.def")
design.evalTclString("read_sdc timing_api_3.sdc")
timing = Timing(design)

for inst in design.getBlock().getInsts():
    for iTerm in inst.getITerms():
        print(iTerm.getName(), "  ", design.isInClock(iTerm))
        if not iTerm.getNet():
            continue
        if design.isInSupply(iTerm):
            continue
        print(
            "{} {:.3g} {:.3g} {:.3g} {:.3g}".format(
                design.getITermName(iTerm),
                timing.getPinSlack(iTerm, Timing.Rise, Timing.Max),
                timing.getPinSlack(iTerm, Timing.Fall, Timing.Max),
                timing.getPinSlack(iTerm, Timing.Rise, Timing.Min),
                timing.getPinSlack(iTerm, Timing.Fall, Timing.Min),
            )
        )
        for i, corner in enumerate(timing.getCorners()):
            print(
                "Corner {} {:.3g} {:.3g}".format(
                    i,
                    timing.getPortCap(iTerm, corner, Timing.Max),
                    timing.getPortCap(iTerm, corner, Timing.Min),
                )
            )
