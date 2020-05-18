/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"
#include	"internal.h"


int       improve_internal(graph, nvtxs, assign, goal, int_list, set_list,
	vtx_elems, set1, locked, nlocked, using_ewgts, vwgt_max, total_vwgt)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
short    *assign;		/* current assignment */
double   *goal;			/* desired set sizes */
struct bidint *int_list;	/* sorted list of internal vtx values */
struct bidint *set_list;	/* headers of vtx_elems lists */
struct bidint *vtx_elems;	/* lists of vtxs in each set */
short     set1;			/* set to try to improve */
short    *locked;		/* indicates vertices not allowed to move */
int      *nlocked;		/* number of vertices that can't move */
int       using_ewgts;		/* are edge weights being used? */
int       vwgt_max;		/* largest vertex weight */
int      *total_vwgt;		/* total vertex weight in each set */
{
    struct bidint *move_list;	/* list of vertices changing sets */
    struct bidint *ptr, *ptr2;	/* loop through bidints */
    struct bidint *changed_sets;/* list of sets that were modified */
    double    vwgt_avg;		/* average vertex weight in current set */
    double    degree_avg;	/* average vertex degree in current set */
    double    frac = .4;	/* fraction of neighbors acceptable to move. */
    double    cost, min_cost;	/* cost of making a vertex internal */
    double    min_cost_start;	/* larger than any possible cost */
    double    cost_limit;	/* acceptable cost of internalization */
    double    ratio;		/* possible wgt / desired wgt */
    float     ewgt;		/* weight of an edge */
    short     set2, set3;	/* sets of two vertices */
    int       vtx, best_vtx;	/* vertex to make internal */
    int       move_vtx;		/* vertex to move between sets */
    int       neighbor;		/* neighbor of a vertex */
    int       nguys;		/* number of vertices in current set */
    int       internal;		/* is a vertex internal or not? */
    int       balanced;		/* are two sets balanced? */
    int       flag;		/* did I improve things: return code */
    int       size;		/* array spacing */
    int       i, j;		/* loop counters */

    /* First find best candidate vertex to internalize. */
    /* This is vertex which is already most nearly internal. */
    min_cost_start = 2.0 * vwgt_max * nvtxs;
    min_cost = min_cost_start;
    vwgt_avg = 0;
    degree_avg = 0;
    size = (int) (&(vtx_elems[1]) - &(vtx_elems[0]));
    nguys = 0;
    for (ptr = set_list[set1].next; ptr != NULL; ptr = ptr->next) {
	++nguys;
	vtx = ((int) (ptr - vtx_elems)) / size;
	vwgt_avg += graph[vtx]->vwgt;
	degree_avg += (graph[vtx]->nedges - 1);
	cost = 0;
	for (i = 1; i < graph[vtx]->nedges; i++) {
	    neighbor = graph[vtx]->edges[i];
	    set2 = assign[neighbor];
	    if (set2 != set1) {
		if (locked[neighbor])
		    cost = min_cost_start;
		else
		    cost += graph[neighbor]->vwgt;
	    }
	}
	if (cost == 0) {	/* Lock vertex and all it's neighbors. */
	    for (i = 1; i < graph[vtx]->nedges; i++) {
		neighbor = graph[vtx]->edges[i];
		if (!locked[neighbor]) {
		    locked[neighbor] = TRUE;
		    ++(*nlocked);
		}
	    }
	}

	if (cost < min_cost && cost != 0) {
	    min_cost = cost;
	    best_vtx = vtx;
	}
    }

    vwgt_avg /= nguys;
    degree_avg /= nguys;
    cost_limit = frac * vwgt_avg * degree_avg;

    if (min_cost > cost_limit) {
	return (FALSE);
    }

    /* Lock the candidate vertex in current set */
    if (!locked[best_vtx]) {
	locked[best_vtx] = TRUE;
	++(*nlocked);
    }

    /* Also lock all his neighbors in set. */
    for (i = 1; i < graph[best_vtx]->nedges; i++) {
	neighbor = graph[best_vtx]->edges[i];
	set2 = assign[neighbor];
	if (set1 == set2 && !locked[neighbor]) {
	    locked[neighbor] = TRUE;
	    ++(*nlocked);
	}
	vtx_elems[neighbor].val = set1;
    }

    ewgt = 1;
    move_list = NULL;

    /* Now move neighbors of best_vtx to set1. */
    for (i = 1; i < graph[best_vtx]->nedges; i++) {
	neighbor = graph[best_vtx]->edges[i];
	set2 = assign[neighbor];
	if (set2 != set1) {
	    /* Add vertex to list of guys to move to set1. */
	    /* Don't move it yet in case I get stuck later. */
	    /* But change his assignment so that swapping vertex has current info. */
	    /* Note: This will require me to undo changes if I fail. */

	    locked[neighbor] = TRUE;
	    ++(*nlocked);

	    /* Remove him from his set list. */
	    if (vtx_elems[neighbor].next != NULL) {
		vtx_elems[neighbor].next->prev = vtx_elems[neighbor].prev;
	    }
	    if (vtx_elems[neighbor].prev != NULL) {
		vtx_elems[neighbor].prev->next = vtx_elems[neighbor].next;
	    }

	    /* Put him in list of moved vertices */
	    vtx_elems[neighbor].next = move_list;
	    vtx_elems[neighbor].val = set2;
	    move_list = &(vtx_elems[neighbor]);
	    assign[neighbor] = set1;

	    total_vwgt[set2] -= graph[neighbor]->vwgt;
	    total_vwgt[set1] += graph[neighbor]->vwgt;
	}
    }

    /* Now check if vertices need to be handed back to restore balance. */
    flag = TRUE;
    for (i = 1; i < graph[best_vtx]->nedges && flag; i++) {
	neighbor = graph[best_vtx]->edges[i];
	set2 = vtx_elems[neighbor].val;
	if (set2 != set1) {
	    ratio = (total_vwgt[set1] + total_vwgt[set2]) /
		     (goal[set1] + goal[set2]);
	    balanced = (total_vwgt[set1] - goal[set1]*ratio +
	                goal[set2]*ratio - total_vwgt[set2]) <= vwgt_max;
	    while (!balanced && flag) {
		/* Find a vertex to move back to set2. Use a KL metric. */
		min_cost = min_cost_start;

		for (ptr = set_list[set1].next; ptr != NULL; ptr = ptr->next) {
		    vtx = ((int) (ptr - vtx_elems)) / size;
		    if (!locked[vtx]) {
			cost = 0;
			for (j = 1; j < graph[vtx]->nedges; j++) {
			    neighbor = graph[vtx]->edges[j];
			    if (using_ewgts)
				ewgt = graph[vtx]->ewgts[j];
			    set3 = assign[neighbor];
			    if (set3 == set1)
				cost += ewgt;
			    else if (set3 == set2)
				cost -= ewgt;
			}
			if (cost < min_cost) {
			    min_cost = cost;
			    move_vtx = vtx;
			}
		    }
		}
		if (min_cost >= min_cost_start)
		    flag = FALSE;
		else {
		    /* Add move_vtx to list of guys to move to set2. */
		    /* Don't move it yet in case I get stuck later. */
		    /* But change assign so later decisions have up-to-date info. */
		    if (vtx_elems[move_vtx].next != NULL) {
			vtx_elems[move_vtx].next->prev = vtx_elems[move_vtx].prev;
		    }
		    if (vtx_elems[move_vtx].prev != NULL) {
			vtx_elems[move_vtx].prev->next = vtx_elems[move_vtx].next;
		    }
		    vtx_elems[move_vtx].next = move_list;
		    vtx_elems[move_vtx].val = -(set2 + 1);
		    move_list = &(vtx_elems[move_vtx]);
		    assign[move_vtx] = set2;

		    total_vwgt[set2] += graph[move_vtx]->vwgt;
		    total_vwgt[set1] -= graph[move_vtx]->vwgt;
		}
	        balanced = total_vwgt[set1] - goal[set1] +
		           goal[set2] - total_vwgt[set2] <= vwgt_max;
	    }
	}
    }

    if (!flag) {
	/* Can't rebalance sets.  Give up, but first restore the data structures. */
	/* These include vtx_lists, total_vwgts and assign. */

	for (ptr = move_list; ptr != NULL;) {
	    ptr2 = ptr->next;
	    vtx = ((int) (ptr - vtx_elems)) / size;
	    if (ptr->val >= 0) {/* Almost moved from set2 to set1. */
		set2 = ptr->val;
		assign[vtx] = set2;
		total_vwgt[set2] += graph[vtx]->vwgt;
		total_vwgt[set1] -= graph[vtx]->vwgt;
		locked[vtx] = FALSE;
		--(*nlocked);
	    }
	    else {		/* Almost moved from set1 to set2. */
		set2 = -(ptr->val + 1);
		assign[vtx] = set1;
		total_vwgt[set2] -= graph[vtx]->vwgt;
		total_vwgt[set1] += graph[vtx]->vwgt;
		set2 = set1;
	    }

	    /* Now add vertex back into its old vtx_list (now indicated by set2) */
	    ptr->next = set_list[set2].next;
	    if (ptr->next != NULL)
		ptr->next->prev = ptr;
	    ptr->prev = &(set_list[set2]);
	    set_list[set2].next = ptr;

	    ptr = ptr2;
	}
	return (FALSE);
    }

    else {			/* Now perform actual moves. */
	/* First, update assignment and place vertices into their new sets. */
	changed_sets = NULL;
	for (ptr = move_list; ptr != NULL;) {
	    ptr2 = ptr->next;
	    vtx = ((int) (ptr - vtx_elems)) / size;
	    if (ptr->val >= 0)
		set2 = set1;
	    else
		set2 = -(ptr->val + 1);

	    ptr->next = set_list[set2].next;
	    if (ptr->next != NULL)
		ptr->next->prev = ptr;
	    ptr->prev = &(set_list[set2]);
	    set_list[set2].next = ptr;

	    /* Pull int_list[set2] out of its list to be used later. */
	    if (ptr->val >= 0)
		set2 = ptr->val;
	    if (int_list[set2].val >= 0) {
		int_list[set2].val = -(int_list[set2].val + 1);
		if (int_list[set2].next != NULL) {
		    int_list[set2].next->prev = int_list[set2].prev;
		}
		if (int_list[set2].prev != NULL) {
		    int_list[set2].prev->next = int_list[set2].next;
		}

		int_list[set2].next = changed_sets;
		changed_sets = &(int_list[set2]);
	    }
	    ptr = ptr2;
	}
	if (int_list[set1].val >= 0) {
	    if (int_list[set1].next != NULL) {
		int_list[set1].next->prev = int_list[set1].prev;
	    }
	    if (int_list[set1].prev != NULL) {
		int_list[set1].prev->next = int_list[set1].next;
	    }

	    int_list[set1].next = changed_sets;
	    changed_sets = &(int_list[set1]);
	}



	/* Finally, update internal node calculations for all modified sets. */
	while (changed_sets != NULL) {
	    set2 = ((int) (changed_sets - int_list)) / size;
	    changed_sets = changed_sets->next;

	    /* Next line uses fact that list has dummy header so prev isn't NULL. */
	    int_list[set2].next = int_list[set2].prev->next;
	    int_list[set2].val = 0;
	    /* Recompute internal nodes for this set */
	    for (ptr = set_list[set2].next; ptr != NULL; ptr = ptr->next) {
	        vtx = ((int) (ptr - vtx_elems)) / size;
		internal = TRUE;
		for (j = 1; j < graph[vtx]->nedges && internal; j++) {
		    set3 = assign[graph[vtx]->edges[j]];
		    internal = (set3 == set2);
		}
		if (internal) {
		    int_list[set2].val += graph[vtx]->vwgt;
		}
	    }

	    /* Now move internal value in doubly linked list. */
	    /* Move higher in list? */
	    while (int_list[set2].next != NULL &&
		    int_list[set2].val >= int_list[set2].next->val) {
		int_list[set2].prev = int_list[set2].next;
		int_list[set2].next = int_list[set2].next->next;
	    }
	    /* Move lower in list? */
	    while (int_list[set2].prev != NULL &&
		    int_list[set2].val < int_list[set2].prev->val) {
		int_list[set2].next = int_list[set2].prev;
		int_list[set2].prev = int_list[set2].prev->prev;
	    }

	    if (int_list[set2].next != NULL)
		int_list[set2].next->prev = &(int_list[set2]);
	    if (int_list[set2].prev != NULL)
		int_list[set2].prev->next = &(int_list[set2]);
	}
	return (TRUE);
    }
}
