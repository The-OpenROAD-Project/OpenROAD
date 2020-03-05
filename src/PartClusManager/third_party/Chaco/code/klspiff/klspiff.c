/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"params.h"
#include	"defs.h"

/* Think hard about space.
   Put active list into each routine.

   It may be possible to overlap dvals with active in kl, requiring
   a total of nvtxs space.
*/
static void free_kl();


void      klspiff(graph, nvtxs, sets, nsets, hops, goal, term_wgts, max_dev,
	          maxdeg, using_ewgts, bndy_list, weights)
struct vtx_data **graph;	/* list of graph info for each vertex */
int       nvtxs;		/* number of vertices in graph */
short    *sets;			/* local partitioning of vtxs */
int       nsets;		/* number of sets at each level */
short     (*hops)[MAXSETS];	/* hop cost between sets */
double   *goal;			/* desired set sizes */
float    *term_wgts[];		/* weights for terminal propogation */
int       max_dev;		/* largest deviation from balance allowed */
double    maxdeg;		/* largest weighted vertex degree */
int       using_ewgts;		/* are edge weights being used? */
int     **bndy_list;		/* list of vertices on boundary (0 ends) */
double   *weights;		/* vertex weights in each set */
{
    extern FILE *Output_File;	/* output file or null */
    extern double CUT_TO_HOP_COST;	/* relative importance of cuts/hops */
    extern int DEBUG_TRACE;	/* debug flag for Kernighan-Lin */
    extern int DEBUG_KL;	/* debug flag for Kernighan-Lin */
    extern double kl_total_time;
    extern double kl_init_time;
    extern double nway_kl_time;
    struct bilist ****buckets;	/* space for bucket sorts */
    struct bilist **listspace;	/* space for all bidirectional elements */
    float    *twptr;		/* loops through term_wgts */
    int     **dvals;		/* change in penalty for each possible move */
    int     **tops;		/* starting dval for each type of move */
    double    time, time1;	/* timing variables */
    float     maxterm;		/* largest terminal propogation preference */
    int       maxhop;		/* maximum hops between sets */
    int       maxdval;		/* largest transition cost for a vertex */
    double    cut_cost;		/* relative importance of cuts/hops */
    double    hop_cost;		/* relative importance of hops/cuts */
    int       error;		/* out of space? */
    int       i, j;		/* loop counters */
    double    seconds();
    int       kl_init(), nway_kl();
    void      count();

    time = seconds();

    if (DEBUG_TRACE > 0) {
	printf("<Entering klspiff, nvtxs = %d>\n", nvtxs);
    }

    /* Find the largest hop value. */
    maxhop = 0;
    for (i = 0; i < nsets; i++) {
	for (j = 0; j < nsets; j++) {
	    if (hops[i][j] > maxhop)
		maxhop = hops[i][j];
	}
    }

    maxterm = 0;
    cut_cost = hop_cost = 1;
    if (term_wgts[1] != NULL) {
	for (j = 1; j < nsets; j++) {
	    twptr = term_wgts[j];
	    for (i = nvtxs; i; i--) {
		++twptr;
	        if (*twptr > maxterm)
		    maxterm = *twptr;
	        else if (-*twptr > maxterm)
		    maxterm = -*twptr;
	    }
	}
	if (CUT_TO_HOP_COST > 1) { 
	    cut_cost = CUT_TO_HOP_COST;
	}
	else {
	    hop_cost = 1.0 / CUT_TO_HOP_COST;
	}
    }

    maxdval = (2 * maxterm * hop_cost + .5) + (maxdeg * cut_cost + .5) * maxhop;

    /* Allocate a bunch of space for KL. */
    time1 = seconds();
    error = kl_init(&buckets, &listspace, &dvals, &tops, nvtxs, nsets, maxdval);
    kl_init_time += seconds() - time1;
 
    if (!error) {
        if (DEBUG_KL > 0) {
	    printf(" Before KL: ");
	    count(graph, nvtxs, sets, nsets, hops, FALSE, using_ewgts);
        }

        time1 = seconds();
        error = nway_kl(graph, nvtxs, buckets, listspace, tops, dvals, sets,
	        maxdval, nsets, goal, term_wgts, hops, max_dev, using_ewgts,
		bndy_list, weights);
        nway_kl_time += seconds() - time1;

        if (DEBUG_KL > 1) {
	    printf(" After KL:");
	    count(graph, nvtxs, sets, nsets, hops, FALSE, using_ewgts);
	}
    }

    if (error) {
	printf("\nWARNING: No space to perform KL on graph with %d vertices.\n",
		nvtxs);
	printf("         NO LOCAL REFINEMENT PERFORMED.\n\n");

	if (Output_File != NULL) {
	    fprintf(Output_File,
	 	"\nWARNING: No space to perform KL on graph with %d vertices.\n",
		nvtxs);
	    fprintf(Output_File, "         LOCAL REFINEMENT NOT PERFORMED.\n\n");
	}
    }

    free_kl(buckets, listspace, dvals, tops);

    kl_total_time += seconds() - time;
}


static void free_kl(buckets, listspace, dvals, tops)
/* Free everything malloc'd for KL. */
struct bilist ****buckets;	/* space for bucket sorts */
struct bilist **listspace;	/* space for all bidirectional elements */
int     **dvals;		/* change in penalty for each possible move */
int     **tops;			/* starting dval for each type of move */
{
    int       sfree();

    sfree((char *) dvals);
    sfree((char *) tops);

    sfree((char *) listspace[0]);
    sfree((char *) buckets[0][1]);
    sfree((char *) listspace);
    sfree((char *) buckets);
}
