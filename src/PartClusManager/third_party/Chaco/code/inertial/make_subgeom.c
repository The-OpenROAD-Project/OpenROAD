/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */


void      make_subgeom(igeom, coords, subcoords, subnvtxs, loc2glob)
int       igeom;		/* 1, 2 or 3 dimensional geometry? */
float   **coords;		/* x, y and z coordinates of vertices */
float   **subcoords;		/* x, y ans z coordinates in subgraph */
int       subnvtxs;		/* number of vertices in subgraph */
int      *loc2glob;		/* maps from subgraph to graph numbering */
{
    int       i;		/* loop counter */

    if (igeom == 1) {
        for (i = 1; i <= subnvtxs; i++) {
	    subcoords[0][i] = coords[0][loc2glob[i]];
	}
    }
    else if (igeom == 2) {
        for (i = 1; i <= subnvtxs; i++) {
	    subcoords[0][i] = coords[0][loc2glob[i]];
	    subcoords[1][i] = coords[1][loc2glob[i]];
	}
    }
    else if (igeom > 2) {
        for (i = 1; i <= subnvtxs; i++) {
	    subcoords[0][i] = coords[0][loc2glob[i]];
	    subcoords[1][i] = coords[1][loc2glob[i]];
	    subcoords[2][i] = coords[2][loc2glob[i]];
	}
    }
}
