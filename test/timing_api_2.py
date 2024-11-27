from openroad import Tech, Design, Timing

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = Design(tech)
design.readDef("gcd_nangate45.def")
design.evalTclString("read_sdc timing_api_2.sdc")
timing = Timing(design)

for inst in design.getBlock().getInsts():
    print(
        inst.getName(),
        inst.getMaster().getName(),
        design.isSequential(inst.getMaster()),
        design.isInClock(inst),
        design.isBuffer(inst.getMaster()),
        design.isInverter(inst.getMaster()),
    )
    for corner in timing.getCorners():
        print(
            "{:.3g} {:.3g}".format(
                timing.staticPower(inst, corner), timing.dynamicPower(inst, corner)
            )
        )
    for iTerm in inst.getITerms():
        if not iTerm.getNet():
            continue
        if not (design.isInSupply(iTerm)):
            print(
                "{} {:.3g} {:.3g} {:.3g} {}".format(
                    design.getITermName(iTerm),
                    timing.getPinArrival(iTerm, Timing.Rise),
                    timing.getPinArrival(iTerm, Timing.Fall),
                    timing.getPinSlew(iTerm),
                    timing.isEndpoint(iTerm),
                )
            )

for net in design.getBlock().getNets():
    print("{} {}".format(net.getName(), design.getNetRoutedLength(net)))
