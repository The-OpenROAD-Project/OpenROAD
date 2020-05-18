/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "structs.h"
#include "defs.h"

/* Return maximum entries of vector over range */
void      scanmax(vec, beg, end, scanlist)
double   *vec;			/* vector to scan */
int       beg, end;		/* index range */
struct scanlink **scanlist;	/* pntr to list holding results of scan */
{
    extern double DOUBLE_MAX;
    struct scanlink *top;
    struct scanlink *curlnk;
    struct scanlink *prevlnk;
    double    val;
    int       i;

    curlnk = *scanlist;
    while (curlnk != NULL) {
	curlnk->indx = 0;
	curlnk->val = -DOUBLE_MAX;
	curlnk = curlnk->pntr;
    }

/* Note: Uses current top link (which would need to be deleted anyway) each time
	 an insertion to the list is required. */

    for (i = beg; i <= end; i++) {
	/* consider each element for insertion */
	top = *scanlist;
	val = vec[i];
	if (val > top->val) {
	    if (top->pntr == NULL) {
		/* the list is only one long, so just replace */
		top->val = val;
		top->indx = i;
	    }
	    else {
		/* beats top element; scan for insertion point */
		if (val > (top->pntr)->val) {
		    /* 2nd link becomes list pntr; otherwise stays same */
		    *scanlist = top->pntr;
		}
		prevlnk = curlnk = top;
		while ((val > curlnk->val) && (curlnk->pntr != NULL)) {
		    prevlnk = curlnk;
		    curlnk = curlnk->pntr;
		}
		if (val > curlnk->val) {
		    /* got to end of list; add top to bottom */
		    curlnk->pntr = top;
		    top->val = val;
		    top->indx = i;
		    top->pntr = NULL;
		}
		else {
		    /* stopped within list; insert top here */
		    prevlnk->pntr = top;
		    top->val = val;
		    top->indx = i;
		    top->pntr = curlnk;
		}
	    }
	}
    }
}
