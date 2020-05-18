/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"
#include	"refine_map.h"


void      init_cube_edata(edata, node1, dim, mask)
struct refine_edata *edata;	/* desire data for current edge */
int       node1;		/* processor incident to current wire */
int       dim;			/* direction of wire */
int       mask;			/* bit set in wire dimension */
{

    edata->node1 = (short) node1;
    edata->node2 = (short) node1 ^ mask;
    edata->dim = (short) dim;
}
