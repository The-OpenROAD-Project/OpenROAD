// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/abc/abc.h"
#include "db_sta/dbSta.hh"
#include "map/mio/mio.h"
#include "map/mio/mioInt.h"
#include "map/scl/sclLib.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "sta/TableModel.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace cut {

class AbcLibrary
{
 public:
  AbcLibrary(utl::UniquePtrWithDeleter<abc::SC_Lib> abc_library,
             utl::Logger* logger);
  AbcLibrary(AbcLibrary&&) = default;
  ~AbcLibrary() = default;
  abc::SC_Lib* abc_library() { return abc_library_.get(); }
  abc::Mio_Library_t* mio_library();
  bool IsSupportedCell(const std::string& cell_name);
  bool IsConst0Cell(const std::string& cell_name);
  bool IsConst1Cell(const std::string& cell_name);
  bool IsConstCell(const std::string& cell_name);
  std::pair<abc::SC_Cell*, abc::SC_Pin*> ConstantZeroCell();
  std::pair<abc::SC_Cell*, abc::SC_Pin*> ConstantOneCell();

 private:
  void InitializeConstGates();

  utl::UniquePtrWithDeleter<abc::SC_Lib> abc_library_;
  utl::UniquePtrWithDeleter<abc::Mio_Library_t> mio_library_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::set<std::string> supported_cells_;
  std::unordered_set<std::string> const1_gates_;
  std::unordered_set<std::string> const0_gates_;
  std::optional<std::pair<abc::SC_Cell*, abc::SC_Pin*>> const0_cell_;
  std::optional<std::pair<abc::SC_Cell*, abc::SC_Pin*>> const1_cell_;
  bool const_gates_initalized_ = false;
};

// A Factory to construct an abc::SC_Lib* from an OpenSTA library.
// It is key to reconstructing cuts from OpenROAD in ABC's AIG
// data structure.
class AbcLibraryFactory
{
 public:
  explicit AbcLibraryFactory(utl::Logger* logger) : logger_(logger) {}
  AbcLibraryFactory& AddDbSta(sta::dbSta* db_sta);
  AbcLibraryFactory& AddResizer(rsz::Resizer* resizer);
  AbcLibraryFactory& SetCorner(sta::Corner* corner);
  AbcLibrary Build();

 private:
  void PopulateAbcSclLibFromSta(abc::SC_Lib* sc_library,
                                std::vector<sta::LibertyCell*>& cells,
                                sta::Units* units);
  void PopulateLibraryDetails(abc::SC_Lib* sc_library,
                              sta::LibertyLibrary* library);
  int ScaleAbbreviationToExponent(const std::string& scale_abbreviation);
  int StaTimeUnitToAbcInt(sta::Unit* time_unit);
  float StaCapacitanceToAbc(sta::Unit* cap_unit);
  std::vector<abc::SC_Pin*> CreateAbcOutputPins(
      sta::LibertyCell* cell,
      const std::vector<std::string>& input_names);
  void AbcPopulateAbcSurfaceFromSta(abc::SC_Surface* abc_table,
                                    const sta::TableModel* model,
                                    sta::Units* units);
  std::vector<sta::LibertyCell*> GetLibertyCellsFromCorner(sta::Corner* corner);
  std::vector<abc::SC_Pin*> CreateAbcInputPins(sta::LibertyCell* cell);

  utl::Logger* logger_;
  sta::dbSta* db_sta_ = nullptr;
  sta::Corner* corner_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
};

}  // namespace cut
