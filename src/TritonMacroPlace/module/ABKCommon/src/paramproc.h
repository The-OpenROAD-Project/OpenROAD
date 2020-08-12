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









//! author="Igor Markov, June 15, 1997"

/*
 970622 ilm    Added conversions into double and char * for Param
 970622 ilm    Added type NOPARAM for no parameters.
 970723 ilm    Changed #include <bool.h> to #include <stl_config.h>
               for STL 2.01 compliance
 970820 ilm    moved enum ParamType inside class Param
               added new ctor for use with NOPARAM
               allowed for +option as well as -option
               added member bool on() to tell if it was +option
 970824 ilm    took off conversions in class Param
               defined classes NoParams, BoolParam, UnsignedParam etc
                  with unambiguous conversions and clever found()
 980313 ilm    fixed const-correctness
*/ 

#ifndef _PARAMPROC_H_
#define _PARAMPROC_H_

#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <limits.h>

//#ifndef _MSC_VER
///* Includes from STL for min/max/abs/bool/true/false/etc */
//#include <stl_config.h>
//#endif 

#include "abkassert.h"

//:(base class) Catches a given parameter from the command line 
class Param            
{               
public:
    enum Type { NOPARAM, BOOL, INT, UNSIGNED, DOUBLE, STRING };
    // NOPARAM means "empty command line"
    // BOOL means "no value: either found in command line or not"
private:
    bool          _b;  // found
    bool          _on; 
    int           _i;
    unsigned      _u;
    double        _d;
    const char *  _s;
    Type          _pt;
    const char *  _key;
public:
    Param(const char * keyy, Type part, int argc, const char * const argv[]);
    Param(Type part, int argc, const char * const argv[]); // for NOPARAM only
   ~Param() {};
    bool      found()       const; 
    // for any Param::Type, always need to check before anything else
    bool      on()          const;  
    // for any Param::Type; true if the option was invoked with +

    int       getInt()      const;
    unsigned  getUnsigned() const;
    double    getDouble()   const;
    const char* getString() const;

/*  operator  double()      const;  // deprecated : use below classes */
/*  operator  char* ()      const;  //              instead of Param */
};

//:Constructed from argc/argv, returns to true 
// if the command line had no parameters 
class NoParams  : private Param
{
public:
     NoParams(int argc, const char * const argv[]):Param(Param::NOPARAM,argc,argv) {}
     bool found()    const { return Param::found(); }
     operator bool() const { return Param::found(); }
     using Param::on;      // base class member access adjustment
};

//: Catches a given boolean parameter
class BoolParam : private Param
{
public:
    BoolParam(const char * key, int argc, const char * const argv[]) 
    : Param(key,Param::BOOL,argc,argv) {}
    bool found() const    { return Param::found(); }
    operator bool() const { return Param::found(); }
    using Param::on;      // base class member access adjustment
};

//: Catches a given Unsigned parameter
class UnsignedParam : private Param
{
public:
    UnsignedParam(const char * key, int argc, const char *const argv[])
    : Param(key,Param::UNSIGNED,argc,argv) {}
    bool found() const { return Param::found() && getUnsigned()!=unsigned(-1); }
    operator unsigned() const { return getUnsigned();  }
    using Param::on;     // base class member access adjustment
};

//: Catches a given integer parameter
class IntParam : private Param
{
public:
    IntParam(const char * key, int argc, const char * const argv[])
    : Param(key,Param::INT,argc,argv) {}
    bool found()   const { return Param::found();   }
    operator int() const { return getInt();  }
    using Param::on;      // base class member access adjustment
};

//: Catches a given double parameter
class DoubleParam : private Param
{
public:
    DoubleParam(const char * key, int argc, const char * const argv[])
    : Param(key,Param::DOUBLE,argc,argv) {}
    bool found() const { return Param::found() && getDouble()!=-1.29384756657; }
    operator double() const { return getDouble();  }
    using Param::on;      // base class member access adjustment
};

//: Catches a given string parameter
class StringParam : private Param
{
public:
    StringParam(const char * key, int argc, const char * const argv[])
    : Param(key,Param::STRING,argc,argv) {}
    bool found()     const       
          { return Param::found() && strcmp(getString(),"Uninitialized"); }
    operator const char*() const  { return getString();  }
    using Param::on;      // base class member access adjustment
};

#endif
