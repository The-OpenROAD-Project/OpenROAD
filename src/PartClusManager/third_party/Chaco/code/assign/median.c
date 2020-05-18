/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"


/* Find the median of set of values. */
/* Can also find nested medians of several sets of values */
/* Routine works by repeatedly guessing a value, and discarding those */
/* values which are on the wrong side of the guess. */

void      median(graph, vals, nvtxs, active, goal, using_vwgts, sets)
struct vtx_data **graph;	/* data structure with vertex weights */
double   *vals;			/* values of which to find median */
int       nvtxs;		/* number of values I own */
int      *active;		/* space for list of nvtxs ints */
double   *goal;			/* desired sizes for sets */
int       using_vwgts;		/* are vertex weights being used? */
short    *sets;			/* set each vertex gets assigned to */
{
    double   *vptr;		/* loops through vals array */
    double    val;		/* value in vals array */
    double    maxval;		/* largest active value */
    double    minval;		/* smallest active value */
    double    guess;		/* approximate median value */
    double    nearup;		/* lowest guy above guess */
    double    neardown;		/* highest guy below guess */
    double    whigh;		/* total weight of values above maxval */
    double    wlow;		/* total weight of values below minval */
    double    wabove;		/* total weight of active values above guess */
    double    wbelow;		/* total weight of active values below guess */
    double    wexact;		/* weight of vertices exactly at guess */
    double    lweight;		/* desired weight of lower values in set */
    double    uweight;		/* desired weight of upper values in set */
    double    frac;		/* fraction of values I want less than guess */
    int      *aptr;		/* loops through active array */
    int      *aptr2;		/* helps update active array */
    int       myactive;		/* number of active values I own */
    double    wfree;		/* weight of vtxs not yet divided */
    int       removed;		/* number of my values eliminated */
    /*int npass = 0;*/		/* counts passes required to find median */
    int       done;		/* check for termination criteria */
    int       vtx;		/* vertex being considered */
    int       i;		/* loop counters */
    void      median_assign();

    /* Initialize. */

    /* Determine the desired weight sums for the two different sets. */
    lweight = goal[0];
    uweight = goal[1];

    myactive = nvtxs;
    whigh = wlow = 0;

    /* Find largest and smallest values in vector, and construct active list. */
    vptr = vals;
    aptr = active;
    minval = maxval = *(++vptr);
    *aptr++ = 1;
    for (i = 2; i <= nvtxs; i++) {
	*aptr++ = i;
	val = *(++vptr);
	if (val > maxval)
	    maxval = val;
	if (val < minval)
	    minval = val;
    }

    /* Loop until all sets are partitioned correctly. */
    done = FALSE;
    while (!done) {
	/*npass++;*/

	/* Select a potential dividing value. */
	/* Currently, this assumes a linear distribution. */
	wfree = lweight + uweight - (wlow + whigh);
	frac = (lweight - wlow) / wfree;

	/* Overshoot a bit to try to cut into largest set. */
	frac = .5 * (frac + .5);

	guess = minval + frac * (maxval - minval);

	/* Now count the guys above and below this guess. */
	/* Also find nearest values on either side of guess. */
	wabove = wbelow = wexact = 0;
	nearup = maxval;
	neardown = minval;

	aptr = active;
	for (i = 0; i < myactive; i++) {
	    vtx = *aptr++;
	    val = vals[vtx];
	    if (val > guess) {
		if (using_vwgts)
		    wabove += graph[vtx]->vwgt;
		else
		    wabove++;
		if (val < nearup)
		    nearup = val;
	    }
	    else if (val < guess) {
		if (using_vwgts)
		    wbelow += graph[vtx]->vwgt;
		else
		    wbelow++;
		if (val > neardown)
		    neardown = val;
	    }
	    else {
		if (using_vwgts)
		    wexact += graph[vtx]->vwgt;
		else
		    wexact++;
	    }
	}

	/* Select a half to discard. */
	/* And remove discarded vertices from active list. */
	removed = 0;
/*	if (wlow + wbelow - lweight > whigh + wabove - uweight) {*/
	if (wlow + wbelow - lweight > whigh + wabove - uweight &&
	    whigh + wabove + wexact < uweight ) {
	    /* Discard upper set. */
	    whigh += wabove + wexact;
	    maxval = neardown;
	    done = FALSE;
	    aptr = aptr2 = active;
	    for (i = 0; i < myactive; i++) {
		if (vals[*aptr] >= guess) {
		    ++removed;
		    if (vals[*aptr] == guess) 
		        nearup = guess;
		}
		else
		    *aptr2++ = *aptr;
		aptr++;
	    }
	    myactive -= removed;
	    if (myactive == 0) done = TRUE;
	}
/*	else if ( whigh + wabove - uweight > wlow + wbelow - lweight) {*/
	else if ( whigh + wabove - uweight > wlow + wbelow - lweight &&
	    wlow + wbelow + wexact < lweight ) {
	    /* Discard lower set. */
	    wlow += wbelow + wexact;
	    minval = nearup;
	    done = FALSE;
	    aptr = aptr2 = active;
	    for (i = 0; i < myactive; i++) {
		if (vals[*aptr] <= guess) {
		    ++removed;
		    if (vals[*aptr] == guess) 
		        neardown = guess;
		}
		else
		    *aptr2++ = *aptr;
		aptr++;
	    }
	    myactive -= removed;
	    if (myactive == 0) done = TRUE;
	}
	else {			/* Perfect partition! */
	    wlow += wbelow;
	    whigh += wabove;
	    /*
	    minval = nearup;
	    maxval = neardown;
	    myactive = 0;
	    */
	    done = TRUE;
	}

	/* Check for alternate termination criteria. */
	if (!done && maxval == minval) {
	    guess = maxval;
	    done = TRUE;
	}
    }
    median_assign(graph, vals, nvtxs, goal, using_vwgts, sets, wlow, whigh, guess);
}


void      median_assign(graph, vals, nvtxs, goal, using_vwgts, sets,
			          wlow, whigh, guess)
struct vtx_data **graph;	/* data structure with vertex weights */
double   *vals;			/* values of which to find median */
int       nvtxs;		/* number of values I own */
double   *goal;			/* desired sizes for sets */
int       using_vwgts;		/* are vertex weights being used? */
short    *sets;			/* assigned set for each vertex */
double    wlow;			/* sum of weights below guess */
double    whigh;		/* sum of weights above guess */
double    guess;		/* median value */
{
    int       i;		/* loop counter */

    for (i = 1; i <= nvtxs; i++) {
	if (vals[i] < guess)
	    sets[i] = 0;
	else if (vals[i] > guess)
	    sets[i] = 1;
	else {
	    if (goal[0] - wlow > goal[1] - whigh) {
		sets[i] = 0;
		if (using_vwgts)
		    wlow += graph[i]->vwgt;
		else
		    wlow++;
	    }
	    else {
		sets[i] = 1;
		if (using_vwgts)
		    whigh += graph[i]->vwgt;
		else
		    whigh++;
	    }
	}
    }
}
