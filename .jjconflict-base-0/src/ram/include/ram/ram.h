// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"

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

namespace ram {

using utl::Logger;

////////////////////////////////////////////////////////////////
class Cell;
class Layout;
class Grid;

class RamGen
{
 public:
  RamGen(sta::dbNetwork* network, odb::dbDatabase* db, Logger* logger);
  ~RamGen() = default;

  void generate(int bytes_per_word,
                int word_count,
                int read_ports,
                odb::dbMaster* storage_cell,
                odb::dbMaster* tristate_cell,
                odb::dbMaster* inv_cell);

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
  odb::dbInst* makeCellInst(
      Cell* cell,
      const std::string& prefix,
      const std::string& name,
      odb::dbMaster* master,
      const std::vector<std::pair<std::string, odb::dbNet*>>& connections);
  std::unique_ptr<Cell> makeCellBit(const std::string& prefix,
                                    int read_ports,
                                    odb::dbNet* clock,
                                    std::vector<odb::dbNet*>& select,
                                    odb::dbNet* data_input,
                                    std::vector<odb::dbNet*>& data_output);
  void makeCellByte(
      Grid& ram_grid,
      int byte_number,
      const std::string& prefix,
      int read_ports,
      odb::dbNet* clock,
      odb::dbNet* write_enable,
      const std::vector<odb::dbNet*>& selects,
      const std::array<odb::dbNet*, 8>& data_input,
      const std::vector<std::array<odb::dbBTerm*, 8>>& data_output);

  odb::dbBTerm* makeBTerm(const std::string& name, odb::dbIoType io_type);

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
  Logger* logger_;

  odb::dbMaster* storage_cell_{nullptr};
  odb::dbMaster* tristate_cell_{nullptr};
  odb::dbMaster* inv_cell_{nullptr};
  odb::dbMaster* and2_cell_{nullptr};
  odb::dbMaster* clock_gate_cell_{nullptr};
  odb::dbMaster* buffer_cell_{nullptr};
};

}  // namespace ram
