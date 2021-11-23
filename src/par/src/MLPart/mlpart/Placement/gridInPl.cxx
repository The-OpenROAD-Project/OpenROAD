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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "gridInPl.h"

GridInPlacement::GridInPlacement(unsigned xSize, unsigned ySize, const Placement& pl) : _xSize(xSize), _ySize(ySize), _bbox(pl.getBBox()), _pointIdcs(new unsigned[xSize * ySize]) {
        abkfatal(xSize != 0 && ySize != 0, "Grid with dimension(s) zero");
        unsigned* allPoints = new unsigned[pl.getSize()];
        unsigned i;
        for (i = 0; i != pl.getSize(); i++) allPoints[i] = i;
        std::sort(allPoints, allPoints + pl.getSize(), CompareX(pl));
        double xStepSize = (pl.getSize() * 1.0) / xSize;
        unsigned curPtIdx = 0;
        for (i = 0; i != xSize; i++) {
                unsigned beginXIdx = static_cast<unsigned>(ceil(xStepSize * i));
                unsigned endXIdx = static_cast<unsigned>(ceil(xStepSize * (i + 1)));
                std::sort(allPoints + beginXIdx, allPoints + endXIdx, CompareY(pl));
                double yStepSize = (endXIdx - beginXIdx) / (ySize * 1.0);
                for (unsigned j = 0; j != ySize; j++) {
                        unsigned offset = static_cast<unsigned>(ceil(yStepSize * (j + 0.5)));
                        _pointIdcs[curPtIdx++] = allPoints[beginXIdx + offset];
                }
        }
        delete[] allPoints;
}
