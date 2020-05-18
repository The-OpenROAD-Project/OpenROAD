/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation
***  the rights to use, copy, modify, merge, publish, distribute, sublicense,
***  and/or sell copies of the Software, and to permit persons to whom the
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/

/*
 *
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * Copyright (c) 1997, by ABKGroup and the Regents of the University of Calif.
 *
 */

//! author="July, 1997 by Igor Markov, VLSI CAD ABKGroup UCLA/CS"
//! CHANGES=" abkstring.h 970820  ilm  moved handling of strcasecmp and _stricmp
// from abktempl.h 990119  mro  added analogous corresp. btw. strncasecmp and
//_strnicmp ilm: 970722   added for "symmetry" (NT -> Solaris portability) "

/* using the <string.h> distributed with the SunPro CC compiler
 CHANGES (those made after August 18, 1997)
 970820  ilm  moved handling of strcasecmp and _stricmp from abktempl.h
 990119  mro  added analogous corresp. btw. strncasecmp and _strnicmp
 ilm: 970722   added for "symmetry" (NT -> Solaris portability)
*/
#ifndef _STRINGS_H
#define _STRINGS_H

#if defined(WIN32) && !defined(STRCASECMP)
#define STRCASECMP
#define strcasecmp _stricmp
#endif

#if !defined(WIN32) && !defined(STRICMP) && !defined(STRCASECMP)
#define STRICMP
#define _stricmp strcasecmp
#endif

#if defined(WIN32) && !defined(STRNCASECMP)
#define STRNCASECMP
#define strncasecmp _strnicmp
#endif

#if !defined(WIN32) && !defined(STRNICMP) && !defined(STRCASECMP)
#define STRNICMP
#define _strnicmp strncasecmp
#endif

#ifdef WIN32
char *ulltostr(unsigned value, char *ptr);
char *lltostr(int value, char *ptr);
#endif

/* =========================================================================*/

#ifndef WIN32
#include <string.h>
#else

#if !defined(_XOPEN_SOURCE)
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__)

extern int bcmp(const void *s1, const void *s2, size_t n);
extern void bcopy(const void *s1, void *s2, size_t n);
extern void bzero(void *s, size_t n);

extern char *index(const char *s, int c);
extern char *rindex(const char *s, int c);

#else

extern int bcmp();
extern void bcopy();
extern void bzero();

extern char *index();
extern char *rindex();

#endif /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* defined(WIN32)*/

#endif /* _STRINGS_H */
