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


#include "abkrand.h"
#include <iostream>

int main(void)
{
    // first random variable has mean 7, std. dev. 24
    // second random variable has mean 3, std. dev. 1
    // correlation between the two is -0.37
    // The string "::main(),r" is a "local identifier"
    // to keep this random object from changing the seeding
    // of other random objects.
    RandomNormCorrPairs r(7,24,3,1,-0.37,"::main(),r");
    Timer tm;
    unsigned i;
//  unsigned bins[100]={0};
    const unsigned trials=500000;
    double sumv1=0,sumv2=0,sumv1squared=0,sumv2squared=0,sumv1v2=0;

    for (i=0;i<trials;i++)
        {
        std::pair<double,double> p = r.getPair();
        double v1=p.first;
        double v2=p.second;
        sumv1 += v1;
        sumv2 += v2;
        sumv1squared += v1*v1;
        sumv2squared += v2*v2;
        sumv1v2 += v1*v2;
        }

    tm.stop();


    std::cout << "Time to compute " << trials <<" correlated pairs " << tm << std::endl;


    std::cout << "Stats for v1 are: mean = " << double(sumv1)/trials 
         << " estimated Std. Dev = " << sqrt(
         (sumv1squared-(1.0/trials)*sumv1*sumv1)
         /(trials-1)
         ) << std::endl;

    std::cout << "Stats for v2 are: mean = " << double(sumv2)/trials 
         << " estimated Std. Dev = " << sqrt(
         (sumv2squared-(1.0/trials)*sumv2*sumv2)
         /(trials-1)
         ) << std::endl;

    std::cout << "Correlation = " << (sumv1v2 - (1.0/trials)*sumv1*sumv2)
        / sqrt( (sumv1squared-(1.0/trials)*sumv1*sumv1)
                *
                (sumv2squared-(1.0/trials)*sumv2*sumv2) ) << 
                " (Note: biased estimator of r) " << std::endl;


    return 0;

    }

