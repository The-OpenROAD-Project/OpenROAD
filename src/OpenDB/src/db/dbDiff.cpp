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

#include "dbDiff.h"

#include <stdarg.h>

namespace odb {

dbDiff::dbDiff(FILE* f)
{
  _f                = f;
  _indent_level     = 0;
  _deep_diff        = false;
  _has_differences  = false;
  _indent_per_level = 4;
}

dbDiff::~dbDiff()
{
  if (_f)
    fflush(_f);
}

void dbDiff::indent()
{
  int  i;
  int  n = _indent_level * _indent_per_level;
  char c = ' ';

  if (_f) {
    for (i = 0; i < n; ++i)
      fwrite(&c, 1, 1, _f);
  }
}

void dbDiff::report(const char* fmt, ...)
{
  write_headers();
  indent();

  char  buffer[16384];
  char* p = buffer;

  buffer[16384 - 1] = 0;
  va_list args;
  va_start(args, fmt);
  vsnprintf(p, 16384 - 1, fmt, args);
  va_end(args);

  if (_f)
    fwrite(buffer, 1, strlen(buffer), _f);

  _has_differences = true;
}

void dbDiff::begin(const char* field, const char* objname, uint oid)
{
  if (field) {
    if (!deepDiff())
      begin_object("<> %s (%s[%u])\n", field, objname, oid);
    else
      begin_object("<> %s (%s)\n", field, objname);
  } else {
    if (!deepDiff())
      begin_object("<> %s[%u]\n", objname, oid);
    else
      begin_object("<> %s\n", objname);
  }
}

void dbDiff::begin(const char  side,
                   const char* field,
                   const char* objname,
                   uint        oid)
{
  if (field) {
    if (!deepDiff())
      begin_object("%c %s (%s[%u])\n", side, field, objname, oid);
    else
      begin_object("%c %s (%s)\n", side, field, objname);
  } else {
    if (!deepDiff())
      begin_object("%c %s[%u]\n", side, objname, oid);
    else
      begin_object("%c %s\n", side, objname);
  }
}

void dbDiff::begin_object(const char* fmt, ...)
{
  std::string s("");

  int i;
  int n = _indent_level * _indent_per_level;
  for (i = 0; i < n; ++i)
    s += " ";

  char buffer[16384];
  buffer[16384 - 1] = 0;
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, 16384 - 1, fmt, args);
  va_end(args);

  s += buffer;
  _headers.push_back(s);
  increment();
}

void dbDiff::end_object()
{
  if (!_headers.empty())
    _headers.pop_back();
  decrement();
}

void dbDiff::write_headers()
{
  std::vector<std::string>::iterator itr;

  for (itr = _headers.begin(); itr != _headers.end(); ++itr) {
    const char* buffer = (*itr).c_str();

    if (_f)
      fwrite(buffer, 1, strlen(buffer), _f);

    _has_differences = true;
  }

  _headers.clear();
}

dbDiff& dbDiff::operator<<(bool c)
{
  if (c)
    *this << "true";
  else
    *this << "false";

  return *this;
}

