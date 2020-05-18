/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include  <stdio.h>

/* Dynamically allocate a 2 dimensional array. */
/* Return instead of dying if out of space. */

double   *array_alloc_2D_ret(dim1, dim2, size)
int       dim1;			/* size of first dimension */
int       dim2;			/* size of second dimension */
unsigned int size;		/* size of array elements */
{
    int       total;		/* Total size of the array */
    int       aligned_dim;	/* dim1 or dim1+1 to ensure data alignement */
    int       offset;		/* offset of array elements */
    double   *field;		/* The multi-dimensional array */
    char    **ptr;		/* Pointer offset */
    char     *data;		/* Data offset */
    int       j;		/* loop counter */
    double   *smalloc_ret();

    aligned_dim = (dim1 % 2) ? dim1 + 1 : dim1;
    offset = aligned_dim * sizeof(void *);
    total = offset + dim1 * dim2 * size;
    field = (double *) smalloc_ret((unsigned) total);

    if (field != NULL) {
        ptr = (char **) (field);
        data = (char *) (field);
        data += offset;
        for (j = 0; j < dim1; j++) {
	    ptr[j] = data + j * size * dim2;
	}
    }

    return ((double *) field);
}
