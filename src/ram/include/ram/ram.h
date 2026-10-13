// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "ram/layout.h"
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

enum class PortRoleType
{
  Clock,
  DataIn,
  DataOut,
  WriteEnable,
  TriEnable,
  Select,  // for mux support in future
  Power,
  Ground
};

enum class RamPortType
{
  Read,
  Write,
  ReadWrite
};

struct PortRole
{
  PortRoleType type;
  int index;

// for map so that keys are comparable
#ifndef SWIG
  auto operator<=>(const PortRole&) const = default;
#endif
};

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
                int rw_ports,
                int r_ports,
                int w_ports,
                int column_mux_ratio,
                bool use_latch,
                odb::dbMaster* storage_cell,
                odb::dbMaster* tristate_cell,
                odb::dbMaster* inv_cell,
                odb::dbMaster* tapcell,
                int max_tap_dist,
                const std::string& read_style = "",
                int zero_word = -1,
                const std::string& reset_value = "");

  void ramPdngen(const char* power_net_name,
                 const char* ground_net_name,
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
                              int rw_ports,
                              int r_ports,
                              int w_ports);

 private:
  void findMasters();
  // `inverted`, if given, is set when a flop's only data output is its
  // inverted one (QN), which then stands as DataOut.
  std::map<PortRole, std::string> buildPortMap(odb::dbMaster*,
                                               bool* inverted = nullptr);
  // The AND-OR read path: each read port's bitline a tree over the rows
  // of a bit column, planned before the rows are built (planReadTree) and
  // placed row by row with the bits (treeNodes_).
  struct TreeNode
  {
    odb::dbMaster* master;
    std::string prefix;
    std::string name;
    std::vector<std::pair<std::string, odb::dbNet*>> conns;
  };
  struct TreeEnd
  {
    odb::dbNet* net;
    bool inverted;  // the net is the complement of the OR of the terms
  };
  // the tree over rows[lo, hi): rows lists the rows that store a word
  TreeEnd planReadTree(const std::string& prefix,
                       int word_idx,
                       int bit,
                       const std::vector<int>& rows,
                       int lo,
                       int hi,
                       const std::vector<odb::dbNet*>& selects,
                       odb::dbNet* out);
  bool treeInverted(int lo, int hi) const;
  // -reset_value: the storage flop's data, clear and preset pins, the
  // reset's polarity, what a cleared flop reads, and the tie cell that
  // holds the unused pin off
  void findResetPins();
  // The smallest cell `match` accepts; none is an error unless not
  // `required`, then nullptr.
  odb::dbMaster* findMaster(const std::function<bool(sta::LibertyPort*)>& match,
                            const char* name,
                            bool required = true);
  // The smallest four-input cell whose function is `table` (bit k the
  // output for inputs k in binary, in the order of `inputs`): `inputs`
  // gets its input pins in that order, `output` its output pin.
  odb::dbMaster* findMaster4(uint16_t table,
                             const char* name,
                             std::array<std::string, 4>& inputs,
                             std::string& output,
                             bool required = true);
  odb::dbNet* makeNet(const std::string& prefix, const std::string& name);
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
                                std::vector<odb::dbNet*>& data_output,
                                odb::dbNet* storage_net = nullptr,
                                int reset_bit = -1,
                                odb::dbNet* reset_off = nullptr);
  void makeSlice(int slice_idx,
                 int mask_size,
                 int row_idx,
                 int rw_ports,
                 int r_ports,
                 int w_ports,
                 int word_idx,
                 int column_mux_ratio,
                 odb::dbNet* clock,
                 const std::vector<odb::dbNet*>& write_enable,
                 odb::dbNet* word_select,
                 const std::vector<odb::dbNet*>& selects,
                 const std::vector<odb::dbNet*>& shared_select_b_nets,
                 bool create_select_inv,
                 const std::vector<std::vector<odb::dbNet*>>& data_input,
                 const std::vector<std::vector<odb::dbNet*>>& data_output);

  void makeWord(int slices_per_word,
                int mask_size,
                int row_idx,
                int rw_ports,
                int r_ports,
                int w_ports,
                int word_idx,
                int column_mux_ratio,
                odb::dbNet* clock,
                odb::dbNet* word_select,
                std::vector<std::vector<odb::dbBTerm*>>& write_enable,
                const std::vector<odb::dbNet*>& selects,
                const std::vector<odb::dbNet*>& shared_select_b_nets,
                bool create_select_inv,
                const std::vector<std::vector<odb::dbNet*>>& data_input,
                const std::vector<std::vector<odb::dbNet*>>& data_output);

  odb::dbBTerm* makeBTerm(const std::string& name, odb::dbIoType io_type);

  std::unique_ptr<Layout> generateTapColumn(int num_rows, int tapcell_col);

  void makeDecoderColumn(const std::string& prefix,
                         int num_rows,
                         const std::vector<std::vector<odb::dbNet*>>& addr_nets,
                         const std::vector<odb::dbNet*>& decoder_output_nets);
  void makeBufferColumn(const std::string& prefix,
                        int num_rows,
                        const std::vector<odb::dbNet*>& decoder_output_nets,
                        const std::vector<odb::dbNet*>& select_nets);

  std::unique_ptr<Layout> makeInverterColumn(
      int num_rows,
      int num_row_bits,
      int start_port,
      int end_port,
      int num_word_bits,
      const std::vector<std::vector<odb::dbNet*>>& inv_addr_nets);

  std::vector<odb::dbNet*> makeDecoderOutputNets(const std::string& prefix,
                                                 int num_rows);

  std::vector<odb::dbNet*> makeSelectNets(const std::string& prefix,
                                          int num_rows,
                                          RamPortType port_type);

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
  odb::dbMaster* aoi22_cell_{nullptr};
  // with more than one write port: the write data mux (an AO22, or where
  // the library has none the AOI22 and an inverter) and the write
  // enables' OR
  odb::dbMaster* ao22_cell_{nullptr};
  odb::dbMaster* or2_cell_{nullptr};
  // the AND-OR read path's tree
  odb::dbMaster* nand2_cell_{nullptr};
  odb::dbMaster* nor2_cell_{nullptr};
  odb::dbMaster* latch_cell_{nullptr};
  odb::dbMaster* tapcell_{nullptr};

  std::set<std::string> power_pin_names_;
  std::set<std::string> ground_pin_names_;

  std::map<PortRole, std::string> storage_ports_;
  std::map<PortRole, std::string> tristate_ports_;
  std::map<PortRole, std::string> inv_ports_;
  std::map<PortRole, std::string> and2_ports_;
  std::map<PortRole, std::string> clock_gate_ports_;
  std::map<PortRole, std::string> buffer_ports_;
  std::map<PortRole, std::string> latch_ports_;
  std::map<PortRole, std::string> or2_ports_;
  std::map<PortRole, std::string> nand2_ports_;
  std::map<PortRole, std::string> nor2_ports_;

  std::vector<std::vector<odb::dbBTerm*>> addr_inputs_;
  std::vector<odb::dbBTerm*> data_inputs_;  // every write port's data
  std::vector<std::vector<odb::dbBTerm*>> q_outputs_;
  std::string behavioral_verilog_filename_;
  std::string aoi22_in_a1_, aoi22_in_a2_, aoi22_in_b1_, aoi22_in_b2_,
      aoi22_out_;
  std::array<std::string, 4> ao22_in_;
  std::string ao22_out_;
  Grid ram_grid_;
  bool use_latch_{false};
  int num_write_{1};  // write and read/write ports
  // Reads through AND-OR trees instead of tristate bitlines: asked for,
  // or the library has no tristate cell.
  bool and_or_read_{false};
  std::string read_style_;
  bool storage_inverted_{false};  // the storage flop's output is QN
  int zero_word_{-1};  // the word that reads 0 and is not stored, or -1
  // An asynchronous reset to reset_value_ (a word's value, every word):
  // each bit's flop is cleared or preset by the reset port, the other
  // pin held off. The pins come from the flop's ff group; clear_reads_
  // is the data a cleared flop reads back, after any inversion.
  std::string reset_value_;
  uint64_t reset_bits_{0};
  odb::dbBTerm* reset_port_{nullptr};
  odb::dbMaster* tie_off_cell_{nullptr};  // drives the inactive level
  std::string tie_off_pin_;
  std::string storage_data_pin_;
  std::string storage_clear_pin_;
  std::string storage_preset_pin_;
  bool reset_active_low_{true};
  int clear_reads_{0};
  // per (row, word in the row, bit): the storage net, and the tree
  // cells placed in that bit's cell
  std::map<std::tuple<int, int, int>, odb::dbNet*> storage_nets_;
  std::map<std::tuple<int, int, int>, std::vector<TreeNode>> tree_nodes_;
};

}  // namespace ram