dbDiff& dbDiff::operator<<(char c)
{
  int v = c;

  if (_f)
    fprintf(_f, "%d", v);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(unsigned char c)
{
  unsigned int v = c;

  if (_f)
    fprintf(_f, "%u", v);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(short c)
{
  int v = c;

  if (_f)
    fprintf(_f, "%d", v);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(unsigned short c)
{
  unsigned int v = c;

  if (_f)
    fprintf(_f, "%u", v);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(int c)
{
  if (_f)
    fprintf(_f, "%d", c);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(unsigned int c)
{
  if (_f)
    fprintf(_f, "%u", c);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(float c)
{
  double e = c;

  if (_f)
    fprintf(_f, "%g", e);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(double c)
{
  if (_f)
    fprintf(_f, "%g", c);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(long double c)
{
  if (_f)
    fprintf(_f, "%Lg", c);

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(const char* s)
{
  if (s == NULL) {
    if (_f)
      fprintf(_f, "\"\"");
  } else {
    if (_f)
      fprintf(_f, "%s", s);
  }

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(const Point& p)
{
  if (_f)
    fprintf(_f, "( %d %d )", p.getX(), p.getY());

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(const Rect& r)
{
  if (_f)
    fprintf(_f,
            "[( %d %d ) ( %d %d )]",
            r.ll().getX(),
            r.ll().getY(),
            r.ur().getX(),
            r.ur().getY());

  _has_differences = true;
  return *this;
}

dbDiff& dbDiff::operator<<(const Oct& o)
{
  if (_f)
    fprintf(_f,
            "[( %d %d ) %d ( %d %d )]",
            o.getCenterLow().getX(),
            o.getCenterLow().getY(),
            o.getWidth(),
            o.getCenterHigh().getX(),
            o.getCenterHigh().getY());

  _has_differences = true;
  return *this;
}

void dbDiff::diff(const char* field, bool lhs, bool rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, char lhs, char rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, unsigned char lhs, unsigned char rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, short lhs, short rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, unsigned short lhs, unsigned short rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, int lhs, int rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, unsigned int lhs, unsigned int rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, float lhs, float rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, double lhs, double rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, long double lhs, long double rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, Point lhs, Point rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, Rect lhs, Rect rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}
void dbDiff::diff(const char* field, Oct lhs, Oct rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, const char* lhs, const char* rhs)
{
  if (lhs && rhs) {
    if (strcmp(lhs, rhs) != 0) {
      report("< %s: ", field);
      (*this) << lhs;
      (*this) << "\n";
      report("> %s: ", field);
      (*this) << rhs;
      (*this) << "\n";
    }
  } else if (lhs) {
    report("< %s: ", field);
    (*this) << lhs;
    (*this) << "\n";
  } else if (rhs) {
    report("> %s: ", field);
    (*this) << rhs;
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*         field,
                  dbOrientType::Value lhs,
                  dbOrientType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbOrientType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbOrientType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, dbSigType::Value lhs, dbSigType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbSigType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbSigType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, dbIoType::Value lhs, dbIoType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbIoType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbIoType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*              field,
                  dbPlacementStatus::Value lhs,
                  dbPlacementStatus::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbPlacementStatus(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbPlacementStatus(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*         field,
                  dbMasterType::Value lhs,
                  dbMasterType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbMasterType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbMasterType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*            field,
                  dbTechLayerType::Value lhs,
                  dbTechLayerType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbTechLayerType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbTechLayerType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*           field,
                  dbTechLayerDir::Value lhs,
                  dbTechLayerDir::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbTechLayerDir(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbTechLayerDir(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char* field, dbRowDir::Value lhs, dbRowDir::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbRowDir(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbRowDir(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*       field,
                  dbBoxOwner::Value lhs,
                  dbBoxOwner::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbBoxOwner(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbBoxOwner(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*       field,
                  dbWireType::Value lhs,
                  dbWireType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbWireType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbWireType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*            field,
                  dbWireShapeType::Value lhs,
                  dbWireShapeType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbWireShapeType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbWireShapeType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*        field,
                  dbSiteClass::Value lhs,
                  dbSiteClass::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbSiteClass(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbSiteClass(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*        field,
                  dbOnOffType::Value lhs,
                  dbOnOffType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbOnOffType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbOnOffType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*            field,
                  dbClMeasureType::Value lhs,
                  dbClMeasureType::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbClMeasureType(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbClMeasureType(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::diff(const char*        field,
                  dbDirection::Value lhs,
                  dbDirection::Value rhs)
{
  if (lhs != rhs) {
    report("< %s: ", field);
    (*this) << dbDirection(lhs).getString();
    (*this) << "\n";
    report("> %s: ", field);
    (*this) << dbDirection(rhs).getString();
    (*this) << "\n";
  }
}

void dbDiff::out(char side, const char* field, bool value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, char value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, unsigned char value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, short value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, unsigned short value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, int value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, unsigned int value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, float value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, double value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, long double value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, Point value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, Rect value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}
void dbDiff::out(char side, const char* field, Oct value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, const char* value)
{
  report("%c %s: ", side, field);
  (*this) << (value);
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbOrientType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbOrientType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbSigType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbSigType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbIoType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbIoType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbPlacementStatus::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbPlacementStatus(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbMasterType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbMasterType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbTechLayerType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbTechLayerType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbTechLayerDir::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbTechLayerDir(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbRowDir::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbRowDir(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbBoxOwner::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbBoxOwner(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbWireType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbWireType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbWireShapeType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbWireShapeType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbSiteClass::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbSiteClass(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbOnOffType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbOnOffType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbClMeasureType::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbClMeasureType(value).getString();
  (*this) << "\n";
}

void dbDiff::out(char side, const char* field, dbDirection::Value value)
{
  report("%c %s: ", side, field);
  (*this) << dbDirection(value).getString();
  (*this) << "\n";
}

}  // namespace odb
