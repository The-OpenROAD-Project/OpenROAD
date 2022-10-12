# A minimal LEF file that has been modified to include particular
# antenna values for testing
from openroad import Design, Tech
import ant

tech = Tech()
tech.readLef("ant_check.lef")

design = Design(tech)
design.readDef("ant_check.def")
ack = design.getAntennaChecker()

ack.checkAntennas("", True)

count = ack.antennaViolationCount()
print(f"violation count = {count}", flush=True)

net = "net50"

viol = ack.anyViolations(net)
print(f"Net {net} violations: {int(viol)}")
