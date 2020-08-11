/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_H_
#define __CORE_H_

#include "Coordinate.h"

namespace ioPlacer {

class Core {
       private:
        Coordinate _lowerBound;
        Coordinate _upperBound;
        unsigned _minDstPinsX;
        unsigned _minDstPinsY;
        unsigned _initTracksX;
        unsigned _initTracksY;
        unsigned _numTracksX;
        unsigned _numTracksY;
        unsigned _minAreaX;
        unsigned _minAreaY;
        unsigned _minWidthX;
        unsigned _minWidthY;
        DBU _databaseUnit;

       public:
        Core()
            : _lowerBound(Coordinate(0, 0)),
              _upperBound(Coordinate(0, 0)),
              _minDstPinsX(20),
              _minDstPinsY(20){};
        Core(const Coordinate& lowerBound, const Coordinate& upperBound,
             const DBU& minDstPinsX, const DBU& minDstPinsY,
             const DBU& initTracksX, const DBU& initTracksY,
             const DBU& numTracksX, const DBU& numTracksY,
             const DBU& minAreaX, const DBU& minAreaY,
             const DBU& minWidthX, const DBU& minWidthY,
             const DBU& databaseUnit)
            : _lowerBound(lowerBound),
              _upperBound(upperBound),
              _minDstPinsX(minDstPinsX),
              _minDstPinsY(minDstPinsY),
              _initTracksX(initTracksX),
              _initTracksY(initTracksY),
              _numTracksX(numTracksX),
              _numTracksY(numTracksY),
              _minAreaX(minAreaX),
              _minAreaY(minAreaY),
              _minWidthX(minWidthX),
              _minWidthY(minWidthY),
              _databaseUnit(databaseUnit){}

        Coordinate getLowerBound() const { return _lowerBound; }
        Coordinate getUpperBound() const { return _upperBound; }
        unsigned getMinDstPinsX() const { return _minDstPinsX; }
        unsigned getMinDstPinsY() const { return _minDstPinsY; }
        unsigned getInitTracksX() const { return _initTracksX; }
        unsigned getInitTracksY() const { return _initTracksY; }
        unsigned getNumTracksX() const { return _numTracksX; }
        unsigned getNumTracksY() const { return _numTracksY; }
        unsigned getMinAreaX() const { return _minAreaX; }
        unsigned getMinAreaY() const { return _minAreaY; }
        unsigned getMinWidthX() const { return _minWidthX; }
        unsigned getMinWidthY() const { return _minWidthY; }
        DBU getDatabaseUnit() const { return _databaseUnit; }

        DBU getPerimeter();
};

}

#endif /* __CORE_H_ */
