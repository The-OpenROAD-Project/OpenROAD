/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"

/* Record a return TRUE if answer is yes, FALSE if no. */
int       affirm(prompt)
char     *prompt;
{
    char      reply;		/* character typed in */
    int       done;		/* loop control */
    void      bail();

    if (prompt != NULL && (int) strlen(prompt) > 0) {
        printf("%s? ", prompt);
    }
    done = 0;
    while (!done) {
	reply = (char) getchar();
	/* while (reply == ' ' || reply== '\n') reply= (char) getchar(); */
	while (isspace(reply))
	    reply = (char) getchar();

	if (reply == 'y' || reply == 'Y')
	    done = 1;
	else if (reply == 'n' || reply == 'N')
	    done = 2;
	else if (reply == 'q' || reply == 'Q')
	    done = 3;
	else if (reply == 'x' || reply == 'X')
	    done = 3;

	else {
	    printf("Valid responses begin with: y Y n N q Q x X\n");
	    if (prompt != NULL) printf("%s? ", prompt);
	    /* Flush rest of input line. */
	    while (reply != '\n')
		reply = (char) getchar();
	}
    }
    if (done > 2)
	bail((char *) NULL, 0);
    else if (done == 2)
	return (FALSE);
    return (TRUE);
}
