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
#include "abktempl.h"
#include <iostream>

int main()
{
  std::cout << SgnPartOfFileName("/home/userd/code/OUR/ABKCommon/main2.cxx")<<std::endl;
  std::cout << " Random number generator tests " << std::endl;
  RandomUnsigned ru(2,5);
  RandomDouble   rd(2,5);
  double dsum=0;
  double sum=0;
   int i;
  for(i=0;i<50000;i++)
  {
     double   d=rd;
     unsigned u=ru;
     dsum+=d;
     sum+=u; 
  }
//  std::cout << dsum/50000 << " " << sum/50000 << std::endl;
  unsigned ruSeed = ru.getSeed();
  unsigned rdSeed = rd.getSeed();

  //ru.seed(ru.getSeed());
  //rd.seed(rd.getSeed());
  RandomUnsigned ru2(2,5,ruSeed);
  RandomDouble   rd2(2,5,rdSeed);

  if (std::abs(dsum/50000-3.5)<1e-1 && std::abs(sum/50000-3)<1e-1) 
       std::cout << " Averages correct "<<std::endl;
  else std::cout << " Averages incorrect "<<std::endl;
  double oldsum=sum, olddsum=dsum;
  sum=dsum=0.0; 
  for(i=0;i<50000;i++)
  {
     double   d=rd2;
     unsigned u=ru2;
     dsum+=d;
     sum+=u;
  }
//  std::cout << dsum/50000 << " " << sum/50000 << std::endl;
  if (sum==oldsum && dsum == olddsum) std::cout << " Preseeding works " << std::endl;
/*
  for(i=0;i<30;i++)
  {
     double   d=rd;
     unsigned u=ru;
     std::cout << d << " " << u << std::endl;
  }
*/
  return 0;
}
