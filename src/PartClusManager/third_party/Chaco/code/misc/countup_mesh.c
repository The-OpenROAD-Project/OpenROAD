/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

/* Print metrics of partition quality. */

void      countup_mesh(graph, nvtxs, assignment, mesh_dims, print_lev, outfile,
		                 using_ewgts)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vtxs in graph */
short    *assignment;		/* set number of each vtx (length nvtxs+1) */
int       mesh_dims[3];		/* extent of mesh in each dimension */
int       print_lev;		/* level of output */
FILE     *outfile;		/* output file if not NULL */
int       using_ewgts;		/* are edge weights being used? */
{
    double   *hopsize;		/* number of hops for each set */
    double   *cutsize;		/* number of cuts for each set */
    int      *setsize;		/* number or weight of vtxs in each set */
    int      *setseen;		/* flags for sets adjacent to a particular set */
    int      *inorder;		/* list of vtxs in each set */
    int      *startptr;		/* indices into inorder array */
    double    ncuts;		/* total number of edges connecting sets */
    double    nhops;		/* total cuts weighted by mesh hops */
    double    ewgt;		/* edge weight */
    int       nsets;		/* number of sets after a level */
    int       vtx;		/* vertex in graph */
    int       set, set2, set3;	/* sets neighboring vtxs are assigned to */
    int       onbdy;		/* counts number of neighboring set for a vtx */
    int       min_size, max_size;	/* min and max set sizes */
    int       tot_size;		/* total of all set sizes */
    double    bdyvtxs;		/* sum of onbdy values for a set */
    double    bdyvtx_hops;	/* weights boundary vertices by wires required */
    double    bdyvtx_hops_tot;	/* weights boundary vertices by wires required */
    double    bdyvtx_hops_max;	/* weights boundary vertices by wires required */
    double    bdyvtx_hops_min;	/* weights boundary vertices by wires required */
    int       neighbor_sets;	/* number of neighboring sets for a set */
    int       internal;		/* number of internal vertices in a set */
    int       min_internal;	/* smallest number of internal vertices */
    int       max_internal;	/* largest number of internal vertices */
    int       total_internal;	/* total number of internal nodes */
    double    maxcuts;		/* largest cuts among all processors */
    double    mincuts;		/* smallest cuts among all processors */
    double    maxhops;		/* largest hops among all processors */
    double    minhops;		/* smallest hops among all processors */
    double    maxbdy;		/* largest bdy_vtxs among all processors */
    double    minbdy;		/* smallest bdy_vtxs among all processors */
    int       maxneighbors;	/* largest neighbor_sets among all processors */
    int       minneighbors;	/* smallest neighbor_sets among all processors */
    double    total_bdyvtxs;	/* sum of all onbdy values in whole graph  */
    int       total_neighbors;	/* number of neighboring sets in graph */
    int       neighbor;		/* neighbor of a vertex */
    int       print2file;	/* should I print to a file? */
    int       x1, y1, z1;	/* mesh location of vertex */
    int       x2, y2, z2;	/* mesh location of neighboring vertex */
    int       i, j;		/* loop counters */
    double   *smalloc();
    int       abs(), sfree();

    print2file = (outfile != NULL);
    ewgt = 1;

    nsets = mesh_dims[0] * mesh_dims[1] * mesh_dims[2];
    cutsize = (double *) smalloc((unsigned) nsets * sizeof(double));
    hopsize = (double *) smalloc((unsigned) nsets * sizeof(double));
    setsize = (int *) smalloc((unsigned) nsets * sizeof(int));

    setseen = (int *) smalloc((unsigned) nsets * sizeof(int));
    startptr = (int *) smalloc((unsigned) (nsets + 1) * sizeof(int));
    inorder = (int *) smalloc((unsigned) nvtxs * sizeof(int));
    for (j = 0; j < nsets; j++)
	setsize[j] = 0;
    for (i = 1; i <= nvtxs; i++)
	++setsize[assignment[i]];

    /* Modify setsize to become index into vertex list. */
    for (j = 1; j < nsets; j++)
	setsize[j] += setsize[j - 1];
    for (j = nsets - 1; j > 0; j--)
	startptr[j] = setsize[j] = setsize[j - 1];
    startptr[0] = setsize[0] = 0;
    startptr[nsets] = nvtxs;
    for (i = 1; i <= nvtxs; i++) {
	set = assignment[i];
	inorder[setsize[set]] = i;
	setsize[set]++;
    }

    for (j = 0; j < nsets; j++) {
	cutsize[j] = 0;
	hopsize[j] = 0;
	setsize[j] = 0;
    }

    for (i = 1; i <= nvtxs; i++) {
	set = assignment[i];
	setsize[set] += graph[i]->vwgt;
	x1 = set % mesh_dims[0];
	y1 = (set / mesh_dims[0]) % mesh_dims[1];
	z1 = set / (mesh_dims[0] * mesh_dims[1]);
	for (j = 1; j < graph[i]->nedges; j++) {
	    neighbor = graph[i]->edges[j];
	    set2 = assignment[neighbor];
	    x2 = set2 % mesh_dims[0];
	    y2 = (set2 / mesh_dims[0]) % mesh_dims[1];
	    z2 = set2 / (mesh_dims[0] * mesh_dims[1]);
	    if (set != set2) {
		if (using_ewgts)
		    ewgt = graph[i]->ewgts[j];
		cutsize[set] += ewgt;
		hopsize[set] += ewgt * (abs(x1 - x2) + abs(y1 - y2) + abs(z1 - z2));
	    }
	}
    }

    max_size = 0;
    tot_size = 0;
    for (set = 0; set < nsets; set++) {
	tot_size += setsize[set];
	if (setsize[set] > max_size)
	    max_size = setsize[set];
    }
    min_size = max_size;
    for (set = 0; set < nsets; set++) {
	if (setsize[set] < min_size)
	    min_size = setsize[set];
    }

    ncuts = nhops = 0;
    total_bdyvtxs = total_neighbors = 0;
    bdyvtx_hops_tot = bdyvtx_hops_max = bdyvtx_hops_min = 0;
    maxcuts = mincuts = 0;
    maxhops = minhops = 0;
    maxbdy = minbdy = 0;
    maxneighbors = minneighbors = 0;
    total_internal = 0;
    min_internal = max_size;
    max_internal = 0;
    printf("\nAfter full partitioning  (nsets = %d)\n", nsets);
    if (print2file)
	fprintf(outfile, "\nAfter full partitioning (nsets = %d)\n", nsets);
    if (print_lev < 0) {
	printf("    set    size      cuts       hops   bndy_vtxs    adj_sets\n");
	if (print2file)
	    fprintf(outfile, "    set    size      cuts       hops   bndy_vtxs    adj_sets\n");
    }
    for (set = 0; set < nsets; set++) {
	/* Compute number of set neighbors, and number of vtxs on boundary. */
	internal = setsize[set];
	for (i = 0; i < nsets; i++)
	    setseen[i] = 0;
	x1 = set % mesh_dims[0];
	y1 = (set / mesh_dims[0]) % mesh_dims[1];
	z1 = set / (mesh_dims[0] * mesh_dims[1]);

	set2 = set;
	bdyvtxs = 0;
	bdyvtx_hops = 0;
	for (i = startptr[set2]; i < startptr[set2 + 1]; i++) {
	    onbdy = 0;
	    vtx = inorder[i];
	    for (j = 1; j < graph[vtx]->nedges; j++) {
		neighbor = graph[vtx]->edges[j];
		set3 = assignment[neighbor];
		if (set3 != set) {	/* Is vtx on boundary? */
		    /* Has this neighboring set been seen already? */
		    if (setseen[set3] >= 0) {
			x2 = set3 % mesh_dims[0];
			y2 = (set3 / mesh_dims[0]) % mesh_dims[1];
			z2 = set3 / (mesh_dims[0] * mesh_dims[1]);
			bdyvtx_hops += abs(x1 - x2) + abs(y1 - y2) + abs(z1 - z2);
			++onbdy;
			setseen[set3] = -setseen[set3] - 1;
		    }
		}
	    }
	    /* Now reset all the setseen values to be positive. */
	    if (onbdy != 0) {
		for (j = 1; j < graph[vtx]->nedges; j++) {
		    neighbor = graph[vtx]->edges[j];
		    set3 = assignment[neighbor];
		    if (setseen[set3] < 0)
			setseen[set3] = -setseen[set3];
		}
		internal -= graph[vtx]->vwgt;
	    }
	    bdyvtxs += onbdy;
	}
	total_internal += internal;
	bdyvtx_hops_tot += bdyvtx_hops;
	if (bdyvtx_hops > bdyvtx_hops_max)
	    bdyvtx_hops_max = bdyvtx_hops;
	if (set == 0 || bdyvtx_hops < bdyvtx_hops_min)
	    bdyvtx_hops_min = bdyvtx_hops;
	if (internal > max_internal)
	    max_internal = internal;
	if (set == 0 || internal < min_internal)
	    min_internal = internal;

	/* Now count up the number of neighboring sets. */
	neighbor_sets = 0;
	for (i = 0; i < nsets; i++) {
	    if (setseen[i] != 0)
		++neighbor_sets;
	}

	if (print_lev < 0) {
	    printf(" %5d    %5d    %6g     %6g   %6g      %6d\n",
		   set, setsize[set], cutsize[set], hopsize[set],
		   bdyvtxs, neighbor_sets);
	    if (print2file)
		fprintf(outfile, " %5d    %5d    %6g     %6g   %6g      %6d\n",
			set, setsize[set], cutsize[set], hopsize[set],
			bdyvtxs, neighbor_sets);
	}
	if (cutsize[set] > maxcuts)
	    maxcuts = cutsize[set];
	if (set == 0 || cutsize[set] < mincuts)
	    mincuts = cutsize[set];
	if (hopsize[set] > maxhops)
	    maxhops = hopsize[set];
	if (set == 0 || hopsize[set] < minhops)
	    minhops = hopsize[set];
	if (bdyvtxs > maxbdy)
	    maxbdy = bdyvtxs;
	if (set == 0 || bdyvtxs < minbdy)
	    minbdy = bdyvtxs;
	if (neighbor_sets > maxneighbors)
	    maxneighbors = neighbor_sets;
	if (set == 0 || neighbor_sets < minneighbors)
	    minneighbors = neighbor_sets;
	ncuts += cutsize[set];
	nhops += hopsize[set];
	total_bdyvtxs += bdyvtxs;
	total_neighbors += neighbor_sets;
    }
    ncuts /= 2;
    nhops /= 2;
    printf("\n");
    printf("                            Total      Max/Set      Min/Set\n");
    printf("                            -----      -------      -------\n");
    printf("Set Size:             %11d  %11d  %11d\n",
    	tot_size, max_size, min_size);
    printf("Edge Cuts:            %11g  %11g  %11g\n",
    	ncuts, maxcuts, mincuts);
    printf("Mesh Hops:            %11g  %11g  %11g\n",
    	nhops, maxhops, minhops);
    printf("Boundary Vertices:    %11g  %11g  %11g\n",
    	total_bdyvtxs, maxbdy, minbdy);
    printf("Boundary Vertex Hops: %11g  %11g  %11g\n",
    	bdyvtx_hops_tot, bdyvtx_hops_max, bdyvtx_hops_min);
    printf("Adjacent Sets:        %11d  %11d  %11d\n",
    	total_neighbors, maxneighbors, minneighbors);
    printf("Internal Vertices:    %11d  %11d  %11d\n\n",
    	total_internal, max_internal, min_internal);


    if (print2file) {

	fprintf(outfile, "\n");
	fprintf(outfile, "                            Total      Max/Set      Min/Set\n");
	fprintf(outfile, "                            -----      -------      -------\n");
	fprintf(outfile, "Set Size:             %11d  %11d  %11d\n",
		tot_size, max_size, min_size);
	fprintf(outfile, "Edge Cuts:            %11g  %11g  %11g\n",
		ncuts, maxcuts, mincuts);
	fprintf(outfile, "Hypercube Hops:       %11g  %11g  %11g\n",
		nhops, maxhops, minhops);
	fprintf(outfile, "Boundary Vertices:    %11g  %11g  %11g\n",
		total_bdyvtxs, maxbdy, minbdy);
	fprintf(outfile, "Boundary Vertex Hops: %11g  %11g  %11g\n",
		bdyvtx_hops_tot, bdyvtx_hops_max, bdyvtx_hops_min);
	fprintf(outfile, "Adjacent Sets:        %11d  %11d  %11d\n",
		total_neighbors, maxneighbors, minneighbors);
	fprintf(outfile, "Internal Vertices:    %11d  %11d  %11d\n\n",
		total_internal, max_internal, min_internal);
    }

    sfree((char *) cutsize);
    sfree((char *) hopsize);
    sfree((char *) setsize);
    sfree((char *) setseen);
    sfree((char *) startptr);
    sfree((char *) inorder);
}
