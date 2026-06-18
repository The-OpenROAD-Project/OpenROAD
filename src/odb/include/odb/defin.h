// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "absl/synchronization/mutex.h"

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
 public:
  enum MODE
  {
    DEFAULT,      // creates db from scratch (from def)
    FLOORPLAN,    // update existing COMPONENTS PINS DIEAREA TRACKS ROWS NETS
                  // SNETS
    INCREMENTAL,  // update existing COMPONENTS PINS
    THREE_D_BLOX  // read DEF onto an existing 3DBlox block (created when the
                  // chiplet definition was read); find-or-create
                  // COMPONENTS/PINS/NETS and dedup data shared with the bump
                  // map
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

  /// Read a DEF onto a chip. Returns true on success, false if the DEF file
  /// could not be read.
  bool readChip(std::vector<dbLib*>& search_libs,
                const char* def_file,
                dbChip* chip,
                bool issue_callback = true);

 private:
  definReader* reader_;

  // Protects the DefParser namespace that has static variables
  static absl::Mutex def_mutex_;
};

}  // namespace odb
