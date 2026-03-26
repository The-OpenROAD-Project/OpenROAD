// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "layout.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {
class dbMaster;
}

namespace sta {
class dbNetwork;
class LibertyPort;
}  // namespace sta

namespace utl {
class Logger;
}

namespace pdn {
class PdnGen;
}

namespace ppl {
class IOPlacer;
}

namespace dpl {
class Opendp;
}

namespace grt {
class GlobalRouter;
}

namespace drt {
class TritonRoute;
}

namespace ram {

class RamGen
{
 public:
  RamGen(sta::dbNetwork* network,
         odb::dbDatabase* db,
         utl::Logger* logger,
         pdn::PdnGen* pdngen,
         ppl::IOPlacer* io_placer,
         dpl::Opendp* opendp,
         grt::GlobalRouter* global_router,
         drt::TritonRoute* detailed_router);
  ~RamGen() = default;

  void generate(int mask_size,
                int word_size,
                int num_words,
                int read_ports,
                odb::dbMaster* storage_cell,
                odb::dbMaster* tristate_cell,
                odb::dbMaster* inv_cell,
                odb::dbMaster* tapcell,
                int max_tap_dist);

  void ramPdngen(const char* power_pin,
                 const char* ground_pin,
                 const char* route_name,
                 int route_width,
                 const char* ver_name,
                 int ver_width,
                 int ver_pitch,
                 const char* hor_name,
                 int hor_width,
                 int hor_pitch);
  void ramPinplacer(const char* ver_name, const char* hor_name);
  void ramFiller(const std::vector<std::string>& filler_cells);
  void ramRouting(int thread_count);

  void setBehavioralVerilogFilename(const std::string& filename);
  void writeBehavioralVerilog(const std::string& filename,
                              int slices_per_word,
                              int mask_size,
                              int word_count,
                              int read_ports);

 private:
  void findMasters();
  odb::dbMaster* findMaster(const std::function<bool(sta::LibertyPort*)>& match,
                            const char* name);
  odb::dbNet* makeNet(const std::string& prefix, const std::string& name);
  odb::dbInst* makeInst(
      Layout* layout,
      const std::string& prefix,
      const std::string& name,
      odb::dbMaster* master,
      const std::vector<std::pair<std::string, odb::dbNet*>>& connections);
  odb::dbInst* makeInst(
      Cell* cell,
      const std::string& prefix,
      const std::string& name,
      odb::dbMaster* master,
      const std::vector<std::pair<std::string, odb::dbNet*>>& connections);
  std::unique_ptr<Cell> makeBit(const std::string& prefix,
                                int read_ports,
                                odb::dbNet* clock,
                                std::vector<odb::dbNet*>& select,
                                odb::dbNet* data_input,
                                std::vector<odb::dbNet*>& data_output);
  void makeSlice(int slice_idx,
                 int mask_size,
                 int row_idx,
                 int read_ports,
                 odb::dbNet* clock,
                 odb::dbNet* write_enable,
                 const std::vector<odb::dbNet*>& selects,
                 const std::vector<odb::dbNet*>& data_input,
                 const std::vector<std::vector<odb::dbBTerm*>>& data_output);

  void makeWord(int slices_per_word,
                int mask_size,
                int row_idx,
                int read_ports,
                odb::dbNet* clock,
                std::vector<odb::dbBTerm*>& write_enable,
                const std::vector<odb::dbNet*>& selects,
                const std::vector<odb::dbNet*>& data_input,
                const std::vector<std::vector<odb::dbBTerm*>>& data_output);

  odb::dbBTerm* makeBTerm(const std::string& name, odb::dbIoType io_type);

  std::unique_ptr<Layout> generateTapColumn(int word_count, int tapcell_col);

  std::unique_ptr<Cell> makeDecoder(const std::string& prefix,
                                    int num_word,
                                    int read_ports,
                                    const std::vector<odb::dbNet*>& selects,
                                    const std::vector<odb::dbNet*>& addr_nets);

  std::vector<odb::dbNet*> selectNets(const std::string& prefix,
                                      int read_ports);

  sta::dbNetwork* network_;
  odb::dbDatabase* db_;
  odb::dbBlock* block_{nullptr};
  utl::Logger* logger_;
  pdn::PdnGen* pdngen_{nullptr};
  ppl::IOPlacer* io_placer_{nullptr};
  dpl::Opendp* opendp_{nullptr};
  grt::GlobalRouter* global_router_{nullptr};
  drt::TritonRoute* detailed_router_{nullptr};

  odb::dbMaster* storage_cell_{nullptr};
  odb::dbMaster* tristate_cell_{nullptr};
  odb::dbMaster* inv_cell_{nullptr};
  odb::dbMaster* and2_cell_{nullptr};
  odb::dbMaster* clock_gate_cell_{nullptr};
  odb::dbMaster* buffer_cell_{nullptr};
  odb::dbMaster* tapcell_{nullptr};

  std::vector<odb::dbBTerm*> addr_inputs_;
  std::vector<odb::dbBTerm*> data_inputs_;
  std::vector<std::vector<odb::dbBTerm*>> q_outputs_;
  std::string behavioral_verilog_filename_;
  Grid ram_grid_;
};

}  // namespace ram
