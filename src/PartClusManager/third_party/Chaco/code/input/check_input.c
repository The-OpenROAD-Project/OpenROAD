/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "params.h"
#include "defs.h"
#include "structs.h"


static int check_params(), check_assignment();

/* Check graph and input options and parameters. */
int       check_input(graph, nvtxs, nedges, igeom, coords,
		         graphname, assignment, goal,
			 architecture, ndims_tot, mesh_dims,
		         global_method, local_method, rqi_flag, vmax, ndims,
		         eigtol)
struct vtx_data **graph;	/* linked lists of vertex data */
int       nvtxs;		/* number of vertices */
int       nedges;		/* number of edges */
int       igeom;		/* geometric dimension for inertial method */
float   **coords;		/* coordinates for inertial method */
char     *graphname;		/* graph input file name */
short    *assignment;		/* set numbers if read-from-file */
double   *goal;			/* desired sizes of different sets */
int       architecture;		/* 0=> hypercube, d=> d-dimensional mesh */
int       ndims_tot;		/* number of hypercube dimensions */
int       mesh_dims[3];		/* size of mesh in each dimension */
int       global_method;	/* global partitioning algorithm */
int       local_method;		/* local partitioning algorithm */
int       rqi_flag;		/* flag for RQI/symmlq eigensolver */
int      *vmax;			/* smallest acceptable coarsened nvtxs */
int       ndims;		/* partitioning level */
double    eigtol;		/* tolerance for eigen-pairs */
{
    extern FILE *Output_File;		/* output file or null */
    extern int DEBUG_TRACE;		/* trace main execution path? */
    extern int MATCH_TYPE;	/* which type of contraction matching to use? */
    double    vwgt_sum, vwgt_sum2;	/* sums of values in vwgts and goals */
    int       flag;		/* does input data pass all the tests? */
    int       flag_graph;	/* does graph check out OK? */
    int       flag_params;	/* do params look OK? */
    int       flag_assign;	/* does assignment look good? */
    int       nprocs;		/* number of processors partitioning for */
    int       i;		/* loop counter */
    int       check_graph(), check_params(), check_assignment();

    if (DEBUG_TRACE > 0) {
	printf("<Entering check_input>\n");
    }

    /* First check for consistency in the graph. */
    if (graph != NULL) {
	flag_graph = check_graph(graph, nvtxs, nedges);
	if (flag_graph) {
	    if (graphname != NULL)
		printf("ERRORS in graph input file %s.\n", graphname);
		if (Output_File != NULL) {
		    fprintf(Output_File, "ERRORS in graph input file %s.\n",
			graphname);
		}
	    else
		printf("ERRORS in graph.\n");
		if (Output_File != NULL) {
		    fprintf(Output_File, "ERRORS in graph.\n");
		}
	}
    }

    else {
	/* Only allowed if simple or inertial w/o KL and no weights. */
	flag_graph = FALSE;
	if (global_method == 1 || global_method == 2 || local_method == 1) {
	    printf("No graph input.  Only allowed for inertial or simple methods without KL.\n");
	    flag_graph = TRUE;
	}
    }

    /* Now check the input values. */
    flag = FALSE;
    flag_assign = FALSE;

    if (architecture < 0 || architecture > 3) {
	printf("Machine architecture parameter = %d, must be in [0,3].\n", architecture);
	flag = TRUE;
    }

    else if (architecture == 0) {
	if (ndims_tot < 0) {
	    printf("Dimension of hypercube = %d, must be at least 1.\n", ndims_tot);
	    flag = TRUE;
	}
    }

    else if (architecture > 0) {
	if (architecture == 1 && mesh_dims[0] <= 0) {
	    printf("Size of 1-D mesh improperly specified, %d.\n", mesh_dims[0]);
	    flag = TRUE;
	}
	if (architecture == 2 && (mesh_dims[0] <= 0 || mesh_dims[1] <= 0)) {
	    printf("Size of 2-D mesh improperly specified, %dx%d.\n", mesh_dims[0], mesh_dims[1]);
	    flag = TRUE;
	}
	if (architecture == 2 && (mesh_dims[0] <= 0 || mesh_dims[1] <= 0 || mesh_dims[2] <= 0)) {
	    printf("Size of 3-D mesh improperly specified, %dx%dx%d.\n",
		   mesh_dims[0], mesh_dims[1], mesh_dims[2]);
	    flag = TRUE;
	}
    }

    if (ndims < 1 || ndims > MAXDIMS) {
	printf("Partitioning at each step = %d, should be in [1,%d].\n",
	       ndims, MAXDIMS);
	flag = TRUE;
    }

    if (architecture == 0)
	nprocs = 1 << ndims_tot;
    else if (architecture > 0)
	nprocs = mesh_dims[0] * mesh_dims[1] * mesh_dims[2];
    if (1 << ndims > nprocs) {
	printf("Partitioning step %d too large for %d processors.\n",
	       ndims, nprocs);
	flag = TRUE;
    }

    if (global_method < 1 || global_method > 7) {
	printf("Global partitioning method = %d, must be in [1,7].\n", global_method);
	flag = TRUE;
    }

    if (local_method < 1 || local_method > 2) {
	printf("Local partitioning method = %d, must be in [1,2].\n", local_method);
	flag = TRUE;
    }

    if (global_method == 1 || (global_method == 2 && rqi_flag)) {
	i = 2 * (1 << ndims);
	if (*vmax < i) {
	    printf("WARNING: Number of vertices in coarse graph (%d) being reset to %d.\n",
		   *vmax, i);
	    if (Output_File != NULL) {
	        fprintf(Output_File,
		    "WARNING: Number of vertices in coarse graph (%d) being reset to %d.\n",
		    *vmax, i);
	    }
	    *vmax = i;
	}
    }

    if ((global_method == 1 || global_method == 2) && eigtol <= 0) {
	printf("Eigen tolerance (%g) must be positive value\n", eigtol);
	flag = TRUE;
    }

    if (global_method == 3 ||
	(MATCH_TYPE == 5 && (global_method == 1 ||
	                     (global_method == 2 && rqi_flag)))) {
	if (igeom < 1 || igeom > 3) {
	    printf("Geometry must be 1-, 2- or 3-dimensional\n");
	    flag = TRUE;
	}
	if (igeom > 0 && coords == NULL) {
	    printf("No coordinates given\n");
	    flag = FALSE;
	}
	else if (igeom > 0 && coords[0] == NULL) {
	    printf("No X-coordinates given\n");
	    flag = TRUE;
	}
	else if (igeom > 1 && coords[1] == NULL) {
	    printf("No Y-coordinates given\n");
	    flag = TRUE;
	}
	else if (igeom > 2 && coords[2] == NULL) {
	    printf("No Z-coordinates given\n");
	    flag = TRUE;
	}
    }

    if (global_method == 7 && local_method == 1) {
	if (nprocs > 1<<ndims) {
	    printf("Can only use local method on single level of read-in assignment,\n");
	    printf("  but ndims =  %d, while number of processors = %d.\n",
		ndims, nprocs);
	    flag = TRUE;
	}
    }

    /* Now check for consistency in the goal array. */
    if (graph != NULL)  {
        vwgt_sum = 0;
        for (i = 1; i <= nvtxs; i++) {
	    vwgt_sum += graph[i]->vwgt;
	}
    }
    else {
	vwgt_sum = nvtxs;
    }

    vwgt_sum2 = 0;
    for (i = 0; i < nprocs; i++) {
	if (goal[i] < 0) {
	    printf("goal[%d] is %g, but should be nonnegative.\n", i, goal[i]);
	    flag = TRUE;
	}
	vwgt_sum2 += goal[i];
    }

    if (fabs(vwgt_sum - vwgt_sum2) > 1e-5 * (vwgt_sum + vwgt_sum2)) {
	printf("Sum of values in goal (%g) not equal to sum of vertex weights (%g).\n",
	       vwgt_sum2, vwgt_sum);
	flag = TRUE;
    }

    /* Check assignment file if read in. */
    if (global_method == 7 && !flag) {
	flag_assign = check_assignment(assignment, nvtxs, nprocs, ndims, local_method);
    }

/* Add some checks for model parameters */

    /* Finally, check the parameters. */
    flag_params = check_params(global_method, local_method, rqi_flag, ndims);

    flag = flag || flag_graph || flag_assign || flag_params;

    return (flag);
}





