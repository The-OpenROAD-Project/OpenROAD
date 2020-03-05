/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"params.h"
#include	"structs.h"
#include	"defs.h"


/* Enact a single move from largest (smallest) set in such a way
   that some small (large) set gets bigger. */

int       N_VTX_CHECKS;		/* number of considered moves */
int       N_VTX_MOVES;		/* number of actual moves */
static void nextmove(), undo_coupling(), couple();

void      movevtxs(graph, nvtxs, nsets, dist, indices, vals, startvtx, sets,
		             size, goal, vwgt_max)
struct vtx_data **graph;	/* data structure with vertex weights */
int       nvtxs;		/* number of vertices in graph */
int       nsets;		/* how many sets am I dividing into? */
double   *dist;			/* distances defining splitter */
int      *indices[][MAXSETS];	/* indices that define order in sorted lists */
double   *vals[][MAXSETS];	/* values in sorted lists */
int       startvtx[][MAXSETS];	/* index values corresponding to splitter */
short    *sets;			/* set assignment for each vertex */
double   *size;			/* sizes of the different sets */
double   *goal;			/* desired set sizes */
int       vwgt_max;		/* largest vertex weight */
{
    double    largest;		/* largest overshoot from desired size */
    double    smallest;		/* largest undershoot from desired size */
    int       active[MAXSETS];	/* flags sets trying to change size */
    double    delta;		/* amount distances must change */
    int       vtx;		/* vertex being moved */
    short     to, from;		/* set vertex is being moved to/from */
    int       weight;		/* weight of vertex being moved */
    int       done;		/* have I successfully move a vertex? */

    /* int npass=0; *//* counts passes through main loop */
    int       badset;		/* most unbalanced set */
    int       toobig;		/* badset too large or too small? */
    int       balanced;		/* is balance attained? */
    double    imbalance;	/* amount of imbalance in badset */
    int       i;		/* loop counter */

    /* Find most unbalanced set. */

    imbalance = largest = smallest = 0;
    for (i = 0; i < nsets; i++) {
	if (size[i] - goal[i] > largest) {
	    largest = size[i] - goal[i];
	    if (largest > imbalance) {
		imbalance = largest;
		badset = i;
		toobig = 1;
	    }
	}
	else if (goal[i] - size[i] > smallest) {
	    smallest = goal[i] - size[i];
	    if (smallest > imbalance) {
		imbalance = smallest;
		badset = i;
		toobig = -1;
	    }
	}
    }
    if (largest + smallest <= vwgt_max)
	balanced = TRUE;
    else
	balanced = FALSE;


    /* If not balanced, change distances to move vertices between sets. */
    while (!balanced) {
	/* npass++; */
	for (i = 0; i < nsets; i++)
	    active[i] = FALSE;
	active[badset] = TRUE;

	done = FALSE;
	while (!done) {
	    nextmove(nvtxs, nsets, vals, indices, startvtx, dist, sets,
		     toobig, active, &vtx, &to, &delta);
	    from = sets[vtx];
	    weight = graph[vtx]->vwgt;

	    /* Now adjust all active dists to reflect this move so far. */
	    for (i = 0; i < nsets; i++)
		if (active[i])
		    dist[i] -= toobig * delta;

	    if (toobig > 0) {
		if (size[to] + weight - goal[to] < largest) {
		    done = TRUE;
		    size[from] -= graph[vtx]->vwgt;
		    size[to] += graph[vtx]->vwgt;
		    sets[vtx] = to;
		    undo_coupling(graph, sets, nsets, from, to, toobig, badset,
				  size);
		}
		else {
		    couple(nsets, from, to, vtx);
		    active[to] = TRUE;
		}
	    }
	    else {
		if (goal[from] - (size[from] - weight) < smallest) {
		    done = TRUE;
		    size[from] -= graph[vtx]->vwgt;
		    size[to] += graph[vtx]->vwgt;
		    sets[vtx] = to;
		    undo_coupling(graph, sets, nsets, from, to, toobig, badset,
				  size);
		}
		else {
		    couple(nsets, from, to, vtx);
		    active[from] = TRUE;
		}
	    }
	}

	/* Find most unbalanced set. */
	imbalance = largest = smallest = 0;
	for (i = 0; i < nsets; i++) {
	    if (size[i] - goal[i] > largest) {
		largest = size[i] - goal[i];
		if (largest > imbalance) {
		    imbalance = largest;
		    badset = i;
		    toobig = 1;
		}
	    }
	    else if (goal[i] - size[i] > smallest) {
		smallest = goal[i] - size[i];
		if (smallest > imbalance) {
		    imbalance = smallest;
		    badset = i;
		    toobig = -1;
		}
	    }
	}
	if (largest + smallest <= vwgt_max)
	    balanced = TRUE;
	else
	    balanced = FALSE;
    }
}




/* Find the next move from an active to an inactive set, returning */
/* next_vtx, next_to, and distance. */
static void nextmove(nvtxs, nsets, vals, indices, startvtx, dist, sets,
		               toobig, active, next_vtx, next_to, next_delta)
