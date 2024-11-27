from openroad import Design, Tech
import helpers
import par_aux

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readVerilog("gcd.v")
design.link("gcd")

design.evalTclString("source flow_helpers.tcl")
design.evalTclString('read_sdc "gcd_nangate45.sdc"')

part_file = helpers.make_result_file("partition_gcd.part")
graph_file = helpers.make_result_file("partition_gcd.graph")
paths_file = helpers.make_result_file("partition_gcd.paths")
v_file = helpers.make_result_file("partition_gcd.v")

par_aux.tritonPartDesign(design, solution_file=part_file)

design.getPartitionMgr().writePartitionVerilog(v_file)

# helpers.diff_files("partition_gcd.graphok", graph_file)
# helpers.diff_files("partition_gcd.pathsok", paths_file)
helpers.diff_files("partition_gcd.partok", part_file)
helpers.diff_files("partition_gcd.vok", v_file)
