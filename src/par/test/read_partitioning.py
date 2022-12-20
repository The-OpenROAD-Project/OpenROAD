from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")
design = Design(tech)

design.readVerilog("gcd.v")
design.link("gcd")

pmgr = design.getPartitionMgr()

pid = pmgr.readPartitioningFile("read_partitioning_graph.txt",
                                "read_partitioning_instance_map.txt")

par_file = helpers.make_result_file("read_partitioning.par")

pmgr.writePartitioningToDb(pid)
pmgr.dumpPartIdToFile(par_file)

helpers.diff_files("read_partitioning.parok", par_file)
