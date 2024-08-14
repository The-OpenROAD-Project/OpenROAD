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

#include <endian.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "gdsin.h"
#include "odb/db.h"

namespace odb {

class dbGDSLib;
class dbGDSElement;
class dbGDSBoundary;
class dbGDSPath;
class dbGDSSRef;
class dbGDSStructure;

class GDSWriter
{
 public:
  GDSWriter();

  ~GDSWriter();

  void write_gds(dbGDSLib* lib, const std::string& filename);

 private:
  std::ofstream _file;
  dbGDSLib* _lib;

  void calcRecSize(record_t& r);
  void writeRecord(record_t& r);

  void writeReal8(double real);
  void writeInt32(int32_t i);
  void writeInt16(int16_t i);
  void writeInt8(int8_t i);

  void writeLayer(dbGDSElement* el);
  void writeXY(dbGDSElement* el);
  void writeDataType(dbGDSElement* el);
  void writeEndel();
  void writePropAttr(dbGDSElement* el);

  void writeLib();
  void writeStruct(dbGDSStructure* str);
  void writeElement(dbGDSElement* el);
  void writeBoundary(dbGDSBoundary* bnd);
  void writePath(dbGDSPath* path);
  void writeSRef(dbGDSSRef* sref);
  void writeText(dbGDSText* text);

  void writeSTrans(const dbGDSSTrans& strans);
  void writeTextPres(const dbGDSTextPres& pres);
};

}  // namespace odb