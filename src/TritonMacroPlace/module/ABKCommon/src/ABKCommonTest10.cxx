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
// see verbosity.h

using std::cout;
using std::endl;

int main()
{

 // ===== Initializing verbosity ======

   Verbosity verbsty;
   cout <<  verbsty << endl;    // Default is "silent"

   cout << Verbosity("silent") << endl;

   Verbosity verbsty3("1 3 5 7 ");  // to be used with command line args
// Verbosity verbsty3("1_3_5_7");   // same effect
   cout << verbsty3 << endl;

   // initialization with variable number of args.
   // #args is the first arg 
   Verbosity verbsty2(6,3,2,0,4,1,0); 
   cout << verbsty2 << endl;

   std::vector<unsigned> levs(4,0);
   //std::iota(levs.begin(),levs.end(),2);
   {for (unsigned i=0;i<4;i++) levs[i]=i+2;}
   Verbosity verbsty1(levs);    

   cout << verbsty1 << endl;

// ==== accessing and modifying named verbosity levels

   cout << " For actions : " << verbsty1.getForActions() << endl << endl;
   verbsty1.setForActions(5);

   cout << verbsty1 << endl;
   cout << " For actions : " << verbsty1.getForActions() << endl << endl;

// ===== using verbosity levels to filter debug output

   if (verbsty1.getForActions() > 2) 
   cout << " Debug output for actions" <<
           " to be printed only for levels >2" << endl;
}
