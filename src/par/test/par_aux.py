# Aux functions for par
import utl
import par


# This could use more input validation
def partition_netlist(design, *,
                      tool="mlpart",           #  "chaco" | "gpmetis" | "mlpart"
                      num_partitions=None,     #  int, required
                      graph_model="star",      #  "clique" | "star" | "hybrid"
                      clique_threshold=50,     #  3 <= ct <= 32768
                      weight_model=1,          #  1 <= wm <= 7
                      max_edge_weight=100,     #  1 <= mew <= 32768
                      max_vertex_weight=100,   #  1 <= mew <= 32768
                      num_starts=1,            #  1 <= num_starts <= 32768
                      balance_constraint=2,    #  0 <= balance_c  <= 50
                      coarsening_ratio=0.7,    #  0.5 <= cr <= 1.0
                      coarsening_vertices=2500,#  should be an int
                      enable_term_prop=False,  #  True | False
                      cut_hop_ratio=1.0,       #  0.5 <= chr <= 1.0
                      architecture=None,       #  vector int
                      refinement=0,            #  0 <= ref <= 32768
                      random_seed=42,          #  int, No checking ?
                      seeds=None,              #  vector int
                      partition_id=-1,         #  int
                      force_graph=False        #  bool
                      ):

    pmgr = design.getPartitionMgr()
    popts = pmgr.getOptions()

    popts.setTool(tool)
    if num_partitions == None:
        utl.error(utl.PAR, 701, "Missing manditory argument: num_partitions")
    else:
        popts.setTargetPartitions(num_partitions)

    popts.setGraphModel(graph_model)
    popts.setCliqueThreshold(clique_threshold)
    popts.setWeightModel(weight_model)
    popts.setMaxEdgeWeight(max_edge_weight)
    popts.setMaxVertexWeight(max_vertex_weight)
    popts.setNumStarts(num_starts)
    popts.setBalanceConstraint(balance_constraint)
    popts.setCoarRatio(coarsening_ratio)
    popts.setCoarVertices(coarsening_vertices)
    popts.setTermProp(enable_term_prop)
    popts.setCutHopRatio(cut_hop_ratio)

    if architecture == None:
        pv = par.vector_int([])
    else:
        pv = par.vector_int(architecture)
        
    popts.setArchTopology(pv)

    popts.setRefinement(refinement)
    popts.setRandomSeed(random_seed)

    if seeds == None:
        if num_starts != 1:
            popts.generateSeeds(num_starts)
        else:
            utl.error(utl.PAR, 702, "Need either seeds or num_starts specified")
    else:
        ps = par.set_int(seeds)
        popts.setSeeds(ps)

    popts.setExistingID(partition_id)
    popts.setForceGraph(force_graph)
    pmgr.runPartitioning()
    pid = pmgr.getCurrentId()
    return pid


def dump_partition(design, *, pid=None, par_file=None):
    pmgr = design.getPartitionMgr()
    pmgr.writePartitioningToDb(pid)
    pmgr.dumpPartIdToFile(par_file)


def dump_cluster(design, *, cid=None, clus_file=None):
    pmgr = design.getPartitionMgr()
    pmgr.writeClusteringToDb(cid)
    pmgr.dumpClusIdToFile(clus_file)


# This could use more input validation
def cluster_netlist(design, *,
                    tool="mlpart",            # "chaco | gpmetis | mlpart"
                    coarsening_ratio=None,    #
                    coarsening_vertices=None, #
                    level=1                   #
                    ):
    pmgr  = design.getPartitionMgr()
    popts = pmgr.getOptions()
    popts.setTool(tool)
    if coarsening_ratio != None:
        popts.setCoarRatio(coarsening_ratio)

    if coarsening_vertices != None:
        popts.setCoarVertices(coarsening_vertices)

    popts.setLevel(level)
    popts.generateSeeds(1)
    pmgr.runClustering()
    pid = pmgr.getCurrentClusId()
    return pid
