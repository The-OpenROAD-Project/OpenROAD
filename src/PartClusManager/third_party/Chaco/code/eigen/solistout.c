/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

/* Print out the orthogonalization set, double version */ 
void      solistout(solist, n, ngood, j)
struct orthlink **solist;	/* vector of pntrs to orthlnks */
int       n;			/* length of vecs to orth. against */
int       ngood;		/* number of good vecs on list */
int       j;			/* current number of Lanczos steps */
{
    int       i;		/* index */
    extern int DEBUG_EVECS;	/* debugging output level for eigen computations */

    /* to placate alint */
    n = n;

    for (i = 1; i <= ngood; i++) {
	if ((solist[i])->index <= (int) (j / 2)) {
	    printf(".");
	}
	else {
	    printf("+");
	}
	/* Really detailed output: printf("\n"); printf("depth
	   %d\n",(solist[i])->depth); printf("index %d\n",(solist[i])->index);
	   printf("ritzval %g\n",(solist[i])->ritzval); printf("betaji
	   %g\n",(solist[i])->betaji); printf("tau %g\n",(solist[i])->tau);
	   printf("prevtau %g\n",(solist[i])->prevtau);
	   vecout((solist[i])->vec,1,n,"vec", (char *) NULL); */
    }
    printf("%d\n", ngood);

    if (DEBUG_EVECS > 2) {
	printf("  actual indicies: ");
	for (i = 1; i <= ngood; i++) {
	    printf(" %2d", solist[i]->index);
	}
	printf("\n");
    }
}

/* Print out the orthogonalization set, float version */ 
void      solistout_float(solist, n, ngood, j)
struct orthlink_float **solist;	/* vector of pntrs to orthlnks */
int       n;			/* length of vecs to orth. against */
int       ngood;		/* number of good vecs on list */
int       j;			/* current number of Lanczos steps */
{
    int       i;		/* index */

    /* to placate alint */
    n = n;

    for (i = 1; i <= ngood; i++) {
	if ((solist[i])->index <= (int) (j / 2)) {
	    printf(".");
	}
	else {
	    printf("+");
	}
    }
    printf("%d\n", ngood);
}
