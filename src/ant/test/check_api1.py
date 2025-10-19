from openroad import Design, Tech
import helpers
import ant

tech = Tech()
tech.readLef("merged_spacing.lef")

design = helpers.make_design(tech)
design.readDef("sw130_random.def")
ack = design.getAntennaChecker()

ack.checkAntennas(verbose=True)
count = ack.antennaViolationCount()
print(f"violation count = {count}", flush=True)

# note that the tcl function check_net_violations is a boolean but
# the tcl reg test treats it as an int. So we print an int here
# so we can have the same log file

net_name = "net50"
net = design.getBlock().findNet(net_name)
viol = len(ack.getAntennaViolations(net, None, 0)) > 0

print(f"Net {net_name} violations: {int(viol)}")
