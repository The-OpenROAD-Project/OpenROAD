// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbStream.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "dbDatabase.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace odb {

void dbOStream::pushScope(const std::string& name)
{
  int scope_pos = 0;
  if (db_->getLogger()->debugCheck(utl::ODB, "io_size", 1)) {
    scope_pos = pos();
  }
  scopes_.push_back({name, scope_pos});
}

void dbOStream::popScope()
{
  auto logger = db_->getLogger();
  if (logger->debugCheck(utl::ODB, "io_size", 1)) {
    auto size = pos() - scopes_.back().start_pos;
    if (size >= 1024) {  // hide tiny contributors
      std::ostringstream scope_name;

      std::ranges::transform(
          scopes_,
          std::ostream_iterator<std::string>(scope_name, "/"),
          [](const Scope& scope) { return scope.name; });

      logger->report("{:8.1f} MB in {}", size / 1048576.0, scope_name.str());
    }
  }

  scopes_.pop_back();
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

dbOStream& operator<<(dbOStream& stream, const Point3D& p)
{
  stream << p.x_;
  stream << p.y_;
  stream << p.z_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Point3D& p)
{
  stream >> p.x_;
  stream >> p.y_;
  stream >> p.z_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const Cuboid& c)
{
  stream << c.xlo_;
  stream << c.ylo_;
  stream << c.zlo_;
  stream << c.xhi_;
  stream << c.yhi_;
  stream << c.zhi_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Cuboid& c)
{
  stream >> c.xlo_;
  stream >> c.ylo_;
  stream >> c.zlo_;
  stream >> c.xhi_;
  stream >> c.yhi_;
  stream >> c.zhi_;
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

dbOStream::dbOStream(_dbDatabase* db, std::ostream& f) : f_(f)
{
  db_ = db;
  lef_dist_factor_ = 0.001;
  lef_area_factor_ = 0.000001;

  dbTech* tech = ((dbDatabase*) db)->getTech();

  if (tech && tech->getLefUnits() == 2000) {
    lef_dist_factor_ = 0.0005;
    lef_area_factor_ = 0.00000025;
  }
}

dbIStream::dbIStream(_dbDatabase* db, std::istream& f) : f_(f)
{
  db_ = db;

  lef_dist_factor_ = 0.001;
  lef_area_factor_ = 0.000001;

  dbTech* tech = ((dbDatabase*) db)->getTech();

  if (tech && tech->getLefUnits() == 2000) {
    lef_dist_factor_ = 0.0005;
    lef_area_factor_ = 0.00000025;
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

std::ostream& operator<<(std::ostream& os, const Point3D& pIn)
{
  os << "( " << pIn.x() << " " << pIn.y() << " " << pIn.z() << " )";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Cuboid& cIn)
{
  os << "( " << cIn.xMin() << " " << cIn.yMin() << " " << cIn.zMin() << " ) ( "
     << cIn.xMax() << " " << cIn.yMax() << " " << cIn.zMax() << " )";
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
