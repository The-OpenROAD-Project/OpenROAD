/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */


#include <stdio.h>
#include "defs.h"

/* STATUS:
  film.graph random+KL.  before size = 24, should stay 24.
  However, I'm not finding this much flow.


Start w/ a simple greedy weighted matching.
For each node on left side w/ some remaining unmatched weight:
  Look for augmenting path via recursive call.
  If my neighbor has uncommitted weight, take it.
  Otherwise, if some committed elsewhere, see who it is committed to.
    If this second order neighbor can get flow elsewhere, patch
    up augmenting path.

When no further improvement possible:
  For all unsatisfied nodes on left, include right nodes
    in their search tree, and exclude left nodes.
  Include all other left nodes.

This is similar to matching except:
  Each flow edge has a value associated with it.
  Vertex can have several flow neighbors.

  These differences require a different data structure.
  I need to modify the flow associated w/ an edge and see it
    immediately from either vertex.  Use a single representation
    of flow on an edge, and have each edge representation point to it.


*/



/*
The following code takes a weighted bipartite graph as input (with
n_left+n_right nodes, where node 'i' is adjacent to nodes
'indices[pointers[i]]' through 'indices[pointers[i+1]-1]', all
0-based) and returns a minimum weight edge cover (in sep_nodes[]).

*/

static void bpflow(), reachability(), touch();
static int augment(), touch2();


void      wbpcover(n_left, n_right, pointers, indices, vweight,
		             psep_size, psep_weight, psep_nodes)
int       n_left;		/* number of vertices on left side */
int       n_right;		/* number of vertices on right side */
int      *pointers;		/* start/stop of adjacency lists */
int      *indices;		/* adjacency list for each vertex */
int      *vweight;		/* vertex weights */
int      *psep_size;		/* returned size of separator */
int      *psep_weight;		/* returned weight of separator */
int     **psep_nodes;		/* list of separator nodes */
{
    extern int DEBUG_COVER;	/* debug flag for this routine */
    int      *touched;		/* flags for each vertex */
    int      *resid;		/* remaining, unmatched vertex weight */
    int      *flow;		/* flow on each right->left edge */
    int      *sep_nodes;	/* list of separator nodes */
    int       sep_size;		/* returned size of separator */
    int       sep_weight;	/* returned weight of separator */
    int       nedges;		/* number of edges in bipartite graph */
    int       i, j;		/* loop counter */
    int       wleft, wright, wedges;
    double   *smalloc();
    int       sfree();
    void      confirm_cover();

if (DEBUG_COVER) {
    printf("-> Entering wbpcover, nleft = %d, nright = %d, 2*nedges = %d\n",
	n_left, n_right, pointers[n_left+n_right]-pointers[0]);

    wleft = wright = 0;
    wedges = 0;
    for (i = 0; i < n_left; i++) {
	wleft += vweight[i];
	for (j = pointers[i]; j < pointers[i + 1]; j++)  {
	    wedges += vweight[i] * vweight[indices[j]];
	}
    }
    for (i = n_left; i < n_left + n_right; i++) {
	wright += vweight[i];
	for (j = pointers[i]; j < pointers[i + 1]; j++)  {
	    wedges += vweight[i] * vweight[indices[j]];
	}
    }
    printf("    Corresponds to unweighted, nleft = %d, nright = %d, 2*nedges = %d\n",
	wleft, wright, wedges);
}

    nedges = pointers[n_left + n_right] - pointers[0];

    resid = (int *) smalloc((unsigned) (n_left + n_right) * sizeof(int));
    touched = (int *) smalloc((unsigned) (n_left + n_right) * sizeof(int));
    flow = (int *) smalloc((unsigned) (nedges + 1) * sizeof(int));


    /* Not a matching.  I can be connected to multiple nodes. */
    bpflow(n_left, n_right, pointers, indices, vweight, resid, flow, touched);

    reachability(n_left, n_right, pointers, indices, resid, flow, touched);


    /* Separator includes untouched nodes on left, touched on right. */
    /* Left separator nodes if unconnected to unmatched right node via */
    /* augmenting path, right separator nodes otherwise. */

    /* First count the separator size for malloc. */
    sep_size = 0;
    for (i = 0; i < n_left; i++) {
	if (!touched[i]) {
	    sep_size++;
	}
    }
    for (i = n_left; i < n_left + n_right; i++) {
	if (touched[i]) {
	    sep_size++;
	}
    }

    sep_nodes = (int *) smalloc((unsigned) (sep_size + 1) * sizeof(int));

    sep_size = sep_weight = 0;
    for (i = 0; i < n_left; i++) {
	if (!touched[i]) {
	    sep_nodes[sep_size++] = i;
	    sep_weight += vweight[i];
	}
    }
    for (i = n_left; i < n_left + n_right; i++) {
	if (touched[i]) {
	    sep_nodes[sep_size++] = i;
	    sep_weight += vweight[i];
	}
    }

    sep_nodes[sep_size] = 0;

    *psep_size = sep_size;
    *psep_weight = sep_weight;
    *psep_nodes = sep_nodes;

/* Check the answer. */
if (DEBUG_COVER) {
confirm_cover(n_left, n_right, pointers, indices, flow, vweight, resid,
		             sep_size, sep_nodes);
}

    sfree((char *) flow);
    sfree((char *) touched);
    sfree((char *) resid);
}


