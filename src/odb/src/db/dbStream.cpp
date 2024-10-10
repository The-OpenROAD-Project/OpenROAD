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

#include "odb/dbStream.h"

#include <iostream>
#include <sstream>

#include "dbDatabase.h"
#include "odb/db.h"

namespace odb {

void dbOStream::pushScope(const std::string& name)
{
  _scopes.push_back({name, pos()});
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
