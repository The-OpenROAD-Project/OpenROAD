/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "structs.h"
#include "defs.h"

static struct ipairs *pedges;	/* perturbed edges */
static double *pvals;		/* perturbed values */

/* Inititialize the perturbation */
void      perturb_init(n)
int       n;			/* graph size at this level */
{
    extern int NPERTURB;	/* number of edges to perturb */
    extern double PERTURB_MAX;	/* maximum perturbation */
    int       i, j;		/* loop counter */
    double   *smalloc();
    double    drandom();

    /* Initialize the diagonal perturbation weights */
    pedges = (struct ipairs *) smalloc((unsigned) NPERTURB * sizeof(struct ipairs));
    pvals = (double *) smalloc((unsigned) NPERTURB * sizeof(double));

    if (n <= 1) {
	for (i = 0; i < NPERTURB; i++) {
	    pedges[i].val1 = pedges[i].val2 = 0;
	    pvals[i] = 0;
	}
	return;
    }

    for (i = 0; i < NPERTURB; i++) {
	pedges[i].val1 = 1 + (n * drandom());

	/* Find another vertex to define an edge. */
	j = 1 + (n * drandom());
	while (j == i)
	    j = 1 + (n * drandom());
	pedges[i].val2 = 1 + (n * drandom());

	pvals[i] = PERTURB_MAX * drandom();
    }
}

void      perturb_clear()
{
    int       sfree();

    sfree((char *) pedges);
    sfree((char *) pvals);
    pedges = NULL;
    pvals = NULL;
}


/* Modify the result of splarax to break any graph symmetry */
void      perturb(result, vec)
double   *result;		/* result of matrix-vector multiply */
double   *vec;			/* vector matrix multiplies */
{
    extern int NPERTURB;	/* number of edges to perturb */
    int       i;		/* loop counter */

    for (i = 0; i < NPERTURB; i++) {
	result[pedges[i].val1] +=
	   pvals[i] * (vec[pedges[i].val2] - vec[pedges[i].val1]);
	result[pedges[i].val2] +=
	   pvals[i] * (vec[pedges[i].val1] - vec[pedges[i].val2]);
    }
}


/* Modify the result of splarax to break any graph symmetry, float version */
void      perturb_float(result, vec)
float    *result;		/* result of matrix-vector multiply */
float    *vec;			/* vector matrix multiplies */
{
    extern int NPERTURB;	/* number of edges to perturb */
    int       i;		/* loop counter */

    for (i = 0; i < NPERTURB; i++) {
	result[pedges[i].val1] +=
	   pvals[i] * (vec[pedges[i].val2] - vec[pedges[i].val1]);
	result[pedges[i].val2] +=
	   pvals[i] * (vec[pedges[i].val1] - vec[pedges[i].val2]);
    }
}
