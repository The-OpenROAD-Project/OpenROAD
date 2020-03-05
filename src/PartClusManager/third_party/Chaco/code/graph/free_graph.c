/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"

/* Free a graph data structure. */

void      free_graph(graph)
struct vtx_data **graph;
{
    int       sfree();

    if (graph != NULL) { 
	if (graph[1] != NULL) {
            if (graph[1]->ewgts != NULL)
	        sfree((char *) graph[1]->ewgts);
	    if (graph[1]->edges != NULL) {
                sfree((char *) graph[1]->edges);
	    }
            sfree((char *) graph[1]);
	}
        sfree((char *) graph);
    }
}
