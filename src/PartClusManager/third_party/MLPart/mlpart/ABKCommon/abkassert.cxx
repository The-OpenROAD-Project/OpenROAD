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

// June 18, 1997   Igor Markov  VLSI CAD UCLA ABKGROUP

// 970718 ilm   added a hack to output only a significant
//                  part of filenames in error messages
// 970924 ilm   added #include "pathDelim.h" and deleted equivalent functionlty

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <stdio.h>
#include "abkassert.h"
#include "abkstring.h"
#include "pathDelims.h"

void abkassert_stop_here() {}
void abkguess_stop_here() {}
void abkfatal_stop_here() {}
void abkwarn_stop_here() {}

const char *SgnPartOfFileName(const char *fileName) {
        const char *sp = fileName;
        const char *leftDelim = strchr(fileName, pathDelim);
        const char *rightDelim = strrchr(fileName, pathDelim);
        while (leftDelim != rightDelim) {
                sp = ++leftDelim;
                leftDelim = strchr(sp, pathDelim);
        }
        return sp;
        // static_cast<void>(pathDelimWindows);
        // static_cast<void>(pathDelimUnix);
}

char *BaseFileName(char *fileName) {
        char *leftDelim = strrchr(fileName, pathDelim);
        if (leftDelim)
                leftDelim++;
        else
                leftDelim = fileName;
        char *rightDelim = strrchr(leftDelim, '.');
        if (rightDelim) *rightDelim = '\0';
        return leftDelim;
}
