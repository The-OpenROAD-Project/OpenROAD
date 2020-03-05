/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "params.h"
#include "defs.h"

static void reflect_params();

/* Print out the input options. */
void      reflect_input(nvtxs, nedges, igeom, graphname, geomname,
			    inassignname, outassignname, outfilename,
			    architecture, ndims_tot, mesh_dims,
		            global_method, local_method, rqi_flag, vmax, ndims,
		            eigtol, seed, outfile)
int       nvtxs;		/* number of vertices in graph */
int       nedges;		/* number of edges in graph */
int       igeom;		/* geometric dimension for inertial method */
char     *graphname;		/* name of graph input file */
char     *geomname;		/* name of geometry input file */
char     *inassignname;		/* name of assignment input file */
char     *outassignname;	/* name of assignment output file */
char     *outfilename;		/* name of information output file */
int       architecture;		/* 0=> hypercube, d=> d-dimensional mesh */
int       ndims_tot;		/* total number of cuts to make */
int       mesh_dims[3];		/* size of mesh */
int       global_method;	/* global partitioning algorithm */
int       local_method;		/* local partitioning algorithm */
int       rqi_flag;		/* use RQI/Symmlq eigensolver?  */
int       vmax;			/* smallest acceptable coarsened nvtxs */
int       ndims;		/* partitioning level */
double    eigtol;		/* tolerance on eigenvectors */
long      seed;			/* random number seed */
FILE     *outfile;		/* file to write output to */
{
    extern int DEBUG_TRACE;	/* trace main execution path? */
    extern int ECHO;		/* copy input parameters back to screen? */
    extern int OUT_ASSIGN_INV;	/* assignment output in inverted format? */
    extern int IN_ASSIGN_INV;	/* assignment input in inverted format? */
    extern int PRINT_HEADERS;	/* print section headers for output */
    FILE     *tempfile;		/* file or stdout */
    int       i;		/* loop counter */

    if (DEBUG_TRACE > 0) {
	printf("<Entering reflect_input>\n");
    }

    for (i = 0; i < 2; i++) {
	if (i == 1) {
	    if (ECHO < 0 && outfile != NULL)
		tempfile = outfile;
	    else
		break;
	}
	else {
	    tempfile = stdout;
	}
	fprintf(tempfile, "\n");

	if (PRINT_HEADERS) {
            fprintf(tempfile, "\n           Input and Parameter Values\n\n");
        }

	if (graphname != NULL) {
	    fprintf(tempfile, "Graph file: `%s', ", graphname);
	}
	fprintf(tempfile, "# vertices = %d, # edges = %d\n", nvtxs, nedges);

	/* Print global partitioning strategy. */
	fprintf(tempfile, "Global method: ");
	if (global_method == 1) {
	    fprintf(tempfile, "Multilevel-KL\n");
	}
	else if (global_method == 2) {
	    fprintf(tempfile, "Spectral\n");
	}
	else if (global_method == 3) {
	    fprintf(tempfile, "Inertial\n");
	}
	else if (global_method == 4) {
	    fprintf(tempfile, "Linear\n");
	}
	else if (global_method == 5) {
	    fprintf(tempfile, "Random\n");
	}
	else if (global_method == 6) {
	    fprintf(tempfile, "Scattered\n");
	}
	else if (global_method == 6) {
	    fprintf(tempfile, "Read-From-File ");
	    if (IN_ASSIGN_INV) {
		printf("(inverted format)\n");
	    }
	    else {
		printf("(normal format)\n");
	    }
	}

	if (global_method == 1) {
	    fprintf(tempfile, "Number of vertices to coarsen down to: %d\n", vmax);
	    fprintf(tempfile, "Eigen tolerance: %g\n", eigtol);
	}

	else if (global_method == 2) {
	    if (rqi_flag) {
		fprintf(tempfile, "Multilevel RQI/Symmlq eigensolver\n");
		fprintf(tempfile, "Number of vertices to coarsen down to: %d\n", vmax);
		fprintf(tempfile, "Eigen tolerance: %g\n", eigtol);
	    }
	}

	else if (global_method == 3) {
	    if (geomname != NULL) {
		fprintf(tempfile, "Geometry input file: `%s', Dimensionality = %d\n", geomname, igeom);
	    }
	}

	else if (global_method == 7) {
	    fprintf(tempfile, "Assignment input file: `%s'\n", inassignname);
	}


	/* Now describe local method. */
	if (local_method == 1) {
	    fprintf(tempfile, "Local method: Kernighan-Lin\n");
	}
	else if (local_method == 2) {
	    fprintf(tempfile, "Local method: None\n");
	}

	/* Now describe target architecture. */
	if (architecture == 0) {
	    fprintf(tempfile, "Partitioning target: %d-dimensional hypercube\n", ndims_tot);
	}
	else if (architecture > 0) {
	    fprintf(tempfile, "Partitioning target: %d-dimensional mesh of size ", architecture);
	    if (architecture == 1)
		fprintf(tempfile, "%d\n", mesh_dims[0]);
	    else if (architecture == 2)
		fprintf(tempfile, "%dx%d\n", mesh_dims[0], mesh_dims[1]);
	    else if (architecture == 3)
		fprintf(tempfile, "%dx%dx%d\n", mesh_dims[0], mesh_dims[1], mesh_dims[2]);
	}

	if (ndims == 1) {
	    fprintf(tempfile, "Partitioning mode: Bisection\n");
	}
	else if (ndims == 2) {
	    fprintf(tempfile, "Partitioning mode: Quadrisection\n");
	}
	else if (ndims == 3) {
	    fprintf(tempfile, "Partitioning mode: Octasection\n");
	}

/* Add stuff about communications simulator. */

	fprintf(tempfile, "Random seed: %ld\n", seed);

	if (outassignname != NULL) {
	    fprintf(tempfile, "Assignment output file: `%s' ", outassignname);
	    if (OUT_ASSIGN_INV) {
		printf("(inverted format)\n");
	    }
	    else {
		printf("(normal format)\n");
	    }
	}
	if (outfilename != NULL) {
	    fprintf(tempfile, "Output file: `%s'\n", outfilename);
	}

	if (ECHO > 1 || ECHO < -1) {
	    reflect_params(tempfile, global_method, local_method,
			   rqi_flag, ndims);
	}
	fprintf(tempfile, "\n");
    }
}


