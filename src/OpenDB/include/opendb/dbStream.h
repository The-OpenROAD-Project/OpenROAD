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

#include <string.h>

#include "ZException.h"
#include "odb.h"

namespace odb {

class _dbDatabase;

class dbOStream
{
  _dbDatabase* _db;
  FILE*        _f;
  double       _lef_area_factor;
  double       _lef_dist_factor;

  void write_error()
  {
    throw ZIOError(ferror(_f),
                   "write failed on database stream; system io error: ");
  }

 public:
  dbOStream(_dbDatabase* db, FILE* f);

  _dbDatabase* getDatabase() { return _db; }

  dbOStream& operator<<(bool c)
  {
    unsigned char b = (c == true ? 1 : 0);
    return *this << b;
  }

  dbOStream& operator<<(char c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(unsigned char c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(short c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(unsigned short c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(int c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(unsigned int c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(float c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(double c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(long double c)
  {
    int n = fwrite(&c, sizeof(c), 1, _f);
    if (n != 1)
      write_error();
    return *this;
  }

  dbOStream& operator<<(const char* c)
  {
    if (c == NULL) {
      *this << 0;
    } else {
      int l = strlen(c) + 1;
      *this << l;
      int n = fwrite(c, l, 1, _f);
      if (n != 1)
        write_error();
    }

    return *this;
  }

  void markStream()
  {
    int marker = ftell(_f);
    int magic  = 0xCCCCCCCC;
    *this << magic;
    *this << marker;
  }

  double lefarea(int value) { return ((double) value * _lef_area_factor); }

  double lefdist(int value) { return ((double) value * _lef_dist_factor); }
};

class dbIStream
{
  FILE*        _f;
  _dbDatabase* _db;
  double       _lef_area_factor;
  double       _lef_dist_factor;

  void read_error()
  {
    if (feof(_f))
      throw ZException(
          "read failed on database stream (unexpected end-of-file encounted).");
    else
      throw ZIOError(ferror(_f),
                     "read failed on database stream; system io error: ");
  }

 public:
  dbIStream(_dbDatabase* db, FILE* f);

  _dbDatabase* getDatabase() { return _db; }

  dbIStream& operator>>(bool& c)
  {
    unsigned char b;
    *this >> b;
    c = (b == 1 ? true : false);
    return *this;
  }

  dbIStream& operator>>(char& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(unsigned char& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(short& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(unsigned short& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(int& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(unsigned int& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(float& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(double& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(long double& c)
  {
    int n = fread(&c, sizeof(c), 1, _f);
    if (n != 1)
      read_error();
    return *this;
  }

  dbIStream& operator>>(char*& c)
  {
    int l;
    *this >> l;

    if (l == 0)
      c = NULL;
    else {
      c     = (char*) malloc(l);
      int n = fread(c, l, 1, _f);
      if (n != 1)
        read_error();
    }

    return *this;
  }

  void checkStream()
  {
    int marker = ftell(_f);
    int magic  = 0xCCCCCCCC;
    int smarker;
    int smagic;
    *this >> smagic;
    *this >> smarker;
    ZASSERT((magic == smagic) && (marker == smarker));
  }

  double lefarea(int value) { return ((double) value * _lef_area_factor); }

  double lefdist(int value) { return ((double) value * _lef_dist_factor); }
};

}  // namespace odb


