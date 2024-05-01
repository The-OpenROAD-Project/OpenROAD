// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "map/scl/sclLib.h"
#include "sta/Sta.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {

// A Factory to construct an abc::SC_Lib* from an OpenSTA library.
// It is key to reconstructing cuts from OpenROAD in ABC's AIG
// datastructure.
class AbcLibraryFactory
{
 public:
  explicit AbcLibraryFactory(utl::Logger* logger) : logger_(logger) {}
  AbcLibraryFactory& AddStaLibrary(sta::LibertyLibrary* library);
  utl::deleted_unique_ptr<abc::SC_Lib> Build();

 private:
  void PopulateAbcSclLibFromSta(abc::SC_Lib* sc_library);
  int ScaleAbbreviationToExponent(std::string scale_abbreviation);
  int StaTimeUnitToAbcInt(sta::Unit* time_unit);
  float StaCapacitanceToAbc(sta::Unit* cap_unit);
  std::vector<abc::SC_Pin*> CreateAbcOutputPins(
      sta::LibertyCell* cell,
      const std::vector<std::string>& input_names);
  void AbcPopulateAbcSurfaceFromSta(abc::SC_Surface* abc_table,
                                    const sta::TableModel* model);
  std::vector<abc::SC_Pin*> CreateAbcInputPins(sta::LibertyCell* cell);

  utl::Logger* logger_;
  sta::LibertyLibrary* library_ = nullptr;
};

}  // namespace rmp