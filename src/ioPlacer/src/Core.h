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

#include "opendb/geom.h"

namespace ppl {

using odb::Point;
using odb::Rect;

class Core
{
 private:
  Rect _boundary;
  int _minDstPinsX;
  int _minDstPinsY;
  int _initTracksX;
  int _initTracksY;
  int _numTracksX;
  int _numTracksY;
  int _minAreaX;
  int _minAreaY;
  int _minWidthX;
  int _minWidthY;
  int _databaseUnit;

 public:
  Core() = default;
  Core(const Rect& boundary,
       const int& minDstPinsX,
       const int& minDstPinsY,
       const int& initTracksX,
       const int& initTracksY,
       const int& numTracksX,
       const int& numTracksY,
       const int& minAreaX,
       const int& minAreaY,
       const int& minWidthX,
       const int& minWidthY,
       const int& databaseUnit)
      : _boundary(boundary),
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
        _databaseUnit(databaseUnit)
  {
  }

  Rect getBoundary() const { return _boundary; }
  int getMinDstPinsX() const { return _minDstPinsX; }
  int getMinDstPinsY() const { return _minDstPinsY; }
  int getInitTracksX() const { return _initTracksX; }
  int getInitTracksY() const { return _initTracksY; }
  int getNumTracksX() const { return _numTracksX; }
  int getNumTracksY() const { return _numTracksY; }
  int getMinAreaX() const { return _minAreaX; }
  int getMinAreaY() const { return _minAreaY; }
  int getMinWidthX() const { return _minWidthX; }
  int getMinWidthY() const { return _minWidthY; }
  int getDatabaseUnit() const { return _databaseUnit; }

  int getPerimeter();
};

}  // namespace ppl

#endif /* __CORE_H_ */
