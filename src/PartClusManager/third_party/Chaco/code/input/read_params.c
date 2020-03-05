/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "defs.h"
#include "params.h"

static int read_intTF(), read_long(), read_double(), read_string();

int       SIMULATOR = 0;        /* Run simulator? In what mode? */
int       SIMULATION_ITNS = 1;  /* # iterations simulator is to imitate. */
int       PERCENTAGE_OUTPUT = FALSE;    /* Output in percent? (TRUE/FALSE) */
double    CUT_COST = 0.0;       /* Communication cost of a cut-edge. */
double    HOP_COST = 0.0;       /* Communication cost of a hop. */
double    BDY_COST = 0.0;       /* Cost associated with boundary vertices.  */
double    BDY_HOP_COST = 0.0;   /* Cost associated with boundary hops. */
double    STARTUP_COST = 0.0;   /* Communication cost of a message startup. */
	/* Note: nCUBE2 startup: 112e-6, per byte: 4.6e-6, buffering 5.6e-6 */
	/* Intel Paragon startup: 70e-6, per byte: 5.3e-8 */


void      read_params(pfile)
FILE     *pfile;		/* file with new user parameters */
{
    extern int CHECK_INPUT;	/* check the input for consistency? */
    extern int ECHO;		/* print input/param options? to file? (-2..2) */
    extern int OUTPUT_METRICS;	/* controls displaying of results (0..3) */
    extern int OUTPUT_TIME;	/* at what level to display timings (0..2) */
    extern int OUTPUT_ASSIGN;	/* write assignments to file? */
    extern int OUT_ASSIGN_INV;	/* if so, use inverted form? */
    extern int IN_ASSIGN_INV;	/* if reading assignment, use inverted form? */
    extern int PROMPT;		/* whether to prompt for input */
    extern int PRINT_HEADERS;	/* print lines for output sections */

    extern int LANCZOS_TYPE;		/* type of Lanczos orthogonalization */
    extern double EIGEN_TOLERANCE;	/* convergence criteria for eigen-tol */
    extern double SRESTOL;		/* limit on relative T residual */
    extern int LANCZOS_SO_INTERVAL;	/* interval between orthog checks in SO */
    extern int LANCZOS_MAXITNS;		/* if >0, Lanczos iteration limit */
    extern double BISECTION_SAFETY;	/* divides Lanczos bisection tol */
    extern int LANCZOS_TIME;		/* do detailed Lanczos timing?*/
    extern int LANCZOS_CONVERGENCE_MODE;/* type of Lanczos convergence test */
    extern int RQI_CONVERGENCE_MODE;	/* type of rqi convergence test */
    extern int WARNING_EVECS;		/* warnings in eigenvector generation? */
    extern double WARNING_ORTHTOL;	/* warning if Ares & bjitol have ratio */
    extern double WARNING_MISTOL;	/* warning if Ares & bjitol have ratio */
    extern int TIME_KERNELS;		/* time numerical kernels? */
    extern int LANCZOS_SO_PRECISION;	/* arithmetic type in eigen routines */

    extern int MAKE_CONNECTED;	  /* connect graph if using spectral method? */
    extern int PERTURB;		  /* randomly perturb matrix in spectral method? */
    extern int NPERTURB;	  /* if so, how many edges to modify? */
    extern double PERTURB_MAX;	  /* largest value for perturbation */
    extern int MAPPING_TYPE;	  /* how to map from eigenvectors to partition */
    extern int COARSE_NLEVEL_RQI; /* # levels between RQI uncoarsening calls */
    extern int OPT3D_NTRIES;	  /* # local opts to look for global min in opt3d */

    extern int KL_METRIC;	/* KL interset cost: 1=>cuts, 2=>hops */
    extern int KL_RANDOM;	/* use randomness in Kernighan-Lin? */
    extern int KL_BAD_MOVES;	/* number of unhelpful moves in a row allowed */
    extern int KL_NTRIES_BAD;	/* # unhelpful passes before quitting KL */
    extern int KL_UNDO_LIST;	/* only re-sort vertices changed in prior pass? */
    extern int KL_ONLY_BNDY;	/* only consider moving vtxs on boundary? */
    extern double KL_IMBALANCE;	/* imbalance tolerated by KL */

    extern double COARSEN_RATIO_MIN; /* min vtx reduction at each coarsen stage */
    extern int COARSE_NLEVEL_KL;     /* levels between KL calls in uncoarsening */
    extern int MATCH_TYPE;	     /* matching algorithm for coarsening */
    extern int HEAVY_MATCH;	     /* encourage heavy matching edges? */
    extern int COARSE_KL_BOTTOM;     /* force KL invocation on smallest graph */
    extern int COARSEN_VWGTS;	     /* use vertex weights while coarsening? */
    extern int COARSEN_EWGTS;	     /* use edge weights while coarsening? */

    extern int REFINE_PARTITION;  /* # passes post-processing KL */
    extern int INTERNAL_VERTICES; /* post-process to increase internal nodes */
    extern int REFINE_MAP;	  /* greedy post-processing to reduce hops? */

    extern int ARCHITECTURE;	  /* 0 => hypercube, d => d-dimensional mesh */
    extern int SIMULATOR;	  /* run simulator or not?  In what mode? */
    extern int SIMULATION_ITNS;	  /* number of iterations simulator is to imitate */
    extern int PERCENTAGE_OUTPUT; /* print certain output in percent format */
    extern double CUT_COST;	  /* communication cost of a cut-edge */
    extern double HOP_COST;	  /* communication cost of a hop */
    extern double BDY_COST;	  /* cost associated with boundary node */
    extern double BDY_HOP_COST;	  /* cost associated with boundary hops */
    extern double STARTUP_COST;	  /* communication cost of a message startup */

    extern int TERM_PROP;	  /* perform terminal propagation */
    extern double CUT_TO_HOP_COST;	/* ..if so, relative cut/hop importance */
    extern int SEQUENCE;	  /* filename for sequence output */
    extern char SEQ_FILENAME[];	  /* filename for sequence output */
    extern long RANDOM_SEED;	  /* seed for random number generators */
    extern int NSQRTS;		  /* # square roots to precompute if coarsening */
    extern int MAKE_VWGTS;	  /* impose vertex weights = vertex degree */
    extern int FREE_GRAPH;	  /* free data after reformatting? */
    extern char *PARAMS_FILENAME; /* name of parameters file */

    extern int DEBUG_EVECS;	  /* debug flag for eigenvector generation */
    extern int DEBUG_KL;	  /* debug flag for Kernighan-Lin */
    extern int DEBUG_INERTIAL;	  /* debug flag for inertial method */
    extern int DEBUG_CONNECTED;	  /* debug flag for connected components */
    extern int DEBUG_PERTURB;	  /* debug flag for matrix perturbation */
    extern int DEBUG_ASSIGN;	  /* debug flag for assignment to sets */
    extern int DEBUG_OPTIMIZE;	  /* debug flag for optimization/rotation */
    extern int DEBUG_BPMATCH;	  /* debug flag for bipartite matching code */
    extern int DEBUG_COARSEN;	  /* debug flag for coarsening/uncoarsening */
    extern int DEBUG_MEMORY;	  /* debug flag for smalloc/sfree */
    extern int DEBUG_INPUT;	  /* debug flag for having read input files */
    extern int DEBUG_PARAMS;	  /* debug flag for reading parameter file */
    extern int DEBUG_INTERNAL;	  /* debug flag for forcing internal vtxs */
    extern int DEBUG_REFINE_PART; /* debug flag for refining partition */
    extern int DEBUG_REFINE_MAP;  /* debug flag for refining mapping */
    extern int DEBUG_SIMULATOR;	  /* debug flag for communications simulator */
    extern int DEBUG_TRACE;	  /* trace main execution path */
    extern int DEBUG_MACH_PARAMS; /* print computed machine params? */
    extern int DEBUG_COVER;	  /* debug max flow calculation? */

    extern int LIMIT_KL_EWGTS;	  /* limit edge weights in multilevel method? */
    extern double EWGT_RATIO_MAX; /* if so, max allowed ewgt/nvtxs */

    extern int EXPERT;		  /* expert user or not? */
    extern int VERTEX_SEPARATOR;  /* find vertex instead of edge separator? */
    extern int VERTEX_COVER;	  /* improve/find vtx separator via matching? */
    extern int FLATTEN;		  /* compress redundant vtxs at start? */
    extern int COARSE_KLV;	  /* apply klv as a multilevel smoother? */
    extern int COARSE_BPM;	  /* use bp match/flow as multilevel smoother? */
    extern int KL_MAX_PASS;	  /* max number of outer loops in KL */
    extern FILE *Output_File;	  /* output file or null */

    static char *TFpnames[] = {	/* names of true/false parameters */
	"CHECK_INPUT", "MAKE_CONNECTED", "COARSEN_VWGTS",
	"COARSEN_EWGTS", "KL_RANDOM", "KL_ONLY_BNDY",
	"KL_UNDO_LIST", "COARSE_KL_BOTTOM", "PERTURB",
	"OUTPUT_ASSIGN", "PROMPT", "PERCENTAGE_OUTPUT",
	"MAKE_VWGTS", "REFINE_MAP", "PRINT_HEADERS", 
	"TERM_PROP", "LIMIT_KL_EWGTS", "SIMULATOR",
        "FREE_GRAPH", "SEQUENCE", "LANCZOS_TIME",
	"TIME_KERNELS", "HEAVY_MATCH", "EXPERT",
	"OUT_ASSIGN_INV", "IN_ASSIGN_INV", "VERTEX_SEPARATOR",
	"VERTEX_COVER", "FLATTEN", NULL };

    static int *TFparams[] = {	/* pointers to true/false parameters */
	&CHECK_INPUT, &MAKE_CONNECTED, &COARSEN_VWGTS,
	&COARSEN_EWGTS, &KL_RANDOM, &KL_ONLY_BNDY,
	&KL_UNDO_LIST, &COARSE_KL_BOTTOM, &PERTURB,
	&OUTPUT_ASSIGN, &PROMPT, &PERCENTAGE_OUTPUT,
	&MAKE_VWGTS, &REFINE_MAP, &PRINT_HEADERS, 
	&TERM_PROP, &LIMIT_KL_EWGTS, &SIMULATOR,
        &FREE_GRAPH, &SEQUENCE, &LANCZOS_TIME,
	&TIME_KERNELS, &HEAVY_MATCH, &EXPERT,
	&OUT_ASSIGN_INV, &IN_ASSIGN_INV, &VERTEX_SEPARATOR,
	&VERTEX_COVER, &FLATTEN };

    static char *ipnames[] = {	/* names of integer parameters */
	"ECHO", "NSQRTS", "ARCHITECTURE",
	"LANCZOS_SO_INTERVAL", "LANCZOS_MAXITNS", "MAPPING_TYPE",
	"COARSE_NLEVEL_RQI", "KL_METRIC", "KL_NTRIES_BAD",
	"KL_BAD_MOVES", "COARSE_NLEVEL_KL", "NPERTURB",
	"OPT3D_NTRIES", "DEBUG_CONNECTED", "DEBUG_PERTURB",
	"DEBUG_ASSIGN", "DEBUG_INERTIAL", "DEBUG_OPTIMIZE",
	"DEBUG_BPMATCH", "DEBUG_COARSEN", "DEBUG_EVECS",
	"DEBUG_KL", "DEBUG_MEMORY", "DEBUG_INPUT",
	"DEBUG_PARAMS", "DEBUG_TRACE", "WARNING_EVECS",
	"OUTPUT_TIME", "OUTPUT_METRICS", "SIMULATION_ITNS",
	"DEBUG_SIMULATOR", "LANCZOS_CONVERGENCE_MODE",
	"RQI_CONVERGENCE_MODE", "REFINE_PARTITION", "INTERNAL_VERTICES",
        "LANCZOS_TYPE", "MATCH_TYPE", "DEBUG_INTERNAL",
        "DEBUG_REFINE_PART", "DEBUG_REFINE_MAP", "DEBUG_MACH_PARAMS",
	"DEBUG_COVER", "LANCZOS_SO_PRECISION", "COARSE_KLV",
	"COARSE_BPM", "KL_MAX_PASS", NULL};

    static int *iparams[] = {	/* pointers to integer parameters */
	&ECHO, &NSQRTS, &ARCHITECTURE,
	&LANCZOS_SO_INTERVAL, &LANCZOS_MAXITNS, &MAPPING_TYPE,
	&COARSE_NLEVEL_RQI, &KL_METRIC, &KL_NTRIES_BAD,
	&KL_BAD_MOVES, &COARSE_NLEVEL_KL, &NPERTURB,
	&OPT3D_NTRIES, &DEBUG_CONNECTED, &DEBUG_PERTURB,
	&DEBUG_ASSIGN, &DEBUG_INERTIAL, &DEBUG_OPTIMIZE,
	&DEBUG_BPMATCH, &DEBUG_COARSEN, &DEBUG_EVECS,
	&DEBUG_KL, &DEBUG_MEMORY, &DEBUG_INPUT,
	&DEBUG_PARAMS, &DEBUG_TRACE, &WARNING_EVECS,
	&OUTPUT_TIME, &OUTPUT_METRICS, &SIMULATION_ITNS,
	&DEBUG_SIMULATOR, &LANCZOS_CONVERGENCE_MODE,
	&RQI_CONVERGENCE_MODE, &REFINE_PARTITION, &INTERNAL_VERTICES,
        &LANCZOS_TYPE, &MATCH_TYPE, &DEBUG_INTERNAL,
        &DEBUG_REFINE_PART, &DEBUG_REFINE_MAP, &DEBUG_MACH_PARAMS,
	&DEBUG_COVER, &LANCZOS_SO_PRECISION, &COARSE_KLV,
	&COARSE_BPM, &KL_MAX_PASS };

    static char *lpnames[] = {	/* names of long parameters */
        "RANDOM_SEED", NULL};

    static long *lparams[] = {	/* pointers to long parameters */
        &RANDOM_SEED};

    static char *cpnames[] = {	/* names to character parameters */
        "SEQ_FILENAME", NULL};

    static char *cparams[] = {	/* pointers to character parameters */
         SEQ_FILENAME };


    static char *dpnames[] = {	/* names of double precision parameters */
	"EIGEN_TOLERANCE", "BISECTION_SAFETY", "PERTURB_MAX",
	"WARNING_ORTHTOL", "WARNING_MISTOL", "SRESTOL",
	"COARSEN_RATIO_MIN", "EWGT_RATIO_MAX", "CUT_COST",
	"HOP_COST", "BDY_COST", "BDY_HOP_COST",
	"STARTUP_COST", "CUT_TO_HOP_COST", "KL_IMBALANCE",
	NULL };

    static double *dparams[] = {/* pointers to double precision parameters */
	&EIGEN_TOLERANCE, &BISECTION_SAFETY, &PERTURB_MAX,
	&WARNING_ORTHTOL, &WARNING_MISTOL, &SRESTOL,
	&COARSEN_RATIO_MIN, &EWGT_RATIO_MAX, &CUT_COST,
	&HOP_COST, &BDY_COST, &BDY_HOP_COST,
	&STARTUP_COST, &CUT_TO_HOP_COST, &KL_IMBALANCE };

    char      line[LINE_LENGTH + 1];	/* line from parameters file */
    char     *ptr, *idptr;	/* loops through input line */
    char      id[25];		/* parameter identifier being modified */
    static int linenum = 0;	/* line number within parameter file */
    int       flag;		/* return flag for number scanning routines */
    int       matched;		/* has character string been matched? */
    int       comment;		/* should input line be ignored? */
    int       i;		/* loop counters */
    char     *true_or_false();

    if (DEBUG_TRACE > 0) {
	printf("<Entering read_params>\n");
    }

    if (pfile == NULL) {
	return;
    }

    if (DEBUG_PARAMS > 0) {
	printf("Reading parameter modification file `%s'\n", PARAMS_FILENAME);
    }

    while (fgets(line, LINE_LENGTH, pfile) != NULL) {
	linenum++;

	/* Scan line for first non whitespace character. */
	ptr = line;
	while (isspace(*ptr) && *ptr != '\0')
	    ptr++;

	if (*ptr == '%' || *ptr == '#' || *ptr == '\0') {
	    /* This line is a comment or blank so don't process it. */
	    comment = TRUE;
	}
	else {
	    comment = FALSE;
	    /* Scan line for character string. */
	    while (!isalpha(*ptr) && *ptr != '\0')
		ptr++;

	    idptr = id;
	    /* Copy string to id and capitalize the letters. */
	    while (*ptr == '_' || isalpha(*ptr)) {
		*idptr++ = toupper(*ptr++);
	    }
	    *idptr = '\0';
	}

	if (!comment && (int) strlen(id) > 0) {	/* Don't bother if line blank */

	    /* Check for exit condition. */
	    if (!strcmp(id, "STOP"))
		return;

	    /* Set this flag TRUE when recognize string. */
	    matched = FALSE;

	    /* Now compare against the different identifiers. */
	    for (i = 0; ipnames[i] != NULL && strcmp(id, ipnames[i]); i++);
	    if (ipnames[i] != NULL) {
		matched = TRUE;
		flag = read_intTF(ptr, iparams[i]);
		if (flag) {
		    if (DEBUG_PARAMS > 1) {
			printf("  Parameter `%s' reset to %d\n", ipnames[i],
			       *iparams[i]);
		    }
		}
		else {
		    if (DEBUG_PARAMS > 0) {
			printf(" ERROR reading value for `%s' in line %d parameter file %s\n",
			       ipnames[i], linenum, PARAMS_FILENAME);
			if (Output_File != NULL) {
			    fprintf(Output_File,
				" ERROR reading value for `%s' in line %d parameter file %s\n",
			        ipnames[i], linenum, PARAMS_FILENAME);
			}
		    }
		}
	    }

	    if (!matched) {
		for (i = 0; TFpnames[i] != NULL && strcmp(id, TFpnames[i]); i++);
		if (TFpnames[i] != NULL) {
		    matched = TRUE;
		    flag = read_intTF(ptr, TFparams[i]);
		    if (flag) {
		        if (DEBUG_PARAMS > 1) {
			    printf("  Parameter `%s' reset to %s\n", TFpnames[i],
				   true_or_false(*TFparams[i]));
			}
		    }
		    else {
		        if (DEBUG_PARAMS > 0) {
			    printf(" ERROR reading value for `%s' in line %d parameter file %s\n",
				   TFpnames[i], linenum, PARAMS_FILENAME);
			    if (Output_File != NULL) {
			        fprintf(Output_File,
				    " ERROR reading value for `%s' in line %d parameter file %s\n",
			            TFpnames[i], linenum, PARAMS_FILENAME);
			    }
			}
		    }
		}
	    }

	    if (!matched) {
		for (i = 0; lpnames[i] != NULL && strcmp(id, lpnames[i]); i++);
		if (lpnames[i] != NULL) {
		    matched = TRUE;
		    flag = read_long(ptr, lparams[i]);
		    if (flag) {
		        if (DEBUG_PARAMS > 1) {
			    printf("  Parameter `%s' reset to %ld\n", lpnames[i],
				   *lparams[i]);
			}
		    }
		    else {
		        if (DEBUG_PARAMS > 0) {
			    printf(" ERROR reading value for `%s' in line %d parameter file %s\n",
				   lpnames[i], linenum, PARAMS_FILENAME);
			    if (Output_File != NULL) {
			        fprintf(Output_File,
				    " ERROR reading value for `%s' in line %d parameter file %s\n",
				    lpnames[i], linenum, PARAMS_FILENAME);
			    }
			}
		    }
		}
	    }

	    if (!matched) {
		for (i = 0; cpnames[i] != NULL && strcmp(id, cpnames[i]); i++);
		if (cpnames[i] != NULL) {
		    matched = TRUE;
		    flag = read_string(ptr, cparams[i]);
		    if (flag) {
		        if (DEBUG_PARAMS > 1) {
			    printf("  Parameter `%s' reset to `%s'\n", cpnames[i],
				   cparams[i]);
			}
		    }
		    else {
		        if (DEBUG_PARAMS > 0) {
			    printf(" ERROR reading value for `%s' in line %d parameter file %s\n",
				   cpnames[i], linenum, PARAMS_FILENAME);
			    if (Output_File != NULL) {
			        fprintf(Output_File,
				    " ERROR reading value for `%s' in line %d parameter file %s\n",
				    cpnames[i], linenum, PARAMS_FILENAME);
			    }
			}
		    }
		}
	    }

	    if (!matched) {
		for (i = 0; dpnames[i] != NULL && strcmp(id, dpnames[i]); i++);
		if (dpnames[i] != NULL) {
		    matched = TRUE;
		    flag = read_double(ptr, dparams[i]);
		    if (flag) {
		        if (DEBUG_PARAMS > 1) {
			    printf("  Parameter `%s' reset to %g\n", dpnames[i],
				   *dparams[i]);
			}
		    }
		    else {
		        if (DEBUG_PARAMS > 0) {
			    printf(" ERROR reading value for `%s' in line %d parameter file %s\n",
				   dpnames[i], linenum, PARAMS_FILENAME);
			    if (Output_File != NULL) {
			        fprintf(Output_File,
				    " ERROR reading value for `%s' in line %d parameter file %s\n",
				    dpnames[i], linenum, PARAMS_FILENAME);
			    }
			}
		    }
		}
	    }

	    if (!matched && DEBUG_PARAMS > 0) {
		printf(" WARNING: Parameter `%s' on line %d of file %s not recognized\n",
		       id, linenum, PARAMS_FILENAME);
		if (Output_File != NULL) {
		    fprintf(Output_File,
			" WARNING: Parameter `%s' on line %d of file %s not recognized\n",
			id, linenum, PARAMS_FILENAME);
		}
	    }
	}
    }
    if (DEBUG_PARAMS > 0) {
	printf("\n");
    }
}


