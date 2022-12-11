# A minimal LEF file that has been modified to include particular
# antenna values for testing
from openroad import Design, Tech
import ant

tech = Tech()
tech.readLef("ant_check.lef")

design = Design(tech)
design.readDef("ant_check.def")
ack = design.getAntennaChecker()

ack.checkAntennas(verbose=True)

count = ack.antennaViolationCount()
print(f"violation count = {count}", flush=True)

net_name = "net50"
net = design.getBlock().findNet(net_name)
viol = len(ack.getAntennaViolations(net, None, 0)) > 0

print(f"Net {net_name} violations: {int(viol)}")
