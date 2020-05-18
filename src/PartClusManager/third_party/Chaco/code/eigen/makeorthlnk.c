/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "structs.h"

/* Allocate space for new orthlink, double version. */
struct orthlink *makeorthlnk()
{
    struct orthlink *newlnk;
    double   *smalloc();

    newlnk = (struct orthlink *) smalloc(sizeof(struct orthlink));
    return (newlnk);
}

/* Allocate space for new orthlink, float version. */
struct orthlink_float *makeorthlnk_float()
{
    struct orthlink_float *newlnk;
    double   *smalloc();

    newlnk = (struct orthlink_float *) smalloc(sizeof(struct orthlink_float));
    return (newlnk);
}