static void reflect_params(tempfile, global_method, local_method,
			           rqi_flag, ndims)
FILE     *tempfile;		/* file or stdout */
int       global_method;	/* global partitioning algorithm */
int       local_method;		/* local partitioning algorithm */
int       rqi_flag;		/* use RQI/SYMMLQ eigensolver? */
int       ndims;		/* number of eigenvectors to generate */
{
    extern int CHECK_INPUT;	/* check the input for consistency? */
    extern int OUTPUT_METRICS;	/* controls formatting of output */

    extern int LANCZOS_TYPE;		/* type of Lanczos to use */
    extern double EIGEN_TOLERANCE;	/* eigen-tolerance convergence criteria */
    extern double SRESTOL;		/* relative residual tol on T evec */
    extern int LANCZOS_SO_INTERVAL;	/* interval between orthog checks in SO */
    extern int LANCZOS_MAXITNS;		/* if positive, Lanczos iteration limit */
    extern double BISECTION_SAFETY;	/* divides Lanczos bisection tolerance  */
    extern int LANCZOS_TIME;		/* detailed Lanczos timing? */
    extern int LANCZOS_CONVERGENCE_MODE;/* Lanczos convergence test type */
    extern int RQI_CONVERGENCE_MODE;	/* rqi convergence test type: */
    extern int WARNING_EVECS;		/* warning level for evec calculation */
    extern int LANCZOS_SO_PRECISION;	/* single or double precision? */

    extern int MAKE_CONNECTED;		/* connect graph for spectral method? */
    extern int PERTURB;		/* randomly perturb matrix in spectral method? */
    extern int NPERTURB;	/* if so, how many edges to modify? */
    extern double PERTURB_MAX;	/* largest value for perturbation */
    extern int MAPPING_TYPE;	/* how to map from eigenvectors to partition */
    extern int COARSE_NLEVEL_RQI;	/* levels between RQI calls */
    extern int OPT3D_NTRIES;	/* # local opts to look for global min in opt3d */

    extern int KL_METRIC;	/* KL interset cost: 1=>cuts, 2=>hops */
    extern int KL_RANDOM;	/* use randomness in Kernighan-Lin? */
    extern int KL_BAD_MOVES;	/* number of unhelpful moves in a row allowed */
    extern int KL_NTRIES_BAD;	/* # unhelpful passes before quitting KL */
    extern int KL_UNDO_LIST;	/* only resort vtxs affected by last pass? */
    extern double KL_IMBALANCE;	/* allowed fractional imbalance in KL */

    extern double COARSEN_RATIO_MIN;	/* required coarsening reduction */
    extern int COARSE_NLEVEL_KL;/* # levels between KL calls in uncoarsening */
    extern int MATCH_TYPE;	/* which type of contraction matching to use? */
    extern int HEAVY_MATCH;	/* encourage heavy matching edges? */
    extern int COARSE_KL_BOTTOM;/* force KL invocation at bottom level? */
    extern int COARSEN_VWGTS;	/* use vertex weights while coarsening? */
    extern int COARSEN_EWGTS;	/* use edge weights while coarsening? */
    extern int KL_ONLY_BNDY;	/* start KL w/ vertices on boundary? */

    extern int REFINE_PARTITION;/* Postprocess to improve pairwise cuts? */
    extern int INTERNAL_VERTICES;/* Postprocess to increase interval vtxs? */
    extern int REFINE_MAP;	/* Postprocess sets to reduce hops? */

    extern int SIMULATOR;	/* simulate the communication? */
    extern int SIMULATION_ITNS;	/* simulator iterations */
    extern double CUT_COST;	/* cost of each cut */
    extern double HOP_COST;	/* cost of each hop */
    extern double BDY_COST;	/* cost of each boundary vertex */
    extern double BDY_HOP_COST;	/* cost of each boundary vertex hop */
    extern double STARTUP_COST;	/* initiation cost of each message */

    extern int TERM_PROP;	/* invoke terminal propagation? */
    extern double CUT_TO_HOP_COST;	/* ..if so, importance of cuts/hops */
    extern int SEQUENCE;	/* just generate spectral ordering? */
    extern char SEQ_FILENAME[];	/* name of sequence file */
    extern int MAKE_VWGTS;	/* Force vertex weights to be degrees+1 ? */

    extern int DEBUG_EVECS;	/* Debug flag for eigenvector generation */
    extern int DEBUG_KL;	/* Debug flag for Kernighan-Lin */
    extern int DEBUG_INERTIAL;	/* Debug flag for inertial method */
    extern int DEBUG_CONNECTED;	/* Debug flag for connected components */
    extern int DEBUG_PERTURB;	/* Debug flag for matrix perturbation */
    extern int DEBUG_ASSIGN;	/* Debug flag for assignment to sets */
    extern int DEBUG_OPTIMIZE;	/* Debug flag for optimization/rotation */
    extern int DEBUG_BPMATCH;	/* Debug flag for bipartite matching code */
    extern int DEBUG_COARSEN;	/* Debug flag for coarsening/uncoarsening */
    extern int DEBUG_MEMORY;	/* Debug flag for smalloc/sfree */
    extern int DEBUG_INPUT;	/* Debug flag for having read input files */
    extern int DEBUG_PARAMS;	/* Debug flag for reading parameter file */
    extern int DEBUG_INTERNAL;	/* Debug flag for internal vertices */
    extern int DEBUG_REFINE_PART;	/* Debug flag for refining part. */
    extern int DEBUG_REFINE_MAP;/* Debug flag for refining mapping */
    extern int DEBUG_SIMULATOR;	/* Debug flag for comm simulator */
    extern int DEBUG_TRACE;	/* Trace main execution path */
    extern int DEBUG_MACH_PARAMS;	/* Print computed machine params? */

    extern int EXPERT;		/* Expert user? */

    char     *true_or_false();

    fprintf(tempfile, "Active Parameters:\n");

    fprintf(tempfile, "  CHECK_INPUT = %s\n", true_or_false(CHECK_INPUT));

    if (global_method == 1 || global_method == 2) {
	fprintf(tempfile, "  LANCZOS_TYPE:  ");
	if (LANCZOS_TYPE == 1) {
	    fprintf(tempfile, " Full orthogonalization");
	}
	else if (LANCZOS_TYPE == 2) {
	    fprintf(tempfile, "Full orthogonalization, inverse operator");
	}
	else if (LANCZOS_TYPE == 3) {
	    fprintf(tempfile, "Selective orthogonalization");
	}
	else if (LANCZOS_TYPE == 4) {
	    if (EXPERT) {
	        fprintf(tempfile, "Selective orthogonalization against both ends");
	    }
	    else {	/* Check input should catch this, but just in case ... */
		LANCZOS_TYPE = 3;
	        fprintf(tempfile, "Selective orthogonalization");
	    }
	}
	if (TERM_PROP) {
	    fprintf(tempfile, " OR extended");
	}
	fprintf(tempfile, "\n");
	    

	fprintf(tempfile, "  EIGEN_TOLERANCE = %g\n", EIGEN_TOLERANCE);
	if (SRESTOL > 0) {
	    fprintf(tempfile, "  SRESTOL = %g\n", SRESTOL);
	}
	else {
	    fprintf(tempfile,
		"  SRESTOL = %g ... autoset to square of eigen tolerance\n", SRESTOL);
	}
	if (LANCZOS_MAXITNS > 0) fprintf(tempfile,
	    "  LANCZOS_MAXITNS = %d\n", LANCZOS_MAXITNS);
	else fprintf(tempfile,
	    "  LANCZOS_MAXITNS = %d ... autoset to twice # vertices\n", LANCZOS_MAXITNS);
	if (LANCZOS_SO_PRECISION == 1) fprintf(tempfile,
	    "  LANCZOS_SO_PRECISION = 1 ... single precision\n");  
	else fprintf(tempfile, "  LANCZOS_SO_PRECISION = 2 ... double precision\n");  
	fprintf(tempfile, "  LANCZOS_SO_INTERVAL = %d\n", LANCZOS_SO_INTERVAL);
        if (LANCZOS_CONVERGENCE_MODE == 1) fprintf(tempfile,
	    "  LANCZOS_CONVERGENCE_MODE = 1 ... partition tolerance\n");
        else fprintf(tempfile,
	    "  LANCZOS_CONVERGENCE_MODE = 0 ... residual tolerance\n");
	fprintf(tempfile, "  BISECTION_SAFETY = %g\n", BISECTION_SAFETY);
        if (LANCZOS_TYPE == 3 || LANCZOS_TYPE == 4) {
	    if (LANCZOS_TIME == 0) fprintf(tempfile,
		"  LANCZOS_TIME = 0 ... no detailed timing\n");
	    else fprintf(tempfile,
		"  LANCZOS_TIME = 1 ... detailed timing\n");
	}

	if (WARNING_EVECS > 0) {
	    fprintf(tempfile, "  WARNING_EVECS = %d\n", WARNING_EVECS);
	}

        if (MAPPING_TYPE == 0) fprintf(tempfile, "  MAPPING_TYPE = 0 ... cut at origin\n");
        else if (MAPPING_TYPE == 1) fprintf(tempfile,
	    "  MAPPING_TYPE = 1 ... min-cost assignment\n");
        else if (MAPPING_TYPE == 2) fprintf(tempfile,
	    "  MAPPING_TYPE = 2 ... recursive median\n");
        else if (MAPPING_TYPE == 3) fprintf(tempfile,
	    "  MAPPING_TYPE = 3 ... independent medians\n");

	fprintf(tempfile, "  MAKE_CONNECTED = %s\n", true_or_false(MAKE_CONNECTED));
	fprintf(tempfile, "  PERTURB = %s\n",true_or_false(PERTURB));
	if (PERTURB) {
	    fprintf(tempfile, "    NPERTURB = %d\n",NPERTURB);
	    fprintf(tempfile, "    PERTURB_MAX = %g\n",PERTURB_MAX);
	}
	if (ndims == 3) {
	    fprintf(tempfile, "  OPT3D_NTRIES = %d\n", OPT3D_NTRIES);
	}
    }
    if (global_method == 1) {
	fprintf(tempfile, "  COARSEN_RATIO_MIN = %g\n", COARSEN_RATIO_MIN);
	fprintf(tempfile, "  COARSE_NLEVEL_KL = %d\n", COARSE_NLEVEL_KL);
	fprintf(tempfile, "  MATCH_TYPE = %d\n", MATCH_TYPE);
	fprintf(tempfile, "  HEAVY_MATCH = %s\n", true_or_false(HEAVY_MATCH));
	fprintf(tempfile, "  COARSE_KL_BOTTOM = %s\n", true_or_false(COARSE_KL_BOTTOM));
	fprintf(tempfile, "  COARSEN_VWGTS = %s\n", true_or_false(COARSEN_VWGTS));
	fprintf(tempfile, "  COARSEN_EWGTS = %s\n", true_or_false(COARSEN_EWGTS));
	fprintf(tempfile, "  KL_ONLY_BNDY = %s\n", true_or_false(KL_ONLY_BNDY));
/*
	if (LIMIT_KL_EWGTS) {
	    fprintf(tempfile, "  LIMIT_KL_EWGTS = %s, EWGT_RATIO_MAX = %g\n",
		    true_or_false(LIMIT_KL_EWGTS), EWGT_RATIO_MAX);
	}
	else {
	    fprintf(tempfile, "  LIMIT_KL_EWGTS = %s\n", true_or_false(LIMIT_KL_EWGTS));
	}
*/
    }

    if (global_method == 2 && rqi_flag) {
	fprintf(tempfile, "  COARSE_NLEVEL_RQI = %d\n", COARSE_NLEVEL_RQI);
        if (RQI_CONVERGENCE_MODE == 1)
	    fprintf(tempfile, "  RQI_CONVERGENCE_MODE = 1 ... partition tolerance\n");
        else fprintf(tempfile, "  RQI_CONVERGENCE_MODE = 0 ... residual tolerance\n");
	fprintf(tempfile, "  COARSEN_RATIO_MIN = %g\n", COARSEN_RATIO_MIN);
	fprintf(tempfile, "  COARSEN_VWGTS = %s\n", true_or_false(COARSEN_VWGTS));
	fprintf(tempfile, "  COARSEN_EWGTS = %s\n", true_or_false(COARSEN_EWGTS));
    }

    if (global_method == 1 || local_method == 1) {
	fprintf(tempfile, "  KL_RANDOM = %s\n", true_or_false(KL_RANDOM));
	if (KL_METRIC == 1) {
	    fprintf(tempfile, "  KL_METRIC = Cuts\n");
	}
	else if (KL_METRIC == 2) {
	    fprintf(tempfile, "  KL_METRIC = Hops\n");
	}
	fprintf(tempfile, "  KL_NTRIES_BAD = %d\n", KL_NTRIES_BAD);
	fprintf(tempfile, "  KL_BAD_MOVES = %d\n", KL_BAD_MOVES);
	fprintf(tempfile, "  KL_UNDO_LIST = %s\n", true_or_false(KL_UNDO_LIST));
	fprintf(tempfile, "  KL_IMBALANCE = %g\n", KL_IMBALANCE);
    }

    if (global_method == 1 || global_method == 2 || local_method == 1) {
	fprintf(tempfile, "  TERM_PROP = %s\n", true_or_false(TERM_PROP));
	if (TERM_PROP) {
	    fprintf(tempfile, "    CUT_TO_HOP_COST = %g\n", CUT_TO_HOP_COST);
	}
    }

    if (SEQUENCE) {
	fprintf(tempfile, "  SEQUENCE = %s, Sequence file name = `%s'\n",
		true_or_false(SEQUENCE), SEQ_FILENAME);
    }

    fprintf(tempfile, "  OUTPUT_METRICS = %d\n", OUTPUT_METRICS);
    fprintf(tempfile, "  MAKE_VWGTS = %s\n", true_or_false(MAKE_VWGTS));
    fprintf(tempfile, "  REFINE_MAP = %s\n", true_or_false(REFINE_MAP));
    fprintf(tempfile, "  REFINE_PARTITION = %d\n", REFINE_PARTITION);
    fprintf(tempfile, "  INTERNAL_VERTICES = %s\n", true_or_false(INTERNAL_VERTICES));

    if (SIMULATOR > 0) {
        fprintf(tempfile, "  SIMULATOR = %d\n", SIMULATOR);
        fprintf(tempfile, "  SIMULATION_ITNS = %d\n", SIMULATION_ITNS);
        fprintf(tempfile, "  CUT_COST = %g\n", CUT_COST);
        fprintf(tempfile, "  HOP_COST = %g\n", HOP_COST);
        fprintf(tempfile, "  BDY_COST = %g\n", BDY_COST);
        fprintf(tempfile, "  BDY_HOP_COST = %g\n", BDY_HOP_COST);
        fprintf(tempfile, "  STARTUP_COST = %g\n", STARTUP_COST);
    }

    /* Now print out all the nonzero debug parameters. */
    if (DEBUG_CONNECTED) 
	fprintf(tempfile, "  DEBUG_CONNECTED = %d\n", DEBUG_CONNECTED);
    if (DEBUG_PERTURB) 
	fprintf(tempfile, "  DEBUG_PERTURB = %d\n", DEBUG_PERTURB);
    if (DEBUG_ASSIGN) 
	fprintf(tempfile, "  DEBUG_ASSIGN = %d\n", DEBUG_ASSIGN);
    if (DEBUG_INERTIAL) 
	fprintf(tempfile, "  DEBUG_INERTIAL = %d\n", DEBUG_INERTIAL);
    if (DEBUG_OPTIMIZE) 
	fprintf(tempfile, "  DEBUG_OPTIMIZE = %d\n", DEBUG_OPTIMIZE);
    if (DEBUG_BPMATCH) 
	fprintf(tempfile, "  DEBUG_BPMATCH = %d\n", DEBUG_BPMATCH);
    if (DEBUG_COARSEN) 
	fprintf(tempfile, "  DEBUG_COARSEN = %d\n", DEBUG_COARSEN);
    if (DEBUG_EVECS) 
	fprintf(tempfile, "  DEBUG_EVECS = %d\n", DEBUG_EVECS);
    if (DEBUG_KL) 
	fprintf(tempfile, "  DEBUG_KL = %d\n", DEBUG_KL);
    if (DEBUG_MEMORY) 
	fprintf(tempfile, "  DEBUG_MEMORY = %d\n", DEBUG_MEMORY);
    if (DEBUG_INPUT) 
	fprintf(tempfile, "  DEBUG_INPUT = %d\n", DEBUG_INPUT);
    if (DEBUG_PARAMS) 
	fprintf(tempfile, "  DEBUG_PARAMS = %d\n", DEBUG_PARAMS);
    if (DEBUG_INTERNAL) 
	fprintf(tempfile, "  DEBUG_INTERNAL = %d\n", DEBUG_INTERNAL);
    if (DEBUG_REFINE_PART) 
	fprintf(tempfile, "  DEBUG_REFINE_PART = %d\n", DEBUG_REFINE_PART);
    if (DEBUG_REFINE_MAP) 
	fprintf(tempfile, "  DEBUG_REFINE_MAP = %d\n", DEBUG_REFINE_MAP);
    if (DEBUG_SIMULATOR) 
	fprintf(tempfile, "  DEBUG_SIMULATOR = %d\n", DEBUG_SIMULATOR);
    if (DEBUG_TRACE) 
	fprintf(tempfile, "  DEBUG_TRACE = %d\n", DEBUG_TRACE);
    if (DEBUG_MACH_PARAMS) 
	fprintf(tempfile, "  DEBUG_MACH_PARAMS = %d\n", DEBUG_MACH_PARAMS);
}
