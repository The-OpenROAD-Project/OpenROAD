/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include  <stdio.h>

/* Print a double precision number with filtering format to screen. */
void      doubleout(number, mode)
double    number;		/* argument to print */
int       mode;			/* currently just one */
{
    double    fabs();		/* intrinsic absolute value function */

    if (mode == 1) {
	if (fabs(number) < 100) {
	    printf("  %19.16f", number);
	}
	else {
	    printf("  %19g", number);
	}
    }
}


/* Print a double precision number with filtering format to file. */
void      doubleout_file(outfile, number, mode)
FILE     *outfile;              /* output file if not NULL */
double    number;		/* argument to print */
int       mode;			/* currently just one */
{
    double    fabs();		/* intrinsic absolute value function */

    if (outfile == NULL) return;

    if (mode == 1) {
	if (fabs(number) < 100) {
	    fprintf(outfile,"  %19.16f", number);
	}
	else {
	    fprintf(outfile,"  %19g", number);
	}
    }
}
