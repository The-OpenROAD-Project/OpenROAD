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

//! author=" July 16, 1997 by Igor Markov"

#ifndef _DIMENSIONS_
#define _DIMENSIONS_

#include <iostream>
#include <iomanip>
#include "ABKCommon/abkcommon.h"

class Dimensions;
typedef Dimensions RandomDimensions;

//: Represents dimensions (e.g. X dimensions) of a group
//  of objects, e.g. standard cells. Can generate randomized
//  sets of dimensions with certain statistical properties.

class Dimensions {
       protected:
        uofm::vector<double> _dims;
        // vector of dimensions
        double _totalDim;
        double _minDim;
        double _stepDim;

       public:
        Dimensions(unsigned num, double total, double min = -1, double step = -1);
        Dimensions(const Dimensions& dims);
        const double& operator[](unsigned i) const { return _dims[i]; }
        unsigned getSize() const { return _dims.size(); }
        double getTotal() const { return _totalDim; }
        double getMin() const { return _minDim; }
        double getStep() const { return _stepDim; }
};

Dimensions::Dimensions(unsigned num, double total, double min, double step) : _dims(num, min), _totalDim(total), _minDim(min), _stepDim(step) {
        abkfatal(num * min <= total, "Can\'t set dimensions: min dim too big");
        if (_minDim == -1) {
                _minDim = _totalDim / (10 * num);
                for (unsigned i = 0; i < _dims.size(); i++) _dims[i] = _minDim;
        }
        if (_stepDim == -1) _stepDim = _minDim;
        RandomUnsigned randomIndex(0, num);
        double curTotal = num * _minDim;
        while ((_totalDim - curTotal) > 1e-6) {
                _dims[randomIndex] += _stepDim;
                curTotal += _stepDim;
        }
}

Dimensions::Dimensions(const Dimensions& dims) : _dims(dims._dims), _totalDim(dims._totalDim), _minDim(dims._minDim), _stepDim(dims._stepDim) {}

std::ostream& operator<<(std::ostream& out, const Dimensions& dims) {
        out << " Items: " << dims.getSize() << "  Total dimension: " << dims.getTotal() << "  Min dimension: " << dims.getMin() << "  Step: " << dims.getStep() << std::endl;
        for (unsigned i = 0; i < dims.getSize(); i++) {
                out << std::setw(7) << dims[i] << " ";
                if ((i + 1) % 10 == 0) out << "\n";
        }
        out << std::endl;
        return out;
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "071697, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