static void bpflow(n_left, n_right, pointers, indices, vweight, resid, flow, touched)
int       n_left;		/* number of vertices on left side */
int       n_right;		/* number of vertices on right side */
int      *pointers;		/* start/stop of adjacency lists */
int      *indices;		/* adjacency list for each vertex */
int      *vweight;		/* vertex weights */
int      *resid;		/* residual weight at each vertex */
int      *flow;			/* flow on right->left edges */
int      *touched;		/* flags for each vertex */
{
    int      *seen;		/* space for list of encountered vertices */
    int       neighbor;		/* neighboring vertex */
    int       flow1;		/* flow through a particular edge */
    int       modified;		/* was flow enlarged? */
    int       i, j;		/* loop counters */
    double   *smalloc();
    int       sfree();

    /* First mark all the vertices as unmatched & untouched. */
    for (i = 0; i < n_left + n_right; i++) {
	resid[i] = vweight[i];
	touched[i] = FALSE;
    }

    /* Note that I only keep flow in edges from right to left. */
    for (i = pointers[n_left]; i < pointers[n_left + n_right]; i++) {
	flow[i] = 0;
    }

    /* Now generate a fast, greedy flow to start. */
    for (i = n_left; i < n_left + n_right; i++) {
	for (j = pointers[i]; j < pointers[i + 1] && resid[i]; j++) {
	    neighbor = indices[j];
	    if (resid[neighbor]) {
		flow1 = min(resid[i], resid[neighbor]);
		resid[neighbor] -= flow1;
		resid[i] -= flow1;
		flow[j] = flow1;
	    }
	}
    }

    /* Now try to enlarge flow via augmenting paths. */

    seen = (int *) smalloc((unsigned) (n_left + n_right) * sizeof(int));

    /* Look for an augmenting path. */
    for (i = 0; i < n_left; i++) {
        modified = TRUE;
	while (resid[i] && modified) {
	    modified = augment(i, pointers, indices, resid, flow, touched, seen);
	}
    }

    sfree((char *) seen);
}


static int augment(node, pointers, indices, resid, flow, touched, seen)
int       node;			/* start node in augmenting path */
int      *pointers;		/* start/stop of adjacency lists */
int      *indices;		/* adjacency list for each vertex */
int      *resid;		/* residual weight at each vertex */
int      *flow;			/* flow on right->left edges */
int      *touched;		/* flags for each vertex */
int      *seen;			/* keeps list of vertices encountered */
{
    int       nseen;		/* number of vertices encountered */
    int       flow1;		/* flow redirected via augmenting path */
    int       i;		/* loop counter */

    /* Look for augmenting path in graph. */

    nseen = 0;
    flow1 = resid[node];
    touch(node, pointers, indices, resid, flow, touched, &flow1, seen, &nseen);

    if (flow1) {		/* Found an augmenting path! */
	/* Free all the vertices encountered in search. */
	/* Otherwise they can't be involved in augmentation, */
	/* so leave them touched. */
	for (i = 0; i < nseen; i++) {
	    touched[*seen++] = 0;
	}
	return (TRUE);
    }
    else {
	return (FALSE);
    }
}


