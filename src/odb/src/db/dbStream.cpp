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

#include "dbStream.h"

#include "db.h"

namespace odb {

dbOStream& operator<<(dbOStream& stream, const Rect& r)
{
  stream << r._xlo;
  stream << r._ylo;
  stream << r._xhi;
  stream << r._yhi;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Rect& r)
{
  stream >> r._xlo;
  stream >> r._ylo;
  stream >> r._xhi;
  stream >> r._yhi;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const Point& p)
{
  stream << p._x;
  stream << p._y;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Point& p)
{
  stream >> p._x;
  stream >> p._y;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const Oct& o)
{
  stream << o.center_high;
  stream << o.center_low;
  stream << o.A;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, Oct& o)
{
  stream >> o.center_high;
  stream >> o.center_low;
  stream >> o.A;
  return stream;
}

dbOStream::dbOStream(_dbDatabase* db, FILE* f)
{
  _db = db;
  _f = f;
  _lef_dist_factor = 0.001;
  _lef_area_factor = 0.000001;

  dbTech* tech = ((dbDatabase*) db)->getTech();

  if (tech && tech->getLefUnits() == 2000) {
    _lef_dist_factor = 0.0005;
    _lef_area_factor = 0.00000025;
  }
}

dbIStream::dbIStream(_dbDatabase* db, FILE* f)
{
  _db = db;
  _f = f;

  _lef_dist_factor = 0.001;
  _lef_area_factor = 0.000001;

  dbTech* tech = ((dbDatabase*) db)->getTech();

  if (tech && tech->getLefUnits() == 2000) {
    _lef_dist_factor = 0.0005;
    _lef_area_factor = 0.00000025;
  }
}

}  // namespace odb
