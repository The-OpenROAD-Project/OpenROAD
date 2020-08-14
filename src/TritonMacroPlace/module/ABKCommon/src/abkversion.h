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


//! author="Igor Markov, Feb 8, 1999"
/* name Purpose of abkversion.h 
 This header should only be #included into the .cxx files that
 define main()  DO NOT INCLUDE THIS FILE INTO OTHER HEADERS

 The sole purpose of this header is to define the PRINT_VERSION_INFO macro
 that can be included into main() (only!) This macro does not assume anything,
 but use additional info passed through -Dsymbol=value at compiler call.
 Package name and version in doublequotes can be passed via __ABKPACKAGE__ 
 System specifics in doublequotes can be passed via __ABKSYSTEM__
 user@domainname in doublequotes can be passed via  __ABKUSER__
 Typically those will be packed into ABKSYMBOLS in the Makefile
 and {ABKSYMBOLS} added to the CC call in the .cxx.o rule

 The below code has been tested with SunPro CC and gcc 2.91 (egcs-1.1.1)
 It should work with other compilers (e.g. should recognize unix and
 microsoft visual C++), but will print less information.
 You are welcome to adjust this code to recognize additional compilers
 and their versions (version number of MSVC++ ins't currently caught)
*/
#ifndef ABKVERSION_H_
#define ABKVERSION_H_

#include <iostream> 
#include <iomanip> 

#ifdef unix
#include "pwd.h"
#include "unistd.h"
#endif

#ifdef __ABKPACKAGE__ 
  #define  _ABKPACKAGE_  __ABKPACKAGE__  <<  " b"
#else
  #define  _ABKPACKAGE_ "B"
#endif 

#ifdef __ABKSYSTEM__
  #define _ABKSYSTEM_ " on " <<  __ABKSYSTEM__ 
#elif __unix__
  #define _ABKSYSTEM_ " on Unix "
#else
  #define _ABKSYSTEM_ ""
#endif

#ifdef __ABKLIBBASE__
  #define _ABKLIBBASE_ "in " __ABKLIBBASE__
#else
  #define _ABKLIBBASE_ " "
#endif

#ifdef __GNUC__
//#define _COMPILER_ "GNU C/C++ "<<__GNUC__<<"."<<__GNUC_MINOR__
  #define _COMPILER_ "GNU C/C++ "<<__VERSION__
#elif defined(__SUNPRO_CC)
 #define _COMPILER_ "SUN Pro CC "<<std::setbase(16)<<__SUNPRO_CC << std::setbase(10)
#elif defined(_MSC_VER)
 #define _COMPILER_ "MicroSoft Visual C++ "
#else
 #define _COMPILER_ "unknown"
#endif

#ifdef __ABKUSER__
  #define _ABKUSER_ " by " << __ABKUSER__ << " "
#else
  #define _ABKUSER_ ""
#endif

#define PRINT_VERSION_INFO\
   std::cout << "# "<< _ABKPACKAGE_ <<  "uilt on " << __DATE__ \
	<< " at " << __TIME__ << "\n# "<< _ABKSYSTEM_ << _ABKUSER_ << std::endl;\
   std::cout << "#  from " << __FILE__<<" and libraries "<< _ABKLIBBASE_ << std::endl;\
   std::cout << "#  using "<< _COMPILER_ << " compiler " << std::endl;
#endif
