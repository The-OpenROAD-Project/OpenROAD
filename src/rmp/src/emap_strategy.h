// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "cut/logic_cut.h"
#include "db_sta/dbSta.hh"
#include "mockturtle/networks/aig.hpp"
#include "mockturtle/networks/block.hpp"
#include "mockturtle/utils/tech_library.hpp"
#include "mockturtle/views/cell_view.hpp"
#include "mockturtle/views/names_view.hpp"
#include "resynthesis_strategy.h"
#include "rsz/Resizer.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"

namespace rmp {

class EmapStrategy : public ResynthesisStrategy
{
  using BlockNtk = mockturtle::names_view<
      mockturtle::cell_view<mockturtle::block_network>>;

 public:
  explicit EmapStrategy(sta::Scene* scene,
                        bool map_multioutput,
                        bool verbose,
                        bool create_po_buffers,
                        bool insert_buffers,
                        double min_drive_resistance,
                        double max_drive_resistance)
      : scene_(scene),
        map_multioutput_(map_multioutput),
        verbose_(verbose),
        create_po_buffers_(create_po_buffers),
        insert_buffers_(insert_buffers),
        min_drive_resistance_(min_drive_resistance),
        max_drive_resistance_(max_drive_resistance)
  {
  }

  void OptimizeDesign(sta::dbSta* sta,
                      utl::UniqueName& name_generator,
                      rsz::Resizer* resizer,
                      utl::Logger* logger) override;

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
  ExtractLogicToMockturtle(sta::dbSta* sta,
                           sta::Scene* scene,
                           rsz::Resizer* resizer,
                           mockturtle::tech_library<9u>& tech_lib,
                           utl::Logger* logger);

  template <typename Ntk>
  odb::dbNet* GetDriverNet(mockturtle::topo_view<Ntk, false>& topo_ntk,
                           odb::dbBlock* block,
                           const odb::dbSet<odb::dbLib>& libs,
                           sta::dbSta* sta,
                           utl::Logger* logger,
                           std::vector<std::vector<odb::dbNet*>>& node_out_nets,
                           const BlockNtk::signal& f);

  std::vector<odb::dbMTerm*> GetSignalOutputs(odb::dbMaster* master);

  CellMapping MapCellFromStdCell(const BlockNtk& ntk,
                                 const BlockNtk::node& n,
                                 utl::Logger* logger);

  std::optional<const sta::LibertyCell*> FindLibertyCellByMasterName(
      sta::Sta* sta,
      const std::string& cell_name);

  std::optional<TieMaster> FindTieMaster(const odb::dbSet<odb::dbLib>& libs,
                                         sta::dbSta* sta,
                                         bool value);

  odb::dbNet* EnsureConstNet(bool value,
                             odb::dbBlock* block,
                             const odb::dbSet<odb::dbLib>& libs,
                             sta::dbSta* sta,
                             utl::Logger* logger);

  void ImportMockturtleMappedNetwork(sta::dbSta* sta,
                                     const BlockNtk& ntk,
                                     const odb::dbSet<odb::dbLib>& libs,
                                     cut::LogicCut& cut,
                                     utl::Logger* logger);

  sta::Scene* scene_;
  bool map_multioutput_;
  bool verbose_;
  bool create_po_buffers_;
  bool insert_buffers_;
  double min_drive_resistance_;
  double max_drive_resistance_;

  // Cached const networks for mapped network import
  odb::dbNet* net0_cache_ = nullptr;
  odb::dbNet* net1_cache_ = nullptr;
};

}  // namespace rmp
