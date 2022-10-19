from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLef("merged_spacing.lef")

design = Design(tech)
design.readDef("sw130_random.def")
ack = design.getAntennaChecker()

ack.checkAntennas("", False)
ack.checkAntennas("", True)
ack.checkAntennas("net50", False)

try:
    ack.checkAntennas("xxx", True)
except Exception as inst:
    print(inst.args[0])     # arguments stored in .args

exit(0)
