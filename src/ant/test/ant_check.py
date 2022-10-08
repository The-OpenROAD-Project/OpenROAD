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
# see comments in check_api1.py on check_net_violation 
viol = 1 if ant.anyViolations(net) else 0
print(f"Net {net} violations: {viol}")
