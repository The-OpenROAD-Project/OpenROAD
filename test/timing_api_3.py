from openroad import Tech, Design

tech = Tech()

tech.readLiberty("Nangate45/Nangate45_typ.lib") 
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")


design = Design(tech)
design.readDef("gcd_nangate45.def")

for ITerm in design.getBlock().getITerms():
	if not (design.isInPower(ITerm) or design.isInGround(ITerm)):
		slew_time = design.getPinSlew(ITerm) 
		print(design.getITermName(ITerm), str(slew_time))