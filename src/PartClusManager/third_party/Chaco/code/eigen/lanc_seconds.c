/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */


/* Switchable timer routine  */
double    lanc_seconds()
{
    extern int LANCZOS_TIME; /* perform detailed timing on Lanczos_SO? */
    double seconds();

    if (LANCZOS_TIME) {
	return(seconds());
    }
    else {
	return(0);
    }
}
