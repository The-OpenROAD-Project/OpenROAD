/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"params.h"
#include	"structs.h"
#include	"defs.h"


void      count(graph, nvtxs, sets, nsets, hops, dump, using_ewgts)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vtxs in graph */
short    *sets;			/* processor each vertex is assigned to */
int       nsets;		/* number of sets partitioned into */
short     (*hops)[MAXSETS];	/* hops metric between sets */
int       dump;			/* flag for extended output */
int       using_ewgts;		/* are edge weights being used? */
{
    int      *nguys;		/* number of vtxs in each set */
    int       ncross;		/* number of outgoing edges */
    int       nhops;		/* number of hops */
    int       neighbor;		/* neighbor of a vertex */
    int       nmax, nmin;	/* largest and smallest set sizes */
    int       i, j;		/* loop counters */
    double   *smalloc();
    int       sfree();

    nguys = (int *) smalloc((unsigned) nsets * sizeof(int));

    for (i = 0; i < nsets; i++)
	nguys[i] = 0;

    ncross = nhops = 0;
    for (i = 1; i <= nvtxs; i++) {
	nguys[sets[i]] += graph[i]->vwgt;

	for (j = 1; j < graph[i]->nedges; j++) {
	    neighbor = graph[i]->edges[j];
	    if (sets[neighbor] != sets[i]) {
		if (using_ewgts) {
		    ncross += graph[i]->ewgts[j];
		    nhops += graph[i]->ewgts[j] * hops[sets[i]][sets[neighbor]];
		}
		else {
		    ncross++;
		    nhops += hops[sets[i]][sets[neighbor]];
		}
	    }
	}
    }

    ncross /= 2;
    nhops /= 2;

    nmax = nguys[0];
    nmin = nguys[0];
    for (i = 1; i < nsets; i++) {
	if (nguys[i] > nmax)
	    nmax = nguys[i];
	if (nguys[i] < nmin)
	    nmin = nguys[i];
    }
    printf("In subgraph: Cuts=%d, Hops=%d; Max=%d, Min=%d (nvtxs=%d).\n",
	   ncross, nhops, nmax, nmin, nvtxs);

    if (dump) {
	for (i = 0; i < nsets; i++)
	    printf(" Size of %d = %d\n", i, nguys[i]);

	for (i = 0; i < nvtxs; i++)
	    printf("%d\n", sets[i]);
	printf("\n\n");
    }

    sfree((char *) nguys);
}
