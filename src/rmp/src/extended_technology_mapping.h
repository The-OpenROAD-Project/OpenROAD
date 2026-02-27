// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "cut/logic_cut.h"
#include "db_sta/dbSta.hh"
#include "mockturtle/networks/aig.hpp"
#include "mockturtle/networks/block.hpp"
#include "mockturtle/utils/tech_library.hpp"
#include "mockturtle/views/cell_view.hpp"
#include "mockturtle/views/names_view.hpp"
#include "rsz/Resizer.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"

namespace rmp {

class ExtendedTechnologyMapping
{
  using BlockNtk = mockturtle::names_view<
      mockturtle::cell_view<mockturtle::block_network>>;

 public:
  explicit ExtendedTechnologyMapping(sta::Scene* scene,
                                     bool map_multioutput,
                                     bool area_oriented_mapping,
                                     bool verbose)
      : scene_(scene),
        map_multioutput_(map_multioutput),
        area_oriented_mapping_(area_oriented_mapping),
        verbose_(verbose)
  {
  }
  ~ExtendedTechnologyMapping() = default;

  void map(sta::dbSta* sta,
           odb::dbDatabase* db,
           rsz::Resizer* resizer,
           utl::Logger* logger);

 private:
  // Mapping info for one cell instance
  struct CellMapping
  {
    std::string master_name;              // dbMaster / Liberty cell name
    std::vector<std::string> input_pins;  // in fanin order (from gate::pins)
  };

  struct TieMaster
  {
    odb::dbMaster* master = nullptr;
    std::string out_pin;
  };

  std::tuple<mockturtle::names_view<mockturtle::aig_network>, cut::LogicCut>
  extractLogicToMockturtle(sta::dbSta* sta,
                           sta::Scene* corner,
                           rsz::Resizer* resizer,
                           mockturtle::tech_library<9u>& tech_lib,
                           utl::Logger* logger);

  std::vector<odb::dbMTerm*> getSignalOutputs(odb::dbMaster* master);

  CellMapping mapCellFromStdCell(const BlockNtk& ntk,
                                 const BlockNtk::node& n,
                                 utl::Logger* logger);

  std::optional<const sta::LibertyCell*> findLibertyCellByMasterName(
      sta::Sta* sta,
      const std::string& cell_name);

  std::optional<TieMaster> findTieMaster(const odb::dbSet<odb::dbLib>& libs,
                                         sta::dbSta* sta,
                                         bool value);

  odb::dbNet* ensureConstNet(bool value,
                             odb::dbBlock* block,
                             const odb::dbSet<odb::dbLib>& libs,
                             sta::dbSta* sta,
                             utl::Logger* logger);

  void importMockturtleMappedNetwork(sta::dbSta* sta,
                                     const BlockNtk& ntk,
                                     const odb::dbSet<odb::dbLib>& libs,
                                     cut::LogicCut& cut,
                                     utl::Logger* logger);

  sta::Scene* scene_;
  bool map_multioutput_;
  bool area_oriented_mapping_;
  bool verbose_;

  // Cached const networks for mapped network import
  odb::dbNet* net0_cache_ = nullptr;
  odb::dbNet* net1_cache_ = nullptr;
};

}  // namespace rmp
