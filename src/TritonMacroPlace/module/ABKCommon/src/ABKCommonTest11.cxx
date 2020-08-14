/**************************************************************************
***    
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2010 Regents of the University of Michigan,
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


#include "abkcommon.h"
#include <iostream>
#include <ABKCommon/uofm_alloc.h>

using std::cout;
using std::endl;
using std::vector;

int main()
{

    unsigned vec1[4] = {2,4,3,9};
    unsigned vec2[5] = {24, 36, 48, 64, 72};
    unsigned vec3[2] = {1, 0};
    vector <unsigned> emptyvec;

    unsigned gcd1 = abkGcd (vector <unsigned> (vec1, vec1+4));
    unsigned gcd2 = abkGcd (vector <unsigned> (vec2, vec2+5));
    unsigned gcd3 = abkGcd (vector <unsigned> (1, (unsigned)5));
    unsigned gcd4 = abkGcd (vector <unsigned> (1, 0));
    unsigned gcd5 = abkGcd (vector <unsigned> (vec3, vec3+2));

    cout << gcd1 << " " << gcd2 << " " << gcd3 << " " << gcd4 << " " <<
        gcd5 << endl;
    cout << "=========================================" << endl;

    return 0;
}


