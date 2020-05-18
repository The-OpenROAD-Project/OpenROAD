/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"defs.h"
#include	"refine_map.h"


struct refine_edata *find_edge_cube(node, dim, edata, nsets_tot)
int       node;			/* processor node */
int       dim;			/* direction of edge from node */
struct refine_edata *edata;	/* data structure for edge preferences */
int       nsets_tot;		/* total number of processors */
{
    struct refine_edata *eguy;	/* returned pointer to edge info */
    int       index;		/* computed index into edata */

    /* Squeeze out bit dim from node number. */
    index = node ^ ((node >> dim) << dim);
    index ^= ((node >> (dim+1)) << dim);
    index += dim* nsets_tot/2;

    eguy = &(edata[index]);

    return(eguy);
}
