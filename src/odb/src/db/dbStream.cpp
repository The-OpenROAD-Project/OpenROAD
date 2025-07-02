// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbStream.h"

#include <iostream>
#include <sstream>
#include <string>

#include "dbDatabase.h"
#include "odb/db.h"

namespace odb {

void dbOStream::pushScope(const std::string& name)
{
  int scope_pos = 0;
  if (_db->getLogger()->debugCheck(utl::ODB, "io_size", 1)) {
    scope_pos = pos();
  }
  _scopes.push_back({name, scope_pos});
}

void dbOStream::popScope()
{
  auto logger = _db->getLogger();
  if (logger->debugCheck(utl::ODB, "io_size", 1)) {
    auto size = pos() - _scopes.back().start_pos;
    if (size >= 1024) {  // hide tiny contributors
      std::ostringstream scope_name;

      std::transform(_scopes.begin(),
                     _scopes.end(),
                     std::ostream_iterator<std::string>(scope_name, "/"),
                     [](const Scope& scope) { return scope.name; });

      logger->report("{:8.1f} MB in {}", size / 1048576.0, scope_name.str());
    }
  }

  _scopes.pop_back();
}

dbOStream& operator<<(dbOStream& stream, const Rect& r)
{
  stream << r.xlo_;
  stream << r.ylo_;
  stream << r.xhi_;
  stream << r.yhi_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Rect& r)
{
  stream >> r.xlo_;
  stream >> r.ylo_;
  stream >> r.xhi_;
  stream >> r.yhi_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const Polygon& p)
{
  stream << p.points_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Polygon& p)
{
  stream >> p.points_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const Line& l)
{
  stream << l.pt0_;
  stream << l.pt1_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Line& l)
{
  stream >> l.pt0_;
  stream >> l.pt1_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const Point& p)
{
  stream << p.x_;
  stream << p.y_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Point& p)
{
  stream >> p.x_;
  stream >> p.y_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const Oct& o)
{
  stream << o.center_high_;
  stream << o.center_low_;
  stream << o.A_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Oct& o)
{
  stream >> o.center_high_;
  stream >> o.center_low_;
  stream >> o.A_;
  return stream;
}

dbOStream::dbOStream(_dbDatabase* db, std::ostream& f) : _f(f)
{
  _db = db;
  _lef_dist_factor = 0.001;
  _lef_area_factor = 0.000001;

  dbTech* tech = ((dbDatabase*) db)->getTech();

  if (tech && tech->getLefUnits() == 2000) {
    _lef_dist_factor = 0.0005;
    _lef_area_factor = 0.00000025;
  }
}

dbIStream::dbIStream(_dbDatabase* db, std::istream& f) : _f(f)
{
  _db = db;

  _lef_dist_factor = 0.001;
  _lef_area_factor = 0.000001;

  dbTech* tech = ((dbDatabase*) db)->getTech();

  if (tech && tech->getLefUnits() == 2000) {
    _lef_dist_factor = 0.0005;
    _lef_area_factor = 0.00000025;
  }
}

std::ostream& operator<<(std::ostream& os, const Rect& box)
{
  os << "( " << box.xMin() << " " << box.yMin() << " ) ( " << box.xMax() << " "
     << box.yMax() << " )";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Point& pIn)
{
  os << "( " << pIn.x() << " " << pIn.y() << " )";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Orientation2D& ori)
{
  if (ori == horizontal) {
    os << "horizontal";
  } else {
    os << "vertical";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Orientation3D& ori)
{
  if (ori == horizontal) {
    os << "horizontal";
  } else if (ori == vertical) {
    os << "vertical";
  } else {
    os << "proximal";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Direction1D& dir)
{
  if (dir == low) {
    os << "low";
  } else {
    os << "high";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Direction2D& dir)
{
  if (dir == north) {
    os << "north";
  } else if (dir == south) {
    os << "south";
  } else if (dir == west) {
    os << "west";
  } else {
    os << "east";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Direction3D& dir)
{
  if (dir == north) {
    os << "north";
  } else if (dir == south) {
    os << "south";
  } else if (dir == west) {
    os << "west";
  } else if (dir == east) {
    os << "east";
  } else if (dir == up) {
    os << "up";
  } else {
    os << "down";
  }
  return os;
}

}  // namespace odb
