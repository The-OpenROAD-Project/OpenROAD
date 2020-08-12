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

using std::cout;
using std::endl;

int main(void)
    {
    RandomUnsigned r(0,100000,2);
    Timer tm;
    unsigned x;
    int i;
//  unsigned bins[100]={0};

    for (i=0;i<500000;i++)
        {
        x = r;
        }

    tm.stop();


    cout << "Time to compute 500000 unsigneds" << tm << endl;
    cout << "Unsigned value following that is " << unsigned(r) << endl;

    tm.start();
    RandomUnsigned1279 r1279(0,100000,2);

    for (i=0;i<500000;i++)
        {
        x = r1279;
        }

    tm.stop();


    cout << "Time to compute 500000 unsigneds using r1279 is " << tm << endl;
    cout << "Unsigned value following that is " << unsigned(r1279) << endl;

    cout << "500 unsigneds, starting with no. 1000 in seq:" << endl;
    unsigned n0,n1,n2,n3,n4;
    RandomRawUnsigned rRaw(2);

    for (i=0;i<1000;i++)
        n0 = rRaw;

    for (i=0;i<100;i++)
        {
        n0=rRaw;n1=rRaw;n2=rRaw;n3=rRaw;n4=rRaw;
        cout << n0 << " " << n1 << " " << n2 << " "
             << n3 << " " << n4 << endl;
        }

    tm.start();

    RandomDouble r0(0,1,unsigned(0));
    RandomDouble r1(0,1,1);
//    RandomDouble r0(0,1,10);
//    RandomDouble r1(0,1,11);
    unsigned corrBins[100]={0};
    double x0,x1;
    for (i=0;i<100000;i++)
        {
        x0 = r0;
        x1 = r1;

        corrBins[unsigned(x0*10)*10 + unsigned(x1*10)]++;
        }

    tm.stop();

    cout << endl << "10x10 matrix of pairs (by tenths) from sequences "
         << "with seeds 0 and 1 " << endl;
//         << "with seeds 10 and 11 " << endl;

    for (i=0;i<10;i++)
        cout << corrBins[i*5] << " " << corrBins[i*5+1] << " " <<corrBins[i*5+2]<<" " <<
         corrBins[i*5+3] <<" " << corrBins[i*5+4] << " "
 << corrBins[i*5+5] << " " << corrBins[i*5+6] << " " << corrBins[i*5+7]
 << " " << corrBins[i*5+8] << " " << corrBins[i*5+9] <<endl;

    cout << endl;

    double chiSq = 0;
    double diff;
    for (i=0;i<100;i++)
        {
        diff = corrBins[i]-1000.0;
        chiSq += diff*diff/1000.0;
        }

    cout << "Chi-Square = " << chiSq << endl << endl;


    cout << "Time to compute 2*100000 doubles (and update bins)" << tm << endl;

    RandomDouble1279 rd1279(3,5,278);

    for (i=0;i<5000;i++)
        x0 = rd1279;

    cout << "Reference value for RandomDouble1279 is "<< x0 << endl;

    RandomRawUnsigned rru(8);
    for (i=0;i<5000;i++)
        n0 = rru;

    cout << "Reference value for RandomRawUnsigned is "<< n0 << endl;

    RandomRawUnsigned1279 rru1279(8);
    for (i=0;i<5000;i++)
        n0 = rru1279;

    cout << "Reference value for RandomRawUnsigned1279 is "<< n0 << endl;

    RandomRawDouble rrd(3);
    for (i=0;i<5000;i++)
        x0 = rrd;

    cout << "Reference value for RandomRawDouble is " << x0 << endl;

    RandomRawDouble1279 rrd1279(3);
    for (i=0;i<5000;i++)
        x0 = rrd1279;

    cout << "Reference value for RandomRawDouble1279 is " << x0 << endl;

    RandomNormal rn(17,5,4);
    for (i=0;i<5000;i++)
        x0 = rn;

    x1 = rn;

    cout << "Reference values for RandomNormal are " << x0 << " " << x1 <<endl;

    RandomNormal1279 rn1279(17,5,4);
    for (i=0;i<5000;i++)
        x0 = rn1279;

    x1 = rn;

    cout << "Reference values for RandomNormal1279 are " << x0 << " "<< x1 
         << endl;




    return 0;

    }
