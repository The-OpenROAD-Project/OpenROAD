/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<string.h>

/* Wrapper for a printf statement with a string as only arg.
   Prints to screen and to output file if there is one. */
void      strout(msg)
char     *msg;
{
    extern FILE *Output_File;           /* output file or null */

    printf("%s\n", msg);
    if (Output_File != NULL) {
        fprintf(Output_File,"%s\n", msg);
    }
}
