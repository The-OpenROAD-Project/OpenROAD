/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<string.h>
#include	"defs.h"
#include	"params.h"

/* Robust routine to read an integer */
int       input_int()
{
    char      line[LINE_LENGTH];/* space to read input line */
    int       done;		/* flag for end of integer */
    int       val;		/* value returned */
    int       i;		/* loop counter */
    int       isdigit();

    i = 0;
    done = FALSE;
    while (!done) {
	line[i] = getchar();
	if (isdigit(line[i]) || line[i] == '-')
	    i++;
	else if (i != 0)
	    done = TRUE;
    }

    sscanf(line, "%d", &val);
    return (val);
}
