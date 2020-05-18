/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "structs.h"

/* Print out subgraph in program-readable form. */
void      graph_out(graph, nvtxs, using_ewgts, tag, file_name)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vtxs in graph */
int       using_ewgts;		/* Are edges weighted? */
char     *tag;			/* message to include */
char     *file_name;		/* output file name if not null */
{
    FILE     *file;		/* output file */
    int       using_vwgts;	/* Are vertices weighted? */
    int       nedges;		/* number of edges in graph */
    int       option;		/* output option */
    int       i, j;		/* loop counter */

    if (file_name != NULL)
	file = fopen(file_name, "w");
    else
	file = stdout;

    /* Determine all the appropriate parameters. */
    using_vwgts = FALSE;
    nedges = 0;
    for (i = 1; i <= nvtxs; i++) {
	if (graph[i]->vwgt != 1)
	    using_vwgts = TRUE;
	nedges += graph[i]->nedges - 1;
    }

    option = 0;
    if (using_ewgts)
	option += 1;
    if (using_vwgts)
	option += 10;

    if (tag != NULL)
	fprintf(file, "%% graph_out: %s\n", tag);
    fprintf(file, " %d %d", nvtxs, nedges / 2);
    if (option != 0)
	fprintf(file, "  %d", option);
    fprintf(file, "\n");
    for (i = 1; i <= nvtxs; i++) {
	if (using_vwgts)
	    fprintf(file, "%d ", graph[i]->vwgt);
	for (j = 1; j < graph[i]->nedges; j++) {
	    fprintf(file, " %d", graph[i]->edges[j]);
	    if (using_ewgts)
		fprintf(file, " %.9f ", graph[i]->ewgts[j]);
	}
	fprintf(file, "\n");
    }

    if (file_name != NULL)
	fclose(file);
}