static int check_params(global_method, local_method, rqi_flag, ndims)
int       global_method;	/* global partitioning algorithm */
int       local_method;		/* local partitioning algorithm */
int       rqi_flag;		/* use multilevel eigensolver? */
int       ndims;		/* number of eigenvectors */
{
    extern FILE *Output_File;	/* Output file or null */
    extern int ECHO;		/* print input/param options? to file? (-2..2) */
    extern int EXPERT;		/* is user an expert? */
    extern int OUTPUT_METRICS;	/* controls formatting of output */
    extern int OUTPUT_TIME;	/* at what level to display timing */

    extern int LANCZOS_TYPE;		/* type of Lanczos to use */
    extern double EIGEN_TOLERANCE;	/* eigen-tolerance convergence criteria */
    extern int LANCZOS_SO_INTERVAL;	/* interval between orthog checks in SO */
    extern double BISECTION_SAFETY;	/* divides Lanczos bisection tol */
    extern int LANCZOS_CONVERGENCE_MODE;/* Lanczos convergence test type */
    extern int RQI_CONVERGENCE_MODE;	/* rqi convergence test type: */
    extern double WARNING_ORTHTOL;	/* warn if Ares/bjitol this big */
    extern double WARNING_MISTOL;	/* warn if Ares/bjitol this big */
    extern int LANCZOS_SO_PRECISION;	/* single or double precision? */

    extern int PERTURB;		/* randomly perturb matrix in spectral method? */
    extern int NPERTURB;	/* if so, how many edges to modify? */
    extern double PERTURB_MAX;	/* largest value for perturbation */
    extern int MAPPING_TYPE;	/* how to map from eigenvectors to partition */
    extern int COARSE_NLEVEL_RQI;	/* levels between RQI calls */
    extern int OPT3D_NTRIES;	/* # local opts to look for global min in opt3d */

    extern int KL_METRIC;	/* KL interset cost: 1=>cuts, 2=>hops */
    extern int KL_BAD_MOVES;	/* number of unhelpful moves in a row allowed */
    extern int KL_NTRIES_BAD;	/* # unhelpful passes before quitting KL */
    extern double KL_IMBALANCE;	/* allowed imbalance in KL */

    extern double COARSEN_RATIO_MIN;	/* required coarsening reduction */
    extern int COARSE_NLEVEL_KL;/* # levels between KL calls in uncoarsening */
    extern int MATCH_TYPE;	/* which type of contraction matching to use? */

    extern int SIMULATOR;	/* simulate the communication? */

    extern int TERM_PROP;	/* perform terminal propagation? */
    extern double CUT_TO_HOP_COST;	/* ..if so, relativ cut/hop importance */
    int       flag_params;

    flag_params = FALSE;
    if (ECHO < -2 || ECHO > 2) {
	printf("ECHO (%d) should be in [-2,2].\n", ECHO);
	flag_params = TRUE;
    }

    if (OUTPUT_METRICS < -2 || OUTPUT_METRICS > 2) {
	printf("WARNING: OUTPUT_METRICS (%d) should be in [-2,2].\n", OUTPUT_METRICS);
	if (Output_File != NULL) {
	    fprintf(Output_File,
		"WARNING: OUTPUT_METRICS (%d) should be in [-2,2].\n", OUTPUT_METRICS);
	}
    }
    if (OUTPUT_TIME < 0 || OUTPUT_TIME > 2) {
	printf("WARNING: OUTPUT_TIME (%d) should be in [0,2].\n", OUTPUT_TIME);
	if (Output_File != NULL) {
	    fprintf(Output_File,
		"WARNING: OUTPUT_TIME (%d) should be in [0,2].\n", OUTPUT_TIME);
	}
    }

    if (global_method == 1 || global_method == 2) {
	if (EXPERT) {
	    if (LANCZOS_TYPE < 1 || LANCZOS_TYPE > 4) {
	        printf("LANCZOS_TYPE (%d) should be in [1,4].\n", LANCZOS_TYPE);
	        flag_params = TRUE;
	    }
	}
	else {
	    if (LANCZOS_TYPE < 1 || LANCZOS_TYPE > 3) {
	        printf("LANCZOS_TYPE (%d) should be in [1,3].\n", LANCZOS_TYPE);
	        flag_params = TRUE;
	    }
	}
	if (EIGEN_TOLERANCE <= 0) {
	    printf("EIGEN_TOLERANCE (%g) should be positive.\n", EIGEN_TOLERANCE);
	    flag_params = TRUE;
	}
	if (LANCZOS_SO_INTERVAL <= 0) {
	    printf("LANCZOS_SO_INTERVAL (%d) should be positive.\n",
		   LANCZOS_SO_INTERVAL);
	    flag_params = TRUE;
	}
	if (LANCZOS_SO_INTERVAL == 1) {
	    printf("WARNING: More efficient if LANCZOS_SO_INTERVAL = 2, not 1.\n");
	    if (Output_File != NULL) {
	        fprintf(Output_File,
		    "WARNING: More efficient if LANCZOS_SO_INTERVAL = 2, not 1.\n");
	    }
	}
	if (BISECTION_SAFETY <= 0) {
	    printf("BISECTION_SAFETY (%g) should be positive.\n", BISECTION_SAFETY);
	    flag_params = TRUE;
	}
	if (LANCZOS_CONVERGENCE_MODE < 0 || LANCZOS_CONVERGENCE_MODE > 1) {
	    printf("LANCZOS_CONVERGENCE_MODE (%d) should be in [0,1].\n",
		LANCZOS_CONVERGENCE_MODE);
	    flag_params = TRUE;
	}

	if (WARNING_ORTHTOL == 0) {
	    printf("WARNING: WARNING_ORTHTOL (%g) should be positive.\n",
		WARNING_ORTHTOL);
	    if (Output_File != NULL) {
	        fprintf(Output_File,
		    "WARNING: WARNING_ORTHTOL (%g) should be positive.\n",
		    WARNING_ORTHTOL);
	    }
	}
	if (WARNING_MISTOL == 0) {
	    printf("WARNING: WARNING_MISTOL (%g) should be positive.\n",
		WARNING_MISTOL);
	    if (Output_File != NULL) {
	        fprintf(Output_File,
		    "WARNING: WARNING_MISTOL (%g) should be positive.\n",
		    WARNING_MISTOL);
	    }
	}

	if (LANCZOS_SO_PRECISION < 1 || LANCZOS_SO_PRECISION > 2) {
	    printf("LANCZOS_SO_PRECISION (%d) should be in [1,2].\n",
		LANCZOS_SO_PRECISION);
	    flag_params = TRUE;
	}

	if (PERTURB) {
	    if (NPERTURB < 0) {
		printf("NPERTURB (%d) should be nonnegative.\n", NPERTURB);
		flag_params = TRUE;
	    }
	    if (NPERTURB > 0 && PERTURB_MAX < 0) {
		printf("PERTURB_MAX (%g) should be nonnegative.\n", PERTURB_MAX);
		flag_params = TRUE;
	    }
	}

	if (MAPPING_TYPE < 0 || MAPPING_TYPE > 3) {
	    printf("MAPPING_TYPE (%d) should be in [0,3].\n", MAPPING_TYPE);
	    flag_params = TRUE;
	}

	if (ndims == 3 && OPT3D_NTRIES <= 0) {
	    printf("OPT3D_NTRIES (%d) should be positive.\n", OPT3D_NTRIES);
	    flag_params = TRUE;
	}

	if (global_method == 2 && rqi_flag) {
	    if (COARSE_NLEVEL_RQI <= 0) {
		printf("COARSE_NLEVEL_RQI (%d) should be positive.\n",
		       COARSE_NLEVEL_RQI);
		flag_params = TRUE;
	    }

	    if (RQI_CONVERGENCE_MODE < 0 || RQI_CONVERGENCE_MODE > 1) {
		printf("RQI_CONVERGENCE_MODE (%d) should be in [0,1].\n",
		    RQI_CONVERGENCE_MODE);
		flag_params = TRUE;
	    }

 	    if (TERM_PROP) {	
                 printf("WARNING: Using default Lanczos for extended eigenproblem, not RQI/Symmlq.\n");
 	    }
	}

	if (global_method == 1) {
	    if (COARSE_NLEVEL_KL <= 0) {
		printf("COARSE_NLEVEL_KL (%d) should be positive.\n",
		       COARSE_NLEVEL_KL);
		flag_params = TRUE;
	    }
	}

	if (global_method == 1 || (global_method == 2 && rqi_flag)) {
	    if (COARSEN_RATIO_MIN < .5) {
		printf("COARSEN_RATIO_MIN (%g) should be at least 1/2.\n",
		       COARSEN_RATIO_MIN);
		flag_params = TRUE;
	    }
	    if (MATCH_TYPE < 1 || MATCH_TYPE > 5) {
		printf("MATCH_TYPE (%d) should be in [1,4].\n", MATCH_TYPE);
		flag_params = TRUE;
	    }
	}

/*
	if (global_method == 1 && LIMIT_KL_EWGTS && EWGT_RATIO_MAX <= 0) {
	    printf("EWGT_RATIO_MAX (%g) should be at positive.\n",
		   EWGT_RATIO_MAX);
	    flag_params = TRUE;
	}
*/
    }

    if (global_method == 1 || local_method == 1) {
	if (KL_METRIC < 1 || KL_METRIC > 2) {
	    printf("KL_METRIC (%d) should be in [0,1].\n", KL_METRIC);
	    flag_params = TRUE;
	}
	if (KL_BAD_MOVES < 0) {
	    printf("KL_BAD_MOVES (%d) should be non-negative.\n", KL_BAD_MOVES);
	    flag_params = TRUE;
	}
	if (KL_NTRIES_BAD < 0) {
	    printf("KL_NTRIES_BAD (%d) should be non-negative.\n", KL_NTRIES_BAD);
	    flag_params = TRUE;
	}
	if (KL_IMBALANCE < 0.0 || KL_IMBALANCE > 1.0) {
	    printf("KL_IMBALANCE (%g) should be in [0,1].\n", KL_IMBALANCE);
	    flag_params = TRUE;
	}
    }

    if (SIMULATOR < 0 || SIMULATOR > 3) {
	printf("SIMULATOR (%d) should be in [0,3].\n", SIMULATOR);
	flag_params = TRUE;
    }

    if (TERM_PROP) {
	if (CUT_TO_HOP_COST <= 0) {
	    printf("CUT_TO_HOP_COST (%g) should be positive.\n", CUT_TO_HOP_COST);
	    flag_params = TRUE;
	}
	if (ndims > 1) {
	    printf("WARNING: May ignore terminal propagation in spectral quadri/octa section\n");
	}
    }

    return (flag_params);
}


static int check_assignment(assignment, nvtxs, nsets_tot, ndims, local_method)
short    *assignment;		/* set numbers if read-from-file */
int       nvtxs;		/* number of vertices */
int       nsets_tot;		/* total number of desired sets */
{
    int       flag;		/* return status */
    int       nsets;		/* number of sets created at each level */
    int       i;		/* loop counter */

    nsets = 1<< ndims;
    flag = FALSE;

    for (i=1; i<=nvtxs && !flag; i++) {
        if (assignment[i] < 0) {
	    printf("Assignment[%d] = %d less than zero.\n", i, assignment[i]);
	    flag = TRUE;
	}
	else if (assignment[i] >= nsets_tot) {
	    printf("Assignment[%d] = %d, too large for %d sets.\n",
		i, assignment[i], nsets_tot);
	    flag = TRUE;
	}
	else if (local_method == 1 && assignment[i] >= nsets) {
	    printf("Can only use local method on single level of read-in assignment,\n");
	    printf("  but assignment[%d] =  %d.\n", i, assignment[i]);
	    flag = TRUE;
	}
    }

    return(flag);
}
