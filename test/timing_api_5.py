from openroad import Tech, Design

tech = Tech()


tech.readLiberty("Nangate45/Nangate45_typ.lib") 
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

# tech.readLiberty("../../test/Nangate45/Nangate45_typ.lib") 
# tech.readLef("../../test/Nangate45/Nangate45_tech.lef")
# tech.readLef("../../test/Nangate45/Nangate45_stdcell.lef")



design = Design(tech)
design.readDef("gcd_nangate45.def")
# design.readDef("/home/lmarti83/test_design/6_final.def")


ending_point_names = design.extractEndpointNames()

for ITerm in design.getBlock().getITerms():
    if not (design.isInPower(ITerm) or design.isInGround(ITerm)):
        isEndpoint = design.isPinEndpoint(ending_point_names, ITerm) 
        print(str(isEndpoint))




