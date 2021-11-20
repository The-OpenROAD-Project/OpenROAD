#include <stdio.h>
#include <ctype.h>
#include "newcasecmp.h"

int newstrcasecmp( const char *s1, const char *s2 )
{
    while (1)
    {
        int c1 = tolower( (unsigned char) *s1++ );
        int c2 = tolower( (unsigned char) *s2++ );
        if (c1 == 0 || c1 != c2) return c1 - c2;
    }
}