int       nvtxs;		/* number of vertices in graph */
int       nsets;		/* how many sets am I dividing into? */
double   *dist;			/* distances defining splitter */
int      *indices[][MAXSETS];	/* indices that define order in sorted lists */
double   *vals[][MAXSETS];	/* values in sorted lists */
int       startvtx[][MAXSETS];	/* index values corresponding to splitter */
short    *sets;			/* set assignment for each vertex */
int       toobig;		/* is bad set too big or too small? */
int      *active;		/* flags sets trying to change size */
int      *next_vtx;		/* vertex selected to move next */
short    *next_to;		/* set vertex should be moved to */
double   *next_delta;		/* size of change in distances */
{
    double    delta;		/* amount distance must change */
    double    bestdelta;	/* best value see so far */
    int       good;		/* is this the delta OK? */
    int       first;		/* is this the first OK delta I've seen? */
    int       maxset, minset;	/* larger/smaller of two sets */
    short     bestfrom, bestto;	/* sets best move comes from and goes to */
    int       bestdir;		/* direction to step in list for best move */
    int       bestvtx;		/* vertex being moved between sets */
    short     from, to;		/* sets vertex wants to move from and to */
    int       index;		/* offset into indices array */
    int       dir;		/* direction to step through list */
    int       i, j;		/* loop counter */

    bestdelta = 0;
    first = TRUE;
    while (first) {
	for (i = 0; i < nsets; i++)
	    if (active[i]) {
		for (j = 0; j < nsets; j++)
		    if (!active[j]) {
			/* Look for next move from active set i to inactive set j. */
			if (toobig > 0) {
			    from = (short) i;
			    to = (short) j;
			}
			else {
			    from = (short) j;
			    to = (short) i;
			}
			minset = min(to, from);
			maxset = max(to, from);

			index = startvtx[minset][maxset];
			if (index >= nvtxs || index < 0)
			    good = FALSE;

			else {
			    if (j > i) {
				dir = -1;
			    }
			    else {
				dir = 1;
			    }
			    good = TRUE;
			    delta = -toobig * dir *
			       vals[minset][maxset][indices[minset][maxset][index]] -
			       (dist[to] - dist[from]);
			}

			if (good && (first || delta < bestdelta)) {
			    /* Is this the best so far? */
			    first = FALSE;
			    bestdelta = delta;
			    bestfrom = from;
			    bestto = to;
			    bestdir = dir;
			    bestvtx = indices[minset][maxset][index] + 1;
			}
		    }
	    }

	/* Only accept a vertex if it's from the right set. */
	if (sets[bestvtx] != bestfrom || (toobig > 0 && !active[bestfrom]) ||
		(toobig < 0 && !active[bestto])) {
	    /* Set rejection flag, and increment startvtx pointer. */
	    first = TRUE;
	    minset = min(bestto, bestfrom);
	    maxset = max(bestto, bestfrom);
	    startvtx[minset][maxset] -= toobig * bestdir;
	}
	++N_VTX_CHECKS;
    }
    *next_vtx = bestvtx;
    *next_to = bestto;
    *next_delta = bestdelta;
    ++N_VTX_MOVES;
}



static int ncoupled = 0;
static int coupled_vtxs[MAXSETS];
static short coupled_sets[MAXSETS][MAXSETS];

static void couple(nsets, from, to, vtx)
int       nsets;		/* number of sets being divided into */
short     from, to;		/* sets to be coupled */
int       vtx;			/* vertex that they share */
{
    int       i;		/* loop counter */

    /* Check for degenerate case of vertex shared by more than two sets. */
    for (i = 0; i < ncoupled; i++) {
	if (coupled_vtxs[i] == vtx) {
	    coupled_sets[i][from] = -1;
	    coupled_sets[i][to] = 1;
	    return;
	}
    }

    coupled_vtxs[ncoupled] = vtx;
    for (i = 0; i < nsets; i++)
	coupled_sets[ncoupled][i] = 0;
    coupled_sets[ncoupled][from] = -1;
    coupled_sets[ncoupled][to] = 1;
    ++ncoupled;
}


static void undo_coupling(graph, sets, nsets, from, to, toobig, badset, size)
struct vtx_data **graph;	/* data structure with vertex weights */
short    *sets;			/* sets each vertex is in */
int       nsets;		/* number of sets being divided into */
short     from, to;		/* set final vertex moved from and to */
int       toobig;		/* are we shrinking or enlarging a set? */
short     badset;		/* the set number being shrunk or enlarged */
double   *size;			/* sizes of the different sets */
{
    int       done;		/* have enough vertices been moved? */
    int       found;		/* have I found the right set? */
    int       vtx;		/* vertex being moved between sets */
    int       i, j;		/* loop counter */


    if (ncoupled == 0)
	return;

    if (toobig > 0) {
	done = FALSE;
	if (from == badset)
	    done = TRUE;
	while (!done) {
	    found = FALSE;
	    to = from;
	    for (i = 0; i < ncoupled && !found; i++) {
		if (coupled_sets[i][from] == 1) {	/* Found an edge into set. */
		    found = TRUE;
		    /* And find other end of edge. */
		    for (j = 0; j < nsets; j++) {
			if (coupled_sets[i][j] == -1)
			    from = (short) j;
		    }

		    /* Switch the set for this vertex. */
		    vtx = coupled_vtxs[i];
		    size[from] -= graph[vtx]->vwgt;
		    size[to] += graph[vtx]->vwgt;
		    sets[vtx] = to;
		    coupled_sets[i][from] = 0;

		    if (from == badset)
			done = TRUE;
		}
	    }
	}
    }
    else {
	done = FALSE;
	if (to == badset)
	    done = TRUE;
	while (!done) {
	    found = FALSE;
	    from = to;
	    for (i = 0; i < ncoupled && !found; i++) {
		if (coupled_sets[i][from] == -1) {	/* Found an edge from set. */
		    found = TRUE;
		    /* And find other end of edge. */
		    for (j = 0; j < nsets; j++) {
			if (coupled_sets[i][j] == 1)
			    to = (short) j;
		    }

		    /* Switch the set for this vertex. */
		    vtx = coupled_vtxs[i];
		    size[from] -= graph[vtx]->vwgt;
		    size[to] += graph[vtx]->vwgt;
		    sets[vtx] = to;
		    coupled_sets[i][to] = 0;

		    if (to == badset)
			done = TRUE;
		}
	    }
	}
    }

    ncoupled = 0;
}
