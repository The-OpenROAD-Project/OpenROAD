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

// Aug 27, 1997   Igor Markov  VLSI CAD UCLA ABKGROUP

// CHANGES
// 980212 ilm renamed into abkio.cxx
//            added printRange() and some more from abkio.h
// 980320 ilm added optional lineNo argument to asserting IO manipulators

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <iostream>
#include <iomanip>
#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include "abkstring.h"
#include "abkassert.h"
#include "abkio.h"
#include "newcasecmp.h"
#define eh eathash

using std::istream;

istream& eatblank(istream& i) {
        while (i.peek() == ' ' || i.peek() == '\t') i.get();
        return i;
}

istream& skiptoeol(istream& i) {
        while (!i.eof() && i.peek() != '\n' && i.peek() != '\r') i.get();
        // if (i.peek() == '\n' || i.peek() == '\r') i.get();
        return i;
}

// ================ Now go arrangements for manipulators with arguments

istream& impl_eathash(istream& i, int& lineNo) {
        bool noLineNo = (lineNo == -1);
        i >> eatblank;
        while (i.peek() == '\n' || i.peek() == '\r') {
                lineNo++;
                i.get();
                i >> eatblank;
        }
        // if (i.peek()!='#' && i.peek()!='\n')  {i.putback('\n'); }
        while (i.peek() == '#') {
                while (!i.eof() && i.peek() != '\n' && i.peek() != '\r') i.get();
                while (!i.eof() && (i.peek() == '\n' || i.peek() == '\r')) {
                        i.get();
                        i >> eatblank;
                        lineNo++;
                }
        }
        while (i.peek() == '\n' || i.peek() == '\r') {
                lineNo++;
                i.get();
                i >> eatblank;
        }
        // if (i.peek()!='#' && i.peek()!='\n')  {i.putback('\n'); }
        // if (i.peek() == '\n') { i.get(); lineNo++; }
        if (noLineNo) lineNo = -1;
        return i;
}

istream& impl_needeol(istream& i, int lineNo = -1) {
        i >> eatblank;
        if (lineNo > 0) {
                abkfatal2(i.peek() == '\n' || i.peek() == '\r', " End of line expected near line ", lineNo);
        } else {
                abkfatal(i.peek() == '\n' || i.peek() == '\r', " End of line expected");
        }
        return i;
}

istream& impl_noeol(istream& i, int lineNo = -1) {
        i >> eatblank;
        if (lineNo > 0) {
                abkfatal2(i.peek() != '\n' && i.peek() != '\r', " Unexpected end of line near line ", lineNo);
        } else {
                abkfatal(i.peek() != '\n' && i.peek() != '\r', " Unexpected end of line");
        }
        return i;
}

istream& impl_isnumber(istream& i, int lineNo = -1) {
        i >> eatblank;
        char errMess[255];
        char c = i.peek();
        if (lineNo > 0)
                sprintf(errMess, " near line %d, but starts with %c ", lineNo, c);
        else
                sprintf(errMess, ", but starts with %c ", c);
        abkfatal2(isdigit(i.peek()) || i.peek() == '-', " Number expected", errMess);
        return i;
}

istream& impl_isword(istream& i, int lineNo = -1) {
        i >> eatblank;
        char errMess[255];
        char c = i.peek();
        if (lineNo > 0)
                sprintf(errMess, " near line %d, but starts with %c ", lineNo, c);
        else
                sprintf(errMess, ", but starts with %c ", c);
        abkfatal2(isalpha(c), " Word expected", errMess);
        return i;
}

istream& impl_needword(istream& in, const char* word, int lineNo = -1) {
        char buffer[1024], errMess[255];  // still no way to avoid buffer overflow !
        in >> eatblank >> buffer;
        if (lineNo > 0) {
                sprintf(errMess, " '%s' expected near line %d . Got %s ", word, lineNo, buffer);
                abkfatal(strcmp(buffer, word) == 0, errMess);
        } else {
                sprintf(errMess, " '%s' expected. Got %s ", word, buffer);
                abkfatal(strcmp(buffer, word) == 0, errMess);
        }
        return in;
}

istream& impl_needcaseword(istream& in, const char* word, int lineNo = -1) {
        char buffer[1024], errMess[255];  // still no way to avoid buffer overflow !
        in >> eatblank;
        in >> buffer;
        if (lineNo > 0) {
                sprintf(errMess, " '%s' expected near line %d. Got %s ", word, lineNo, buffer);
                abkfatal2(newstrcasecmp(buffer, word) == 0, errMess, lineNo);
        } else {
                sprintf(errMess, " '%s' expected. Got %s ", word, buffer);
                abkfatal(newstrcasecmp(buffer, word) == 0, errMess);
        }
        return in;
}

ManipFuncObj2<const char*, int> needword(const char* word, int lineNo) { return ManipFuncObj2<const char*, int>(impl_needword, word, lineNo); }

ManipFuncObj2<const char*, int> needcaseword(const char* word, int lineNo) { return ManipFuncObj2<const char*, int>(impl_needcaseword, word, lineNo); }

ManipFuncObj1<int&> eathash(int& lineNo) { return ManipFuncObj1<int&>(impl_eathash, lineNo); }

ManipFuncObj1<int> needeol(int lineNo) { return ManipFuncObj1<int>(impl_needeol, lineNo); }

ManipFuncObj1<int> noeol(int lineNo) { return ManipFuncObj1<int>(impl_noeol, lineNo); }

ManipFuncObj1<int> my_isnumber(int lineNo) { return ManipFuncObj1<int>(impl_isnumber, lineNo); }

ManipFuncObj1<int> isword(int lineNo) { return ManipFuncObj1<int>(impl_isword, lineNo); }
