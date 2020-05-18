/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<string.h>

/* Debug break point. */
void      checkpnt(tag)
char     *tag;
{
    int       affirm();
    void      bail();

    if (tag != NULL && (int) strlen(tag) > 0) printf("%s: ", tag);
    if (!affirm("continue")) {
	bail((char *) NULL, 0);
    }
}