static int read_intTF(ptr, val)
char     *ptr;			/* pointer to string to parse */
int      *val;			/* value returned */
{
    int       nvals;		/* number of values sucessfully read */

    while (*ptr != 'T' && *ptr != 't' && *ptr != 'F' && *ptr != 'f' &&
	    *ptr != '-' && !isdigit(*ptr) && *ptr != '\0')
	ptr++;
    if (*ptr == 'T' || *ptr == 't')
	*val = 1;
    else if (*ptr == 'F' || *ptr == 'f')
	*val = 0;
    else {
	nvals = sscanf(ptr, "%d", val);
	if (nvals != 1)
	    return (FALSE);
    }

    return (TRUE);
}


static int read_double(ptr, val)
char     *ptr;			/* pointer to string to parse */
double   *val;			/* value returned */
{
    int       nvals;		/* number of values sucessfully read */

    while (*ptr != '-' && *ptr != '.' && !isdigit(*ptr) && *ptr != '\0')
	ptr++;

    nvals = sscanf(ptr, "%lf", val);
    if (nvals != 1)
	return (FALSE);
    else
	return (TRUE);
}


static int read_long(ptr, val)
char     *ptr;			/* pointer to string to parse */
long     *val;			/* value returned */
{
    int       nvals;		/* number of values sucessfully read */

    while (*ptr != '-' && !isdigit(*ptr) && *ptr != '\0')
	ptr++;

    nvals = sscanf(ptr, "%ld", val);
    if (nvals != 1)
	return (FALSE);
    else
	return (TRUE);
}


static int read_string(ptr, val)
char     *ptr;			/* pointer to string to parse */
char     *val;			/* value returned */
{
    char      *sptr;		/* loops through val */

    while ((isspace(*ptr) || *ptr == '=') && *ptr != '\0') ptr++;

    if (*ptr == '\0') return(FALSE);

    sptr = val;

    while (!(isspace(*ptr) || *ptr == '\0')) *sptr++ = *ptr++;
    *sptr = '\0';

    return (TRUE);
}
