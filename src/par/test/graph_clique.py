# Check if the constructed star clique maintains the same number of nodes/edges

from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("gcd.def")

pmgr = design.getPartitionMgr()
pmgr.getOptions().setGraphModel("clique")
pmgr.getOptions().setCliqueThreshold(50)
pmgr.reportGraph()
