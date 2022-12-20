# Check partitioning
from openroad import Design, Tech
import helpers
import par
import par_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")
design = Design(tech)

design.readVerilog("gcd.v")
design.link("gcd")

pid = par_aux.partition_netlist(design, tool="mlpart", num_partitions=2, seeds=[-289663928])
par_file = helpers.make_result_file("partition.par")
par_aux.dump_partition(design, pid=pid, par_file=par_file)

helpers.diff_files("partition.parok", par_file)
