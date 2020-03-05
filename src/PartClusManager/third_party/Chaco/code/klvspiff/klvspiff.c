/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"params.h"
#include	"defs.h"

/* Improve (weighted) vertex separator.  Two sets are 0/1; separator = 2. */

int       VERTEX_SEPARATOR = FALSE;

static void free_klv();

void      klvspiff(graph, nvtxs, sets, goal, max_dev,
	           bndy_list, weights)
struct vtx_data **graph;	/* list of graph info for each vertex */
int       nvtxs;		/* number of vertices in graph */
short    *sets;			/* local partitioning of vtxs */
double   *goal;			/* desired set sizes */
int       max_dev;		/* largest deviation from balance allowed */
int     **bndy_list;		/* list of vertices on boundary (0 ends) */
double   *weights;		/* vertex weights in each set */
{
    extern FILE *Output_File;	/* output file or null */
    extern int DEBUG_TRACE;	/* debug flag for Kernighan-Lin */
    extern int DEBUG_KL;	/* debug flag for Kernighan-Lin */
    extern double kl_total_time;
    extern double kl_init_time;
    extern double nway_kl_time;
    struct bilist **lbuckets;	/* space for bucket sorts for left moves */
    struct bilist **rbuckets;	/* space for bucket sorts for right moves */
    struct bilist *llistspace;	/* space for all left bidirectional elements */
    struct bilist *rlistspace;	/* space for all right bidirectional elements */
    int      *ldvals;		/* change in penalty for each possible move */
    int      *rdvals;		/* change in penalty for each possible move */
    int      *edges;		/* loops through neighbor lists */
    double    time, time1;	/* timing variables */
    int       dval;		/* largest transition cost for a vertex */
    int       maxdval;		/* largest transition cost for all vertices */
    int       error;		/* out of space? */
    int       i, j;		/* loop counters */
    double    seconds();
    int       klv_init(), nway_klv();
    void      countup_vtx_sep();

    time = seconds();

    if (DEBUG_TRACE > 0) {
	printf("<Entering klvspiff, nvtxs = %d>\n", nvtxs);
    }

    /* Find largest possible change. */
    maxdval = 0;
    for (i = 1; i <= nvtxs; i++) {
	if (graph[i]->vwgt > maxdval) maxdval = graph[i]->vwgt;
	dval = -graph[i]->vwgt;
	edges = graph[i]->edges;
	for (j = graph[i]->nedges - 1; j; j--) {
	    dval += graph[*(++edges)]->vwgt;
	}
	if (dval > maxdval) maxdval = dval;
    }

    /* Allocate a bunch of space for KLV. */
    time1 = seconds();
    error = klv_init(&lbuckets, &rbuckets, &llistspace, &rlistspace,
		     &ldvals, &rdvals, nvtxs, maxdval);
    kl_init_time += seconds() - time1;

    if (!error) {
        if (DEBUG_KL > 0) {
	    printf(" Before KLV: ");
	    countup_vtx_sep(graph, nvtxs, sets);
        }

        time1 = seconds();
        error = nway_klv(graph, nvtxs, lbuckets, rbuckets, llistspace,
		rlistspace, ldvals, rdvals, sets,
	        maxdval, goal, max_dev, bndy_list, weights);
        nway_kl_time += seconds() - time1;

        if (DEBUG_KL > 1) {
	    printf(" After KLV: ");
	    countup_vtx_sep(graph, nvtxs, sets);
	}
    }

    if (error) {
	printf("\nWARNING: No space to perform KLV on graph with %d vertices.\n",
		nvtxs);
	printf("         NO LOCAL REFINEMENT PERFORMED.\n\n");

	if (Output_File != NULL) {
	    fprintf(Output_File,
	 	"\nWARNING: No space to perform KLV on graph with %d vertices.\n",
		nvtxs);
	    fprintf(Output_File, "         LOCAL REFINEMENT NOT PERFORMED.\n\n");
	}
    }

    free_klv(lbuckets, rbuckets, llistspace, rlistspace, ldvals, rdvals);

    kl_total_time += seconds() - time;
}


static void free_klv(lbuckets, rbuckets, llistspace, rlistspace, ldvals, rdvals)
/* Free everything malloc'd for KLV. */
struct bilist **lbuckets;	/* space for bucket sorts */
struct bilist **rbuckets;	/* space for bucket sorts */
struct bilist *llistspace;	/* space for all bidirectional elements */
struct bilist *rlistspace;	/* space for all bidirectional elements */
int      *ldvals;		/* change in penalty for each possible move */
int      *rdvals;		/* change in penalty for each possible move */
{
    int       sfree();

    sfree((char *) rlistspace);
    sfree((char *) llistspace);
    sfree((char *) rdvals);
    sfree((char *) ldvals);
    sfree((char *) rbuckets);
    sfree((char *) lbuckets);
}
