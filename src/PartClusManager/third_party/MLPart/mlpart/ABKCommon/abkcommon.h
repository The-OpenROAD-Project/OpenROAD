/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
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

//! author="Igor Markov "

/*  This file to be included into all projects in the group

 970723 iml    Changed #include <bool.h> to #include <stl_config.h>
               for STL 2.01 compliance
 970808 mro    added pragmas to get rid of MSVC++ warnings
 970820 ilm    stl_config.h is now not included on MS VC++
*/
#ifndef _ABKCOMMON_H_
#define _ABKCOMMON_H_

#include <stdlib.h>

/* #ifdef _MSC_VER
 #pragma warning (disable : 4786) // will be defaulted at the end of this file
 #endif
*/
/* This has been commented out because it should go
 into stlcomp.h (see below) IF you use MS VC++.
 Add the following three lines right after the
 #ifndef __STLCOMP_H #define __STLCOMP_H pair:

 #pragma warning(disable:4227)
 #pragma warning(disable:4804)
 #pragma warning(disable:4786)
*/
/*
#ifndef _MSC_VER
// Includes from STL for min/max/abs/bool/true/false/etc
#include <stl_config.h>
#endif
*/

/* Added by Xiaojian 08-12-02 */
#ifdef _MSC_VER
#ifndef rint
#define rint(a) floor((a) + 0.5)
#endif
#endif

#include <utility>

#include "abkstring.h"
#include "abkconst.h"
#include "abktypes.h"
#include "abkassert.h"
#include "abktempl.h"
#include "infolines.h"
#include "abkcpunorm.h"
#include "paramproc.h"
#include "abktimer.h"
#include "abkrand.h"
#include "abkio.h"
//#include "pathDelims.h"
#include "verbosity.h"
#include "abkfunc.h"

#include "config.h"

/* #ifdef _MSC_VER
 #pragma warning (default : 4786)
 #endif
*/
#endif
