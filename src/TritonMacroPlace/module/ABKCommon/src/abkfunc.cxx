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


// Created: Apr 26, 1998 by Dv
// This file contains functions for finding gcd of 2, n unsigned numbers.

// CHANGES
// 980512 ilm added abkFactorial()

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "abkcommon.h"


unsigned abkGcd (unsigned x, unsigned y)
{
    unsigned temp;

    if (x < y)
    {
        std::swap(x, y);
    }

    if (y == 0) return x;

    while (y != 0)
    {
        temp = x % y; x = y; y = temp;
    }
    return x;
}

unsigned abkGcd (const std::vector<unsigned>& numbers)
{
    unsigned count = numbers.size();
	abkfatal (count!=0, "Vector of size 0 passed to abkGcd");

    std::vector<unsigned> numsLocal(numbers);
    while (1)
    {
        if (count == 1) return numsLocal[0];

        unsigned i=0;
        for (; i < (count/2); i++)
          numsLocal[i] = abkGcd (numsLocal[2*i], numsLocal[2*i+1]);

        if (2*i == count) count = i;
        else
        {
            numsLocal[i] = numsLocal[count-1];
            count = i+1;
        }
    }
}

unsigned abkFactorial(unsigned n)
{
        unsigned fact=n--;
        for(;n!=1;n--) fact*=n;
        return fact;
}

