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
namespace utl {
class Logger;
}

namespace odb {

class definReader;
class dbDatabase;
class dbLib;
class dbBlock;
class dbChip;
class dbTech;

class defin
{
  definReader* _reader;

 public:
  enum MODE
  {
    DEFAULT,     // creates db from scratch (from def)
    FLOORPLAN,   // update existing COMPONENTS PINS DIEAREA TRACKS ROWS NETS
                 // SNETS
    INCREMENTAL  // update existing COMPONENTS PINS
  };
  defin(dbDatabase* db, utl::Logger* logger, MODE mode = DEFAULT);
  ~defin();

  void skipWires();
  void skipConnections();
  void skipSpecialWires();
  void skipShields();
  void skipBlockWires();
  void skipFillWires();
  void continueOnErrors();
  void namesAreDBIDs();
  void setAssemblyMode();
  void useBlockName(const char* name);

  /// Create a new chip
  dbChip* createChip(std::vector<dbLib*>& search_libs, const char* def_file);

  /// Create a new hierachical block
  dbBlock* createBlock(dbBlock* parent,
                       std::vector<dbLib*>& search_libs,
                       const char* def_file);

  /// Replace the wires of this block.
  bool replaceWires(dbBlock* block, const char* def_file);
};

}  // namespace odb
