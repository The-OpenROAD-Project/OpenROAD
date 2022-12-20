from openroad import Design, Tech
import helpers
import par_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")
design = Design(tech)

design.readVerilog("gcd.v")
design.link("gcd")

cid = par_aux.cluster_netlist(design, tool="mlpart", coarsening_ratio=0.6)
clus_file = helpers.make_result_file("cluster.par")
par_aux.dump_cluster(design, cid=cid, clus_file=clus_file)
helpers.diff_files("cluster.parok", clus_file)
