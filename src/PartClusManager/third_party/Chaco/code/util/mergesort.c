/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"defs.h"


static void recursesort();
static void merge();

void      mergesort(vals, nvals, indices, space)
/* Merge sort values in vals, returning sorted indices. */
double   *vals;			/* values to be sorted */
int       nvals;		/* number of values */
int      *indices;		/* indices of sorted list */
int      *space;		/* space for nvals integers */
{
    extern int DEBUG_BPMATCH;	/* debugging flag for bipartite matching */
    int       flag;		/* has sorting screwed up? */
    int       i;

    for (i = 0; i < nvals; i++)
	indices[i] = i;

    recursesort(vals, nvals, indices, space);

    if (DEBUG_BPMATCH > 0) {
	flag = FALSE;
	for (i = 1; i < nvals; i++) {
	    if (vals[indices[i - 1]] > vals[indices[i]])
		flag = TRUE;
	}
	if (flag) {
	    printf("List improperly sorted in mergesort\n");
	    if (DEBUG_BPMATCH > 1) {
		for (i = 1; i < nvals; i++) {
		    printf("%d  %f\n", indices[i], vals[indices[i]]);
		}
	    }
	}
    }
}


static void recursesort(vals, nvals, indices, space)
/* Recursive implementation of mergesort. */
double   *vals;			/* values to be sorted */
int       nvals;		/* number of values */
int      *indices;		/* indices of sorted list */
int      *space;		/* space for nvals integers */
{
    int       length1, length2;	/* lengths of two sublists */
    int       i;		/* temporary value */

    /* First consider base cases */
    if (nvals <= 1)
	return;

    else if (nvals == 2) {
	if (vals[indices[0]] <= vals[indices[1]])
	    return;
	else {
	    i = indices[0];
	    indices[0] = indices[1];
	    indices[1] = i;
	    return;
	}
    }

    else {
	length1 = nvals / 2;
	length2 = nvals - length1;
	recursesort(vals, length1, indices, space);
	recursesort(vals, length2, indices + length1, space);
	merge(vals, indices, length1, length2, space);
    }
}


static void merge(vals, indices, length1, length2, space)
/* Merge two sorted lists to create longer sorted list. */
double   *vals;			/* values to be sorted */
int      *indices;		/* start of first sorted list */
int       length1;		/* number of values in first list */
int       length2;		/* number of values in second list */
int      *space;		/* sorted answer */
{
    int       n1, n2;		/* number of values seen in each list */
    int      *spaceptr;		/* loops through space array */
    int      *index1, *index2;	/* pointers into two lists */

    n1 = n2 = 0;
    spaceptr = space;
    index1 = indices;
    index2 = indices + length1;

    while (n1 < length1 && n2 < length2) {
	if (vals[*index1] <= vals[*index2]) {
	    *spaceptr++ = *index1++;
	    ++n1;
	}
	else {
	    *spaceptr++ = *index2++;
	    ++n2;
	}
    }

    /* Now add on remaining elements */
    while (n1 < length1) {
	*spaceptr++ = *index1++;
	++n1;
    }

    while (n2 < length2) {
	*spaceptr++ = *index2++;
	++n2;
    }

    spaceptr = space;
    index1 = indices;
    for (n1=length1+length2; n1; n1--) *index1++ = *spaceptr++;
}