/* Mark everybody in my alternating path tree, and return vertex at */
/* end of augmenting path if found. */
static void touch(node, pointers, indices, resid, flow, touched, flow1, seen, nseen)
int       node;
int      *pointers;		/* start/stop of adjacency lists */
int      *indices;		/* adjacency list for each vertex */
int      *resid;		/* residual weight at each vertex */
int      *flow;			/* flow on right->left edges */
int      *touched;		/* flags for each vertex */
int      *flow1;		/* max flow we are looking for */
int      *seen;			/* list of vertices encountered */
int      *nseen;		/* number of vertices encountered */
{
    int       flow2;		/* flow passed down recursion tree */
    int       neighbor;		/* neighbor of a vertex */
    int       i, j, k;		/* loop counters */

    touched[node] = TRUE;
    seen[(*nseen)++] = node;

    for (i = pointers[node]; i < pointers[node + 1]; i++) {
	neighbor = indices[i];
	if (!touched[neighbor]) {	/* Not yet considered. */
	    touched[neighbor] = TRUE;
	    seen[(*nseen)++] = neighbor;
	    if (resid[neighbor]) {	/* Has flow to spare. */
		flow2 = min(*flow1, resid[neighbor]);

		/* Adjust flow & resid values. */
		resid[neighbor] -= flow2;
		resid[node] -= flow2;
		/* Perhaps better to compute these upfront once? */
		for (k = pointers[neighbor];
			k < pointers[neighbor + 1] && indices[k] != node; k++);
		flow[k] += flow2;
		*flow1 = flow2;
		return;
	    }
	    else {			/* Has no flow to spare. */
		for (j = pointers[neighbor];
			j < pointers[neighbor + 1]; j++) {
		    if (flow[j] && !touched[indices[j]]) {
			flow2 = min(*flow1, flow[j]);
			touch(indices[j], pointers, indices, resid, flow,
			      touched, &flow2, seen, nseen);
			if (flow2) {	/* Found some flow to spare! */
			    /* Adjust flow & resid values. */
			    resid[indices[j]] += flow2;
			    resid[node] -= flow2;
			    flow[j] -= flow2;
			    for (k = pointers[neighbor];
			    k < pointers[neighbor + 1] && indices[k] != node; k++);
			    flow[k] += flow2;
			    *flow1 = flow2;
			    return;
			}
		    }
		}
	    }
	}
    }
    *flow1 = 0;
    return;
}


static void reachability(n_left, n_right, pointers, indices, resid, flow, touched)
int       n_left;		/* number of vertices on left side */
int       n_right;		/* number of vertices on right side */
int      *pointers;		/* start/stop of adjacency lists */
int      *indices;		/* adjacency list for each vertex */
int      *resid;		/* residual weight at each vertex */
int      *flow;			/* flow on right->left edges */
int      *touched;		/* flags for each vertex */
{
    int       i;		/* loop counter */

    /* Initialize all the vertices to be untouched */
    for (i = 0; i < n_left + n_right; i++)
	touched[i] = 0;

    for (i = 0; i < n_left; i++)
	if (!touched[i] && resid[i]) {
	    touch2(i, pointers, indices, flow, touched);
	}
}


/* Mark everybody in my alternating path tree, and return vertex at */
/* end of augmenting path if found. */
static int touch2(node, pointers, indices, flow, touched)
int       node;
int      *pointers;		/* start/stop of adjacency lists */
int      *indices;		/* adjacency list for each vertex */
int      *flow;			/* flow on right->left edges */
int      *touched;		/* flags for each vertex */
{
    int       neighbor;		/* neighbor of a vertex */
    int       result;		/* return value */
    int       i, j;		/* loop counters */

    touched[node] = TRUE;

    for (i = pointers[node]; i < pointers[node + 1]; i++) {
	neighbor = indices[i];
	if (!touched[neighbor]) {	/* Not yet considered. */
	    touched[neighbor] = TRUE;
	    for (j = pointers[neighbor]; j < pointers[neighbor + 1]; j++) {
		if (flow[j] && !touched[indices[j]]) {
		    result = touch2(indices[j], pointers, indices, flow, touched);
		    if (result) {
			return (1);
		    }
		}
	    }
	}
    }
    return (0);
}



void confirm_cover(n_left, n_right, pointers, indices, flow, vweight, resid,
		  sep_size, sep_nodes)
