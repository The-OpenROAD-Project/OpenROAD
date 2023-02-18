from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readVerilog("gcd.v")
design.link("gcd")

design.evalTclString('source flow_helpers.tcl')
design.evalTclString('read_sdc "gcd_nangate45.sdc"')

part_file = helpers.make_result_file("partition_gcd.part")
graph_file = helpers.make_result_file("partition_gcd.graph")
paths_file = helpers.make_result_file("partition_gcd.paths")

design.getPartitionMgr().tritonPartDesign(2, 1.0, 0, part_file,
                                          paths_file, graph_file)

helpers.diff_files("partition_gcd.graphok", graph_file)
helpers.diff_files("partition_gcd.pathsok", paths_file)
helpers.diff_files("partition_gcd.partok", part_file)
