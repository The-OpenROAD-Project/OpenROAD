from openroad import Design, Tech
import helpers
import ant

tech = Tech()
tech.readLef("merged_spacing.lef")

design = Design(tech)
design.readDef("sw130_random.def")
ack = design.getAntennaChecker()

ack.checkAntennas("", False)
count = ack.antennaViolationCount()
print(f"violation count = {count}", flush=True)

# note that the tcl function check_net_violations is a boolean but
# the tcl reg test treats it as an int. So we print an int here
# so we can have the same log file
net = "net50"
viol = ack.anyViolations(net)
print(f"Net {net} violations: {int(viol)}")
