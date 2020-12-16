///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#pragma once

#include <vector>

#include "odb.h"
#include "defiMisc.hpp"
#include "definTypes.h"
#include "dbTypes.h"
#include "geom.h"

namespace odb {

class dbBlock;
class dbTech;

class definBase
{
 public:
  dbTech*  _tech;
  dbBlock* _block;
  int      _errors;
  int      _dist_factor;

  definBase();
  virtual ~definBase() {}
  void         setTech(dbTech* tech);
  void         setBlock(dbBlock* block);
  void         units(int d);
  virtual void init();

  int dbdist(int value) { return (int) (value * _dist_factor); }

  int dbdist(double value)
  {
    if (value < 0.0)
      return (int) (value * _dist_factor - 0.5);
    else
      return (int) (value * _dist_factor + 0.5);
  }

  int round(double value)
  {
    if (value < 0.0)
      return (int) (value - 0.5);
    else
      return (int) (value + 0.5);
  }

  void translate(const std::vector<defPoint>& defpoints,
                 std::vector<Point>&       points)
  {
    points.clear();
    std::vector<defPoint>::const_iterator itr;

    for (itr = defpoints.begin(); itr != defpoints.end(); ++itr) {
      const defPoint& p = *itr;
      Point        point(dbdist(p._x), dbdist(p._y));
      points.push_back(point);
    }
  }

  void translate(const defiPoints& defpoints, std::vector<Point>& points)
  {
    points.clear();

    for (int i = 0; i < defpoints.numPoints; ++i) {
      Point point(dbdist(defpoints.x[i]), dbdist(defpoints.y[i]));
      points.push_back(point);
    }
  }

  static dbOrientType translate_orientation(int orient);
};

}  // namespace odb
