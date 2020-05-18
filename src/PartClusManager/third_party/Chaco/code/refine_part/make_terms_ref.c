/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <math.h>
#include "structs.h"

/* Compute the terminal constraints for next partition. */
void      make_terms_ref(graph, using_ewgts, subnvtxs, loc2glob, set0, set1,
    assignment, architecture, mesh_dims, term_wgts)
struct vtx_data **graph;	/* data structure for graph */
int       using_ewgts;		/* are edge weights being used? */
int       subnvtxs;		/* number of vtxs in subgraph */
int      *loc2glob;		/* mapping from subgraph to graph */
int       set0, set1;		/* two processors I'm choosing between */
short    *assignment;		/* set for each vertex */
int       architecture;		/* 0 => hypercube, 1 => mesh */
int       mesh_dims[3];		/* if mesh, size of mesh */
float    *term_wgts[];		/* terminal weights for each vertex */
{
    double    term_wgt;		/* terminal weight */
    float     edge_wgt;		/* weight of an edge */
    int       dist0, dist1;	/* distance from set to set0 and set1 */
    int       set;		/* set neighbor vtx belongs to */
    int       vtx;		/* vertex number */
    int       neighbor;		/* neighboring vertex number */
    int       x;		/* bitwise difference between sets */
    int       i, j;		/* loop counters */
    int       abs();

    /* NOTE: CURRENTLY ONLY WORKING FOR BISECTION. */

    edge_wgt = 1;
    for (i = 1; i <= subnvtxs; i++) {
	term_wgt = 0;
	vtx = loc2glob[i];

	for (j = 1; j < graph[vtx]->nedges; j++) {
	    neighbor = graph[vtx]->edges[j];
	    set = assignment[neighbor];
	    if (set != set0 && set != set1) {
		if (architecture == 0) {
		    dist0 = 0;
		    x = set ^ set0;
		    while (x) {
			if (x & 1) ++dist0;
			x >>= 1;
		    }
		    dist1 = 0;
		    x = set ^ set1;
		    while (x) {
			if (x & 1) ++dist1;
			x >>= 1;
		    }
		}

		else if (architecture > 0) {
		    dist0 =  abs((set % mesh_dims[0]) - (set0 % mesh_dims[0]));
		    dist0 += abs(((set  / mesh_dims[0]) % mesh_dims[1]) -
		                 ((set0 / mesh_dims[0]) % mesh_dims[1]));
		    dist0 += abs((set  / (mesh_dims[0] * mesh_dims[1])) -
		                 (set0 / (mesh_dims[0] * mesh_dims[1])));

		    dist1 =  abs((set % mesh_dims[0]) - (set1 % mesh_dims[0]));
		    dist1 += abs(((set  / mesh_dims[0]) % mesh_dims[1]) -
		                 ((set1 / mesh_dims[0]) % mesh_dims[1]));
		    dist1 += abs((set  / (mesh_dims[0] * mesh_dims[1])) -
		                 (set1 / (mesh_dims[0] * mesh_dims[1])));
		}

		if (using_ewgts) {
		    edge_wgt = graph[vtx]->ewgts[j];
		}
	        term_wgt += edge_wgt * (dist0 - dist1);
	    }
	}
	(term_wgts[1])[i] = term_wgt;
    }
}
