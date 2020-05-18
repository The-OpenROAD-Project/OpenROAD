/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"
#include	"internal.h"


void      check_internal(graph, nvtxs, int_list, set_list, vtx_elems, 
			           total_vwgt, assign, nsets_tot)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
struct bidint *int_list;	/* sorted list of internal weights per set */
struct bidint *set_list;	/* head of vtx_elems lists */
struct bidint *vtx_elems;	/* start of vertex data */
int      *total_vwgt;		/* total weight in each set */
short    *assign;		/* current assignment */
int       nsets_tot;		/* total number of sets */
{
    struct bidint *ptr, *ptr2;	/* elements in int_list */
    struct bidint *old_ptr, *old_ptr2;	/* elements in set_list */
    int       vwgt_sum;		/* sum of vertex weights */
    short     set, set2;	/* sets two vertices are in */
    int       sum;		/* sum of internal weights */
    int       nseen;		/* number of vertices found in set_lists */
    int       old_val, val;	/* consecutive values in int_list */
    int       vtx;		/* vertex in set list */
    int       internal;		/* is a vertex internal or not? */
    int       size;		/* array spacing */
    int       j, k;		/* loop counters */

    k = 0;
    size = (int) (&(int_list[1]) - &(int_list[0]));
    nseen = 0;
    old_val = -1;
    old_ptr = &(int_list[nsets_tot]);
    for (ptr = int_list[nsets_tot].next; ptr != NULL; ptr = ptr->next) {
	set = ((int) (ptr - int_list)) / size;
	val = ptr->val;
	if (val < old_val) {
	    printf("int_list out of order, k=%d, set = %d, old_val=%d, val = %d\n",
		   k, set, old_val, val);
	}
	if (ptr->prev != old_ptr) {
	    printf(" int_list back link screwed up, set=%d, k=%d, old_ptr=%ld, ptr->prev = %ld\n",
		   set, k, (long) old_ptr, (long) ptr->prev);
	}
	old_ptr = ptr;
	old_val = val;

	vwgt_sum = 0;
	sum = 0;
	old_ptr2 = &(set_list[set]);
	for (ptr2 = set_list[set].next; ptr2 != NULL; ptr2 = ptr2->next) {
	    vtx = ((int) (ptr2 - vtx_elems)) / size;
	    vwgt_sum += graph[vtx]->vwgt;
	    if (ptr2->prev != old_ptr2) {
		printf(" set_list back link screwed up, set=%d, k=%d, old_ptr2=%ld, ptr2->prev = %ld\n",
		       set, k, (long) old_ptr2, (long) ptr2->prev);
	    }
	    old_ptr2 = ptr2;

	    ++nseen;
	    if (assign[vtx] != set) {
		printf("assign[%d] = %d, but in set_list[%d]\n",
		       vtx, assign[vtx], set);
	    }
	    internal = TRUE;
	    for (j = 1; j < graph[vtx]->nedges && internal; j++) {
		set2 = assign[graph[vtx]->edges[j]];
		internal = (set2 == set);
	    }
	    if (internal) {
		sum += graph[vtx]->vwgt;
	    }
	}
	if (sum != val) {
	    printf("set = %d, val = %d, but I compute internal = %d\n",
		   set, val, sum);
	}
	if (vwgt_sum != total_vwgt[set]) {
	    printf(" vwgt_sum = %d, but total_vwgt[%d] = %d\n",
		   vwgt_sum, set, total_vwgt[set]);
	}
	k++;
    }
    if (k != nsets_tot) {
	printf(" Only %d sets in int_sets list, but nsets_tot = %d\n", k, nsets_tot);
    }
    if (nseen != nvtxs) {
	printf(" Only %d vertices found in int_sets lists, but nvtxs = %d\n", nseen, nvtxs);
    }
}
