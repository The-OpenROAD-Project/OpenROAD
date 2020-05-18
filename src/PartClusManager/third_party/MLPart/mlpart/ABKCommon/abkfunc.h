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

#include <cmath>
#include <ABKCommon/uofm_alloc.h>

//! author=" Devendra"
/* This file contains all the declarations of functions for ABKCommon */

unsigned abkGcd(unsigned, unsigned);
unsigned abkGcd(const uofm::vector<unsigned>&);
unsigned abkFactorial(unsigned n);

//: Be true if the two string are same
class CompareNameEqual {
       public:
        CompareNameEqual() {}
        bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) == 0; }
};

inline bool equalDouble(const double a, const double b) {
        // standard def of equality
        if (a == b) return true;

        double absdiff = fabs(a - b);
        const double fraction = 1.e-6;

        // special case for a = 0 or b = 0
        if (((a == 0.) || (b == 0.)) && (absdiff < fraction)) return true;

        double absa = fabs(a);
        double absb = fabs(b);

        // difference insignificant compared to a and b
        return ((absdiff < fraction * absa) && (absdiff < fraction * absb));
}

inline bool lessOrEqualDouble(const double a, const double b) { return (a <= b) || equalDouble(a, b); }

inline bool lessThanDouble(const double a, const double b) { return (a < b) && !equalDouble(a, b); }

inline bool greaterOrEqualDouble(const double a, const double b) { return (a >= b) || equalDouble(a, b); }

inline bool greaterThanDouble(const double a, const double b) { return (a > b) && !equalDouble(a, b); }

#if defined(sun)
#define fabsf(x) ((x >= 0.f) ? x : -x)
#endif

inline bool equalFloat(const float a, const float b) {
        // standard def of equality
        if (a == b) return true;

        float absdiff = fabsf(a - b);
        const float fraction = 1.e-6f;

        // special case for a = 0 or b = 0
        if (((a == 0.) || (b == 0.)) && (absdiff < fraction)) return true;

        float absa = fabsf(a);
        float absb = fabsf(b);

        // difference insignificant compared to a and b
        return ((absdiff < fraction * absa) && (absdiff < fraction * absb));
}

inline bool lessOrEqualFloat(const float a, const float b) { return (a <= b) || equalFloat(a, b); }

inline bool lessThanFloat(const float a, const float b) { return (a < b) && !equalFloat(a, b); }

inline bool greaterOrEqualFloat(const float a, const float b) { return (a >= b) || equalFloat(a, b); }

inline bool greaterThanFloat(const float a, const float b) { return (a > b) && !equalFloat(a, b); }

#include <iostream>
#include "abklimits.h"

inline void compilerCheck(void) {
        double doubleInf = std::numeric_limits<double>::infinity();
        double doubleMax = std::numeric_limits<double>::max();
        float floatInf = std::numeric_limits<float>::infinity();
        float floatMax = std::numeric_limits<float>::max();

        if (doubleInf < doubleMax || floatInf < floatMax) {
                std::cout << std::endl << std::endl << "                 ****** WARNING ******" << std::endl << "  This binary has detected a bug in "
                                                                                                                "the code produced by your" << std::endl << "current compiler. This may be a bug "
                                                                                                                                                            "in the compiler or shared" << std::endl << "libraries on your machine. It is "
                                                                                                                                                                                                        "likely that binaries compiled" << std::endl << "in this setup may not behave "
                                                                                                                                                                                                                                                        "properly. If you insist on using" << std::endl << "this binary which may behave "
                                                                                                                                                                                                                                                                                                           "unexpectedly, you must remove this" << std::endl << "bug check and recompile." << std::endl << std::endl;
                // exit(0);
        }
}
