/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"defs.h"
#include	"params.h"
#include	"structs.h"


static void free2d();

void      map2d(graph, xvecs, nvtxs, sets, goal, vwgt_max)
struct vtx_data **graph;	/* data structure with vertex weights */
double  **xvecs;		/* vectors to partition */
int       nvtxs;		/* number of vertices */
short    *sets;			/* set each vertex gets assigned to */
double   *goal;			/* desired set sizes */
int       vwgt_max;		/* largest vertex weight */
{
    extern int DEBUG_BPMATCH;	/* turn on debuging for bipartite matching */
    extern int N_VTX_MOVES;	/* total number of vertex moves */
    extern int N_VTX_CHECKS;	/* total number of moves contemplated */
    double   *vals[4][MAXSETS];	/* values in sorted lists */
    double    dist[4];		/* trial separation point */
    double    size[4];		/* sizes of each set being modified */
    int      *indices[4][MAXSETS];	/* indices sorting lists */
    int       startvtx[4][MAXSETS];	/* indices defining separation */
    int       nsection = 2;	/* number of xvectors */
    int       nsets = 4;	/* number of sets being divided into */
    void      genvals2d(), sorts2d(), inits2d(), checkbp(), movevtxs();

    N_VTX_CHECKS = N_VTX_MOVES = 0;

    /* Generate all the lists of values that need to be sorted. */
    genvals2d(xvecs, vals, nvtxs);

    /* Sort the lists of values. */
    sorts2d(vals, indices, nvtxs);

    /* Now initialize dists and assign to sets. */
    inits2d(graph, xvecs, vals, indices, nvtxs, dist, startvtx, size, sets);

    /* Determine the largest and smallest allowed set sizes. */
    /* (For now, assume all sets must be same size, but can easily change.) */

    if (DEBUG_BPMATCH > 1) {
	printf(" Calling check before movevtxs\n");
	checkbp(graph, xvecs, sets, dist, nvtxs, nsection);
    }

    movevtxs(graph, nvtxs, nsets, dist, indices, vals, startvtx, sets, size,
	     goal, vwgt_max);

    if (DEBUG_BPMATCH > 0) {
	printf(" N_VTX_CHECKS = %d, N_VTX_MOVES = %d\n", N_VTX_CHECKS, N_VTX_MOVES);
	checkbp(graph, xvecs, sets, dist, nvtxs, nsection);
    }

    free2d(vals, indices);
}


/* Free the space used in the bpmatch routines. */
static void free2d(vals, indices)
double   *vals[4][MAXSETS];
int      *indices[4][MAXSETS];
{
    int       sfree();

    sfree((char *) vals[0][1]);
    sfree((char *) vals[0][2]);
    sfree((char *) vals[0][3]);
    sfree((char *) vals[1][2]);

    sfree((char *) indices[0][1]);
    sfree((char *) indices[0][2]);
    sfree((char *) indices[0][3]);
    sfree((char *) indices[1][2]);
}
