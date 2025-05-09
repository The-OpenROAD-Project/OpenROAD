// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <mutex>
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

  // Protects the DefParser namespace that has static variables
  static std::mutex _def_mutex;

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
  void useBlockName(const char* name);

  /// Create a new chip
  dbChip* createChip(std::vector<dbLib*>& search_libs,
                     const char* def_file,
                     odb::dbTech* tech);

  /// Create a new hierachical block
  dbBlock* createBlock(dbBlock* parent,
                       std::vector<dbLib*>& search_libs,
                       const char* def_file,
                       odb::dbTech* tech);
};

}  // namespace odb
