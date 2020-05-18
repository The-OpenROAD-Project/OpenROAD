/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Compute the binary reflected Gray code of a value. */
int       gray(i)
int       i;
{
    return ((i >> 1) ^ i);
}


/* Compute the inverse of the binary reflected Gray code of a value. */
/*
int       invgray(i)
int       i;
{
    int       k;

    k = i;
    while (k) {
	k >>= 1;
	i ^= k;
    }
    return (i);
}
*/
