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

void chiSqCompare(RandomDouble &r0,RandomDouble &r1, const char *secDescription)
    {
    Timer tm;

    unsigned corrBins[100]={0};
    double x0,x1;
    {
    for (unsigned i=0;i<100000;i++)
        {
        x0 = r0.operator double();// .operator double();
        x1 = r1;

        corrBins[unsigned(x0*10)*10 + unsigned(x1*10)]++;
        }
    }

    tm.stop();

    std::cout << std::endl << "10x10 matrix of pairs (by tenths) from sequences "
         << secDescription << std::endl;
//         << "with seeds 10 and 11 " << std::endl;

    {
    for (unsigned i=0;i<10;i++)
        std::cout << corrBins[i*10] << " " << corrBins[i*10+1] << " " <<corrBins[i*10+2]<<" " <<
         corrBins[i*10+3] <<" " << corrBins[i*10+4] << " "
 << corrBins[i*10+5] << " " << corrBins[i*10+6] << " " << corrBins[i*10+7]
 << " " << corrBins[i*10+8] << " " << corrBins[i*10+9] <<std::endl;
    }
    std::cout << std::endl;

    double chiSq = 0;
    double diff;
    {
    for (unsigned i=0;i<100;i++)
        {
        diff = corrBins[i]-1000.0;
        chiSq += diff*diff/1000.0;
        }

    std::cout << "Chi-Square = " << chiSq << std::endl << std::endl;


    std::cout << "Time to compute 2*100000 doubles (and update bins)" << tm << std::endl;

    }
    }

int main(int argc, const char *argv[])
    {
    Verbosity verb(argc,argv);
    RandomUnsigned r17(0,100000,"commonTest16,r17",UINT_MAX,verb);
    RandomUnsigned r(0,100000,"commonTest16,r",UINT_MAX,verb);
    RandomUnsigned r18(0,100000,"commonTest16,r",UINT_MAX,verb);
    RandomUnsigned r19(0,100000,"commonTest16,r0",UINT_MAX,verb);
    Timer tm;
    unsigned x;
    int i;
//  unsigned bins[100]={0};

    for (i=0;i<5000000;i++)
        {
        x = r;
        }

    tm.stop();
  
    std::cout << "Time to compute 5000000 unsigneds" << tm << std::endl;
    std::cout << "Unsigned value following that is " << unsigned(r) << std::endl;
    RandomDouble r0(0,1,"commonTest16,r0",UINT_MAX,verb);
    RandomDouble r1(0,1,"commonTest16,r0",UINT_MAX,verb);

    chiSqCompare(r0,r1," with same local identifier");

    RandomDouble r0a(0,1,"commonTest16,r0",17,verb);
    RandomDouble r0b(0,1,"commonTest16,r0",17,verb);

    for (i=0;i<10000;i++)
        {
        abkfatal(double(r0a)==double(r0b),"Failure of counter override");
        }

    chiSqCompare(r0a,r0b,"same counter, same id");

    std::cout << "10000 doubles with the same local identifier and "
        << "counter agree.  Value following that is " << r0a << std::endl;

    RandomDouble r0c(0,1,"commonTest16,r0",UINT_MAX,verb);

    std::cout << "Testing RandomRoot inspectors:" << std::endl <<
        "\tExternalSeed=" << r0c.getExternalSeed() << " Local identifier= "
        << r0c.getLocIdent() << " counter= " << r0c.getCounter() << std::endl;
   
    RandomDouble s0(0,1,"subtlyDifferent001",33,verb);
    RandomDouble s1(0,1,"subtlyDifferent002",33,verb);
    
    chiSqCompare(s0,s1,"with same counter, different local id");

    std::cout << "now we test that all the random classes accept "
        << "loc ident and counter" << std::endl;


    RandomDouble t1(2,5,"commonTest16,t1",UINT_MAX,verb);
    RandomDouble t1a(2,5,"commonTest16,t1",UINT_MAX,verb);
    RandomDouble t1b(2,5,"commonTest16,t1",7,verb);

    RandomDouble1279 t2(2,5,"commonTest16,t2",UINT_MAX,verb);
    RandomDouble1279 t2a(2,5,"commonTest16,t2",UINT_MAX,verb);
    RandomDouble1279 t2b(2,5,"commonTest16,t2",7,verb);

    RandomRawDouble t3("commonTest16,t3",UINT_MAX,verb);
    RandomRawDouble t3a("commonTest16,t3",UINT_MAX,verb);
    RandomRawDouble t3b("commonTest16,t3",7,verb);
   
    RandomRawDouble1279 t4("commonTest16,t4",UINT_MAX,verb);
    RandomRawDouble1279 t4a("commonTest16,t4",UINT_MAX,verb);
    RandomRawDouble1279 t4b("commonTest16,t4",7,verb);

    RandomUnsigned tu1(2,5,"commonTest16,tu1",UINT_MAX,verb);
    RandomUnsigned tu1a(2,5,"commonTest16,tu1",UINT_MAX,verb);
    RandomUnsigned tu1b(2,5,"commonTest16,tu1",7,verb);

    RandomUnsigned1279 tu2(2,5,"commonTest16,tu2",UINT_MAX,verb);
    RandomUnsigned1279 tu2a(2,5,"commonTest16,tu2",UINT_MAX,verb);
    RandomUnsigned1279 tu2b(2,5,"commonTest16,tu2",7,verb);

    unsigned temp;
    RandomRawUnsigned tu3("commonTest16,tu3",UINT_MAX,verb);
    temp = tu3;
    RandomRawUnsigned tu3a("commonTest16,tu3",UINT_MAX,verb);
    temp = tu3a;
    RandomRawUnsigned tu3b("commonTest16,tu3",7,verb);
    temp = tu3b;

    RandomRawUnsigned1279 tu4("commonTest16,tu4",UINT_MAX,verb);
    temp = tu4;
    RandomRawUnsigned1279 tu4a("commonTest16,tu4",UINT_MAX,verb);
    temp = tu4a;
    RandomRawUnsigned1279 tu4b("commonTest16,tu4",7,verb);
    temp = tu4b;

    RandomNormal tn1(2,5,"commonTest16,tn1",UINT_MAX,verb);
    RandomNormal tn1a(2,5,"commonTest16,tn1",UINT_MAX,verb);
    RandomNormal tn1b(2,5,"commonTest16,tn1",7,verb);

    return 0;

    }
