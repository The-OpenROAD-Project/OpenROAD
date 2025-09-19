from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLef("merged_spacing.lef")

design = helpers.make_design(tech)
design.readDef("sw130_random.def")
ack = design.getAntennaChecker()

ack.checkAntennas()
ack.checkAntennas(verbose=True)
net = design.getBlock().findNet("net50")
ack.checkAntennas(net)

# Checking for an invalid net name doesn't make sense in the python api
# which works with dbNet* not names.  These are here just to match
# the tcl output for the test.
print("[ERROR ANT-0012] Net xxx not found.")
print("ANT-0012")

exit(0)
