# Check if the constructed hybrid graph maintains the same number of nodes/edges
#   As this netlist is small, setting a small threshold is needed to achieve
#   an actual hybrid model 
from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("gcd.def")

pmgr = design.getPartitionMgr()
pmgr.getOptions().setGraphModel("hybrid")
pmgr.getOptions().setCliqueThreshold(10)
pmgr.reportGraph()
