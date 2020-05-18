#include "params.h"
#include "structs.h"

int       interface_wrap(int nvtxs, int *start, int *adjacency, int *vwgts, float *ewgts, float *x, float *y, float *z,
		              char *outassignname, char *outfilename,
		              short *assignment,
		              int architecture, int ndims_tot, int mesh_dims[3], double *goal,
		              int global_method, int local_method, int rqi_flag, int vmax, int ndims,
		              double eigtol, long seed,
					  int tprop, double kl_inbalance, double coarsening_ratio, double cut_to_hop_cost,
					  int debug_print, int refine_part, int level)
{

	extern int TERM_PROP;	  /* perform terminal propagation */
    extern double KL_IMBALANCE;	/* imbalance tolerated by KL */
	extern double COARSEN_RATIO_MIN; /* min vtx reduction at each coarsen stage */
    extern double CUT_TO_HOP_COST;	/* ..if so, relative cut/hop importance */
	extern int DEBUG_PARTCLUSMANAGER;
	extern int REFINE_PARTITION;
	extern int CLUSTERING_EXPORT;

	static int *termprop = &TERM_PROP;
	static double *inbalance = &KL_IMBALANCE;
	static double *coarratio = &COARSEN_RATIO_MIN;
	static double *cutcost = &CUT_TO_HOP_COST;
	static int *printtext = &DEBUG_PARTCLUSMANAGER;
	static int *refine = &REFINE_PARTITION;
	static int *cluslevel = &CLUSTERING_EXPORT;

	*termprop = tprop;
	*inbalance = kl_inbalance;
	*coarratio = coarsening_ratio;
	*cutcost = cut_to_hop_cost;
	*printtext = debug_print;
	*refine = refine_part;
	*cluslevel = level;


	interface(nvtxs, start, adjacency, vwgts, ewgts, x, y, z,
		              outassignname, outfilename,
		              assignment,
		              architecture, ndims_tot, mesh_dims, goal,
		              global_method, local_method, rqi_flag, vmax, ndims,
		              eigtol, seed);
}

int*      clustering_wrap()
{
	extern struct coarlist CLUSTERING_RESULTS;
	static struct coarlist *clusresults = &CLUSTERING_RESULTS;

	return clusresults->vec;
}
