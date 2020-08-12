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


#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "abkrand.h"
#include <iostream>
#include <ABKCommon/uofm_alloc.h>

int main(void)
{
    double meansA[]={3,4,5};
    std::vector<double> means(meansA,meansA+3);

    double stdDevA[]={7,8,9};
    std::vector<double> stdDevs(stdDevA,stdDevA+3);

    double row1A[]={0.3,0.1};
    std::vector<double> row1(row1A,row1A+2);

    double row2A[]={0.2};
    std::vector<double> row2(row2A,row2A+1);

    std::vector<std::vector<double> > corrs(2);
    corrs[0]=row1;
    corrs[1]=row2;

    SeedHandler::overrideExternalSeed(1742);
    SeedHandler::turnOffLogging();

    RandomNormCorrTuples r(means,stdDevs,corrs,"::main(),r");

    abkfatal(!r.bad(),"Specified correlations are inconsistent or "
        "lead to linearly dependent vectors");
    Timer tm;
    unsigned i;
//  unsigned bins[100]={0};
    const unsigned trials=500000;
    double sumv0=0,sumv1=0,sumv2=0;
    double sumv0squared=0,sumv1squared=0,sumv2squared=0;
    double sumv0v1=0,sumv0v2=0,sumv1v2=0;
    std::vector<double> tuple(3,DBL_MAX);

    for (i=0;i<trials;i++)
        {
        r.getTuple(tuple);
        double v0=tuple[0];
        double v1=tuple[1];
        double v2=tuple[2];
        sumv0 += v0;
        sumv1 += v1;
        sumv2 += v2;
        sumv0squared += v0*v0;
        sumv1squared += v1*v1;
        sumv2squared += v2*v2;
        sumv0v1 += v0*v1;
        sumv0v2 += v0*v2;
        sumv1v2 += v1*v2;
        }

    tm.stop();


    std::cout << "Time to compute " << trials <<" correlated pairs " << tm << std::endl;


    std::cout << "Stats for v0 are: mean = " << double(sumv0)/trials 
         << " estimated Std. Dev = " << sqrt(
         (sumv0squared-(1.0/trials)*sumv0*sumv0)
         /(trials-1)
         ) << std::endl;

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

    std::cout << "Correlation(v0,v2) = " << (sumv0v2 - (1.0/trials)*sumv0*sumv2)
        / sqrt( (sumv0squared-(1.0/trials)*sumv0*sumv0)
                *
                (sumv2squared-(1.0/trials)*sumv2*sumv2) ) << 
                " (Note: biased estimator of r) " << std::endl;

    std::cout << "Correlation(v0,v1) = " << (sumv0v1 - (1.0/trials)*sumv0*sumv1)
        / sqrt( (sumv0squared-(1.0/trials)*sumv0*sumv0)
                *
                (sumv1squared-(1.0/trials)*sumv1*sumv1) ) << 
                " (Note: biased estimator of r) " << std::endl;

    std::cout << "Correlation(v1,v2) = " << (sumv1v2 - (1.0/trials)*sumv1*sumv2)
        / sqrt( (sumv1squared-(1.0/trials)*sumv1*sumv1)
                *
                (sumv2squared-(1.0/trials)*sumv2*sumv2) ) << 
                " (Note: biased estimator of r) " << std::endl;


    return 0;

    }