int       n_left;
int       n_right;
int      *pointers;
int      *indices;
int      *flow;	
int      *vweight;
int      *resid;		/* residual weight at each vertex */
int       sep_size;
int      *sep_nodes;
{
    int     *marked;
    int      sep_weight;
    int      i, j, neighbor;
    double  *smalloc();
    int      sfree(), count_flow();
    void     count_resid(), check_resid();

    marked = (int *) smalloc((unsigned) (n_left + n_right) * sizeof(int));

    for (i = 0; i < n_left + n_right; i++) {
	marked[i] = FALSE;
    }

    sep_weight = 0;
    for (i = 0; i < sep_size; i++) {
	marked[sep_nodes[i]] = TRUE;
	sep_weight += vweight[sep_nodes[i]];
    }

    for (i = 0; i < n_left; i++) {
	if (!marked[i]) {
            for (j = pointers[i]; j < pointers[i + 1]; j++) {
	        neighbor = indices[j];
		if (!marked[neighbor]) {
		    printf("Edge (%d, %d) not covered\n", i, neighbor);
		}
	    }
	}
    }

    i = count_flow(n_left, n_right, pointers, flow);
    if (i != sep_weight) {
        printf("ERROR: total_flow = %d, sep_weight = %d, sep_size = %d\n",
	i, sep_weight, sep_size);
    }
    else {
        printf("total_flow = %d, sep_weight = %d, sep_size = %d\n",
	i, sep_weight, sep_size);
    }

    /* Now check if any separator nodes have remaining flow. */
    count_resid(n_left, n_right, resid, vweight, marked);

    check_resid(n_left, n_right, vweight, resid, pointers, indices, flow);

    sfree((char *) marked);
}

int count_flow(n_left, n_right, pointers, flow)
int       n_left;
int       n_right;
int      *pointers;
int      *flow;	
{
    int i, total_flow;

    total_flow = 0;
    for (i = pointers[n_left]; i < pointers[n_left + n_right]; i++) {
	total_flow += flow[i];
    }
    return(total_flow);
}

void count_resid(n_left, n_right, resid, vweight, marked)
int       n_left;
int       n_right;
int      *resid;
int      *vweight;
int      *marked;
{
    int i, left_used, right_used;

    for (i = 0; i < n_left + n_right; i++) {
	if (resid[i] < 0 || resid[i] > vweight[i]) {
	    printf("BAD resid[%d] = %d, vweight = %d\n", i, resid[i], vweight[i]);
	}
    }

    left_used = right_used = 0;
    for (i = 0; i < n_left; i++) {
	left_used += vweight[i] - resid[i];
	if (marked[i] && resid[i]) {
	    printf("Vertex %d in separator, but resid = %d (vweight = %d)\n",
		    i, resid[i], vweight[i]);
	}
    }
    for (i = n_left; i < n_left + n_right; i++) {
	right_used += vweight[i] - resid[i];
	if (marked[i] && resid[i]) {
	    printf("Vertex %d in separator, but resid = %d (vweight = %d)\n",
		    i, resid[i], vweight[i]);
	}
    }
    if (left_used != right_used) {
	printf("left_used = %d, NOT EQUAL TO right_used = %d\n",
	    left_used, right_used);
    }
}

void check_resid(n_left, n_right, vweight, resid, pointers, indices, flow)
int       n_left;
int       n_right;
int      *vweight;		/* vertex weights */
int      *resid;
int      *pointers;
int      *indices;
int      *flow;
{
    int i, j, left_used, right_used;
    int *diff;
    double *smalloc();
    int sfree();

    for (i = 0; i < n_left + n_right; i++) {
	if (resid[i] < 0 || resid[i] > vweight[i]) {
	    printf("BAD resid[%d] = %d, vweight = %d\n", i, resid[i], vweight[i]);
	}
    }

    left_used = right_used = 0;
    for (i = 0; i < n_left; i++) {
	left_used += vweight[i] - resid[i];
    }
    for (i = n_left; i < n_left + n_right; i++) {
	right_used += vweight[i] - resid[i];
    }
    if (left_used != right_used) {
	printf("left_used = %d, NOT EQUAL TO right_resid = %d\n",
	    left_used, right_used);
    }

    diff = (int *) smalloc((unsigned) (n_left + n_right) * sizeof(int));
    for (i = 0; i < n_left + n_right; i++) {
	diff[i] = 0;
    }

    for (i = n_left; i < n_left + n_right; i++) {
        for (j = pointers[i]; j < pointers[i + 1]; j++) {
            if (flow[j] < 0) {
		printf("Negative flow (%d,%d) = %d\n", i, indices[j], flow[j]);
	    }
	    diff[i] += flow[j];
	    diff[indices[j]] += flow[j];
	}
    }
    for (i = 0; i < n_left + n_right; i++) {
	if (diff[i] != vweight[i] - resid[i]) {
	    printf("ERROR: diff[%d] = %d, but vweight = %d and resid = %d\n",
		    i, diff[i], vweight[i], resid[i]);
	}
    }

    sfree((char *) diff);
}
