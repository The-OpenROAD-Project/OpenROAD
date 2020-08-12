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
#include <cmath>

using std::cout;
using std::endl;
using std::flush;

int main(int argc, const char *argv[])
{
  cout << TimeStamp();
  cout << CmdLine(argc,argv);

  StringParam  auxFileName("aux", argc, argv);
  BoolParam    helpRequest("help",argc,argv);
  BoolParam    timeRequest("time",argc,argv);
  BoolParam    cloak("cloak",argc,argv); // +cloak vs -cloak
  UnsignedParam  someNumber("n",argc,argv);
  DoubleParam  sanityValue("sanity",argc,argv);
 
  if (helpRequest.found() || NoParams(argc,argv))
  {
      cout<<" -aux filename.aux \n" <<
            " -help \n"<<
            " -time \n"<<
            " -n  <unsigned integer> \n"<<
            " -sanity <double> \n" <<
            " +/-cloak \n";
      return 0;
  }

  if (cloak.found() && cloak.on())
  {
      cout << " *** Cloaking device on ;-)" << endl;
      return 0;
  }  

  if (timeRequest.found())
  {
     Timer tm;
     double d;
     for(int i=0;i<100008;i++)
           d=sqrt(i*i+25.0*i+13.12);
     tm.stop();
     cout << tm << endl;
     exit(0);
  } 

  
  if (auxFileName.found())
   cout << "File name : " << auxFileName << "\n"; 
  else cout<<"Aux File Name not found\n";

  cout  << "Let's print the filename (again if found) : " << 
          auxFileName << "\n";

//  abkguess(2<1," Seems like 2<1 ");
//  abkwarn(3<1," Yey, 3<1 !");

 if (!someNumber.found())
  cout << "Some number not found\n";

//An "intentional mistake" to show what happens when
//print an uninitialized value

  cout << "Some number  : " << someNumber << "\n";

 if (!sanityValue.found())
  cout << "Sanity value not found\n";

//An "intentional mistake" to show what happens when
//print an uninitialized value

  cout << "Sanity check\n" << flush;
  cout << "Sanity value : " << flush << sanityValue << "\n";

  return 0;
}

