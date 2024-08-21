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
  /**
   * Constructor for GDSReader
   * No operations are performed in the constructor
   */
  GDSWriter();

  /**
   * Destructor
   *
   * Does not free the dbGDSLib objects, as they are owned by the database
   */
  ~GDSWriter();

  /**
   * Writes a dbGDSLib object to a GDS file
   *
   * @param filename The path to the output file
   * @param lib The dbGDSLib object to write
   * @throws std::runtime_error if the file cannot be opened, or if the GDS is
   * corrupted
   */
  void write_gds(dbGDSLib* lib, const std::string& filename);

 private:
  /** Output filestream */
  std::ofstream _file;
  /** Current dbGDSLib object */
  dbGDSLib* _lib;

  /**
   * Calculates and sets the size of a record
   * 
   * @param r The record to evaluate
   */
  void calcRecSize(record_t& r);

  /**
   * Writes a record to the output file stream
   * 
   * @param r The record to write
   */
  void writeRecord(record_t& r);

  /**
   * Writes a real8 to _file
   *
   * NOTE: real8 is not the same as double. This conversion is not lossless.
   */
  void writeReal8(double real);

  /** Writes an int32 to _file */
  void writeInt32(int32_t i);

  /** Writes an int16 to _file */
  void writeInt16(int16_t i);

  /** Writes an int8 to _file */
  void writeInt8(int8_t i);

  /** Helper function to write layer record of a dbGDSElement to _file */
  void writeLayer(dbGDSElement* el);
  /** Helper function to write XY record  of a dbGDSElement to _file */
  void writeXY(dbGDSElement* el);
  /** Helper function to write the datatype record of a dbGDSElement to _file */
  void writeDataType(dbGDSElement* el);
  /** Helper function to end an element in _file */
  void writeEndel();

  /** Helper function a property attribute to _file */
  void writePropAttr(dbGDSElement* el);

  /** Writes _lib to the _file */
  void writeLib();

  /** Writes a dbGDSStructure to _file */
  void writeStruct(dbGDSStructure* str);

  /** Writes a dbGDSElement to _file */
  void writeElement(dbGDSElement* el);

  /** Writes different variants of dbGDSElement to _file */
  void writeBoundary(dbGDSBoundary* bnd);
  void writePath(dbGDSPath* path);
  void writeSRef(dbGDSSRef* sref);
  void writeText(dbGDSText* text);
  void writeBox(dbGDSBox* box);
  void writeNode(dbGDSNode* node);

  /** Writes a Transform to _file */
  void writeSTrans(const dbGDSSTrans& strans);

  /** Writes a Text Presentation to _file */
  void writeTextPres(const dbGDSTextPres& pres);
};

}  // namespace odb