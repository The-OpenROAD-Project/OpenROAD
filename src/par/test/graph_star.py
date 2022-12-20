from openroad import Design, Tech
import helpers
# Check if the constructed star graph maintains the same number of nodes/edges

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("gcd.def")

pmgr = design.getPartitionMgr()
pmgr.getOptions().setGraphModel("star")
pmgr.getOptions().setCliqueThreshold(50)
pmgr.reportGraph()
