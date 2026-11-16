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
                const std::string& reset_value = "",
                int banks = 1,
                int bank_columns = 1,
                const std::string& bank_order = "contiguous",
                int rb_ports = 0,
                int register_read_address = 0,
                const std::string& write_style = "clock_gate",
                const std::string& write_priority = "last",
                const std::string& periphery = "placed",
                const std::string& pin_layer = "",
                const std::string& cells = "",
                const std::string& name = "",
                const std::string& decoder = "row",
                int bit_folds = 1,
                const std::string& wrapper_verilog = "",
                int service_every = 0,
                int service_sites = 0,
                const std::string& bank_column_layout = "interleaved");

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
  // one term of a read tree: the word at (row, word in the row) and its
  // read select
  struct Term
  {
    int row;
    int word_idx;
    odb::dbNet* select;
  };
  // the tree over terms[lo, hi), each node in the bit cell of a term it
  // covers
  TreeEnd planReadTree(const std::string& prefix,
                       int bit,
                       const std::vector<Term>& terms,
                       int lo,
                       int hi,
                       odb::dbNet* out);
  // the word at (row, word in the row)
  int wordAt(int row, int word_idx) const;
  bool treeInverted(int lo, int hi) const;
  // -reset_value: the storage flop's data, clear and preset pins, the
  // reset's polarity, what a cleared flop reads, and the tie cell that
  // holds the unused pin off
  void findResetPins();
  // The smallest cell with an output at the constant `one` (or 0), or the
  // cell `given`, checked; `pin` gets that output. A cell may drive both
  // levels on two pins, as sky130's conb_1 does.
  odb::dbMaster* findTie(bool one, odb::dbMaster* given, std::string& pin);
  // The smallest cell `match` accepts; none is an error unless not
  // `required`, then nullptr. A cell `given` is checked by `match`
  // instead, on any of its outputs. Cells with more than one output are
  // passed over, except, with `q_and_qn`, a flop whose outputs are its
  // Q and QN.
  odb::dbMaster* findMaster(const std::function<bool(sta::LibertyPort*)>& match,
                            const char* name,
                            bool required = true,
                            odb::dbMaster* given = nullptr,
                            bool q_and_qn = false);
  // The smallest four-input cell whose function is `table` (bit k the
  // output for inputs k in binary, in the order of `inputs`): `inputs`
  // gets its input pins in that order, `output` its output pin.
  odb::dbMaster* findMaster4(uint16_t table,
                             const char* name,
                             std::array<std::string, 4>& inputs,
                             std::string& output,
                             bool required = true,
                             odb::dbMaster* given = nullptr);
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
                        const std::vector<odb::dbNet*>& select_nets,
                        int words_per_row = 1);
  // banks: a decoder per word, an AND2 tree over all its address
  // literals, the words of a row in that row's cell
  void makeWordDecoderColumn(
      const std::string& prefix,
      int num_rows,
      int words_per_row,
      const std::vector<std::vector<odb::dbNet*>>& literals,
      const std::vector<odb::dbNet*>& decoder_output_nets);

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
  // Banks: the words in `banks_` banks (contiguous or interleaved
  // addresses), bank_columns_ of them side by side, the rest stacked.
  // Each port decodes every word's whole address, so any placement of
  // words reads and writes alike; a row holds bank_columns_ words, its
  // bits interleaved as the column mux's.
  bool banked_{false};
  int banks_{1};
  int bank_columns_{1};
  bool bank_interleaved_{false};
  int words_per_bank_{0};
  int words_per_row_{1};
  // Banked read ports: each an address and a data output per bank, read
  // independently (rb_ports_ of them, after the plain read ports)
  int rb_ports_{0};
  int r_plain_{0};  // the plain read ports, read/write ones included
  // the bank of the word at (row, word in the row), and its index there
  int bankAt(int row, int word_idx) const;
  int indexInBank(int row) const;
  // -register_read_address 1: each read port's address through a flop (a plain
  // positive-edge one) in the address inverter's cell. The decoders read
  // addr_true_ (the address, or the flop's output) and its complement;
  // where the flop's output is inverted the complement is the flop's and
  // the inverter makes the true one.
  int register_read_address_{0};
  // -write_style mux: no clock gate; each bit's flop on the clock, its D
  // the new data where the word is written and its own value elsewhere
  bool write_mux_{false};
  // -write_priority or: where ports write the same word in a cycle, the
  // word takes the OR of their data, not the last port's
  bool write_or_{false};
  // -periphery unplaced: the periphery (address inverters and registers,
  // decoders, select buffers, each word's select cell, the input
  // buffers) left for a parent to place; the outline the placed core and
  // a row; each pin where its connections land, on pin_layer_'s tracks
  bool periphery_unplaced_{false};
  std::string ram_name_;  // the block's and the behavioral model's name
  // -bit_folds k: a word's mask slices in k rows instead of side by side,
  // so a wide word with many ports is as tall as it is wide; every
  // column is k rows a word
  int bit_folds_{1};
  int word_bits_{0};    // the word's bits, short of the folds' pads
  int slice_width_{0};  // a slice's columns: its bits, and its select
  // with -bit_folds a word is written whole: its write logic (enables,
  // priority, hold, clock gate, reset tie, select inverters) built in its
  // first fold and shared by the others, keyed (row, word in the row)
  struct WordWrite
  {
    odb::dbNet* we{nullptr};
    odb::dbNet* hold{nullptr};
    odb::dbNet* gclock{nullptr};
    odb::dbNet* reset_off{nullptr};
    std::vector<odb::dbNet*> take;
    std::vector<odb::dbNet*> select_b;
  };
  std::map<std::pair<int, int>, WordWrite> word_write_;
  int sliceOffset(int slice) const
  {
    return bit_folds_ > 1 ? 0 : slice * slice_width_;
  }
  // -bank_column_layout: a row's words interleaved bit by bit, as the
  // column mux lays them out (bit b of every word, then bit b+1), or
  // contiguous, each word's bits side by side and its select column
  // after them, so a word's selects and write enables span its own bits
  // and not the row
  bool words_contiguous_{false};
  int slices_per_row_{1};  // a word's slices in a row: 1 with -bit_folds
  // contiguous: a word's slices side by side, each its bits and then its
  // select column, the row's next word after them
  int contiguousColumn(int slice, int word, int mask_size) const
  {
    return (word * slices_per_row_ + (bit_folds_ > 1 ? 0 : slice))
           * (mask_size + 1);
  }
  int bitColumn(int slice, int bit, int word, int mask_size) const
  {
    return words_contiguous_ ? contiguousColumn(slice, word, mask_size) + bit
                             : sliceOffset(slice) + bit * words_per_row_ + word;
  }
  int selColumn(int slice, int word, int mask_size) const
  {
    return words_contiguous_
               ? contiguousColumn(slice, word, mask_size) + mask_size
               : sliceOffset(slice) + mask_size * words_per_row_ + word;
  }
  std::string pin_layer_;
  bool marking_periphery_{false};
  Cell* periphery_cell_{nullptr};
  std::vector<odb::dbInst*> periphery_insts_;
  void unplacePeriphery();
  // -periphery verilog: the periphery leaves the block, which becomes the
  // core alone (<name>_core), for a wrapper module in Verilog written to
  // wrapper_verilog_: the periphery's logic cells as assigns of their
  // liberty functions, its other cells (clock gates, flops, ties) as
  // instances, around an instance of the core. A net the core shares
  // with the periphery becomes a core port.
  bool periphery_verilog_{false};
  std::string wrapper_verilog_;
  void splitPeriphery();
  odb::dbMaster* addr_flop_cell_{nullptr};
  std::string addr_flop_d_, addr_flop_clk_, addr_flop_q_;
  bool addr_flop_inverted_{false};
  odb::dbNet* clock_net_{nullptr};
  std::vector<std::vector<odb::dbNet*>> addr_true_;
  std::vector<std::vector<odb::dbNet*>> addr_flop_out_;  // or nullptr
  // An asynchronous reset to reset_value_ (a word's value, every word):
  // each bit's flop is cleared or preset by the reset port, the other
  // pin held off. The pins come from the flop's ff group; clear_reads_
  // is the data a cleared flop reads back, after any inversion.
  std::string reset_value_;
  std::vector<bool> reset_bits_;  // a word, least significant bit first
  odb::dbBTerm* reset_port_{nullptr};
  odb::dbMaster* tie_off_cell_{nullptr};  // drives the inactive level
  std::string tie_off_pin_;
  // a clock gate's inputs besides its clock and enable (a test enable),
  // each tied low by tie_low_cell_
  std::vector<std::string> clock_gate_test_pins_;
  odb::dbMaster* tie_low_cell_{nullptr};
  std::string tie_low_pin_;
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
