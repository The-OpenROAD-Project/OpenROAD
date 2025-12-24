// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <vector>

#include "defiMisc.hpp"
#include "definTypes.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}

namespace odb {

class dbBlock;
class dbTech;

class definBase
{
 public:
  defin::MODE _mode;
  dbTech* _tech;
  dbBlock* _block;
  utl::Logger* _logger;
  int _errors;
  int _dist_factor;

  definBase();
  virtual ~definBase() = default;
  void setTech(dbTech* tech);
  void setBlock(dbBlock* block);
  void setLogger(utl::Logger* logger);
  void units(int d);
  void setMode(defin::MODE mode);

  int dbdist(int value) { return value * _dist_factor; }

  int dbdist(double value) { return lround(value * _dist_factor); }

  void translate(const std::vector<defPoint>& defpoints,
                 std::vector<Point>& points)
  {
    points.clear();
    std::vector<defPoint>::const_iterator itr;

    for (itr = defpoints.begin(); itr != defpoints.end(); ++itr) {
      const defPoint& p = *itr;
      Point point(dbdist(p._x), dbdist(p._y));
      points.push_back(point);
    }
  }

  void translate(const DefParser::defiPoints& defpoints,
                 std::vector<Point>& points)
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
