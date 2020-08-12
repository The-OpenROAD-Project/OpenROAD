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

#include "abktimer.h"
#include "abkrand.h"
#include "abkcpunorm.h"
#include <cstdio>
#include <list>


static const double   oneUpdateTakesXUltra1sec=0.41;
static const unsigned updateSize=500000;

unsigned CPUNormalizer::_numUpdates=0;
double   CPUNormalizer::_totTime   =0.0;
char     CPUNormalizer::_buffer[80];

double CPUNormalizer::getNormalizingFactor() const
{ return (_numUpdates * oneUpdateTakesXUltra1sec) / _totTime; }

CPUNormalizer::operator const char*() const
{ 
  if (_totTime<0) strcpy(_buffer," CPU time normalizer failed \n");
  else 
     sprintf(_buffer,
     "# Run-time conversion to Ultra-1 @143.5/71.5MHz, SunProCC4.2/-O5 : %f \n",
             getNormalizingFactor());
  return _buffer;
}

void CPUNormalizer::update()
{
  Timer tm;
  if (_numUpdates>10000) { _numUpdates=0; _totTime=0.0; }

  unsigned *sample1, *sample2, *sample3; // *sample4, *sample5;
  sample1=new unsigned[updateSize];
  sample2=new unsigned[updateSize];
  sample3=new unsigned[updateSize];
//sample4=new unsigned[updateSize];
//sample5=new unsigned[updateSize];
  
  RandomRawUnsigned ru("CPUNormalizer::update(),ru",999);
  for(unsigned k=0; k!=updateSize/3; k++)
  {
     unsigned addr=ru%updateSize;
     sample1[addr]=k / 3;
     sample2[addr]=k*k;
     sample3[addr]=0;
//   sample4[addr]=0;
//   sample5[addr]=0;
  }
  delete[] sample1;
  delete[] sample2;
  delete[] sample3;
//delete[] sample4;
//delete[] sample5;

/*list<unsigned>   sampleList(sampleVec.size());
  copy(sampleVec.begin(),sampleVec.end(),sampleList.begin());
*/
  tm.stop();
  double newTime=tm.getUserTime();
  if (newTime>0) _totTime+=newTime;
  else _totTime = -1.0;  // failed
  _numUpdates++;
//cout << "CPU sampling took : " << tm << endl;
}

