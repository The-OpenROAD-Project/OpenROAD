/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "defs.h"
#include "params.h"
#include "structs.h"

double   *SQRTS;		/* precomputed square roots for efficiency */

double    DOUBLE_EPSILON;	/* machine precision */
double    DOUBLE_MAX;		/* largest double precision value */

char     *Graph_File_Name = NULL;	/* Input graph file name */
char     *Geometry_File_Name = NULL;	/* Input coordinate file name */
char     *Assign_In_File_Name = NULL;	/* Input assignment file name */

FILE     *Output_File = NULL;	/* Pointer to output file or NULL */

int	  EXPERT = 0;		/* user mode */

int       submain(graph, nvtxs, nedges, using_vwgts, using_ewgts, igeom, coords,
		            outassignname, outfilename,
		            assignment, goal,
		            architecture, ndims_tot, mesh_dims,
		            global_method, local_method, rqi_flag, vmax, ndims,
		            eigtol, seed)
struct vtx_data **graph;	/* data structure for graph */
int       nvtxs;		/* number of vertices in full graph */
int       nedges;		/* number of edges in graph */
int       using_vwgts;		/* are vertex weights being used? */
int       using_ewgts;		/* are edge weights being used? */
int       igeom;		/* geometry dimension if using inertial method */
float   **coords;		/* coordinates of vertices if used */
char     *outassignname;	/* name of assignment output file */
char     *outfilename;		/* in which to print output metrics */
short    *assignment;		/* set number of each vtx (length n) */
double   *goal;			/* desired sizes for each set */
int       architecture;		/* 0=> hypercube, d=> d-dimensional mesh */
int       ndims_tot;		/* total number hypercube dimensions */
int       mesh_dims[3];		/* extent of mesh in 3 directions */
int       global_method;	/* global partitioning algorithm */
int       local_method;		/* local partitioning algorithm */
int       rqi_flag;		/* use RQI/Symmlq eigensolver? */
int       vmax;			/* if so, how many vtxs to coarsen down to */
int       ndims;		/* number of eigenvectors (2^d sets) */
double    eigtol;		/* tolerance on eigenvectors */
long      seed;			/* for random graph mutations */
{
    extern int ECHO;		/* controls output to file or screen */
    extern int CHECK_INPUT;	/* should I check input for correctness? */
    extern int SEQUENCE;	/* just generate spectal ordering? */
    extern int OUTPUT_METRICS;	/* whether to tally and display cost metrics */
    extern int OUTPUT_ASSIGN;	/* print assignment to a file? */
    extern int PERTURB;		/* perturb matrix if quad/octasection? */
    extern int NSQRTS;		/* number of square roots to precompute */
    extern int KL_METRIC;	/* KL interset cost: 1=>cuts, 2=>hops */
    extern int RAND_MAXIMUM;	/* largest value returnable by rand() */
    extern int LANCZOS_TYPE;	/* type of Lanczos to use */
    extern int REFINE_MAP;	/* use greedy strategy to improve mapping? */
    extern int REFINE_PARTITION;/* number of calls to pairwise_refine to make */
    extern int VERTEX_COVER;	/* use matching to reduce vertex separator? */
    extern int INTERNAL_VERTICES;	/* greedily increase internal vtxs? */
    extern int DEBUG_INTERNAL;		/* debug code about force_internal? */
    extern int DEBUG_REFINE_PART;	/* debug code about refine_part? */
    extern int DEBUG_REFINE_MAP;	/* debug code about refine_map? */
    extern int DEBUG_MACH_PARAMS;	/* print out computed machine params? */
    extern int DEBUG_TRACE;	/* trace main execution path */
    extern int PRINT_HEADERS;	/* print section headings for output? */
    extern int TIME_KERNELS;	/* benchmark some numerical kernels? */
    extern double start_time;	/* time code was entered */
    extern double total_time;	/* (almost) total time spent in code */
    extern double check_input_time;	/* time spent checking input */
    extern double partition_time;	/* time spent partitioning graph */
    extern double kernel_time;	/* time spent benchmarking kernels */
    extern double count_time;	/* time spent evaluating the answer */
    extern double print_assign_time;	/* time spent writing output file */
    FILE     *outfile;		/* output file */
    struct vtx_data **graph2;	/* data structure for graph */
    short     hop_mtx[MAXSETS][MAXSETS];	/* between-set hop cost for KL */
    double   *vwsqrt;		/* sqrt of vertex weights (length nvtxs+1) */
    double    time, time1;	/* timing variables */
    char     *graphname, *geomname;	/* names of input files */
    char     *inassignname;	/* name of assignment input file */
    int       old_nsqrts;	/* old value of NSQRTS */
    int       append;		/* append output to existing file? */
    int       nsets;		/* number of sets created by each divide */
    int       nsets_tot;	/* total number of sets */
    int       bits;		/* used in computing hops */
    int       flag;		/* return code from check_input */
    int       old_perturb;	/* saves original pertubation flag */
    int       i, j, k;		/* loop counters */
    double    seconds();
    void      setrandom();
    double   *smalloc_ret();
    int       check_input(), sfree(), refine_part();
    void      setrandom(), makevwsqrt(), balance(), countup();
    void      force_internal(), sequence(), reflect_input();
    void      machine_params(), assign_out(), refine_map();
    void      time_out(), time_kernels(), strout();


    if (DEBUG_TRACE > 0) {
	printf("<Entering submain>\n");
    }

    /* First check all the input for consistency. */

    if (architecture == 1)
	mesh_dims[1] = mesh_dims[2] = 1;
    else if (architecture == 2)
	mesh_dims[2] = 1;

    graphname = Graph_File_Name;
    geomname = Geometry_File_Name;
    inassignname = Assign_In_File_Name;

    /* Turn of perturbation if using bisection */
    if (ndims == 1) {
        old_perturb = PERTURB;
	PERTURB = FALSE;
    }

    if (ECHO < 0 && outfilename != NULL) { /* Open output file */
	outfile = fopen(outfilename, "r");
	if (outfile != NULL) append = TRUE;
	else append = FALSE;
	fclose(outfile);
	outfile = fopen(outfilename, "a");
	if (append) {
	    fprintf(outfile, "\n------------------------------------------------\n\n");
	}
    }
    else {
	outfile = NULL;
    }

    Output_File = outfile;

    if (outfile != NULL && PRINT_HEADERS) {
        fprintf(outfile, "\n                    Chaco 2.0\n");
        fprintf(outfile, "          Sandia National Laboratories\n\n");
    }

    if (CHECK_INPUT) {		/* Check the input for inconsistencies. */
	time1 = seconds();

	flag = check_input(graph, nvtxs, nedges, igeom, coords,
			   graphname, assignment, goal,
			   architecture, ndims_tot, mesh_dims,
			   global_method, local_method, rqi_flag, &vmax, ndims,
			   eigtol);

	check_input_time += seconds() - time1;

	if (flag) {
	    strout("ERROR IN INPUT.\n");
	    return (1);
	}
    }

    if (ECHO != 0) {
	reflect_input(nvtxs, nedges, igeom, graphname, geomname,
		      inassignname, outassignname, outfilename,
		      architecture, ndims_tot, mesh_dims,
		      global_method, local_method, rqi_flag, vmax, ndims,
		      eigtol, seed, outfile);
    }

    if (PRINT_HEADERS) {
        printf("\n\nStarting to partition ...\n\n");
	if (Output_File != NULL ) {
            fprintf(Output_File,
	    "\n\nStarting to partition ... (residual, warning and error messages only)\n\n");
	}
    }

    time = seconds();

    /* Perform some one-time initializations. */
    setrandom(seed);
    machine_params(&DOUBLE_EPSILON, &DOUBLE_MAX, &RAND_MAXIMUM);

    if (DEBUG_MACH_PARAMS > 0) {
	printf("Machine parameters:\n");
	printf("  DOUBLE_EPSILON = %e\n", DOUBLE_EPSILON);
	printf("  DOUBLE_MAX = %e\n", DOUBLE_MAX);
	printf("  RAND_MAXIMUM = %d\n\n", RAND_MAXIMUM);
    }

    nsets = (1 << ndims);

    old_nsqrts = NSQRTS;
    if (nvtxs < NSQRTS && !using_vwgts) {
	NSQRTS = nvtxs;
    }
    SQRTS = (double *) smalloc_ret((unsigned) (NSQRTS + 1) * sizeof(double));
    if (SQRTS == NULL) {
	strout("ERROR: No space to allocate sqrts\n");
	return(1);
    }
    for (i = 1; i <= NSQRTS; i++)
	SQRTS[i] = sqrt((double) i);

    if (using_vwgts && (global_method == 1 || global_method == 2)) {
	vwsqrt = (double *) smalloc_ret((unsigned) (nvtxs + 1) * sizeof(double));
	if (vwsqrt == NULL) {
	    strout("ERROR: No space to allocate vwsqrt\n");
	    sfree((char *) SQRTS);
	    NSQRTS = old_nsqrts;
	    return(1);
        }
	makevwsqrt(vwsqrt, graph, nvtxs);
    }
    else
	vwsqrt = NULL;

    if (TIME_KERNELS) {
	time1 = seconds();
	time_kernels(graph, nvtxs, vwsqrt);
	kernel_time += seconds() - time1;
    }

    if (SEQUENCE) {
	sequence(graph, nvtxs, nedges, using_ewgts, vwsqrt,
		 LANCZOS_TYPE, rqi_flag, vmax, eigtol);
	goto End_Label;
    }

    /* Initialize cost function for KL-spiff */
    if (global_method == 1 || local_method == 1) {
	for (i = 0; i < nsets; i++) {
	    hop_mtx[i][i] = 0;
	    for (j = 0; j < i; j++) {
		if (KL_METRIC == 2) {	/* Count hypercube hops */
		    hop_mtx[i][j] = 0;
		    bits = i ^ j;
		    while (bits) {
			if (bits & 1) {
			    ++hop_mtx[i][j];
			}
			bits >>= 1;
		    }
		}
		else if (KL_METRIC == 1) {	/* Count cut edges */
		    hop_mtx[i][j] = 1;
		}
		hop_mtx[j][i] = hop_mtx[i][j];
	    }
	}
    }

    graph2 = graph;
    if (global_method == 3 && local_method != 1 && !VERTEX_COVER && !using_vwgts) {
	graph2 = NULL;
    }

    if (!(global_method == 7 && local_method == 2)) {
        balance(graph2, nvtxs, nedges, using_vwgts, using_ewgts, vwsqrt,
	    igeom, coords, assignment, goal,
	    architecture, ndims_tot, mesh_dims,
	    global_method, local_method, rqi_flag, vmax, ndims,
	    eigtol, hop_mtx);
    }

    partition_time += seconds() - time - kernel_time;

    if (architecture == 0)
	nsets_tot = 1 << ndims_tot;
    else if (architecture > 0)
	nsets_tot = mesh_dims[0] * mesh_dims[1] * mesh_dims[2];

    if (graph != NULL) {
	j = TRUE;
        for (i = 1; i <= REFINE_PARTITION && j; i++) {	/* Reduce cuts w/ KL? */
	    if (DEBUG_REFINE_PART > 0) {
		printf("\n\nBefore pass %d to refine partition:\n", i);
		if (outfile != NULL) {
		    fprintf(outfile, "\n\nBefore pass %d to refine partition:\n", i);
		}
	        countup(graph, nvtxs, assignment, ndims, architecture, ndims_tot,
		        mesh_dims, OUTPUT_METRICS, outfile, using_ewgts);
	    }
	    j = refine_part(graph, nvtxs, using_ewgts, assignment, architecture,
		ndims_tot, mesh_dims, goal);
        }
    }

    if (graph != NULL) {
	if (INTERNAL_VERTICES) {
	    if (DEBUG_INTERNAL > 0) {
		printf("\n\nBefore increasing internal vertices:\n");
		if (outfile != NULL) {
		    fprintf(outfile, "\n\nBefore increasing internal vertices:\n");
		}
	        countup(graph, nvtxs, assignment, ndims, architecture, ndims_tot,
		        mesh_dims, OUTPUT_METRICS, outfile, using_ewgts);
	    }
	    force_internal(graph, nvtxs, using_ewgts, assignment, goal, nsets_tot,
			   INTERNAL_VERTICES);
	}
    }

    if (graph != NULL) {
        if (REFINE_MAP) {		/* Improve the mapping to processors? */
	    if (DEBUG_REFINE_MAP > 0) {
		printf("\n\nBefore refining mapping to processors:\n");
		if (outfile != NULL) {
		    fprintf(outfile, "\n\nBefore refining mapping to processors:\n");
		}
	        countup(graph, nvtxs, assignment, ndims, architecture, ndims_tot,
		        mesh_dims, OUTPUT_METRICS, outfile, using_ewgts);
	    }
	    refine_map(graph, nvtxs, using_ewgts, assignment, architecture, ndims_tot,
		   mesh_dims);
        }
    }

    if (OUTPUT_ASSIGN > 0 && outassignname != NULL) {
	time1 = seconds();
	if (architecture == 0) {
	    k = 1 << ndims_tot;
	}
	else if (architecture > 0) {
	    k = mesh_dims[0] * mesh_dims[1] * mesh_dims[2];
	}
	assign_out(nvtxs, assignment, k, outassignname);
	print_assign_time += seconds() - time1;
    }

    if (OUTPUT_METRICS != 0) {	/* Compute graph metrics of partition. */
	time1 = seconds();
	if (graph != NULL) {
            if (PRINT_HEADERS) {
                printf("\n\n                     Partitioning Results\n");
		if (outfile != NULL) {
                fprintf(outfile, "\n\n                     Partitioning Results\n");
		}
            }

	    countup(graph, nvtxs, assignment, ndims, architecture, ndims_tot,
		    mesh_dims, OUTPUT_METRICS, outfile, using_ewgts);
	}
	count_time += seconds() - time1;
    }


    /* Invoke communication simulator? */
    /*
    if (graph != NULL) {
        if (SIMULATOR > 0) {
	    simulate(graph, nvtxs, ndims, architecture, ndims_tot,
		 mesh_dims, assignment, using_ewgts, outfile);
        }
    }
    */

End_Label:

    if (vwsqrt != NULL)
	sfree((char *) vwsqrt);
    if (SQRTS != NULL)
	sfree((char *) SQRTS);

    /* Turn perturbation back on for next invocation. */
    if (ndims == 1) {
	PERTURB = old_perturb;
    }
    NSQRTS = old_nsqrts;


    total_time += seconds() - start_time;
    time_out(outfile);

    if (outfile != NULL) fclose(outfile);
    Output_File = NULL;

    return(0);
}
