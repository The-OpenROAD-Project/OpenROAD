/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include	"structs.h"
#include	"params.h"
#include	"defs.h"

/* Idea:
   'buckets[i][j]' is a set of buckets to sort moves from i to j.
   listspace[i] is space for lists in buckets[i][j].
   Loop through all nonequal pairs [i][j], taking the first element
   in each list.  Compare them all to find the largest allowed move.
   Make that move, and save it in movelist.
*/

int KL_MAX_PASS = -1;		/* max KL passes; infinite if <= 0 */


int       nway_kl(graph, nvtxs, buckets, listspace, tops, dvals, sets,
	          maxdval, nsets, goal, term_wgts, hops, max_dev,
		  using_ewgts, bndy_list, startweight)
struct vtx_data **graph;	/* data structure for graph */
int       nvtxs;		/* number of vtxs in graph */
struct bilist ****buckets;	/* array of lists for bucket sort */
struct bilist **listspace;	/* list data structure for each vertex */
int     **tops;			/* 2-D array of top of each set of buckets */
int     **dvals;		/* d-values for each transition */
short    *sets;			/* processor each vertex is assigned to */
int       maxdval;		/* maximum d-value for a vertex */
int       nsets;		/* number of sets divided into */
double   *goal;			/* desired set sizes */
float    *term_wgts[];		/* weights for terminal propogation */
short     (*hops)[MAXSETS];	/* cost of set transitions */
int       max_dev;		/* largest allowed deviation from balance */
int       using_ewgts;		/* are edge weights being used? */
int     **bndy_list;		/* list of vertices on boundary (0 ends) */
double   *startweight;		/* sum of vweights in each set (in and out) */

/* Suaris and Kedem algorithm for quadrisection, generalized to an */
/* arbitrary number of sets, with intra-set cost function specified by hops. */
/* Note: this is for a single divide step. */
/* Also, sets contains an intial (possibly crummy) partitioning. */

{
    extern double kl_bucket_time;	/* time spent in KL bucketsort */
    extern int KL_BAD_MOVES;	/* # bad moves in a row to stop KL */
    extern int DEBUG_KL;	/* debug flag for KL */
    extern int KL_RANDOM;	/* use randomness in KL? */
    extern int KL_NTRIES_BAD;	/* number of unhelpful passes before quitting */
    extern int KL_UNDO_LIST;	/* should I back out of changes or start over? */
    extern int KL_MAX_PASS;	/* maximum number of outer KL loops */
    extern double CUT_TO_HOP_COST;	/* if term_prop; cut/hop importance */
    struct bilist *movelist;	/* list of vtxs to be moved */
    struct bilist **endlist;	/* end of movelists */
    struct bilist *bestptr;	/* best vertex in linked list */
    struct bilist *bptr;	/* loops through bucket list */
    float    *ewptr;		/* loops through edge weights */
    double   *locked;		/* weight of vertices locked in a set */
    double   *loose;		/* weight of vtxs that can move from a set */
    int      *bspace;		/* list of active vertices for bucketsort */
    double   *weightsum;	/* sum of vweights for each partition */
    int      *edges;		/* edge list for a vertex */
    int      *bdy_ptr;	 	/* loops through bndy_list */
    double    time;		/* timing parameter */
    double    delta;		/* desire of sets to change size */
    double    bestdelta;	/* strongest delta value */
    double    deltaplus;	/* largest negative deviation from goal size */
    double    deltaminus;	/* largest negative deviation from goal size */
    int       list_length;	/* how long is list of vertices to bucketsort? */
    int       balanced;		/* is partition balanced? */
    int       temp_balanced;	/* is intermediate partition balanced? */
    int       ever_balanced;	/* has any partition been balanced? */
    int       bestvtx;		/* best vertex to move */
    int       bestval;		/* best change in value for a vtx move */
    int       bestfrom, bestto;	/* sets best vertex moves between */
    int       vweight;		/* weight of best vertex */
    int       gtotal;		/* sum of changes from moving */
    int       improved;		/* total improvement from KL */
    double    balance_val;	/* how imbalanced is it */
    double    balance_best;	/* best balance yet if trying hard */
    double    bestg;		/* maximum gtotal found in KL loop */
    double    bestg_min;	/* smaller than any possible bestg */
    int       beststep;		/* step where maximum value occurred */
    int       neighbor;		/* neighbor of a vertex */
    int       step_cutoff;	/* number of negative steps in a row allowed */
    int       cost_cutoff;	/* amount of negative d-values allowed */
    int       neg_steps;	/* number of negative steps in a row */
    int       neg_cost;		/* decrease in sum of d-values */
    int       vtx;		/* vertex number */
    int       dval;		/* dval of a vertex */
    int       group;		/* set that a vertex is assigned to */
    double    cut_cost;		/* if term_prop; relative cut/hop importance */
    int       diff;		/* change in a d-value */
    int       stuck1st, stuck2nd;	/* how soon will moves be disallowed? */
    int       beststuck1, beststuck2;	/* best stuck values for tie-breaking */
    int       eweight;		/* a particular edge weight */
    int       worth_undoing;	/* is it worth undoing list? */
    float     undo_frac;	/* fraction of vtxs indicating worth of undoing */
    int       step;		/* loops through movements of vertices */
    int       parity;		/* sort forwards or backwards? */
    int       done;		/* has termination criteria been achieved? */
    int       nbad;		/* number of unhelpful passes in a row */
    int       npass;		/* total number of passes */
    int       nbadtries;	/* number of unhelpful passes before quitting */
    int       enforce_balance;	/* force a balanced partition? */
    int       enforce_balance_hard;	/* really force a balanced partition? */
    int       balance_trouble;	/* even balance_hard isn't working */
    int       size;		/* array spacing */
    int       i, j, k, l;	/* loop counters */
    double   *smalloc_ret();
    int       sfree();
    double    drandom(), seconds();
    int       make_kl_list();
    void      bucketsorts(), bucketsorts_bi(), bucketsort1();
    void      pbuckets(), removebilist(), movebilist(), make_bndy_list();

    nbadtries = KL_NTRIES_BAD;

    enforce_balance = FALSE;
    temp_balanced = FALSE;
    enforce_balance_hard = FALSE;
    balance_trouble = FALSE;

    size = (int) (&(listspace[0][1]) - &(listspace[0][0]));

    undo_frac = .3;

    cut_cost = 1;
    if (term_wgts[1] != NULL) {
	if (CUT_TO_HOP_COST > 1) {
	    cut_cost = CUT_TO_HOP_COST;
	}
    }

    bspace = (int *) smalloc_ret((unsigned) nvtxs * sizeof(int));
    weightsum = (double *) smalloc_ret((unsigned) nsets * sizeof(double));
    locked = (double *) smalloc_ret((unsigned) nsets * sizeof(double));
    loose = (double *) smalloc_ret((unsigned) nsets * sizeof(double));

    if (bspace == NULL || weightsum == NULL || locked == NULL || loose == NULL) {
        sfree((char *) loose);
        sfree((char *) locked);
        sfree((char *) weightsum);
        sfree((char *) bspace);
	return(1);
    }

    if (*bndy_list != NULL) {
	bdy_ptr = *bndy_list;
	list_length = 0;
	while (*bdy_ptr != 0) {
	    bspace[list_length++] = *bdy_ptr++;
	}
	sfree((char *) *bndy_list);

	if (list_length == 0) {		/* No boundary -> make everybody bndy. */
	    for (i = 0; i < nvtxs; i++) {
		bspace[i] = i + 1;
	    }
	    list_length = nvtxs;
	}
	/* Set dvals to flag uninitialized vertices. */
	for (i = 1; i <= nvtxs; i++) {
	    dvals[i][0] = 3 * maxdval;
	}
    }
    else {
        list_length = nvtxs;
    }

    step_cutoff = KL_BAD_MOVES;
    cost_cutoff = maxdval * step_cutoff / 7;
    if (cost_cutoff < step_cutoff)
	cost_cutoff = step_cutoff;

    deltaminus = deltaplus = 0;
    for (i = 0; i < nsets; i++) {
	if (startweight[i] - goal[i] > deltaplus) {
	    deltaplus = startweight[i] - goal[i];
	}
	else if (goal[i] - startweight[i] > deltaminus) {
	    deltaminus = goal[i] - startweight[i];
	}
    }
    balanced = (deltaplus + deltaminus <= max_dev);

    bestg_min = -2.0 * nvtxs * maxdval;
    parity = FALSE;
    eweight = cut_cost + .5;
    nbad = 0;
    npass = 0;
    improved = 0;
    done = FALSE;
    while (!done) {
	npass++;
	ever_balanced = FALSE;

	/* Initialize various quantities. */
	balance_best = 0;
	for (i = 0; i < nsets; i++) {
	    for (j = 0; j < nsets; j++)
		tops[i][j] = 2 * maxdval;
	    weightsum[i] = startweight[i];
	    loose[i] = weightsum[i];
	    locked[i] = 0;
	    balance_best += goal[i];
	}

	gtotal = 0;
	bestg = bestg_min;
	beststep = -1;

	movelist = NULL;
	endlist = &movelist;

	neg_steps = 0;

	/* Compute the initial d-values, and bucket-sort them. */
	time = seconds();
	if (nsets == 2) {
	    bucketsorts_bi(graph, nvtxs, buckets, listspace, dvals, sets, term_wgts,
			   maxdval, nsets, parity, hops, bspace, list_length, npass,
			   using_ewgts);
	}
	else {
	    bucketsorts(graph, nvtxs, buckets, listspace, dvals, sets, term_wgts,
			maxdval, nsets, parity, hops, bspace, list_length, npass,
			using_ewgts);
	}
	parity = !parity;
	kl_bucket_time += seconds() - time;

	if (DEBUG_KL > 2) {
	    pbuckets(buckets, listspace, maxdval, nsets);
	}

	/* Now determine the set of K-L moves. */

	for (step = 1;; step++) {

	    /* Find the highest d-value in each set. */
	    /* But only consider moves from large to small sets, or moves */
	    /* in which balance is preserved. */
	    /* Break ties in some nonarbitrary manner. */
	    bestval = -maxdval - 1;
	    for (i = 0; i < nsets; i++)
		for (j = 0; j < nsets; j++)
		    /* Only allow moves from large sets to small sets, or */
		    /* moves which preserve balance. */
		    if (i != j) {
			/* Find the best move from i to j. */
			for (k = tops[i][j]; k >= 0 && buckets[i][j][k] == NULL;
			     k--) ;
			tops[i][j] = k;

			if (k >= 0) {
			    l = (j > i) ? j - 1 : j;
			    vtx = ((int) (buckets[i][j][k] - listspace[l])) / size;
			    vweight = graph[vtx]->vwgt;

			    if ((enforce_balance_hard && 
				  weightsum[i] >= goal[i] && weightsum[j] <= goal[j] &&
				  weightsum[i] - goal[i] - (weightsum[j] - goal[j]) >
				   max_dev) ||
				(!enforce_balance_hard &&
				  weightsum[i] >= goal[i] && weightsum[j] <= goal[j]) ||
				(!enforce_balance_hard &&
			    weightsum[i] - vweight - goal[i] > -(max_dev + 1) / 2 &&
			    weightsum[j] + vweight - goal[j] < (max_dev + 1) / 2)) {

				/* Is it the best move seen so far? */
				if (k - maxdval > bestval) {
				    bestval = k - maxdval;
				    bestvtx = vtx;
				    bestto = j;
				    /* DO I NEED ALL THIS DATA?  Just to break ties. */
				    bestdelta = fabs(weightsum[i] - vweight - goal[i]) +
				                fabs(weightsum[j] + vweight - goal[j]);
				    beststuck1 = min(loose[i], goal[j] - locked[j]);
				    beststuck2 = max(loose[i], goal[j] - locked[j]);
				}

				else if (k - maxdval == bestval) {
				    /* Tied.  Is better balanced than current best? */
				    /* If tied, move among sets with most freedom. */
				    stuck1st = min(loose[i], goal[j] - locked[j]);
				    stuck2nd = max(loose[i], goal[j] - locked[j]);
				    delta = fabs(weightsum[i] - vweight - goal[i]) +
				            fabs(weightsum[j] + vweight - goal[j]);

				    /* NOTE: Randomization in this check isn't ideal */
				    /* if more than two guys are tied. */
				    if (delta < bestdelta ||
				    (delta == bestdelta && (stuck1st > beststuck1 ||
							 (stuck1st == beststuck1 &&
							  (stuck2nd > beststuck2 ||
							 (stuck2nd == beststuck2 &&
					      (KL_RANDOM && drandom() < .5))))))) {
					bestval = k - maxdval;
					bestvtx = vtx;
					bestto = j;
					bestdelta = delta;
					beststuck1 = stuck1st;
					beststuck2 = stuck2nd;
				    }
				}
			    }
			}
		    }

	    if (bestval == -maxdval - 1) {	/* No allowed moves */
		if (DEBUG_KL > 0) {
		    printf("No KL moves at step %d.  bestg = %g at step %d.\n",
			   step, bestg, beststep);
		}
		break;
	    }

	    bestptr = &(listspace[0][bestvtx]);
	    bestfrom = sets[bestvtx];

	    vweight = graph[bestvtx]->vwgt;
	    weightsum[bestto] += vweight;
	    weightsum[bestfrom] -= vweight;
	    loose[bestfrom] -= vweight;
	    locked[bestto] += vweight;

	    if (enforce_balance) {	/* Check if this partition is balanced. */
		deltaminus = deltaplus = 0;
		for (i = 0; i < nsets; i++) {
		    if (weightsum[i] - goal[i] > deltaplus) {
			deltaplus = weightsum[i] - goal[i];
		    }
		    else if (goal[i] - weightsum[i] > deltaminus) {
			deltaminus = goal[i] - weightsum[i];
		    }
		}
		balance_val = deltaminus + deltaplus;
		temp_balanced = (balance_val <= max_dev);
		ever_balanced = (ever_balanced || temp_balanced);
	    }

	    gtotal += bestval;
	    if (((gtotal > bestg && (!enforce_balance || temp_balanced)) ||
		 (enforce_balance_hard && balance_val < balance_best)) &&
		 step != nvtxs) {
		bestg = gtotal;
		beststep = step;
		if (enforce_balance_hard) {
		    balance_best = balance_val;
		}
		if (temp_balanced) {
		    enforce_balance_hard = FALSE;
		}
	    }

	    if (DEBUG_KL > 1) {
		printf("At KL step %d, bestvtx=%d, bestval=%d (%d-> %d)\n",
		       step, bestvtx, bestval, bestfrom, bestto);
	    }

	    /* Monitor the stopping criteria. */
	    if (bestval < 0) {
		if (!enforce_balance || ever_balanced)
		    neg_steps++;
		if (bestg != bestg_min)
		    neg_cost = bestg - gtotal;
		else
		    neg_cost = -maxdval - 1;
		if ((neg_steps > step_cutoff || neg_cost > cost_cutoff) &&
			!(enforce_balance && bestg == bestg_min) &&
			(beststep != step)) {
		    if (DEBUG_KL > 0) {
			if (neg_steps > step_cutoff) {
			    printf("KL step cutoff at step %d.  bestg = %g at step %d.\n",
				   step, bestg, beststep);
			}
			else if (neg_cost > cost_cutoff) {
			    printf("KL cost cutoff at step %d.  bestg = %g at step %d.\n",
				   step, bestg, beststep);
			}
		    }
		    break;
		}
	    }
	    else if (bestval > 0) {
		neg_steps = 0;
	    }

	    /* Remove vertex from its buckets, and flag it as finished. */
	    l = 0;
	    for (k = 0; k < nsets; k++) {
		if (k != bestfrom) {
		    dval = dvals[bestvtx][l] + maxdval;
		    removebilist(&listspace[l][bestvtx],
				 &buckets[bestfrom][k][dval]);
		    l++;
		}
	    }


	    /* Is there a better way to do this? */
	    sets[bestvtx] = -sets[bestvtx] - 1;

	    /* Set up the linked list of moved vertices. */
	    bestptr->next = NULL;
	    bestptr->prev = (struct bilist *) bestto;
	    *endlist = bestptr;
	    endlist = &(bestptr->next);

	    /* Now update the d-values of all the neighbors */
	    edges = graph[bestvtx]->edges;
	    if (using_ewgts)
		ewptr = graph[bestvtx]->ewgts;
	    for (j = graph[bestvtx]->nedges - 1; j; j--) {
		neighbor = *(++edges);
		if (using_ewgts)
		    eweight = *(++ewptr) * cut_cost + .5;

		/* First make sure neighbor is alive. */
		if (sets[neighbor] >= 0) {
		    group = sets[neighbor];

                    if (dvals[neighbor][0] >= 3 * maxdval) {
		        /* New vertex, not yet in buckets. */
		        /* Can't be neighbor of moved vtx, so compute */
		        /* inital dvals and buckets, then update. */
			bucketsort1(graph, neighbor, buckets, listspace, dvals, sets,
			    term_wgts, maxdval, nsets, hops, using_ewgts);
		    }

		    l = 0;
		    for (k = 0; k < nsets; k++) {
			if (k != group) {
			    diff = eweight * (
					hops[k][bestfrom] - hops[group][bestfrom] +
					    hops[group][bestto] - hops[k][bestto]);
			    dval = dvals[neighbor][l] + maxdval;
			    movebilist(&listspace[l][neighbor],
				       &buckets[group][k][dval],
				       &buckets[group][k][dval + diff]);
			    dvals[neighbor][l] += diff;
			    dval += diff;
			    if (dval > tops[group][k])
				tops[group][k] = dval;
			    l++;
			}
		    }
		}
	    }
	    if (DEBUG_KL > 2) {
		pbuckets(buckets, listspace, maxdval, nsets);
	    }
	}

	/* Done with a pass; should we actually perform any swaps? */
	bptr = movelist;
	if (bestg > 0 || (bestg != bestg_min && !balanced && enforce_balance) ||
	    (bestg != bestg_min && balance_trouble)) {
	    improved += bestg;
	    for (i = 1; i <= beststep; i++) {
		vtx = ((int) (bptr - listspace[0])) / size;
		bestto = (int) bptr->prev;
		startweight[bestto] += graph[vtx]->vwgt;
		startweight[-sets[vtx] - 1] -= graph[vtx]->vwgt;
		sets[vtx] = (short) bestto;
		bptr = bptr->next;
	    }

	    deltaminus = deltaplus = 0;
	    for (i = 0; i < nsets; i++) {
		if (startweight[i] - goal[i] > deltaplus) {
		    deltaplus = startweight[i] - goal[i];
		}
		else if (goal[i] - startweight[i] > deltaminus) {
		    deltaminus = goal[i] - startweight[i];
		}
	    }
/*
printf(" deltaplus = %f, deltaminus = %f, max_dev = %d\n", deltaplus, deltaminus, max_dev);
*/
	    balanced = (deltaplus + deltaminus <= max_dev);
	}
	else {
	    nbad++;
	}

	if (!balanced || bptr == movelist) {
	    if (enforce_balance) {
	        if (enforce_balance_hard) {
		     balance_trouble = TRUE;
		}
		enforce_balance_hard = TRUE;
	    }
	    enforce_balance = TRUE;
	    nbad++;
	}

	worth_undoing = (step < undo_frac * nvtxs);
	done = (nbad >= nbadtries && balanced);
	if (KL_MAX_PASS > 0) {
	    done = done || (npass == KL_MAX_PASS && balanced);
	}
	if (!done) {		/* Prepare for next pass. */
	    if (KL_UNDO_LIST && worth_undoing && !balance_trouble) {
		/* Make a list of modified vertices for next bucketsort. */
		/* Also, ensure these vertices are removed from their buckets. */
		list_length = make_kl_list(graph, movelist, buckets, listspace,
					   sets, nsets, bspace, dvals, maxdval);
	    }
	}
	if (done || !(KL_UNDO_LIST && worth_undoing && !balance_trouble)) {
	    /* Restore set numbers of remaining, altered vertices. */
	    while (bptr != NULL) {
		vtx = ((int) (bptr - listspace[0])) / size;
		sets[vtx] = -sets[vtx] - 1;
		bptr = bptr->next;
	    }
	    list_length = nvtxs;
	}

	if (done && *bndy_list != NULL) {
	    make_bndy_list(graph, movelist, buckets, listspace, sets, nsets,
			   bspace, tops, bndy_list);
	}
    }

    if (DEBUG_KL > 0) {
	printf("   KL required %d passes to improve by %d.\n", npass, improved);
    }

    sfree((char *) loose);
    sfree((char *) locked);
    sfree((char *) weightsum);
    sfree((char *) bspace);
    return(0);
}
