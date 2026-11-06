// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ram/ram.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "dpl/Opendp.h"
#include "drt/TritonRoute.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/isotropy.h"
#include "ord/OpenRoad.hh"
#include "pdn/PdnGen.hh"
#include "ppl/IOPlacer.h"
#include "ram/layout.h"
#include "sta/ConcreteLibrary.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"
#include "utl/Logger.h"

namespace ram {

using odb::dbBTerm;
using odb::dbInst;
using odb::dbIoType;
using odb::dbMaster;
using odb::dbNet;
using odb::dbRow;

using utl::RAM;

using std::array;
using std::vector;

////////////////////////////////////////////////////////////////

RamGen::RamGen(sta::dbNetwork* network,
               odb::dbDatabase* db,
               utl::Logger* logger,
               pdn::PdnGen* pdngen,
               ppl::IOPlacer* io_placer,
               dpl::Opendp* opendp,
               grt::GlobalRouter* global_router,
               drt::TritonRoute* detailed_router)

    : network_(network),
      db_(db),
      logger_(logger),
      pdngen_(pdngen),
      io_placer_(io_placer),
      opendp_(opendp),
      global_router_(global_router),
      detailed_router_(detailed_router),
      ram_grid_(odb::horizontal)
{
}

dbInst* RamGen::makeInst(
    Cell* cell,
    const std::string& prefix,
    const std::string& name,
    dbMaster* master,
    const vector<std::pair<std::string, dbNet*>>& connections)
{
  const auto inst_name = fmt::format("{}.{}", prefix, name);
  auto inst = dbInst::create(block_, master, inst_name.c_str());
  if (marking_periphery_ || (cell && cell == periphery_cell_)) {
    periphery_insts_.push_back(inst);
  }
  for (auto& [mterm_name, net] : connections) {
    auto mterm = master->findMTerm(mterm_name.c_str());
    if (!mterm) {
      logger_->error(
          RAM, 11, "term {} of cell {} not found.", name, master->getName());
    }
    auto iterm = inst->getITerm(mterm);
    iterm->connect(net);
  }

  cell->addInst(inst);
  return inst;
}

dbNet* RamGen::makeNet(const std::string& prefix, const std::string& name)
{
  const auto net_name = fmt::format("{}.{}", prefix, name);
  return dbNet::create(block_, net_name.c_str());
}

dbBTerm* RamGen::makeBTerm(const std::string& name, dbIoType io_type)
{
  auto net = dbNet::create(block_, name.c_str());
  auto bTerm = dbBTerm::create(net, name.c_str());
  bTerm->setIoType(io_type);

  return bTerm;
}

std::unique_ptr<Cell> RamGen::makeBit(const std::string& prefix,
                                      const int read_ports,
                                      dbNet* clock,
                                      vector<odb::dbNet*>& select,
                                      dbNet* data_input,
                                      vector<odb::dbNet*>& data_output,
                                      dbNet* storage_net,
                                      const int reset_bit,
                                      dbNet* reset_off)
{
  auto bit_cell = std::make_unique<Cell>();

  // the AND-OR read path names the storage net ahead, for its trees
  if (storage_net == nullptr) {
    storage_net = makeNet(prefix, "storage");
  }

  if (!use_latch_) {
    vector<std::pair<std::string, dbNet*>> conns
        = {{storage_ports_[{PortRoleType::Clock, 0}], clock},
           {storage_ports_[{PortRoleType::DataIn, 0}], data_input},
           {storage_ports_[{PortRoleType::DataOut, 0}], storage_net}};
    if (reset_bit >= 0) {
      // the reset clears the flop where a cleared flop reads the bit's
      // value, else presets it; the other pin held off
      dbNet* reset = reset_port_->getNet();
      const bool clear = reset_bit == clear_reads_;
      conns.emplace_back(storage_clear_pin_, clear ? reset : reset_off);
      conns.emplace_back(storage_preset_pin_, clear ? reset_off : reset);
    }
    makeInst(bit_cell.get(), prefix, "bit", storage_cell_, conns);
  } else {
    makeInst(bit_cell.get(),
             prefix,
             "bit",
             latch_cell_,
             {{latch_ports_[{PortRoleType::Clock, 0}], clock},
              {latch_ports_[{PortRoleType::DataIn, 0}], data_input},
              {latch_ports_[{PortRoleType::DataOut, 0}], storage_net}});
  }

  for (int read_port = 0; read_port < read_ports; ++read_port) {
    if (select[read_port] == nullptr) {
      continue;  // a banked read port's other banks
    }
    makeInst(
        bit_cell.get(),
        prefix,
        fmt::format("obuf{}", read_port),
        tristate_cell_,
        {{tristate_ports_[{PortRoleType::DataIn, 0}], storage_net},
         {tristate_ports_[{PortRoleType::TriEnable, 0}], select[read_port]},
         {tristate_ports_[{PortRoleType::DataOut, 0}],
          data_output[read_port]}});
  }

  return bit_cell;
}

// The AND-OR read path: one read port's bitline of one bit column, rows
// [lo, hi), as a tree: an AOI22 over each pair of rows (a NAND2 over a
// lone row), then NAND2 and NOR2 levels by turns, each node in the last
// row of its left half, beside its inputs. Planned before the rows are
// built; each row's bit cell takes its nodes from tree_nodes_.
bool RamGen::treeInverted(const int lo, const int hi) const
{
  const int n = hi - lo;
  if (n <= 2) {
    return true;  // AOI22 or NAND2: the complement of the terms' OR
  }
  int half = 1;
  while (half * 2 < n) {
    half *= 2;
  }
  // the right half is brought to the left's polarity, and the node
  // (NAND2 of complements, NOR2 of trues) flips it
  return !treeInverted(lo, lo + half);
}

int RamGen::bankAt(const int row, const int word_idx) const
{
  return (row / words_per_bank_) * bank_columns_ + word_idx;
}

int RamGen::indexInBank(const int row) const
{
  return row % words_per_bank_;
}

int RamGen::wordAt(const int row, const int word_idx) const
{
  if (!banked_) {
    return row * words_per_row_ + word_idx;
  }
  const int bank = (row / words_per_bank_) * bank_columns_ + word_idx;
  const int i = row % words_per_bank_;
  return bank_interleaved_ ? i * banks_ + bank : bank * words_per_bank_ + i;
}

RamGen::TreeEnd RamGen::planReadTree(const std::string& prefix,
                                     const int bit,
                                     const vector<Term>& terms,
                                     const int lo,
                                     const int hi,
                                     dbNet* out)
{
  auto add = [&](const Term& at,
                 dbMaster* master,
                 const std::string& name,
                 const vector<std::pair<std::string, dbNet*>>& conns) {
    tree_nodes_[{at.row, at.word_idx, bit}].push_back(
        {master, prefix, name, conns});
  };
  // the zero word's term, where the trees read inverted data: its own
  // select, the inverted form of a stored 0 (the zero word is in the
  // tree only then)
  auto q = [&](const Term& t) {
    return wordAt(t.row, t.word_idx) == zero_word_
               ? t.select
               : storage_nets_.at({t.row, t.word_idx, bit});
  };
  // a leaf's name: its row, and with banks its word in the row too
  auto leaf = [&](const char* kind, const Term& t) {
    return banked_ ? fmt::format("{}_r{}_w{}", kind, t.row, t.word_idx)
                   : fmt::format("{}_r{}", kind, t.row);
  };
  auto net = [&](dbNet* given) {
    return given ? given : makeNet(prefix, fmt::format("t{}_{}", lo, hi));
  };
  const int n = hi - lo;
  if (n == 1) {
    const Term& t = terms[lo];
    dbNet* y = net(out);
    add(t,
        nand2_cell_,
        leaf("nand", t),
        {{nand2_ports_[{PortRoleType::DataIn, 0}], t.select},
         {nand2_ports_[{PortRoleType::DataIn, 1}], q(t)},
         {nand2_ports_[{PortRoleType::DataOut, 0}], y}});
    return {y, true};
  }
  if (n == 2) {
    const Term& t0 = terms[lo];
    const Term& t1 = terms[lo + 1];
    dbNet* y = net(out);
    add(t0,
        aoi22_cell_,
        leaf("aoi", t0),
        {{aoi22_in_a1_, t0.select},
         {aoi22_in_a2_, q(t0)},
         {aoi22_in_b1_, t1.select},
         {aoi22_in_b2_, q(t1)},
         {aoi22_out_, y}});
    return {y, true};
  }
  int half = 1;
  while (half * 2 < n) {
    half *= 2;
  }
  const int mid = lo + half;
  const Term& host = terms[mid - 1];
  const TreeEnd a = planReadTree(prefix, bit, terms, lo, mid, nullptr);
  TreeEnd b = planReadTree(prefix, bit, terms, mid, hi, nullptr);
  if (b.inverted != a.inverted) {
    dbNet* nb = makeNet(prefix, fmt::format("t{}_{}_inv", mid, hi));
    add(terms[mid],
        inv_cell_,
        fmt::format("inv_r{}_{}", mid, hi),
        {{inv_ports_[{PortRoleType::DataIn, 0}], b.net},
         {inv_ports_[{PortRoleType::DataOut, 0}], nb}});
    b = {nb, a.inverted};
  }
  dbNet* y = net(out);
  if (a.inverted) {
    add(host,
        nand2_cell_,
        fmt::format("nand_r{}_{}", lo, hi),
        {{nand2_ports_[{PortRoleType::DataIn, 0}], a.net},
         {nand2_ports_[{PortRoleType::DataIn, 1}], b.net},
         {nand2_ports_[{PortRoleType::DataOut, 0}], y}});
    return {y, false};
  }
  add(host,
      nor2_cell_,
      fmt::format("nor_r{}_{}", lo, hi),
      {{nor2_ports_[{PortRoleType::DataIn, 0}], a.net},
       {nor2_ports_[{PortRoleType::DataIn, 1}], b.net},
       {nor2_ports_[{PortRoleType::DataOut, 0}], y}});
  return {y, true};
}

// Note: For column_mux_ratio > 1, a single shared select_b net and select_inv
// gate are reused across all words in the same physical row (passed via
// shared_select_b_nets). This avoids creating redundant nets and gates per
// word that would cause horizontal routing congestion. Word selection on the
// read path is handled by the AOI mux in the buffer row, so all tristates in
// a row can share the same row-select enable signal.

void RamGen::makeSlice(const int slice_idx,
                       const int mask_size,
                       const int row_idx,
                       const int rw_ports,
                       const int r_ports,
                       const int w_ports,
                       const int word_idx,
                       const int column_mux_ratio,
                       dbNet* clock,
                       const vector<dbNet*>& write_enable,
                       dbNet* word_select,
                       const vector<dbNet*>& selects,
                       const vector<dbNet*>& shared_select_b_nets,
                       const bool create_select_inv,
                       const vector<vector<dbNet*>>& data_input,
                       const vector<vector<dbNet*>>& data_output)
{
  // selects: the write ports' row selects, then the read ports'
  const int num_write = rw_ports + w_ports;
  const int start_bit_idx = slice_idx * mask_size;
  std::string prefix
      = fmt::format("storage_{}_{}_{}", row_idx, word_idx, start_bit_idx);

  // The zero word: no flops, no clock gate; its row's cells hold only
  // the read trees' nodes planned there.
  if (wordAt(row_idx, word_idx) == zero_word_) {
    for (int local_bit = 0; local_bit < mask_size; ++local_bit) {
      const int bit = start_bit_idx + local_bit;
      std::unique_ptr<Cell> cell;
      auto nodes = tree_nodes_.find({row_idx, word_idx, bit});
      if (nodes != tree_nodes_.end() && !nodes->second.empty()) {
        cell = std::make_unique<Cell>();
        for (const auto& node : nodes->second) {
          makeInst(cell.get(), node.prefix, node.name, node.master, node.conns);
        }
      }
      ram_grid_.addCell(std::move(cell),
                        bitColumn(slice_idx, local_bit, word_idx, mask_size));
    }
    ram_grid_.addCell(nullptr, selColumn(slice_idx, word_idx, mask_size));
    return;
  }

  int r_total = rw_ports + r_ports;
  // -bit_folds: the word's first fold builds its write logic, the others
  // share it
  const bool shared = bit_folds_ > 1 && slice_idx > 0;
  WordWrite& word_write = word_write_[{row_idx, word_idx}];

  vector<dbNet*> select_b_nets(r_total);
  if (shared) {
    select_b_nets = word_write.select_b;
  } else if (and_or_read_) {
    // the AND-OR trees read the true selects
  } else if (column_mux_ratio == 1 || banked_) {
    for (int i = 0; i < r_total; ++i) {
      if (selects[num_write + i] != nullptr) {
        select_b_nets[i] = makeNet(prefix, fmt::format("select{}_b", i));
      }
    }
  } else {
    select_b_nets = shared_select_b_nets;
  }

  // the flops' clock: the word's gated clock, or with -write_style mux
  // the clock itself
  auto gclock_net = shared       ? word_write.gclock
                    : write_mux_ ? clock
                                 : makeNet(prefix, "gclock");
  auto we_net = shared ? word_write.we : makeNet(prefix, "we");
  auto sel_cell = std::make_unique<Cell>();
  periphery_cell_ = sel_cell.get();  // each word's select cell
  // -write_style mux: the word's hold, where it is not written
  dbNet* hold = shared ? word_write.hold : nullptr;
  if (write_mux_ && !shared) {
    hold = makeNet(prefix, "hold");
    makeInst(sel_cell.get(),
             prefix,
             "hold_inv",
             inv_cell_,
             {{inv_ports_[{PortRoleType::DataIn, 0}], we_net},
              {inv_ports_[{PortRoleType::DataOut, 0}], hold}});
  }
  // (sel0 & a) | (sel1 & b): an AO22, or the AOI22 and an inverter where
  // the library has no AO22
  auto ao22 = [&](Cell* cell,
                  const std::string& name,
                  const std::string& inst,
                  dbNet* a1,
                  dbNet* a2,
                  dbNet* b1,
                  dbNet* b2,
                  dbNet* y) {
    if (ao22_cell_) {
      makeInst(cell,
               name,
               inst,
               ao22_cell_,
               {{ao22_in_[0], a1},
                {ao22_in_[1], a2},
                {ao22_in_[2], b1},
                {ao22_in_[3], b2},
                {ao22_out_, y}});
      return;
    }
    auto y_b = makeNet(name, inst + "_b");
    makeInst(cell,
             name,
             inst,
             aoi22_cell_,
             {{aoi22_in_a1_, a1},
              {aoi22_in_a2_, a2},
              {aoi22_in_b1_, b1},
              {aoi22_in_b2_, b2},
              {aoi22_out_, y_b}});
    makeInst(cell,
             name,
             inst + "_inv",
             inv_cell_,
             {{inv_ports_[{PortRoleType::DataIn, 0}], y_b},
              {inv_ports_[{PortRoleType::DataOut, 0}], y}});
  };

  // -reset_value: one tie per word slice holds the flops' unused pins off
  dbNet* reset_off = shared ? word_write.reset_off : nullptr;
  if (reset_port_ && !shared) {
    reset_off = makeNet(prefix, "reset_off");
    makeInst(sel_cell.get(),
             prefix,
             "reset_tie",
             tie_off_cell_,
             {{tie_off_pin_, reset_off}});
  }

  // More than one write port: port w writes this word when its row select
  // and its write enable are both high (we{w}), and its data is taken
  // unless a later port writes the word in the same cycle (take{w}): the
  // last port wins, as in the behavioral model. With -write_priority or
  // every writing port's data is taken (take{w} is we{w}) and the word
  // gets their OR. The clock gate opens for any of them.
  vector<dbNet*> take(num_write);
  if (shared) {
    take = word_write.take;
  } else if (num_write > 1) {
    vector<dbNet*> we(num_write);
    for (int w = 0; w < num_write; ++w) {
      we[w] = makeNet(prefix, fmt::format("we{}", w));
      makeInst(sel_cell.get(),
               prefix,
               fmt::format("gcand{}", w),
               and2_cell_,
               {{and2_ports_[{PortRoleType::DataIn, 0}], selects[w]},
                {and2_ports_[{PortRoleType::DataIn, 1}], write_enable[w]},
                {and2_ports_[{PortRoleType::DataOut, 0}], we[w]}});
    }
    take[num_write - 1] = we[num_write - 1];
    dbNet* later = we[num_write - 1];
    for (int w = num_write - 2; w >= 0; --w) {
      if (write_or_) {
        take[w] = we[w];
      } else {
        auto later_b = makeNet(prefix, fmt::format("later{}_b", w));
        makeInst(sel_cell.get(),
                 prefix,
                 fmt::format("later_inv{}", w),
                 inv_cell_,
                 {{inv_ports_[{PortRoleType::DataIn, 0}], later},
                  {inv_ports_[{PortRoleType::DataOut, 0}], later_b}});
        take[w] = makeNet(prefix, fmt::format("take{}", w));
        makeInst(sel_cell.get(),
                 prefix,
                 fmt::format("take_and{}", w),
                 and2_cell_,
                 {{and2_ports_[{PortRoleType::DataIn, 0}], we[w]},
                  {and2_ports_[{PortRoleType::DataIn, 1}], later_b},
                  {and2_ports_[{PortRoleType::DataOut, 0}], take[w]}});
      }
      // any port from w on writes: the next earlier port's later, and at
      // port 0 the clock gate's enable
      auto any = w == 0 ? we_net : makeNet(prefix, fmt::format("later{}", w));
      makeInst(sel_cell.get(),
               prefix,
               fmt::format("any_or{}", w),
               or2_cell_,
               {{or2_ports_[{PortRoleType::DataIn, 0}], later},
                {or2_ports_[{PortRoleType::DataIn, 1}], we[w]},
                {or2_ports_[{PortRoleType::DataOut, 0}], any}});
      later = any;
    }
  }

  if (!shared) {
    word_write = {we_net, hold, gclock_net, reset_off, take, select_b_nets};
  }

  for (int local_bit = 0; local_bit < mask_size; ++local_bit) {
    if (start_bit_idx + local_bit >= word_bits_) {
      // a short fold's pad: its column empty in this row
      ram_grid_.addCell(nullptr,
                        bitColumn(slice_idx, local_bit, word_idx, mask_size));
      continue;
    }
    auto name = fmt::format("{}.bit{}", prefix, start_bit_idx + local_bit);
    vector<dbNet*> outs(r_total);
    for (int r_port = 0; r_port < r_total; ++r_port) {
      outs[r_port] = data_output[r_port][local_bit];
    }

    // the written data, and the flop's D: the written data, or with
    // -write_style mux the hold mux's output
    dbNet* d_data = data_input[0][local_bit];
    if (num_write > 1) {
      d_data = makeNet(name, "d");
    }
    dbNet* d = write_mux_ ? makeNet(name, "dhold") : d_data;
    const int bit = start_bit_idx + local_bit;
    dbNet* storage
        = and_or_read_ ? storage_nets_.at({row_idx, word_idx, bit}) : nullptr;
    if (write_mux_ && storage == nullptr) {
      storage = makeNet(name, "storage");
    }
    const int reset_bit
        = reset_port_ ? static_cast<int>(reset_bits_.at(bit)) : -1;
    auto bit_cell = makeBit(name,
                            and_or_read_ ? 0 : r_total,
                            gclock_net,
                            select_b_nets,
                            d,
                            outs,
                            storage,
                            reset_bit,
                            reset_off);
    if (write_mux_) {
      // the word's own value back where it is not written: the flop's
      // output, through an inverter where that output is inverted
      dbNet* old = storage;
      if (storage_inverted_) {
        old = makeNet(name, "q");
        makeInst(bit_cell.get(),
                 name,
                 "qinv",
                 inv_cell_,
                 {{inv_ports_[{PortRoleType::DataIn, 0}], storage},
                  {inv_ports_[{PortRoleType::DataOut, 0}], old}});
      }
      ao22(bit_cell.get(), name, "hold_mux", we_net, d_data, hold, old, d);
    }
    if (and_or_read_) {
      for (const auto& node : tree_nodes_[{row_idx, word_idx, bit}]) {
        makeInst(
            bit_cell.get(), node.prefix, node.name, node.master, node.conns);
      }
    }
    if (num_write > 1) {
      // the taken port's data: AO22 over pairs of ports, OR2 across pairs
      dbNet* acc = nullptr;
      for (int w = 0; w < num_write; w += 2) {
        const bool last = w + 2 >= num_write;
        dbNet* out = last && acc == nullptr
                         ? d_data
                         : makeNet(name, fmt::format("d_pair{}", w / 2));
        if (w + 1 < num_write && !ao22_cell_) {
          // no AO22 in the library: the AOI22 and an inverter
          auto out_b = makeNet(name, fmt::format("d_pair{}_b", w / 2));
          makeInst(bit_cell.get(),
                   name,
                   fmt::format("dmux{}", w / 2),
                   aoi22_cell_,
                   {{aoi22_in_a1_, take[w]},
                    {aoi22_in_a2_, data_input[w][local_bit]},
                    {aoi22_in_b1_, take[w + 1]},
                    {aoi22_in_b2_, data_input[w + 1][local_bit]},
                    {aoi22_out_, out_b}});
          makeInst(bit_cell.get(),
                   name,
                   fmt::format("dmux{}_inv", w / 2),
                   inv_cell_,
                   {{inv_ports_[{PortRoleType::DataIn, 0}], out_b},
                    {inv_ports_[{PortRoleType::DataOut, 0}], out}});
        } else if (w + 1 < num_write) {
          makeInst(bit_cell.get(),
                   name,
                   fmt::format("dmux{}", w / 2),
                   ao22_cell_,
                   {{ao22_in_[0], take[w]},
                    {ao22_in_[1], data_input[w][local_bit]},
                    {ao22_in_[2], take[w + 1]},
                    {ao22_in_[3], data_input[w + 1][local_bit]},
                    {ao22_out_, out}});
        } else {
          makeInst(bit_cell.get(),
                   name,
                   fmt::format("dmux{}", w / 2),
                   and2_cell_,
                   {{and2_ports_[{PortRoleType::DataIn, 0}], take[w]},
                    {and2_ports_[{PortRoleType::DataIn, 1}],
                     data_input[w][local_bit]},
                    {and2_ports_[{PortRoleType::DataOut, 0}], out}});
        }
        if (acc != nullptr) {
          dbNet* sum
              = last ? d_data : makeNet(name, fmt::format("d_or{}", w / 2));
          makeInst(bit_cell.get(),
                   name,
                   fmt::format("dor{}", w / 2),
                   or2_cell_,
                   {{or2_ports_[{PortRoleType::DataIn, 0}], acc},
                    {or2_ports_[{PortRoleType::DataIn, 1}], out},
                    {or2_ports_[{PortRoleType::DataOut, 0}], sum}});
          out = sum;
        }
        acc = out;
      }
    }

    int bit_col = bitColumn(slice_idx, local_bit, word_idx, mask_size);
    ram_grid_.addCell(std::move(bit_cell), bit_col);
  }

  // Make clock gate (with -write_style mux, none: the flops hold by mux)
  if (!write_mux_ && !shared) {
    std::vector<std::pair<std::string, dbNet*>> connections
        = {{clock_gate_ports_[{PortRoleType::Clock, 0}], clock},
           {clock_gate_ports_[{PortRoleType::DataIn, 0}], we_net},
           {clock_gate_ports_[{PortRoleType::DataOut, 0}], gclock_net}};
    if (!clock_gate_test_pins_.empty()) {
      dbNet* test_off = makeNet(prefix, "cg_test_off");
      makeInst(sel_cell.get(),
               prefix,
               "cg_tie",
               tie_low_cell_,
               {{tie_low_pin_, test_off}});
      for (const std::string& pin : clock_gate_test_pins_) {
        connections.emplace_back(pin, test_off);
      }
    }
    makeInst(sel_cell.get(), prefix, "cg", clock_gate_cell_, connections);
  }

  // Write path: this net is row_select AND with word_select so clock gate only
  // fires for the addressed word within the row. Read path handled by AOI mux.
  // word_select is nullptr when column_mux_ratio=1 (no mux needed).
  dbNet* write_sel = num_write > 1 ? nullptr : selects[0];
  if (word_select && num_write == 1) {
    write_sel = makeNet(prefix, "write_sel");
    makeInst(sel_cell.get(),
             prefix,
             "word_and",
             and2_cell_,
             {{and2_ports_[{PortRoleType::DataIn, 0}], selects[0]},
              {and2_ports_[{PortRoleType::DataIn, 1}], word_select},
              {and2_ports_[{PortRoleType::DataOut, 0}], write_sel}});
  }

  // Make clock and
  // this AND gate needs to be fed a net created by a decoder
  // adding any net will automatically connect with any port
  if (num_write == 1 && !shared) {
    makeInst(sel_cell.get(),
             prefix,
             "gcand",
             and2_cell_,
             {{and2_ports_[{PortRoleType::DataIn, 0}], write_sel},
              {and2_ports_[{PortRoleType::DataIn, 1}], write_enable[0]},
              {and2_ports_[{PortRoleType::DataOut, 0}], we_net}});
  }

  // Make select inverters
  if (create_select_inv && !and_or_read_ && !shared) {
    for (int i = 0; i < r_total; ++i) {
      if (selects[num_write + i] == nullptr) {
        continue;  // a banked read port's other banks
      }
      makeInst(sel_cell.get(),
               prefix,
               fmt::format("select_inv_{}", i),
               inv_cell_,
               {{inv_ports_[{PortRoleType::DataIn, 0}], selects[num_write + i]},
                {inv_ports_[{PortRoleType::DataOut, 0}], select_b_nets[i]}});
    }
  }

  int sel_col = selColumn(slice_idx, word_idx, mask_size);
  // a fold sharing its word's logic has an empty select cell
  ram_grid_.addCell(shared ? nullptr : std::move(sel_cell), sel_col);
}

// Note: For column_mux_ratio > 1, creates one shared select_b net per row and
// passes it to all makeSlice calls for words in the same row. Only the first
// word_idx creates the select_inv gate and subsequent words reuse the net
// This reduces horizontal routing congestion where
// col_mux_ratio=2: 1 shared net per row instead of 2
// col_mux ratio=4: 1 shared net per row instead of 4

void RamGen::makeWord(const int slices_per_word,
                      const int mask_size,
                      const int row_idx,
                      const int rw_ports,
                      const int r_ports,
                      const int w_ports,
                      const int word_idx,
                      const int column_mux_ratio,
                      dbNet* clock,
                      dbNet* word_select,
                      vector<vector<dbBTerm*>>& write_enable,
                      const vector<dbNet*>& selects,
                      const vector<dbNet*>& shared_select_b_nets,
                      const bool create_select_inv,
                      const vector<vector<dbNet*>>& data_input,
                      const vector<vector<dbNet*>>& data_output)
{
  int r_total = rw_ports + r_ports;
  for (int slice = 0; slice < slices_per_word; ++slice) {
    int start_idx = slice * mask_size;
    // per write port: its data bits and its write enable for this slice
    vector<vector<dbNet*>> slice_inputs;
    vector<dbNet*> slice_enables;
    for (size_t w = 0; w < data_input.size(); ++w) {
      slice_inputs.emplace_back(data_input[w].begin() + start_idx,
                                data_input[w].begin() + start_idx + mask_size);
      slice_enables.push_back(
          write_enable[w][bit_folds_ > 1 ? 0 : slice]->getNet());
    }
    std::vector<std::vector<odb::dbNet*>> slice_outputs;
    slice_outputs.reserve(r_total);
    for (int port = 0; port < r_total; ++port) {
      const auto& port_outputs = data_output[port];
      slice_outputs.emplace_back(port_outputs.begin() + start_idx,
                                 port_outputs.begin() + start_idx + mask_size);
    }
    makeSlice(slice,
              mask_size,
              row_idx,
              rw_ports,
              r_ports,
              w_ports,
              word_idx,
              column_mux_ratio,
              clock,
              slice_enables,
              word_select,
              selects,
              shared_select_b_nets,
              create_select_inv,
              slice_inputs,
              slice_outputs);
  }
}

std::unique_ptr<Layout> RamGen::generateTapColumn(const int num_rows,
                                                  const int tapcell_col)
{
  auto tapcell_layout = std::make_unique<Layout>(odb::vertical);
  for (int i = 0; i <= num_rows; ++i) {
    auto tapcell_cell = std::make_unique<Cell>();
    makeInst(tapcell_cell.get(),
             "tapcell",
             fmt::format("cell{}_{}", tapcell_col, i),
             tapcell_,
             {});
    tapcell_layout->addCell(std::move(tapcell_cell));
  }
  return tapcell_layout;
}

void RamGen::makeDecoderColumn(
    const std::string& prefix,
    const int num_rows,
    const std::vector<std::vector<dbNet*>>& addr_nets,
    const std::vector<dbNet*>& decoder_output_nets)
{
  auto decoder_layout = std::make_unique<Layout>(odb::vertical);
  marking_periphery_ = true;

  const int layers = std::ceil(std::log2(num_rows)) - 1;

  for (int word = 0; word < num_rows; ++word) {
    auto word_cell = std::make_unique<Cell>();
    dbNet* prev_net = decoder_output_nets[word];
    if (layers == 0) {
      // 2-word RAM: single address bit drives output net directly via buffer
      makeInst(word_cell.get(),
               prefix,
               fmt::format("buf_word{}", word),
               buffer_cell_,
               {{buffer_ports_[{PortRoleType::DataIn, 0}], addr_nets[word][0]},
                {buffer_ports_[{PortRoleType::DataOut, 0}], prev_net}});
    } else {
      for (int i = 0; i < layers; ++i) {
        dbNet* input_net = nullptr;
        if (i == layers - 1) {
          input_net = addr_nets[word][i + 1];
        } else {
          input_net  // NOLINTNEXTLINE(misc-include-cleaner)
              = makeNet(prefix, fmt::format("layer_in{}_word{}", i, word));
        }
        makeInst(word_cell.get(),
                 prefix,
                 fmt::format("and_layer{}_word{}", i, word),
                 and2_cell_,
                 {{and2_ports_[{PortRoleType::DataIn, 0}], addr_nets[word][i]},
                  {and2_ports_[{PortRoleType::DataIn, 1}], input_net},
                  {and2_ports_[{PortRoleType::DataOut, 0}], prev_net}});
        prev_net = input_net;
      }
    }
    decoder_layout->addCell(std::move(word_cell));
    for (int f = 1; f < bit_folds_; ++f) {
      decoder_layout->addCell(nullptr);  // the word's other folds
    }
  }
  marking_periphery_ = false;
  ram_grid_.addLayout(std::move(decoder_layout));
}

void RamGen::makeBufferColumn(const std::string& prefix,
                              const int num_rows,
                              const std::vector<dbNet*>& decoder_output_nets,
                              const std::vector<dbNet*>& select_nets,
                              const int words_per_row)
{
  auto buffer_layout = std::make_unique<Layout>(odb::vertical);
  marking_periphery_ = true;
  for (int row = 0; row < num_rows; ++row) {
    auto buf_cell = std::make_unique<Cell>();
    // a buffer per select: one per row, or with banks one per word
    for (int k = 0; k < words_per_row; ++k) {
      const int word = row * words_per_row + k;
      if (decoder_output_nets[word] == nullptr) {
        continue;  // a banked read port's other banks
      }
      makeInst(
          buf_cell.get(),
          prefix,
          fmt::format("sel_buf_{}", word),
          buffer_cell_,
          {{buffer_ports_[{PortRoleType::DataIn, 0}],
            decoder_output_nets[word]},
           {buffer_ports_[{PortRoleType::DataOut, 0}], select_nets[word]}});
    }
    buffer_layout->addCell(std::move(buf_cell));
    for (int f = 1; f < bit_folds_; ++f) {
      buffer_layout->addCell(nullptr);  // the word's other folds
    }
  }
  marking_periphery_ = false;
  ram_grid_.addLayout(std::move(buffer_layout));
}

void RamGen::makeWordDecoderColumn(
    const std::string& prefix,
    const int num_rows,
    const int words_per_row,
    const std::vector<std::vector<dbNet*>>& literals,
    const std::vector<dbNet*>& decoder_output_nets)
{
  auto decoder_layout = std::make_unique<Layout>(odb::vertical);
  marking_periphery_ = true;
  for (int row = 0; row < num_rows; ++row) {
    auto row_cell = std::make_unique<Cell>();
    for (int k = 0; k < words_per_row; ++k) {
      const int word = row * words_per_row + k;
      if (literals[word].empty()) {
        continue;  // a banked read port's other banks
      }
      // a balanced AND2 tree over the word's address literals
      vector<dbNet*> level = literals[word];
      int node = 0;
      while (level.size() > 1) {
        vector<dbNet*> next;
        for (size_t i = 0; i + 1 < level.size(); i += 2) {
          dbNet* y
              = level.size() == 2
                    ? decoder_output_nets[word]
                    : makeNet(prefix, fmt::format("y{}_word{}", node, word));
          makeInst(row_cell.get(),
                   prefix,
                   fmt::format("and{}_word{}", node++, word),
                   and2_cell_,
                   {{and2_ports_[{PortRoleType::DataIn, 0}], level[i]},
                    {and2_ports_[{PortRoleType::DataIn, 1}], level[i + 1]},
                    {and2_ports_[{PortRoleType::DataOut, 0}], y}});
          next.push_back(y);
        }
        if (level.size() % 2 == 1) {
          next.push_back(level.back());
        }
        level = std::move(next);
      }
      if (literals[word].size() == 1) {
        // a single literal: buffered to the decoder's output
        makeInst(row_cell.get(),
                 prefix,
                 fmt::format("buf_word{}", word),
                 buffer_cell_,
                 {{buffer_ports_[{PortRoleType::DataIn, 0}], level[0]},
                  {buffer_ports_[{PortRoleType::DataOut, 0}],
                   decoder_output_nets[word]}});
      }
    }
    decoder_layout->addCell(std::move(row_cell));
    for (int f = 1; f < bit_folds_; ++f) {
      decoder_layout->addCell(nullptr);  // the word's other folds
    }
  }
  marking_periphery_ = false;
  ram_grid_.addLayout(std::move(decoder_layout));
}

std::unique_ptr<Layout> RamGen::makeInverterColumn(
    const int num_rows,
    const int num_row_bits,
    const int start_port,
    const int end_port,
    const int num_word_bits,
    const std::vector<std::vector<dbNet*>>& inv_addr_nets)
{
  auto cell_inv_layout = std::make_unique<Layout>(odb::vertical);
  int ports_in_col = end_port - start_port;
  marking_periphery_ = true;

  // number of inverters per bit group (one per port in this column)
  // fillers evenly space each bit group within the column
  int fillers_per_group = (num_rows / num_row_bits) - ports_in_col;
  // ensures we don't go negative for small rams
  fillers_per_group = std::max(0, fillers_per_group);

  for (int bit = num_row_bits - 1; bit >= 0; --bit) {
    for (int p = start_port; p < end_port; ++p) {
      // a banked read port's address is narrower than the others
      if (num_word_bits + bit >= static_cast<int>(addr_inputs_[p].size())) {
        cell_inv_layout->addCell(nullptr);
        continue;
      }
      auto inv_grid_cell = std::make_unique<Cell>();
      const int a = num_word_bits + bit;
      dbNet* flop_out = addr_flop_out_[p][a];
      if (flop_out) {
        // -register_read_address 1: the address through a flop, beside its
        // inverter
        makeInst(inv_grid_cell.get(),
                 fmt::format("decoder_p{}", p),
                 fmt::format("addr_reg_p{}_a{}", p, bit),
                 addr_flop_cell_,
                 {{addr_flop_d_, addr_inputs_[p][a]->getNet()},
                  {addr_flop_clk_, clock_net_},
                  {addr_flop_q_, flop_out}});
      }
      // the inverter: the address (or the flop's) in, its complement out;
      // from a flop holding the complement, the address out
      const bool from_complement = flop_out && addr_flop_inverted_;
      makeInst(inv_grid_cell.get(),
               fmt::format("decoder_p{}", p),
               fmt::format("inv_p{}_a{}", p, bit),
               inv_cell_,
               {{inv_ports_[{PortRoleType::DataIn, 0}],
                 from_complement ? inv_addr_nets[p][a] : addr_true_[p][a]},
                {inv_ports_[{PortRoleType::DataOut, 0}],
                 from_complement ? addr_true_[p][a] : inv_addr_nets[p][a]}});
      cell_inv_layout->addCell(std::move(inv_grid_cell));
    }
    // add fillers between bit groups
    for (int f = 0; f < fillers_per_group; ++f) {
      cell_inv_layout->addCell(nullptr);
    }
  }

  marking_periphery_ = false;
  // pad any remaining rows
  int total_placed = (ports_in_col + fillers_per_group) * num_row_bits;
  for (int f = total_placed; f < num_rows; ++f) {
    cell_inv_layout->addCell(nullptr);
  }

  return cell_inv_layout;
}

std::vector<dbNet*> RamGen::makeDecoderOutputNets(const std::string& prefix,
                                                  const int num_rows)
{
  std::vector<dbNet*> decoder_output_nets(num_rows);
  for (int word = 0; word < num_rows; ++word) {
    decoder_output_nets[word] = makeNet(prefix, fmt::format("word{}", word));
  }
  return decoder_output_nets;
}

std::vector<dbNet*> RamGen::makeSelectNets(const std::string& prefix,
                                           const int num_rows,
                                           RamPortType port_type)
{
  std::vector<dbNet*> select_nets(num_rows);
  std::string net_prefix;
  if (port_type == RamPortType::ReadWrite) {
    net_prefix = "select_wr";
  } else if (port_type == RamPortType::Write) {
    net_prefix = "select_w";
  } else {
    net_prefix = "select_r";
  }
  for (int i = 0; i < num_rows; ++i) {  // NOLINTNEXTLINE(misc-include-cleaner)
    select_nets[i] = makeNet(prefix, fmt::format("{}{}", net_prefix, i));
  }
  return select_nets;
}

dbMaster* RamGen::findMaster(
    const std::function<bool(sta::LibertyPort*)>& match,
    const char* name,
    const bool required,
    dbMaster* given,
    const bool q_and_qn)
{
  if (given) {
    auto liberty = network_->libertyCell(network_->dbToSta(given));
    bool matched = false;
    if (liberty && match) {
      std::unique_ptr<sta::ConcreteCellPortIterator> ports(
          liberty->portIterator());
      while (ports->hasNext() && !matched) {
        auto port = ports->next()->libertyPort();
        matched = port && port->direction()->isAnyOutput() && match(port);
      }
    }
    if (!matched) {
      logger_->error(
          RAM, 79, "Cell {} is not a {} cell.", given->getName(), name);
    }
    return given;
  }
  dbMaster* best = nullptr;
  float best_area = std::numeric_limits<float>::max();

  for (auto lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      auto cell = network_->dbToSta(master);
      if (!cell) {
        continue;
      }
      auto liberty = network_->libertyCell(cell);
      if (!liberty) {
        continue;
      }
      // not a power domain's isolation cell or level shifter, whose
      // functions look like logic
      if (liberty->isIsolationCell() || liberty->isLevelShifter()) {
        continue;
      }

      if (!match) {
        if (liberty->portCount() == 0) {
          best_area = liberty->area();
          best = master;
        }
        continue;
      }

      auto port_iter = std::unique_ptr<sta::ConcreteCellPortIterator>(
          liberty->portIterator());

      sta::ConcretePort* out = nullptr;
      int outputs = 0;
      bool flop_outputs = true;  // every output a flop's Q or QN
      while (port_iter->hasNext()) {
        auto lib_port = port_iter->next();
        auto dir = lib_port->direction();
        if (dir->isAnyOutput()) {
          ++outputs;
          flop_outputs &= lib_port->libertyPort()->isRegOutput();
          if (!out || (q_and_qn && !match(out->libertyPort()))) {
            out = lib_port;
          }
        }
      }
      if (outputs > 1 && !(q_and_qn && outputs == 2 && flop_outputs)) {
        continue;  // no multi-output gates
      }

      if (!out || !match(out->libertyPort())) {
        continue;
      }
      // not a cell the library keeps from use (a tie: findTie)
      if (liberty->dontUse()) {
        continue;
      }

      if (liberty->area() < best_area) {
        best_area = liberty->area();
        best = master;
      }
    }
  }

  if (!best) {
    if (!required) {
      return nullptr;
    }
    logger_->error(RAM, 10, "Can't find {} cell", name);
  }
  logger_->info(RAM, 16, "Selected {} cell {}", name, best->getName());
  return best;
}

std::map<PortRole, std::string> RamGen::buildPortMap(dbMaster* master,
                                                     bool* inverted)
{
  if (inverted) {
    *inverted = false;
  }
  std::string inverted_out;  // a flop's inverted output (QN)
  auto sta_cell = network_->dbToSta(master);
  auto liberty = network_->libertyCell(sta_cell);
  std::map<PortRole, std::string> pin_map;
  int in_idx = 0;

  // needed since there is no tristate enable flag
  std::string tri_enable_name;

  auto port_iter
      = std::unique_ptr<sta::ConcreteCellPortIterator>(liberty->portIterator());
  while (port_iter->hasNext()) {
    auto concrete = port_iter->next();
    auto lib_port = concrete->libertyPort();
    auto dir = concrete->direction();

    if (lib_port->isPwrGnd()) {
      auto pwr_gnd_type = lib_port->pwrGndType();
      if (pwr_gnd_type == sta::PwrGndType::primary_power) {
        pin_map[{PortRoleType::Power, 0}] = lib_port->name();
      } else if (pwr_gnd_type == sta::PwrGndType::primary_ground) {
        pin_map[{PortRoleType::Ground, 0}] = lib_port->name();
      }
    } else if (lib_port->isClock() || lib_port->isRegClk()
               || lib_port->isClockGateClock()) {
      pin_map[{PortRoleType::Clock, 0}] = lib_port->name();
    } else if (dir->isTristate()) {
      pin_map[{PortRoleType::DataOut, 0}] = lib_port->name();
      auto tri_expr = lib_port->tristateEnable();
      // can only get the name of enable once the output is found to be a
      // tristate
      if (tri_expr && tri_expr->op() == sta::FuncExpr::Op::port) {
        tri_enable_name = tri_expr->port()->name();
      } else if (tri_expr && tri_expr->op() == sta::FuncExpr::Op::not_) {
        tri_enable_name = tri_expr->left()->port()->name();
      }
    } else if (dir->isAnyOutput()) {  // catches isOutput()
      auto lib_cell = lib_port->libertyCell();
      auto func = lib_port->function();
      bool is_seq_output = false;
      bool is_seq_inverted = false;

      if (func && func->op() == sta::FuncExpr::Op::port) {
        auto internal_port = lib_cell->findLibertyPort(func->port()->name());
        if (internal_port) {
          auto seq = lib_cell->outputPortSequential(internal_port);
          if (seq) {
            is_seq_output = true;
            // outputInv() is the negative state variable (IQN); a state
            // that stores the data inverted (next_state "!D", asap7's
            // DFFHQNx1) inverts it again
            const bool stores_inverted
                = seq->data() && seq->data()->op() == sta::FuncExpr::Op::not_;
            is_seq_inverted
                = (seq->outputInv() == internal_port) != stores_inverted;
          }
        }
      }

      if (is_seq_output) {
        // only assign if this is the non-inverted output (Q and not Qn)
        if (!is_seq_inverted) {
          pin_map[{PortRoleType::DataOut, 0}] = lib_port->name();
        } else {
          inverted_out = lib_port->name();
        }
      } else {
        // non-sequential cell (AND2, INV etc) assign as DataOut
        pin_map[{PortRoleType::DataOut, 0}] = lib_port->name();
      }
    } else if (dir->isInput()) {
      pin_map[{PortRoleType::DataIn, in_idx++}] = lib_port->name();
    }
  }

  // A flop whose only data output is QN: QN stands as DataOut, for a
  // caller that says it can take the inversion.
  if (inverted && !inverted_out.empty()
      && pin_map.find({PortRoleType::DataOut, 0}) == pin_map.end()) {
    pin_map[{PortRoleType::DataOut, 0}] = inverted_out;
    *inverted = true;
  }

  // second pass for to assign tristate enable correct role
  // first pass can not classify without a dedicated tristate flag
  if (!tri_enable_name.empty()) {
    // find and remove it from DataIn
    for (auto it = pin_map.begin(); it != pin_map.end(); ++it) {
      if (it->second == tri_enable_name
          && it->first.type == PortRoleType::DataIn) {
        pin_map.erase(it);
        break;
      }
    }
    pin_map[{PortRoleType::TriEnable, 0}] = std::move(tri_enable_name);
  }

  // validate power/ground after classification is complete
  int power_count = 0, ground_count = 0;
  for (auto& [role, name] : pin_map) {
    if (role.type == PortRoleType::Power) {
      ++power_count;
    }
    if (role.type == PortRoleType::Ground) {
      ++ground_count;
    }
  }
  if (power_count != 1) {
    logger_->error(RAM,
                   28,
                   "Cell {} must have exactly 1 primary power pin, found {}",
                   master->getName(),
                   power_count);
  }
  if (ground_count != 1) {
    logger_->error(RAM,
                   29,
                   "Cell {} must have exactly 1 primary ground pin, found {}",
                   master->getName(),
                   ground_count);
  }

  for (auto& [role, name] : pin_map) {
    if (role.type == PortRoleType::Power) {
      power_pin_names_.insert(name);
    } else if (role.type == PortRoleType::Ground) {
      ground_pin_names_.insert(name);
    }
  }

  return pin_map;
}

// Evaluates a liberty function expression given its input ports' values.
static bool evalFunc(const sta::FuncExpr* expr,
                     const std::vector<sta::LibertyPort*>& ports,
                     const std::vector<bool>& vals)
{
  if (!expr) {
    return false;
  }
  switch (expr->op()) {
    case sta::FuncExpr::Op::port: {
      // look up this port's value in the input vector
      auto p = expr->port();
      for (int i = 0; i < (int) ports.size(); ++i) {
        if (ports[i] == p) {
          return vals[i];
        }
      }
      return false;
    }
    case sta::FuncExpr::Op::not_:
      return !evalFunc(expr->left(), ports, vals);
    case sta::FuncExpr::Op::and_:
      return evalFunc(expr->left(), ports, vals)
             && evalFunc(expr->right(), ports, vals);
    case sta::FuncExpr::Op::or_:
      return evalFunc(expr->left(), ports, vals)
             || evalFunc(expr->right(), ports, vals);
    case sta::FuncExpr::Op::xor_:
      return evalFunc(expr->left(), ports, vals)
             != evalFunc(expr->right(), ports, vals);
    case sta::FuncExpr::Op::one:
      return true;
    case sta::FuncExpr::Op::zero:
      return false;
    default:
      return false;
  }
}

// The truth table of a two-input single-output cell's output, bit k its
// value for inputs k in binary; -1 for any other cell. Symmetric
// functions only (AND, OR, NAND, NOR), so the inputs' order is moot.
static int twoInputTable(sta::LibertyPort* out)
{
  auto func = out->function();
  if (!func || !out->direction()->isOutput()) {
    return -1;
  }
  std::vector<sta::LibertyPort*> inputs;
  std::unique_ptr<sta::ConcreteCellPortIterator> port_iter(
      out->libertyCell()->portIterator());
  while (port_iter->hasNext()) {
    auto p = static_cast<sta::ConcretePort*>(port_iter->next());
    if (p->direction()->isInput()) {
      inputs.push_back(p->libertyPort());
    }
  }
  if (inputs.size() != 2) {
    return -1;
  }
  int table = 0;
  for (int k = 0; k < 4; ++k) {
    if (evalFunc(func, inputs, {(k & 1) != 0, (k & 2) != 0})) {
      table |= 1 << k;
    }
  }
  return table;
}

dbMaster* RamGen::findTie(const bool one, dbMaster* given, std::string& pin)
{
  const auto level = one ? sta::FuncExpr::Op::one : sta::FuncExpr::Op::zero;
  const char* name = one ? "tie high" : "tie low";
  // the cell's output at `level`, or none
  auto output = [level](sta::LibertyCell* liberty) -> sta::LibertyPort* {
    std::unique_ptr<sta::ConcreteCellPortIterator> ports(
        liberty->portIterator());
    while (ports->hasNext()) {
      auto port = ports->next()->libertyPort();
      if (port && port->direction()->isOutput() && port->function()
          && port->function()->op() == level) {
        return port;
      }
    }
    return nullptr;
  };
  dbMaster* best = nullptr;
  sta::LibertyPort* best_port = nullptr;
  float best_area = std::numeric_limits<float>::max();
  for (auto lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      if (given && master != given) {
        continue;
      }
      auto liberty = network_->libertyCell(network_->dbToSta(master));
      if (!liberty || liberty->isIsolationCell() || liberty->isLevelShifter()) {
        continue;
      }
      // a tie cell may be dont_use, which keeps it from synthesis
      // (Nangate45's LOGIC0_X1 and LOGIC1_X1); it is the cell to tie with
      auto port = output(liberty);
      if (port && liberty->area() < best_area) {
        best_area = liberty->area();
        best = master;
        best_port = port;
      }
    }
  }
  if (!best) {
    if (given) {
      logger_->error(
          RAM, 107, "Cell {} is not a {} cell.", given->getName(), name);
    }
    logger_->error(RAM, 108, "Can't find {} cell", name);
  }
  if (!given) {
    logger_->info(RAM, 109, "Selected {} cell {}", name, best->getName());
  }
  pin = best_port->name();
  return best;
}

// `text` as a word of `width` bits, least significant first: decimal,
// hexadecimal after 0x or octal after 0, as C writes them. Empty where
// `text` is not such a number or needs more than `width` bits; `fits`
// says which.
static std::vector<bool> parseWord(const std::string& text,
                                   const int width,
                                   bool& fits)
{
  fits = true;
  int base = 10;
  size_t i = 0;
  if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    base = 16;
    i = 2;
  } else if (text.size() > 1 && text[0] == '0') {
    base = 8;
    i = 1;
  }
  if (i >= text.size()) {
    return {};
  }
  std::vector<uint32_t> limbs{0};  // least significant first
  for (; i < text.size(); ++i) {
    const char c
        = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    int digit = base;
    if (c >= '0' && c <= '9') {
      digit = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      digit = c - 'a' + 10;
    }
    if (digit >= base) {
      return {};
    }
    uint64_t carry = digit;
    for (uint32_t& limb : limbs) {
      const uint64_t v = static_cast<uint64_t>(limb) * base + carry;
      limb = static_cast<uint32_t>(v);
      carry = v >> 32;
    }
    if (carry) {
      limbs.push_back(static_cast<uint32_t>(carry));
    }
  }
  std::vector<bool> bits(width);
  for (size_t b = 0; b < limbs.size() * 32; ++b) {
    const bool one = (limbs[b / 32] >> (b % 32)) & 1;
    if (one && b >= static_cast<size_t>(width)) {
      fits = false;
      return {};
    }
    if (b < static_cast<size_t>(width)) {
      bits[b] = one;
    }
  }
  return bits;
}

void RamGen::findResetPins()
{
  auto liberty = network_->libertyCell(network_->dbToSta(storage_cell_));
  const sta::Sequential* seq = nullptr;
  for (const auto& s : liberty->sequentials()) {
    if (s.isRegister() && s.clear() && s.preset()) {
      seq = &s;
      break;
    }
  }
  if (!seq) {
    logger_->error(RAM,
                   61,
                   "Storage cell {} has no asynchronous clear and preset for "
                   "-reset_value.",
                   storage_cell_->getName());
  }
  // a pin of the ff group, and whether it acts low: "!RN" or "RN"
  auto pin = [&](const sta::FuncExpr* e, bool& low) -> std::string {
    low = e->op() == sta::FuncExpr::Op::not_;
    const sta::FuncExpr* p = low ? e->left() : e;
    if (p->op() != sta::FuncExpr::Op::port) {
      logger_->error(RAM,
                     62,
                     "Storage cell {}: a clear, preset or data that is not "
                     "one pin is not supported.",
                     storage_cell_->getName());
    }
    return p->port()->name();
  };
  bool clear_low = false;
  bool preset_low = false;
  bool data_inverted = false;
  storage_clear_pin_ = pin(seq->clear(), clear_low);
  storage_preset_pin_ = pin(seq->preset(), preset_low);
  storage_data_pin_ = pin(seq->data(), data_inverted);
  if (clear_low != preset_low) {
    logger_->error(RAM,
                   63,
                   "Storage cell {}: clear and preset act on opposite levels.",
                   storage_cell_->getName());
  }
  reset_active_low_ = clear_low;
  storage_ports_[{PortRoleType::DataIn, 0}] = storage_data_pin_;

  // A clear sets the ff group's first state variable to 0; the output
  // pin reads it or its complement, and the data read back is that,
  // inverted again where the storage output is (asap7's DFFASRHQNx1
  // stores !D: its `clear` sets the data to 1).
  auto out = liberty->findLibertyPort(
      storage_ports_[{PortRoleType::DataOut, 0}].c_str());
  bool reads_complement = false;
  if (out && out->function()
      && out->function()->op() == sta::FuncExpr::Op::port) {
    reads_complement = seq->outputInv() == out->function()->port();
  }
  clear_reads_ = (reads_complement ? 1 : 0) ^ (storage_inverted_ ? 1 : 0);

  // the unused pin of each flop held at its inactive level
  const bool off_high = reset_active_low_;
  tie_off_cell_ = findTie(off_high, tie_off_cell_, tie_off_pin_);

  bool fits = true;
  reset_bits_ = parseWord(reset_value_, word_bits_, fits);
  if (!fits) {
    logger_->error(RAM,
                   100,
                   "-reset_value {} does not fit a word of {} bits.",
                   reset_value_,
                   word_bits_);
  }
  if (reset_bits_.empty()) {
    logger_->error(RAM,
                   64,
                   "-reset_value {} is not a number: decimal, 0x hexadecimal "
                   "or 0 octal.",
                   reset_value_);
  }
}

void RamGen::findMasters()
{
  inv_cell_ = findMaster(
      [](sta::LibertyPort* port) { return port->libertyCell()->isInverter(); },
      "inverter",
      /*required=*/true,
      inv_cell_);
  inv_ports_ = buildPortMap(inv_cell_);

  // The read path: tristate bitlines where the library has a tristate
  // cell, AND-OR trees where it has none, or as -read_style says.
  if (read_style_ == "and_or") {
    and_or_read_ = true;
    if (tristate_cell_) {
      logger_->warn(RAM,
                    104,
                    "-tristate_cell {} is not used: -read_style and_or reads "
                    "without tristates.",
                    tristate_cell_->getName());
    }
  } else {
    if (!tristate_cell_) {
      tristate_cell_ = findMaster(
          [](sta::LibertyPort* port) {
            if (!port->direction()->isTristate()) {
              return false;
            }
            auto function = port->function();
            return function->op() != sta::FuncExpr::Op::not_;
          },
          "tristate",
          /*required=*/read_style_ == "tristate");
    }
    if (tristate_cell_) {
      tristate_ports_ = buildPortMap(tristate_cell_);
    } else {
      and_or_read_ = true;
      logger_->info(RAM, 57, "No tristate cell: reads through AND-OR trees.");
    }
  }

  and2_cell_ = findMaster(
      [](sta::LibertyPort* port) {
        if (!port->direction()->isOutput()) {
          return false;
        }
        auto function = port->function();
        return function && function->op() == sta::FuncExpr::Op::and_
               && function->left()->op() == sta::FuncExpr::Op::port
               && function->right()->op() == sta::FuncExpr::Op::port;
      },
      "and2",
      /*required=*/true,
      and2_cell_);
  and2_ports_ = buildPortMap(and2_cell_);

  if (!use_latch_) {
    if (!storage_cell_) {
      // FIXME
      // Still needs changes to get right type of flip-flop
      const bool reset = !reset_value_.empty();
      storage_cell_ = findMaster(
          [reset](sta::LibertyPort* port) {
            if (!port->isRegOutput()) {
              return false;
            }
            // with a reset, a positive-edge flop with both an asynchronous
            // clear and preset, so any bit can take either value
            if (reset) {
              bool clear_preset = false;
              for (const auto& seq : port->libertyCell()->sequentials()) {
                clear_preset |= seq.isRegister() && seq.clear() && seq.preset()
                                && seq.clock()
                                && seq.clock()->op() == sta::FuncExpr::Op::port;
              }
              if (!clear_preset) {
                return false;
              }
            }
            // looking for DFF specifically
            auto cell = port->libertyCell();
            auto port_iter = cell->portIterator();
            while (port_iter->hasNext()) {
              auto p = port_iter->next()->libertyPort();
              // check to filter out latches
              if (p && p->isLatchData()) {
                delete port_iter;
                return false;
              }
            }
            delete port_iter;
            return true;
          },
          "storage",
          /*required=*/true,
          /*given=*/nullptr,
          /*q_and_qn=*/reset);
    }
    // the AND-OR read path takes a flop whose only output is QN; its
    // trees undo the inversion
    storage_ports_ = buildPortMap(storage_cell_,
                                  and_or_read_ ? &storage_inverted_ : nullptr);
    if (storage_ports_.find({PortRoleType::DataOut, 0})
        == storage_ports_.end()) {
      logger_->error(RAM,
                     58,
                     "Storage cell {} has no non-inverted output; use "
                     "-read_style and_or.",
                     storage_cell_->getName());
    }
    if (!reset_value_.empty()) {
      findResetPins();
    }
  }

  // -write_style mux holds every flop by a mux, with no clock gate
  if (!write_mux_) {
    clock_gate_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          return port->libertyCell()->isClockGate();
        },
        "clock gate",
        /*required=*/true,
        clock_gate_cell_);
    clock_gate_ports_ = buildPortMap(clock_gate_cell_);
    // The enable by its liberty role: an integrated clock gate's enable
    // and test enable (asap7's SE) are both inputs, in no set order. The
    // test enable is tied low, so the gate opens on the enable alone.
    clock_gate_test_pins_.clear();
    auto liberty = network_->libertyCell(network_->dbToSta(clock_gate_cell_));
    std::string enable;
    std::vector<std::string> others;
    std::unique_ptr<sta::ConcreteCellPortIterator> ports(
        liberty->portIterator());
    while (ports->hasNext()) {
      auto port = ports->next()->libertyPort();
      if (port == nullptr || port->isPwrGnd() || !port->direction()->isInput()
          || port->isClockGateClock() || port->isClock()) {
        continue;
      }
      if (port->isClockGateEnable()) {
        enable = port->name();
      } else {
        others.emplace_back(port->name());
      }
    }
    if (!enable.empty()) {
      clock_gate_ports_[{PortRoleType::DataIn, 0}] = enable;
      clock_gate_test_pins_ = others;
    }
    if (!clock_gate_test_pins_.empty()) {
      tie_low_cell_ = findTie(/*one=*/false, nullptr, tie_low_pin_);
    }
  }

  // for input buffers
  buffer_cell_ = findMaster(
      [](sta::LibertyPort* port) { return port->libertyCell()->isBuffer(); },
      "buffer",
      /*required=*/true,
      buffer_cell_);
  buffer_ports_ = buildPortMap(buffer_cell_);

  // latch cells for latch-based ram: find neg/pos latch cell
  if (use_latch_ && !latch_cell_) {
    latch_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          if (!port->direction()->isOutput()) {
            return false;
          }
          auto cell = port->libertyCell();
          if (!cell->isSequential()) {
            return false;
          }
          bool has_latch_data = false;
          bool has_gate = false;
          auto port_iter = cell->portIterator();
          while (port_iter->hasNext()) {
            auto lp = port_iter->next()->libertyPort();
            if (lp) {
              if (lp->isLatchData()) {
                has_latch_data = true;
              }
              if (lp->isClock()) {
                has_gate = true;
              }
            }
          }
          delete port_iter;
          return has_latch_data && has_gate;
        },
        "latch");
  }

  if (use_latch_ && latch_cell_) {
    latch_ports_ = buildPortMap(latch_cell_);
  }

  // aoi cells used for column mux functionality when column_mux_ratio > 1
  // uses truth table simulation to identify AOI22 and discover port names
  // to work for all PDKs, avoiding hardcoding PDK-specific expression parsing
  {
    // AOI22 truth table: Y = !(A1&A2 | B1&B2); a given cell's pins read
    // the same way
    std::array<std::string, 4> in;
    aoi22_cell_ = findMaster4(
        0x0777, "aoi22", in, aoi22_out_, /*required=*/true, aoi22_cell_);
    aoi22_in_a1_ = in[0];
    aoi22_in_a2_ = in[1];
    aoi22_in_b1_ = in[2];
    aoi22_in_b2_ = in[3];
  }

  // The AND-OR read path's tree: NAND2 and NOR2 levels over AOI22 leaves.
  if (and_or_read_) {
    // by truth table: asap7 writes its NAND2 as (!A) + (!B)
    nand2_cell_ = findMaster(
        [](sta::LibertyPort* port) { return twoInputTable(port) == 0x7; },
        "nand2",
        /*required=*/true,
        nand2_cell_);
    nand2_ports_ = buildPortMap(nand2_cell_);
    nor2_cell_ = findMaster(
        [](sta::LibertyPort* port) { return twoInputTable(port) == 0x1; },
        "nor2",
        /*required=*/true,
        nor2_cell_);
    nor2_ports_ = buildPortMap(nor2_cell_);
  }

  // More than one write port: the write data mux, an AO22 per pair of
  // ports, and an OR2 for the write enables and across pairs.
  if (num_write_ > 1 || write_mux_) {
    // AO22 truth table: Y = A1&A2 | B1&B2
    ao22_cell_ = findMaster4(
        0xF888, "ao22", ao22_in_, ao22_out_, /*required=*/false, ao22_cell_);
  }
  if (num_write_ > 1) {
    or2_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          if (!port->direction()->isOutput()) {
            return false;
          }
          auto function = port->function();
          return function && function->op() == sta::FuncExpr::Op::or_
                 && function->left()->op() == sta::FuncExpr::Op::port
                 && function->right()->op() == sta::FuncExpr::Op::port;
        },
        "or2",
        /*required=*/true,
        or2_cell_);
    or2_ports_ = buildPortMap(or2_cell_);
  }
}

dbMaster* RamGen::findMaster4(const uint16_t table,
                              const char* name,
                              std::array<std::string, 4>& inputs_out,
                              std::string& output,
                              const bool required,
                              dbMaster* given)
{
  // for 4 inputs ordered (A1,A2,B1,B2) mapped to bits (0,1,2,3),
  // bit k of the table = output when inputs = k in binary.

  // compute 16-entry truth table for a function given 4 ordered input ports.
  // each bit k of the result = output when inputs = k in binary. (i.e. k= 2 =
  // 10)
  auto computeTable
      = [](const sta::FuncExpr* func,
           const std::vector<sta::LibertyPort*>& inputs) -> uint16_t {
    uint16_t table = 0;
    for (int k = 0; k < 16; ++k) {
      std::vector<bool> vals(4);
      for (int i = 0; i < 4; ++i) {
        vals[i] = (k >> i) & 1;
      }
      if (evalFunc(func, inputs, vals)) {
        table |= (1 << k);
      }
    }
    return table;
  };

  // there are exactly 3 unique ways to split 4 ports into 2 pairs for AOI22,
  // each pairing assigns (inputs[0],inputs[1]) to group A and
  // (inputs[2],inputs[3]) to group B. Try all 3 and check which pattern
  // pairing produces the correct AOI22 truth table, which is the correct port
  // mapping
  static const int kPairings[3][4] = {
      {0, 1, 2, 3},  // group A=(p0,p1), group B=(p2,p3)
      {0, 2, 1, 3},  // group A=(p0,p2), group B=(p1,p3)
      {0, 3, 1, 2},  // group A=(p0,p3), group B=(p1,p2)
  };

  auto match = [&](sta::LibertyPort* out_port) -> bool {
    if (!out_port->direction()->isOutput()) {
      return false;
    }
    auto func = out_port->function();
    if (!func) {
      return false;
    }
    auto cell = out_port->libertyCell();

    // collect all input ports for this cell
    std::vector<sta::LibertyPort*> inputs;
    std::unique_ptr<sta::ConcreteCellPortIterator> port_iter(
        cell->portIterator());
    while (port_iter->hasNext()) {
      auto p = static_cast<sta::ConcretePort*>(port_iter->next());
      if (p->direction()->isInput()) {
        inputs.push_back(p->libertyPort());
      }
    }

    // aoi22 must have exactly 4 inputs
    if (inputs.size() != 4) {
      return false;
    }

    // sort ports by name for deterministic ordering across builds
    std::ranges::sort(inputs, [](sta::LibertyPort* a, sta::LibertyPort* b) {
      return std::string(a->name()) < std::string(b->name());
    });

    // try each pairing, if truth table matches, record port names
    for (auto& pairing : kPairings) {
      std::vector<sta::LibertyPort*> ordered = {inputs[pairing[0]],
                                                inputs[pairing[1]],
                                                inputs[pairing[2]],
                                                inputs[pairing[3]]};
      if (computeTable(func, ordered) == table) {
        // found matching pairing so this is the correct port mapping,
        // store port names for later use
        // ordered[0] = A1, ordered[1] = A2, ordered[2] = B1, ordered[3] =
        // B2
        for (int i = 0; i < 4; ++i) {
          inputs_out[i] = ordered[i]->name();
        }
        output = out_port->name();
        return true;
      }
    }
    return false;
  };
  // the cell given, or the smallest that matches; its pins are read from
  // it, whichever matched last in the search
  dbMaster* best = given ? given : findMaster(match, name, required);
  if (best) {
    bool matched = false;
    auto liberty = network_->libertyCell(network_->dbToSta(best));
    std::unique_ptr<sta::ConcreteCellPortIterator> ports(
        liberty->portIterator());
    while (ports->hasNext() && !matched) {
      auto port = ports->next()->libertyPort();
      if (port && port->direction()->isOutput()) {
        matched = match(port);
      }
    }
    if (!matched) {
      logger_->error(
          RAM, 110, "Cell {} is not a {} cell.", best->getName(), name);
    }
  }
  return best;
}

void RamGen::ramPdngen(const char* power_net_name,
                       const char* ground_net_name,
                       const char* route_name,
                       int route_width,
                       const char* ver_name,
                       int ver_width,
                       int ver_pitch,
                       const char* hor_name,
                       int hor_width,
                       int hor_pitch)
{
  const odb::Rect& die = block_->getDieArea();
  const double dbu_per_um = block_->getDb()->getDbuPerMicron();

  // check that vertical strap pitch is at least 4 bits wide for column side
  if (ver_pitch < 4 * ram_grid_.getWidth()) {
    logger_->warn(RAM,
                  35,
                  "Vertical strap pitch ({:.2f} um) is less than 4 bit-columns "
                  "wide. try increasing -ver_layer pitch.",
                  ver_pitch / dbu_per_um);
  }

  if (die.dy() < hor_pitch) {
    logger_->error(RAM,
                   31,
                   "Die height ({:.2f} um) is less than horizontal strap pitch "
                   "({:.2f} um). "
                   "Use a smaller -hor_layer pitch.",
                   die.dy() / dbu_per_um,
                   hor_pitch / dbu_per_um);
  }

  if (die.dx() < ver_pitch) {
    logger_->error(
        RAM,
        32,
        "Die width ({:.2f} um) is less than vertical strap pitch ({:.2f} um). "
        "Use a smaller -ver_layer pitch.",
        die.dx() / dbu_per_um,
        ver_pitch / dbu_per_um);
  }

  auto power_net = block_->findNet(power_net_name);
  if (!power_net) {
    power_net = dbNet::create(block_, power_net_name);
  }
  if (!power_net) {
    logger_->error(RAM, 38, "Failed to create power net '{}'", power_net_name);
  }

  auto ground_net = block_->findNet(ground_net_name);
  if (!ground_net) {
    ground_net = dbNet::create(block_, ground_net_name);
  }
  if (!ground_net) {
    logger_->error(
        RAM, 39, "Failed to create ground net '{}'", ground_net_name);
  }

  power_net->setSpecial();
  power_net->setSigType(odb::dbSigType::POWER);
  ground_net->setSpecial();
  ground_net->setSigType(odb::dbSigType::GROUND);

  for (const auto& pin_name : power_pin_names_) {
    block_->addGlobalConnect(nullptr, ".*", pin_name.c_str(), power_net, true);
  }

  for (const auto& pin_name : ground_pin_names_) {
    block_->addGlobalConnect(nullptr, ".*", pin_name.c_str(), ground_net, true);
  }

  block_->globalConnect(false, false);

  std::string grid_name = "ram_grid";
  pdngen_->setCoreDomain(power_net, nullptr, ground_net, {});
  pdngen_->makeCoreGrid(pdngen_->findDomain("Core"),
                        grid_name,
                        pdn::StartsWith::kGround,
                        {},
                        {},
                        nullptr,
                        nullptr,
                        "STAR",
                        {});

  // variables for convenience
  auto pdn_tech = block_->getDb()->getTech();
  auto grid = pdngen_->findGrid(grid_name).front();

  // parameters are the same in the tcl script
  // add_followpin
  pdngen_->makeFollowpin(grid,
                         pdn_tech->findLayer(route_name),
                         route_width,
                         pdn::ExtensionMode::kBoundary);

  // add_pdn_stripe
  pdngen_->makeStrap(grid,
                     pdn_tech->findLayer(ver_name),
                     ver_width,
                     0,
                     ver_pitch,
                     0,
                     0,
                     false,
                     pdn::StartsWith::kGrid,
                     pdn::ExtensionMode::kBoundary,
                     {},
                     false);
  pdngen_->makeStrap(grid,
                     pdn_tech->findLayer(hor_name),
                     hor_width,
                     0,
                     hor_pitch,
                     0,
                     0,
                     false,
                     pdn::StartsWith::kGrid,
                     pdn::ExtensionMode::kBoundary,
                     {},
                     false);

  // add_pdn_connect
  pdngen_->makeConnect(grid,
                       pdn_tech->findLayer(route_name),
                       pdn_tech->findLayer(ver_name),
                       0,
                       0,
                       {},
                       {},
                       0,
                       0,
                       {},
                       {},
                       {},
                       "");
  pdngen_->makeConnect(grid,
                       pdn_tech->findLayer(ver_name),
                       pdn_tech->findLayer(hor_name),
                       0,
                       0,
                       {},
                       {},
                       0,
                       0,
                       {},
                       {},
                       {},
                       "");

  // pdngen
  pdngen_->checkSetup();
  pdngen_->buildGrids(true);
  pdngen_->writeToDb(true, "");
  pdngen_->resetShapes();
}

void RamGen::ramPinplacer(const char* ver_name, const char* hor_name)
{
  const odb::Rect& die_bounds = block_->getDieArea();

  // Q output and D input pins on top
  odb::Rect top_constraint = block_->findConstraintRegion(
      odb::Direction2D::North, die_bounds.xMin(), die_bounds.xMax());
  block_->addBTermConstraintByDirection(dbIoType::OUTPUT, top_constraint);
  block_->addBTermsToConstraint(data_inputs_, top_constraint);

  // clk, we, addr_rw pins on the right
  odb::Rect right_constraint = block_->findConstraintRegion(
      odb::Direction2D::East, die_bounds.yMin(), die_bounds.yMax());

  vector<dbBTerm*> clk_pins, we_pins;
  for (auto bterm : block_->getBTerms()) {
    std::string name = bterm->getName();
    if (name == "clk") {
      clk_pins.push_back(bterm);
    } else if (name.starts_with("we") && name.find('[') != std::string::npos
               && std::all_of(name.begin() + 2,
                              name.begin() + name.find('['),
                              [](char c) { return std::isdigit(c); })) {
      we_pins.push_back(bterm);
    }
  }
  // moving clock and we pins to the top
  block_->addBTermsToConstraint(clk_pins, top_constraint);
  block_->addBTermsToConstraint(we_pins, top_constraint);
  for (const auto& inputs : addr_inputs_) {
    block_->addBTermsToConstraint(inputs, right_constraint);
  }

  auto pin_tech = block_->getDb()->getTech();
  odb::dbTechLayer* hor_layer = pin_tech->findLayer(hor_name);
  odb::dbTechLayer* ver_layer = pin_tech->findLayer(ver_name);
  io_placer_->addHorLayer(hor_layer);
  io_placer_->addVerLayer(ver_layer);

  // no pin on a power strap of its own layer: where a strap reaches an
  // edge, a pin on the tracks under it cannot be reached (asap7's M4
  // straps start on the bottom edge, its first track 12 nm above it)
  for (dbNet* net : block_->getNets()) {
    if (!net->getSigType().isSupply()) {
      continue;
    }
    for (odb::dbSWire* swire : net->getSWires()) {
      for (odb::dbSBox* box : swire->getWires()) {
        if (box->isVia()) {
          continue;
        }
        // the interval holds pin centres: a pin's half width and the
        // layer's spacing from the strap on either side
        const odb::Rect r = box->getBox();
        if (box->getTechLayer() == hor_layer) {
          const int s = hor_layer->getSpacing() + hor_layer->getWidth() / 2;
          io_placer_->excludeInterval(
              ppl::Edge::right, r.yMin() - s, r.yMax() + s);
          io_placer_->excludeInterval(
              ppl::Edge::left, r.yMin() - s, r.yMax() + s);
        } else if (box->getTechLayer() == ver_layer) {
          const int s = ver_layer->getSpacing() + ver_layer->getWidth() / 2;
          io_placer_->excludeInterval(
              ppl::Edge::top, r.xMin() - s, r.xMax() + s);
          io_placer_->excludeInterval(
              ppl::Edge::bottom, r.xMin() - s, r.xMax() + s);
        }
      }
    }
  }
  io_placer_->getParameters()->setCornerAvoidance(0);
  io_placer_->runHungarianMatching();
}

void RamGen::ramFiller(const vector<std::string>& filler_cells)
{
  vector<odb::dbMaster*> filler_masters;
  filler_masters.reserve(filler_cells.size());
  for (const std::string& cell : filler_cells) {
    filler_masters.push_back(db_->findMaster(cell.c_str()));
  }
  opendp_->fillerPlacement(filler_masters, "FILLER_", false);
}

void RamGen::ramRouting(int thread_count)
{
  const odb::Rect& die_bounds = block_->getDieArea();
  global_router_->setGridOrigin(die_bounds.xMin(), die_bounds.yMin());
  global_router_->setCongestionIterations(50);
  global_router_->setCongestionReportIterStep(0);
  global_router_->setAllowCongestion(false);
  global_router_->setResistanceAware(false);
  global_router_->globalRoute(true);
  drt::ParamStruct params;
  params.verbose = 0;
  params.num_threads = thread_count;
  params.enableViaGen = true;
  params.orSeed = -1;
  params.doPa = true;
  detailed_router_->setParams(params);
  detailed_router_->main();
  detailed_router_->setDistributed(false);
}

void RamGen::generate(const int mask_size,
                      const int word_bits,
                      const int num_words,
                      const int rw_ports,
                      const int r_ports,
                      const int w_ports,
                      const int column_mux_ratio_arg,
                      const bool use_latch,
                      dbMaster* storage_cell,
                      dbMaster* tristate_cell,
                      dbMaster* inv_cell,
                      dbMaster* tapcell,
                      int max_tap_dist,
                      const std::string& read_style,
                      const int zero_word,
                      const std::string& reset_value,
                      const int banks,
                      const int bank_columns,
                      const std::string& bank_order,
                      const int rb_ports,
                      const int register_read_address,
                      const std::string& write_style,
                      const std::string& write_priority,
                      const std::string& periphery,
                      const std::string& pin_layer,
                      const std::string& cells,
                      const std::string& name,
                      const std::string& decoder,
                      const int bit_folds,
                      const std::string& wrapper_verilog,
                      const int service_every,
                      const int service_sites,
                      const std::string& bank_column_layout)
{
  if (banks < 1 || bank_columns < 1) {
    logger_->error(RAM,
                   92,
                   "-banks {} and -bank_columns {} must be at least 1.",
                   banks,
                   bank_columns);
  }
  if (bit_folds < 1) {
    logger_->error(RAM, 93, "-bit_folds {} must be at least 1.", bit_folds);
  }
  if (rb_ports < 0) {
    logger_->error(RAM, 94, "-rb_ports {} must not be negative.", rb_ports);
  }
  // Banks: every port decodes every word's whole address, and a row holds
  // bank_columns words, laid out as the column mux lays out its words;
  // without banks, the column mux as it was.
  // -decoder word: each port decodes every word's whole address through
  // a balanced AND2 tree per word, the banks' decoders, banks or none
  banked_ = banks > 1 || bank_columns > 1 || decoder == "word";
  banks_ = banks;
  bank_columns_ = bank_columns;
  bank_interleaved_ = bank_order == "interleaved";
  if (banked_) {
    // the option that asked for the banks' decoders, for the messages
    const std::string asked
        = banks > 1 || bank_columns > 1 ? "-banks" : "-decoder word";
    if (column_mux_ratio_arg != 1) {
      logger_->error(RAM,
                     66,
                     "{} is not supported with -column_mux_ratio above 1.",
                     asked);
    }
    if (num_words % banks_ != 0 || banks_ % bank_columns_ != 0) {
      logger_->error(RAM,
                     67,
                     "-num_words {} must divide into -banks {}, and the banks "
                     "into -bank_columns {}.",
                     num_words,
                     banks_,
                     bank_columns_);
    }
    if (use_latch) {
      logger_->error(RAM, 68, "{} is not supported with -use_latch.", asked);
    }
  }
  words_per_bank_ = num_words / banks_;
  rb_ports_ = rb_ports;
  register_read_address_ = register_read_address;
  write_mux_ = write_style == "mux";
  write_or_ = write_priority == "or";
  periphery_verilog_ = periphery == "verilog";
  periphery_unplaced_ = periphery == "unplaced" || periphery_verilog_;
  wrapper_verilog_ = wrapper_verilog;
  if (periphery_verilog_ && wrapper_verilog_.empty()) {
    logger_->error(
        RAM, 85, "-periphery verilog needs -wrapper_verilog, its file.");
  }
  if (!periphery_verilog_ && !wrapper_verilog_.empty()) {
    logger_->error(
        RAM, 102, "-wrapper_verilog is written by -periphery verilog only.");
  }
  if (write_or_ && rw_ports + w_ports < 2) {
    logger_->warn(RAM,
                  103,
                  "-write_priority or with one write port: no two writes "
                  "collide, so it changes nothing.");
  }
  pin_layer_ = pin_layer;
  periphery_insts_.clear();
  marking_periphery_ = false;
  periphery_cell_ = nullptr;
  if (write_mux_ && use_latch) {
    logger_->error(
        RAM, 73, "-write_style mux is not supported with -use_latch.");
  }
  // More than one write port: each has its own address decoder and its
  // own data, a word written by two in a cycle keeps the last one's.
  if (w_ports > 1 && use_latch) {
    logger_->error(
        RAM, 54, "More than one write port is not supported with -use_latch.");
  }
  if (w_ports > 1 && column_mux_ratio_arg > 1) {
    logger_->error(RAM,
                   55,
                   "More than one write port is not supported with "
                   "-column_mux_ratio above 1.");
  }
  if (register_read_address_ != 0 && register_read_address_ != 1) {
    logger_->error(RAM,
                   71,
                   "-register_read_address is 0 or 1; more than one cycle is "
                   "not supported.");
  }
  if (register_read_address_ > 0) {
    if (column_mux_ratio_arg > 1 || rw_ports > 0) {
      logger_->error(
          RAM,
          72,
          "-register_read_address 1 is not supported with -column_mux_ratio "
          "above 1 or with read/write ports.");
    }
  }
  r_plain_ = rw_ports + r_ports;
  if (rb_ports_ > 0 && !banked_) {
    logger_->error(RAM, 70, "-rb_ports needs -banks or -decoder word.");
  }
  // a banked read port is a read port per bank to the generator below
  const int r_ports_all = r_ports + rb_ports_ * banks_;
  // the address bits of a banked read port's per-bank address
  const int bank_inputs
      = std::max(1, static_cast<int>(std::ceil(std::log2(words_per_bank_))));
  // the words in a row: the column mux's, or the banks side by side
  const int column_mux_ratio = banked_ ? bank_columns_ : column_mux_ratio_arg;
  words_per_row_ = column_mux_ratio;
  words_contiguous_ = bank_column_layout == "contiguous";
  if (words_contiguous_ && !banked_) {
    logger_->error(RAM,
                   90,
                   "-bank_column_layout contiguous needs -banks, -bank_columns "
                   "or -decoder word: the column mux reads a row's words "
                   "interleaved.");
  }
  bit_folds_ = bit_folds;
  slice_width_ = mask_size * column_mux_ratio + column_mux_ratio;
  // the word's own bits; with -bit_folds the last fold may be short, its
  // missing bits pads: columns with no port, flop or read tree
  word_bits_ = word_bits;
  const int word_size = bit_folds_ > 1 ? mask_size * bit_folds_ : word_bits;
  if (bit_folds_ > 1) {
    const int fold = (word_bits + bit_folds_ - 1) / bit_folds_;
    if (mask_size != fold) {
      logger_->error(RAM,
                     83,
                     "-bit_folds {} needs -mask_size {}: a fold is a slice.",
                     bit_folds_,
                     fold);
    }
    if (word_size - word_bits >= mask_size) {
      logger_->error(RAM,
                     88,
                     "-bit_folds {} leaves a fold of -word_size {} empty.",
                     bit_folds_,
                     word_bits);
    }
    if (use_latch || (!banked_ && column_mux_ratio > 1)) {
      logger_->error(RAM,
                     84,
                     "-bit_folds is not supported with -use_latch or the row "
                     "decoder's -column_mux_ratio above 1.");
    }
  }
  if (num_words / column_mux_ratio == 1) {
    logger_->error(RAM, 42, "1-row RAMs are not supported.");
  }
  const int slices_per_word = word_size / mask_size;
  slices_per_row_ = bit_folds_ > 1 ? 1 : slices_per_word;
  // the block's name: -name, or RAM<words>x<bits>
  const std::string ram_name
      = name.empty() ? fmt::format("RAM{}x{}", num_words, word_bits) : name;
  ram_name_ = ram_name;

  // error checking for column_mux_ratio
  if (!banked_ && column_mux_ratio != 1 && column_mux_ratio != 2
      && column_mux_ratio != 4) {
    logger_->error(RAM,
                   33,
                   "The ram generator currently only supports column_mux_ratio "
                   "values of 1, 2, or 4.");
  }

  // TODO: add support for non-divisble word counts, for these cases the last
  // row will have empty spaces/filler cells instead of errorring out
  if (num_words % column_mux_ratio != 0) {
    logger_->error(RAM,
                   34,
                   "num_words ({}) must be divisible by column_mux_ratio ({}).",
                   num_words,
                   column_mux_ratio);
  }

  // can't support multi-port if the number of rows is too small
  if (column_mux_ratio == 4 && rw_ports + r_ports + w_ports > 1) {
    logger_->info(
        RAM, 43, "Generated layout may not pass DRC due to congestion");
  }

  const int num_inputs = std::ceil(std::log2(num_words));
  // compute information to support col/mux ratio feature
  const int num_rows = num_words / column_mux_ratio;
  // if column mux ratio > 1, then the lower log2(column_mux_ratio) bits are
  // used to select the word within a row
  // (with banks, none: each port decodes every word's whole address)
  const int num_word_bits = (column_mux_ratio > 1 && !banked_)
                                ? static_cast<int>(std::log2(column_mux_ratio))
                                : 0;
  // the remaining upper bits are used to select the row
  const int num_row_bits = num_inputs - num_word_bits;

  logger_->info(RAM, 3, "Generating {}", ram_name);

  use_latch_ = use_latch;
  num_write_ = rw_ports + w_ports;
  read_style_ = read_style;
  zero_word_ = zero_word;
  reset_value_ = reset_value;
  reset_port_ = nullptr;
  tie_off_cell_ = nullptr;
  and_or_read_ = false;
  storage_inverted_ = false;
  storage_nets_.clear();
  tree_nodes_.clear();
  word_write_.clear();
  storage_cell_ = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_ = inv_cell;
  tapcell_ = tapcell;
  and2_cell_ = nullptr;
  clock_gate_cell_ = nullptr;
  buffer_cell_ = nullptr;
  aoi22_cell_ = nullptr;
  ao22_cell_ = nullptr;
  or2_cell_ = nullptr;
  nand2_cell_ = nullptr;
  nor2_cell_ = nullptr;
  latch_cell_ = nullptr;
  addr_flop_cell_ = nullptr;
  tie_off_cell_ = nullptr;

  // -cells {role master ...}: the cell for a role, instead of the
  // smallest the libraries have of its function
  {
    std::istringstream in(cells);
    const std::map<std::string, dbMaster**> roles
        = {{"inv", &inv_cell_},
           {"and2", &and2_cell_},
           {"buffer", &buffer_cell_},
           {"clock_gate", &clock_gate_cell_},
           {"aoi22", &aoi22_cell_},
           {"ao22", &ao22_cell_},
           {"or2", &or2_cell_},
           {"nand2", &nand2_cell_},
           {"nor2", &nor2_cell_},
           {"tie", &tie_off_cell_},
           {"addr_flop", &addr_flop_cell_}};
    std::vector<std::string> words{std::istream_iterator<std::string>(in),
                                   std::istream_iterator<std::string>()};
    if (words.size() % 2 != 0) {
      logger_->error(RAM,
                     99,
                     "-cells takes pairs of a role and a cell; {} has no cell.",
                     words.back());
    }
    for (size_t i = 0; i < words.size(); i += 2) {
      const std::string& role = words[i];
      const std::string& master_name = words[i + 1];
      auto slot = roles.find(role);
      if (slot == roles.end()) {
        logger_->error(RAM,
                       80,
                       "-cells: no role {}; the roles are inv, and2, buffer, "
                       "clock_gate, aoi22, ao22, or2, nand2, nor2, tie and "
                       "addr_flop.",
                       role);
      }
      dbMaster* master = db_->findMaster(master_name.c_str());
      if (master == nullptr) {
        logger_->error(RAM, 81, "-cells: {} can't be found.", master_name);
      }
      *slot->second = master;
    }
  }

  if (use_latch_ && storage_cell_) {
    auto sta_cell = network_->dbToSta(storage_cell_);
    auto liberty = network_->libertyCell(sta_cell);
    bool is_latch = false;
    auto port_iter = std::unique_ptr<sta::ConcreteCellPortIterator>(
        liberty->portIterator());
    while (port_iter->hasNext()) {
      auto lp = port_iter->next()->libertyPort();
      if (lp && lp->isLatchData()) {
        is_latch = true;
        break;
      }
    }
    if (!is_latch) {
      logger_->error(RAM,
                     37,
                     "-storage_cell {} is not a latch cell. Please provide a "
                     "latch cell when setting -use_latch 1.",
                     storage_cell_->getName());
    }
    latch_cell_ = storage_cell_;
    storage_cell_ = nullptr;
  }

  findMasters();

  // -register_read_address 1: a plain positive-edge flop for the read addresses
  if (register_read_address_ > 0) {
    // a plain flop: positive-edge, no clear or preset, its data a pin
    // (D or !D: a scan flop's is a mux)
    auto plain = [](const sta::LibertyCell* cell) {
      for (const auto& seq : cell->sequentials()) {
        const sta::FuncExpr* d = seq.data();
        if (d && d->op() == sta::FuncExpr::Op::not_) {
          d = d->left();
        }
        if (seq.isRegister() && !seq.clear() && !seq.preset() && seq.clock()
            && seq.clock()->op() == sta::FuncExpr::Op::port && d
            && d->op() == sta::FuncExpr::Op::port) {
          return true;
        }
      }
      return false;
    };
    // the storage cell where it is one, else the smallest (Nangate45's
    // flops all have Q and QN)
    auto flop = [plain](sta::LibertyPort* port) {
      return port->isRegOutput() && plain(port->libertyCell());
    };
    if (addr_flop_cell_) {
      // given by -cells
      findMaster(flop, "address flop", /*required=*/true, addr_flop_cell_);
    } else if (storage_cell_
               && plain(
                   network_->libertyCell(network_->dbToSta(storage_cell_)))) {
      addr_flop_cell_ = storage_cell_;
    } else {
      addr_flop_cell_ = findMaster(flop,
                                   "address flop",
                                   /*required=*/true,
                                   /*given=*/nullptr,
                                   /*q_and_qn=*/true);
    }
    auto ports = buildPortMap(addr_flop_cell_, &addr_flop_inverted_);
    auto liberty = network_->libertyCell(network_->dbToSta(addr_flop_cell_));
    for (const auto& seq : liberty->sequentials()) {
      if (seq.isRegister()) {
        // the D pin (buildPortMap's polarity already counts a "!D")
        const sta::FuncExpr* d = seq.data();
        if (d->op() == sta::FuncExpr::Op::not_) {
          d = d->left();
        }
        addr_flop_d_ = d->port()->name();
        break;
      }
    }
    addr_flop_clk_ = ports[{PortRoleType::Clock, 0}];
    addr_flop_q_ = ports[{PortRoleType::DataOut, 0}];
  }

  // A zero word reads 0 and drops its writes, as a RISC-V x0: its row
  // has no flops, and the read trees leave it out.
  if (zero_word_ >= 0) {
    if (zero_word_ >= num_words) {
      logger_->error(
          RAM, 59, "-zero_word {} is not a word of {}.", zero_word_, num_words);
    }
    if (!and_or_read_ || (column_mux_ratio != 1 && !banked_)) {
      logger_->error(RAM,
                     60,
                     "-zero_word needs -read_style and_or and "
                     "-column_mux_ratio 1.");
    }
  }

  auto chip = db_->getChip();
  if (!chip) {
    chip = odb::dbChip::create(
        db_, db_->getTech(), ram_name, odb::dbChip::ChipType::DIE);
  }

  block_ = chip->getBlock();
  if (!block_) {
    // -periphery verilog: the block is the core, the wrapper the name
    const std::string block_name
        = periphery_verilog_ ? ram_name + "_core" : ram_name;
    block_ = odb::dbBlock::create(chip, block_name.c_str());
  }

  // One column per bit plus one select/control column per slice,
  // Each read/write port gets a buffer col and decoder col,
  // plus inverter columns shared across ports
  const int total_ports = rw_ports + r_ports_all + w_ports;
  // at least one port's inverters a column: an array with fewer rows than
  // address bits (four banks side by side, say) otherwise got none
  const int ports_per_col
      = (num_row_bits > 0) ? std::max(1, num_rows / num_row_bits) : total_ports;
  const int inv_col_count
      = std::ceil(static_cast<float>(total_ports) / ports_per_col);

  // With column_mux_ratio = 1: One column per bit plus one select/control
  // column per slice, plus one extra decoder column With column_mux_ratio > 1:
  // Each slice has (mask_size * column_mux_ratio) columns (one per bit per word
  // in the row), plus one select/control column per slice (one per word), plus
  // one extra decoder column at far right shared across all row

  // mux layout columns + per-port buffer/decoder columns + inverter columns
  int col_cell_count = (bit_folds_ > 1 ? 1 : slices_per_word)
                       * (mask_size * column_mux_ratio + column_mux_ratio);
  ram_grid_.setNumLayouts(col_cell_count);

  auto clock = makeBTerm("clk", dbIoType::INPUT);
  clock_net_ = clock->getNet();

  // -reset_value: an asynchronous reset at the storage flop's own
  // polarity, rst_n or rst; a parent adapts any other reset to it
  if (!reset_value_.empty()) {
    if (use_latch_) {
      logger_->error(RAM, 65, "-reset_value is not supported with -use_latch.");
    }
    reset_port_
        = makeBTerm(reset_active_low_ ? "rst_n" : "rst", dbIoType::INPUT);
  }

  // One write enable bus per write port: we, or with more than one write
  // port we0, we1, ...
  const int num_write = num_write_;
  vector<vector<dbBTerm*>> write_enable(
      num_write, vector<dbBTerm*>(slices_per_word, nullptr));
  for (int w = 0; w < num_write; ++w) {
    const std::string we = num_write == 1 ? "we" : fmt::format("we{}", w);
    // with -bit_folds a word is written whole: one enable a port
    for (int slice = 0; slice < (bit_folds_ > 1 ? 1 : slices_per_word);
         ++slice) {
      write_enable[w][slice]
          = makeBTerm(fmt::format("{}[{}]", we, slice), dbIoType::INPUT);
    }
  }

  // When column_mux_ratio > 1:  word_sel_nets[word_idx] is high/active when
  // word_idx is the addressed word within a physical row. Derived from the
  // lower num_word_bits address bits. Used only on the write path to gate the
  // clock per word in makeSlice. Read path word selection uses addr[0]/addr[1]
  // directly in the AOI mux.

  // When column_mux_ratio = 1: set to nullptr for all entries because only one
  // word per row
  vector<dbNet*> word_sel_nets(column_mux_ratio, nullptr);

  // indices for creating BTerms
  int rw_idx = 0;
  int w_idx = 0;
  int r_idx = 0;

  // indices for select nets
  int r_sel_idx = 0;
  int w_sel_idx = 0;

  addr_inputs_.clear();
  addr_inputs_.resize(total_ports);
  addr_true_.assign(total_ports, {});
  addr_flop_out_.assign(total_ports, {});
  vector<vector<dbNet*>> inv_addr_nets(total_ports);
  // for column muxing, deocder input nets uses only the upper address bits
  // (num_row_bits) to determine row
  vector<vector<vector<dbNet*>>> decoder_input_nets(total_ports);

  vector<vector<dbNet*>> decoder_output_nets(total_ports);
  vector<vector<dbNet*>> read_select_nets(total_ports);
  vector<vector<dbNet*>> write_select_nets(total_ports);

  for (int p = 0; p < total_ports; ++p) {
    std::string port_prefix;
    if (p < rw_ports) {  // NOLINTBEGIN(misc-include-cleaner)
      port_prefix = "addr_rw";
      rw_idx++;
    } else if (p < rw_ports + w_ports) {
      port_prefix = num_write == 1 ? "addr_w" : fmt::format("addr_w{}", w_idx);
      w_idx++;
    } else if (r_idx < r_ports) {
      port_prefix = fmt::format("addr_r{}", r_idx++);
    } else {
      // a banked read port's address for one bank
      const int kb = r_idx++ - r_ports;
      port_prefix = fmt::format("addr_rb{}_b{}", kb / banks_, kb % banks_);
    }
    // NOLINTEND(misc-include-cleaner)
    // a banked read port's bank, or -1
    const int port_bank = p >= rw_ports + w_ports + r_ports
                              ? (p - rw_ports - w_ports - r_ports) % banks_
                              : -1;
    const int port_inputs = port_bank >= 0 ? bank_inputs : num_inputs;

    addr_inputs_[p].resize(port_inputs);
    inv_addr_nets[p].resize(port_inputs);
    addr_true_[p].resize(port_inputs);
    addr_flop_out_[p].assign(port_inputs, nullptr);
    const bool registered
        = register_read_address_ > 0 && p >= rw_ports + w_ports;

    for (int i = 0; i < port_inputs; ++i) {
      addr_inputs_[p][i]
          = makeBTerm(fmt::format("{}[{}]", port_prefix, i), dbIoType::INPUT);
      if (registered && addr_flop_inverted_) {
        // the flop holds the complement; the inverter makes the address
        addr_flop_out_[p][i] = inv_addr_nets[p][i]
            = makeNet(port_prefix, fmt::format("inv_addr[{}]", i));
        addr_true_[p][i] = makeNet(port_prefix, fmt::format("addr_q[{}]", i));
      } else {
        inv_addr_nets[p][i]
            = makeNet(port_prefix, fmt::format("inv_addr[{}]", i));
        addr_true_[p][i] = addr_inputs_[p][i]->getNet();
        if (registered) {
          addr_flop_out_[p][i] = addr_true_[p][i]
              = makeNet(port_prefix, fmt::format("addr_q[{}]", i));
        }
      }
    }

    // inputs to decoders: with banks, every word's whole address, one
    // decoder per word (slot row * words per row + word in the row)
    if (banked_) {
      decoder_input_nets[p].assign(num_rows * column_mux_ratio,
                                   vector<dbNet*>(num_inputs));
      for (int row = 0; row < num_rows; ++row) {
        for (int w = 0; w < column_mux_ratio; ++w) {
          auto& literals = decoder_input_nets[p][row * column_mux_ratio + w];
          if (port_bank >= 0 && bankAt(row, w) != port_bank) {
            literals.clear();  // a word of another bank: no decoder
            continue;
          }
          // a banked read port decodes the word's index in its bank
          const int word = port_bank >= 0 ? indexInBank(row) : wordAt(row, w);
          literals.resize(port_inputs);
          for (int input = 0; input < port_inputs; ++input) {
            literals[input] = ((word >> input) & 1) ? addr_true_[p][input]
                                                    : inv_addr_nets[p][input];
          }
        }
      }
    }
    if (!banked_) {
      decoder_input_nets[p].assign(num_rows, vector<dbNet*>(num_row_bits));
    }
    for (int row = 0; row < num_rows && !banked_; ++row) {
      int row_num = row;
      // start at right most bit
      for (int input = 0; input < num_row_bits; ++input) {
        if (row_num % 2 == 0) {
          // places inverted address for each input
          decoder_input_nets[p][row][input]
              = inv_addr_nets[p][num_word_bits + input];
        } else {  // puts original input in invert nets
          decoder_input_nets[p][row][input]
              = addr_true_[p][num_word_bits + input];
        }
        row_num /= 2;
      }
    }

    // output nets of each port decoder
    const int num_selects = banked_ ? num_rows * column_mux_ratio : num_rows;
    decoder_output_nets[p]
        = makeDecoderOutputNets(fmt::format("decoder_p{}", p), num_selects);
    // output nets of each decoder buffers
    // the write-capable ports first: the read-write port, then the
    // write ports
    if (p < rw_ports) {
      write_select_nets[w_sel_idx] = makeSelectNets(
          fmt::format("rw_sel_p{}", p), num_selects, RamPortType::ReadWrite);
      read_select_nets[r_sel_idx++] = write_select_nets[w_sel_idx];
      ++w_sel_idx;
    } else if (p < rw_ports + w_ports) {
      write_select_nets[w_sel_idx] = makeSelectNets(
          fmt::format("w_sel_p{}", p), num_selects, RamPortType::Write);
      ++w_sel_idx;
    }
    if (p >= rw_ports + w_ports) {
      read_select_nets[r_sel_idx++] = makeSelectNets(
          fmt::format("r_sel_p{}", p), num_selects, RamPortType::Read);
    }
    // a banked read port selects only its bank's words
    if (port_bank >= 0) {
      for (int slot = 0; slot < num_selects; ++slot) {
        if (decoder_input_nets[p][slot].empty()) {
          dbNet::destroy(decoder_output_nets[p][slot]);
          dbNet::destroy(read_select_nets[r_sel_idx - 1][slot]);
          decoder_output_nets[p][slot] = nullptr;
          read_select_nets[r_sel_idx - 1][slot] = nullptr;
        }
      }
    }
  }

  // start of input/output net creation
  int r_total = rw_ports + r_ports_all;
  q_outputs_.resize(r_total);
  // One data bus per write port: D, or with more than one D0, D1, ...
  vector<vector<dbBTerm*>> data_inputs(num_write);
  vector<vector<dbNet*>> D_nets(num_write, vector<dbNet*>(word_size));
  for (int bit = 0; bit < word_size; ++bit) {
    if (bit >= word_bits_) {
      // a short fold's pad: no ports
      for (int w = 0; w < num_write; ++w) {
        data_inputs[w].push_back(nullptr);
      }
      for (auto& q : q_outputs_) {
        q.push_back(nullptr);
      }
      continue;
    }
    for (int w = 0; w < num_write; ++w) {
      const std::string d = num_write == 1 ? "D" : fmt::format("D{}", w);
      data_inputs[w].push_back(
          makeBTerm(fmt::format("{}[{}]", d, bit), dbIoType::INPUT));
      data_inputs_.push_back(data_inputs[w].back());
      D_nets[w][bit] = makeNet(num_write == 1 ? "D_nets" : d + "_nets",
                               fmt::format("b{}", bit));
    }

    // if total read ports == 1, only have Q outputs
    if (r_plain_ == 1) {
      auto out_name = fmt::format("Q[{}]", bit);
      q_outputs_[0].push_back(makeBTerm(out_name, dbIoType::OUTPUT));
    } else {
      for (int port = 0; port < r_plain_; ++port) {
        auto out_name = fmt::format("Q{}[{}]", port, bit);
        q_outputs_[port].push_back(makeBTerm(out_name, dbIoType::OUTPUT));
      }
    }
    // a banked read port's data, one bus per bank
    for (int port = r_plain_; port < r_total; ++port) {
      const int kb = port - r_plain_;
      auto out_name
          = fmt::format("Qrb{}_b{}[{}]", kb / banks_, kb % banks_, bit);
      q_outputs_[port].push_back(makeBTerm(out_name, dbIoType::OUTPUT));
    }
  }

  // Intermediate nets between tristate outputs and Q BTerms
  // When column_mux_ratio = 1: tristate drives Q directly, so reuse Q BTerm
  // nets When column_mux_ratio > 1: tristate drives intermediate net, AOI mux
  // selects correct net based on lower address bits (word_select net)
  // word_q_nets[word_idx][bit] = intermed. net for that word and bit pair
  vector<vector<vector<dbNet*>>> word_q_nets(
      column_mux_ratio,
      vector<vector<dbNet*>>(r_total, vector<dbNet*>(word_size)));
  // with banks every word drives the read port's own net: a tristate
  // bitline shared by every word, or one AND-OR tree over all of them
  if (column_mux_ratio == 1 || banked_) {
    for (int w = 0; w < column_mux_ratio; ++w) {
      for (int port = 0; port < r_total; ++port) {
        for (int bit = 0; bit < word_size; ++bit) {
          word_q_nets[w][port][bit] = q_outputs_[port][bit]
                                          ? q_outputs_[port][bit]->getNet()
                                          : nullptr;
        }
      }
    }

  } else {
    for (int w = 0; w < column_mux_ratio; ++w) {
      for (int port = 0; port < r_total; ++port) {
        for (int bit = 0; bit < word_size; ++bit) {
          word_q_nets[w][port][bit]  // NOLINTNEXTLINE(misc-include-cleaner)
              = makeNet("word_q", fmt::format("w{}_p{}_b{}", w, port, bit));
        }
      }
    }
  }

  // Word selects for column mux, placed before word creation
  if (column_mux_ratio == 2 && !banked_) {
    word_sel_nets[0] = inv_addr_nets[0][0];
    word_sel_nets[1] = addr_inputs_[0][0]->getNet();
  } else if (column_mux_ratio == 4 && !banked_) {
    for (int c = 0; c < 4; ++c) {
      word_sel_nets[c] = makeNet("word_sel", fmt::format("{}", c));
    }
  }
  // For latch-based ram: mid_nets carry data from negative latch down to all
  // pos latches per bit in the same column
  vector<dbNet*> mid_nets(word_size);
  dbNet* clk_b_net = nullptr;
  if (use_latch_) {
    clk_b_net = makeNet("global", "clk_b");
    for (int bit = 0; bit < word_size; ++bit) {
      mid_nets[bit] = makeNet("mid", fmt::format("b{}", bit));
    }
  }

  // For column_mux_ratio > 1, create one shared select_b net per row before
  // iterating over word_idx. All words in the same row share this net as the
  // tristate TE_B enable signal. Only word_idx=0 instantiates the select_inv
  // gate — all other word_idx reuse the existing net, reducing cell count
  // and horizontal routing congestion in storage rows.

  // a port's select of the word at (row, word in the row): with banks a
  // select per word, else per row
  auto select_at
      = [&](const vector<dbNet*>& selects, const int row, const int w) {
          return banked_ ? selects[row * column_mux_ratio + w] : selects[row];
        };

  // The AND-OR read path: every storage net named ahead, and every
  // bitline's tree planned, so each row's bit cells take their nodes.
  // Where the storage flop's output is QN, or the tree's root is
  // inverted, but not both, an inverter at the bottom row puts it right.
  if (and_or_read_) {
    // The words in the trees: where the storage output is inverted the
    // final inverter assumes some word's term is the read's, so the zero
    // word stays, its term a 1; otherwise its term would be a 0, and it
    // is left out.
    for (int row = 0; row < num_rows; ++row) {
      for (int w = 0; w < column_mux_ratio; ++w) {
        if (wordAt(row, w) == zero_word_) {
          continue;
        }
        for (int bit = 0; bit < word_bits_; ++bit) {
          const std::string prefix = fmt::format(
              "storage_{}_{}_{}", row, w, (bit / mask_size) * mask_size);
          storage_nets_[{row, w, bit}]
              = makeNet(fmt::format("{}.bit{}", prefix, bit), "storage");
        }
      }
    }
    // a read port's terms: with banks every word, a column at a time; else
    // the words of column w
    auto terms_of = [&](const int port, const int only_w) {
      vector<Term> terms;
      for (int w = 0; w < column_mux_ratio; ++w) {
        if (!banked_ && w != only_w) {
          continue;
        }
        for (int row = 0; row < num_rows; ++row) {
          if (wordAt(row, w) == zero_word_ && !storage_inverted_) {
            continue;
          }
          dbNet* select = select_at(read_select_nets[port], row, w);
          if (select == nullptr) {
            continue;  // a banked read port's other banks
          }
          terms.push_back({row, w, select});
        }
      }
      return terms;
    };
    for (int w = 0; w < (banked_ ? 1 : column_mux_ratio); ++w) {
      for (int port = 0; port < r_total; ++port) {
        const vector<Term> terms = terms_of(port, w);
        const int n = static_cast<int>(terms.size());
        const bool flip = treeInverted(0, n) != storage_inverted_;
        for (int bit = 0; bit < word_bits_; ++bit) {
          const std::string prefix
              = banked_ ? fmt::format("read_p{}_b{}", port, bit)
                        : fmt::format("read_w{}_p{}_b{}", w, port, bit);
          dbNet* out = word_q_nets[w][port][bit];
          const TreeEnd root
              = planReadTree(prefix, bit, terms, 0, n, flip ? nullptr : out);
          if (flip) {
            const Term& at
                = banked_ ? terms.back() : Term{num_rows - 1, w, nullptr};
            tree_nodes_[{at.row, at.word_idx, bit}].push_back(
                {inv_cell_,
                 prefix,
                 "out_inv",
                 {{inv_ports_[{PortRoleType::DataIn, 0}], root.net},
                  {inv_ports_[{PortRoleType::DataOut, 0}], out}}});
          }
        }
      }
    }
  }

  for (int row = 0; row < num_rows; ++row) {
    // one select_b_net per row for column_mux_ratio > 1
    vector<dbNet*> shared_select_b_nets(r_total);
    // (the AND-OR trees read the true selects: none)
    if (column_mux_ratio > 1 && !banked_ && !and_or_read_) {
      for (int i = 0; i < r_total; ++i) {
        shared_select_b_nets[i]
            = makeNet(fmt::format("row{}", row), fmt::format("select{}_b", i));
      }
    }

    for (int word_idx = 0; word_idx < column_mux_ratio; ++word_idx) {
      // the write ports' row selects, then the read ports'
      vector<dbNet*> row_selects(num_write + r_total);
      for (int p = 0; p < num_write; ++p) {
        row_selects[p] = select_at(write_select_nets[p], row, word_idx);
      }
      for (int p = 0; p < r_total; ++p) {
        row_selects[num_write + p]
            = select_at(read_select_nets[p], row, word_idx);
      }
      vector<vector<dbNet*>> word_output_nets(r_total);
      for (int port = 0; port < r_total; ++port) {
        for (int bit = 0; bit < word_size; ++bit) {
          word_output_nets[port].push_back(word_q_nets[word_idx][port][bit]);
        }
      }

      makeWord(slices_per_word,
               mask_size,
               row,
               rw_ports,
               r_ports_all,
               w_ports,
               word_idx,
               column_mux_ratio,
               clock->getNet(),
               word_sel_nets[word_idx],
               write_enable,
               row_selects,
               shared_select_b_nets,
               (word_idx == 0 || column_mux_ratio == 1 || banked_),
               use_latch_ ? vector<vector<dbNet*>>{mid_nets} : D_nets,
               word_output_nets);
    }
  }

  // cell placement for mux made of AOI22 (and inverter if needed) for column
  // muxing (with banks each port's tree or bitline spans every word)
  if (column_mux_ratio > 1 && !banked_) {
    for (int slice = 0; slice < slices_per_word; ++slice) {
      for (int bit = 0; bit < mask_size; ++bit) {
        const int global_bit = slice * mask_size + bit;
        const int base_col
            = slice * (mask_size * column_mux_ratio + column_mux_ratio)
              + bit * column_mux_ratio;
        const std::string prefix = fmt::format("mux_slice{}_bit{}", slice, bit);
        auto mux_cell = std::make_unique<Cell>();
        auto s1_cell_0 = std::make_unique<Cell>();
        auto s1_cell_1 = std::make_unique<Cell>();
        auto s2_cell = std::make_unique<Cell>();

        for (int port = 0; port < r_total; ++port) {
          // collect this bit's net from each word
          vector<dbNet*> bit_word_q_nets(column_mux_ratio);
          for (int word_idx = 0; word_idx < column_mux_ratio; ++word_idx) {
            bit_word_q_nets[word_idx] = word_q_nets[word_idx][port][global_bit];
          }
          // NOLINTNEXTLINE(misc-include-cleaner)
          const std::string port_prefix = fmt::format("{}_p{}", prefix, port);
          // determine which port's address bits to use for mux select
          // Write-capable ports use index 0, read-only ports use their own
          // address
          const int addr_port = (port < rw_ports)
                                    ? port
                                    : (rw_ports + w_ports) + (port - rw_ports);

          if (column_mux_ratio == 2) {
            // mux placement
            // col base_col+0: buffer
            // col base_col+1: AOI22 + inverter side by side in same cell/column
            auto aoi_out = makeNet(port_prefix, "aoi_out");
            makeInst(mux_cell.get(),
                     port_prefix,
                     "aoi",
                     aoi22_cell_,
                     {{aoi22_in_a1_, inv_addr_nets[addr_port][0]},
                      {aoi22_in_a2_, bit_word_q_nets[0]},
                      {aoi22_in_b1_, addr_inputs_[addr_port][0]->getNet()},
                      {aoi22_in_b2_, bit_word_q_nets[1]},
                      {aoi22_out_, aoi_out}});
            makeInst(mux_cell.get(),
                     port_prefix,
                     "inv",
                     inv_cell_,
                     {{inv_ports_[{PortRoleType::DataIn, 0}], aoi_out},
                      {inv_ports_[{PortRoleType::DataOut, 0}],
                       q_outputs_[port][global_bit]->getNet()}});

          } else if (column_mux_ratio == 4) {
            // mux placement:
            // col base_col+0: buffer
            // col base_col+1: s1_AOI_0 (w0+w1)
            // col base_col+2: s2_AOI final (even stages so no inverter needed)
            // col base_col+3: s1_AOI_1 (w2+w3)
            auto s1_out_0 = makeNet(port_prefix, "s1_out_0");
            auto s1_out_1 = makeNet(port_prefix, "s1_out_1");

            // col base_col+1: stage1 AOI for word0+word1
            makeInst(s1_cell_0.get(),
                     port_prefix,
                     "s1_aoi_0",
                     aoi22_cell_,
                     {{aoi22_in_a1_, inv_addr_nets[addr_port][0]},
                      {aoi22_in_a2_, bit_word_q_nets[0]},
                      {aoi22_in_b1_, addr_inputs_[addr_port][0]->getNet()},
                      {aoi22_in_b2_, bit_word_q_nets[1]},
                      {aoi22_out_, s1_out_0}});

            // col base_col+3: stage1 AOI for word2 + word3
            // NOTE: must be placed before s2 so s1_out_1 net exists for s2
            // input
            makeInst(s1_cell_1.get(),
                     port_prefix,
                     "s1_aoi_1",
                     aoi22_cell_,
                     {{aoi22_in_a1_, inv_addr_nets[addr_port][0]},
                      {aoi22_in_a2_, bit_word_q_nets[2]},
                      {aoi22_in_b1_, addr_inputs_[addr_port][0]->getNet()},
                      {aoi22_in_b2_, bit_word_q_nets[3]},
                      {aoi22_out_, s1_out_1}});

            // col base_col+2: stage2 AOI combining stage 1 outputs and drives Q
            // directly
            makeInst(s2_cell.get(),
                     port_prefix,
                     "s2_aoi",
                     aoi22_cell_,
                     {{aoi22_in_a1_, inv_addr_nets[addr_port][1]},
                      {aoi22_in_a2_, s1_out_0},
                      {aoi22_in_b1_, addr_inputs_[addr_port][1]->getNet()},
                      {aoi22_in_b2_, s1_out_1},
                      {aoi22_out_, q_outputs_[port][global_bit]->getNet()}});
          }
        }
        if (column_mux_ratio == 2) {
          ram_grid_.addCell(std::move(mux_cell), base_col + 1);
        } else if (column_mux_ratio == 4) {
          ram_grid_.addCell(std::move(s1_cell_0), base_col + 1);
          ram_grid_.addCell(std::move(s1_cell_1), base_col + 3);
          ram_grid_.addCell(std::move(s2_cell), base_col + 2);
        }
      }
    }
  }

  if (use_latch_) {
    auto clk_inv_cell = std::make_unique<Cell>();
    makeInst(clk_inv_cell.get(),
             "neg_row",
             "clk_inv",
             inv_cell_,
             {{inv_ports_[{PortRoleType::DataIn, 0}], clock->getNet()},
              {inv_ports_[{PortRoleType::DataOut, 0}], clk_b_net}});
    // add to column with clock gates
    ram_grid_.addCell(std::move(clk_inv_cell), col_cell_count - 1);

    const int nl_col_offset = (column_mux_ratio == 4) ? 1 : 0;

    for (int slice = 0; slice < slices_per_word; ++slice) {
      for (int bit = 0; bit < mask_size; ++bit) {
        const int global_bit = slice * mask_size + bit;
        const int group_start
            = slice * (mask_size * column_mux_ratio + column_mux_ratio)
              + bit * column_mux_ratio;
        const int nl_col = group_start + nl_col_offset;
        for (int c = group_start; c < group_start + column_mux_ratio; ++c) {
          if (c == nl_col) {
            auto nl_cell = std::make_unique<Cell>();
            makeInst(nl_cell.get(),
                     // NOLINTNEXTLINE(misc-include-cleaner)
                     fmt::format("neg_lat_b{}", global_bit),
                     "nlat",
                     latch_cell_,
                     {{latch_ports_[{PortRoleType::Clock, 0}], clk_b_net},
                      {latch_ports_[{PortRoleType::DataIn, 0}],
                       D_nets[0][global_bit]},
                      {latch_ports_[{PortRoleType::DataOut, 0}],
                       mid_nets[global_bit]}});
            ram_grid_.addCell(std::move(nl_cell), c);
          } else {
            ram_grid_.addCell(nullptr, c);
          }
        }
      }
    }
    for (int slice = 0; slice < slices_per_word; ++slice) {
      for (int w = 0; w < column_mux_ratio; ++w) {
        int sel_col = slice * (mask_size * column_mux_ratio + column_mux_ratio)
                      + mask_size * column_mux_ratio + w;
        ram_grid_.addCell(nullptr, sel_col);
      }
    }
  }

  // the input buffers, a cell a column and slice: with -bit_folds a
  // column's folds stack theirs in k rows, so the buffers of a wide word
  // do not set the column's width
  std::map<std::pair<int, int>, std::unique_ptr<Cell>> buffer_cells;
  for (int slice = 0; slice < slices_per_word; ++slice) {
    for (int bit = 0; bit < mask_size; ++bit) {
      int bit_idx = bit + slice * mask_size;
      int buf_col = bitColumn(slice, bit, 0, mask_size);
      auto& buffer_grid_cell
          = buffer_cells[{buf_col, bit_folds_ > 1 ? slice : 0}];
      if (bit_idx >= word_bits_) {
        continue;  // a short fold's pad: an empty cell
      }
      if (!buffer_grid_cell) {
        buffer_grid_cell = std::make_unique<Cell>();
      }
      marking_periphery_ = true;  // the input buffers
      for (int w = 0; w < num_write; ++w) {
        makeInst(
            buffer_grid_cell.get(),
            num_write == 1 ? "buffer" : fmt::format("buffer{}", w),
            fmt::format("in[{}]", bit_idx),
            buffer_cell_,
            {{buffer_ports_[{PortRoleType::DataIn, 0}],
              data_inputs[w][bit_idx]->getNet()},
             {buffer_ports_[{PortRoleType::DataOut, 0}], D_nets[w][bit_idx]}});
      }
      marking_periphery_ = false;
    }
  }
  for (auto& [key, cell] : buffer_cells) {
    ram_grid_.addCell(std::move(cell), key.first);
  }

  // indices for creating buffers
  int r_buf_idx = 0;
  int w_buf_idx = 0;
  // append buffer, decoder, and inverter columns per port
  for (int p = 0; p < total_ports; ++p) {
    // buffer column
    // the write-capable ports first, as above
    if (p < rw_ports) {
      makeBufferColumn(fmt::format("sel_buf_rw_p{}", p),
                       num_rows,
                       decoder_output_nets[p],
                       write_select_nets[w_buf_idx++],
                       banked_ ? column_mux_ratio : 1);
      ++r_buf_idx;  // so that next read port doesn't use the rw_select
    } else if (p < rw_ports + w_ports) {
      makeBufferColumn(fmt::format("sel_buf_w_p{}", p),
                       num_rows,
                       decoder_output_nets[p],
                       write_select_nets[w_buf_idx++],
                       banked_ ? column_mux_ratio : 1);
    }
    if (p >= rw_ports + w_ports) {
      makeBufferColumn(fmt::format("sel_buf_r_p{}", p),
                       num_rows,
                       decoder_output_nets[p],
                       read_select_nets[r_buf_idx++],
                       banked_ ? column_mux_ratio : 1);
    }

    // decoder column creation
    if (banked_) {
      makeWordDecoderColumn(fmt::format("decoder_p{}", p),
                            num_rows,
                            column_mux_ratio,
                            decoder_input_nets[p],
                            decoder_output_nets[p]);
    } else {
      makeDecoderColumn(fmt::format("decoder_p{}", p),
                        num_rows,
                        decoder_input_nets[p],
                        decoder_output_nets[p]);
    }
  }

  if (num_row_bits > 0) {
    // building out inverter columns
    for (int col = 0; col < inv_col_count; ++col) {
      int start_port = col * ports_per_col;
      int end_port = std::min(start_port + ports_per_col, total_ports);
      ram_grid_.addLayout(makeInverterColumn(num_rows * bit_folds_,
                                             num_row_bits,
                                             start_port,
                                             end_port,
                                             num_word_bits,
                                             inv_addr_nets));
    }
  }

  std::unique_ptr<Cell> inv_sel_cell = std::make_unique<Cell>();
  std::vector<std::unique_ptr<Cell>> word_sel_cells(column_mux_ratio);

  if (column_mux_ratio == 2 && !banked_) {
    // place inv_addr[0] inverter in sel column
    makeInst(
        inv_sel_cell.get(),
        "word_sel",
        "inv_addr_0",
        inv_cell_,
        {{inv_ports_[{PortRoleType::DataIn, 0}], addr_inputs_[0][0]->getNet()},
         {inv_ports_[{PortRoleType::DataOut, 0}], inv_addr_nets[0][0]}});
  } else if (column_mux_ratio == 4 && !banked_) {
    for (int c = 0; c < 4; ++c) {
      word_sel_cells[c] = std::make_unique<Cell>();
    }
    makeInst(word_sel_cells[0].get(),
             "word_sel",
             "and_0",
             and2_cell_,
             {{and2_ports_[{PortRoleType::DataIn, 0}], inv_addr_nets[0][1]},
              {and2_ports_[{PortRoleType::DataIn, 1}], inv_addr_nets[0][0]},
              {and2_ports_[{PortRoleType::DataOut, 0}], word_sel_nets[0]}});
    makeInst(
        word_sel_cells[1].get(),
        "word_sel",
        "and_1",
        and2_cell_,
        {{and2_ports_[{PortRoleType::DataIn, 0}], inv_addr_nets[0][1]},
         {and2_ports_[{PortRoleType::DataIn, 1}], addr_inputs_[0][0]->getNet()},
         {and2_ports_[{PortRoleType::DataOut, 0}], word_sel_nets[1]}});
    makeInst(
        word_sel_cells[2].get(),
        "word_sel",
        "and_2",
        and2_cell_,
        {{and2_ports_[{PortRoleType::DataIn, 0}], addr_inputs_[0][1]->getNet()},
         {and2_ports_[{PortRoleType::DataIn, 1}], inv_addr_nets[0][0]},
         {and2_ports_[{PortRoleType::DataOut, 0}], word_sel_nets[2]}});
    makeInst(
        word_sel_cells[3].get(),
        "word_sel",
        "and_3",
        and2_cell_,
        {{and2_ports_[{PortRoleType::DataIn, 0}], addr_inputs_[0][1]->getNet()},
         {and2_ports_[{PortRoleType::DataIn, 1}], addr_inputs_[0][0]->getNet()},
         {and2_ports_[{PortRoleType::DataOut, 0}], word_sel_nets[3]}});
    // inverters in alternated cells
    makeInst(
        word_sel_cells[1].get(),
        "word_sel",
        "inv_addr_0",
        inv_cell_,
        {{inv_ports_[{PortRoleType::DataIn, 0}], addr_inputs_[0][0]->getNet()},
         {inv_ports_[{PortRoleType::DataOut, 0}], inv_addr_nets[0][0]}});
    makeInst(
        word_sel_cells[0].get(),
        "word_sel",
        "inv_addr_1",
        inv_cell_,
        {{inv_ports_[{PortRoleType::DataIn, 0}], addr_inputs_[0][1]->getNet()},
         {inv_ports_[{PortRoleType::DataOut, 0}], inv_addr_nets[0][1]}});
  }

  // moving column_mux select cells to the top row to be closer to connected
  // nets also removes extra column that sits to the right of decoders
  if (column_mux_ratio == 2 && !banked_) {
    ram_grid_.addCell(std::move(inv_sel_cell), mask_size * column_mux_ratio);
  } else if (column_mux_ratio == 4 && !banked_) {
    for (int c = 0; c < 4; ++c) {
      int sel_col = mask_size * column_mux_ratio + c;
      ram_grid_.addCell(std::move(word_sel_cells[c]), sel_col);
    }
  }

  // adding filler cells to ease congestion for multi-port
  if (column_mux_ratio > 1 && total_ports > 1 && !banked_) {
    int filler_spacing = num_rows / (column_mux_ratio - 1);
    if (filler_spacing > 0) {
      int filler_count = 0;
      for (int row = filler_spacing; row < num_rows; row += filler_spacing) {
        for (int col = 0; col < ram_grid_.numLayouts(); ++col) {
          ram_grid_.insertCell(nullptr, col, row + filler_count);
        }
        ++filler_count;
      }
    }
  }
  // // One extra row at the top for placing input buffers, one extra row for
  // // negative latches if use latch-based ram
  // the words' rows, then the input buffers' (a row, or one a fold)
  const int num_rows_grid
      = num_rows * bit_folds_ + bit_folds_ + (use_latch_ ? 1 : 0);

  // auto cell_inv_layout = std::make_unique<Layout>(odb::vertical);
  // // check for AND gate, specific case for 2 words
  // int inv_col_cells = 0;
  // if (num_row_bits > 1) {
  //   for (int i = num_inputs - 1; i >= num_word_bits; --i) {
  //     auto inv_grid_cell = std::make_unique<Cell>();
  //     makeInst(
  //         inv_grid_cell.get(),
  //         "decoder",
  //         fmt::format("inv_{}", i),
  //         inv_cell_,
  //         {{inv_ports_[{PortRoleType::DataIn, 0}],
  //         addr_inputs_[i]->getNet()},
  //          {inv_ports_[{PortRoleType::DataOut, 0}], inv_addr[i]}});
  //     cell_inv_layout->addCell(std::move(inv_grid_cell));
  //     ++inv_col_cells;
  //     for (int filler_count = 0; filler_count < num_row_bits - 1;
  //          ++filler_count) {
  //       cell_inv_layout->addCell(nullptr);
  //       ++inv_col_cells;
  //     }
  //   }
  // }
  // // Pad remaining slots so this column matches the grid height
  // (num_rows_grid) while (inv_col_cells < num_rows_grid) {
  //   cell_inv_layout->addCell(nullptr);
  //   ++inv_col_cells;
  // }
  // ram_grid_.addLayout(std::move(cell_inv_layout));

  auto ram_origin(odb::Point(0, 0));

  ram_grid_.setOrigin(ram_origin);
  ram_grid_.gridInit();

  // -service_columns {every sites}: after every `every` array columns
  // an empty column `sites` sites wide, room a parent's buffers can take
  // inside the array
  if (service_every > 0 && service_sites > 0) {
    const int site_width
        = (*(*db_->getLibs().begin())->getSites().begin())->getWidth();
    for (int col = (col_cell_count - 1) / service_every * service_every;
         col > 0;
         col -= service_every) {
      auto service = std::make_unique<Layout>(odb::vertical);
      service->setMinWidth(service_sites * site_width);
      ram_grid_.insertLayout(std::move(service), col);
    }
    ram_grid_.gridInit();
  }

  if (tapcell_) {
    // max tap distance specified is greater than the length of ram
    if (ram_grid_.getRowWidth() <= max_tap_dist) {
      auto tapcell_layout = generateTapColumn(num_rows_grid - 1, 0);
      ram_grid_.insertLayout(std::move(tapcell_layout), 0);
    } else {
      // needed this calculation so first cells have right distance
      int nearest_tap
          = (max_tap_dist / ram_grid_.getWidth()) * ram_grid_.getLayoutWidth(0);
      int tapcell_count = 0;
      // iterates through each of the columns
      for (int col = 0; col < ram_grid_.numLayouts(); ++col) {
        if (nearest_tap + ram_grid_.getLayoutWidth(col) >= max_tap_dist) {
          // if the nearest_tap is too far, generate tap column
          auto tapcell_layout
              = generateTapColumn(num_rows_grid - 1, tapcell_count);
          ram_grid_.insertLayout(std::move(tapcell_layout), col);
          ++col;  // col adjustment after insertion
          nearest_tap = 0;
          ++tapcell_count;
        }
        nearest_tap += ram_grid_.getLayoutWidth(col);
      }
      // check for last column in the grid
      if (nearest_tap >= max_tap_dist) {
        auto tapcell_layout
            = generateTapColumn(num_rows_grid - 1, tapcell_count);
        ram_grid_.addLayout(std::move(tapcell_layout));
      }
    }
  }

  ram_grid_.gridInit();

  auto db_libs = db_->getLibs().begin();
  auto db_sites = *(db_libs->getSites().begin());
  auto sites_width = db_sites->getWidth();

  int num_sites = ram_grid_.getRowWidth() / db_sites->getWidth();

  // // One extra row at the top for placing input buffers
  // const int num_rows_grid = num_rows + filler_rows + 1;
  for (int i = 0; i < num_rows_grid; ++i) {
    auto row_name = fmt::format("RAM_ROW{}", i);
    auto y_coord = i * ram_grid_.getHeight();
    auto row_orient = odb::dbOrientType::R0;
    if (i % 2 == 1) {
      row_orient = odb::dbOrientType::MX;
    }
    dbRow::create(block_,
                  row_name.c_str(),
                  db_sites,
                  ram_origin.getX(),
                  y_coord,
                  row_orient,
                  odb::dbRowDir::HORIZONTAL,
                  num_sites,
                  sites_width);
  }

  ram_grid_.placeGrid();

  int max_y_coord = ram_grid_.getHeight() * (num_rows_grid);
  int max_x_coord = ram_grid_.getRowWidth();

  block_->setDieArea(odb::Rect(0, 0, max_x_coord, max_y_coord));
  block_->setCoreArea(block_->computeCoreArea());
  periphery_cell_ = nullptr;
  if (periphery_unplaced_) {
    unplacePeriphery();
  }

  // with -bit_folds a word is written whole: one slice to the model
  writeBehavioralVerilog(behavioral_verilog_filename_,
                         bit_folds_ > 1 ? 1 : slices_per_word,
                         bit_folds_ > 1 ? word_bits_ : mask_size,
                         num_words,
                         rw_ports,
                         r_ports,
                         w_ports);
}

// A liberty function as a Verilog expression, its pins by their nets;
// none where a pin has no net or the function is incomplete.
static std::optional<std::string> verilogExpr(
    const sta::FuncExpr* f,
    const std::map<std::string, std::string>& nets)
{
  if (f == nullptr) {
    return std::nullopt;
  }
  auto both = [&nets](const sta::FuncExpr* f,
                      const char* op) -> std::optional<std::string> {
    auto left = verilogExpr(f->left(), nets);
    auto right = verilogExpr(f->right(), nets);
    if (!left || !right) {
      return std::nullopt;
    }
    return "(" + *left + op + *right + ")";
  };
  switch (f->op()) {
    case sta::FuncExpr::Op::port: {
      auto it = nets.find(f->port()->name());
      if (it == nets.end()) {
        return std::nullopt;
      }
      return it->second;
    }
    case sta::FuncExpr::Op::not_: {
      auto left = verilogExpr(f->left(), nets);
      if (!left) {
        return std::nullopt;
      }
      return "~(" + *left + ")";
    }
    case sta::FuncExpr::Op::and_:
      return both(f, " & ");
    case sta::FuncExpr::Op::or_:
      return both(f, " | ");
    case sta::FuncExpr::Op::xor_:
      return both(f, " ^ ");
    case sta::FuncExpr::Op::one:
      return "1'b1";
    case sta::FuncExpr::Op::zero:
      return "1'b0";
  }
  return std::nullopt;
}

// Ports as Verilog declarations, `name[i]` grouped into buses.
static std::vector<std::pair<std::string, std::string>> verilogPorts(
    const std::vector<std::pair<std::string, odb::dbIoType>>& ports)
{
  std::map<std::string, std::pair<std::string, int>> buses;
  std::vector<std::string> order;
  for (const auto& [name, io] : ports) {
    std::string base = name;
    int index = -1;
    const auto lb = name.rfind('[');
    if (lb != std::string::npos && name.back() == ']') {
      base = name.substr(0, lb);
      index = std::stoi(name.substr(lb + 1, name.size() - lb - 2));
    }
    if (buses.find(base) == buses.end()) {
      order.push_back(base);
      buses[base] = {io == odb::dbIoType::INPUT ? "input" : "output", -1};
    }
    buses[base].second = std::max(buses[base].second, index);
  }
  std::vector<std::pair<std::string, std::string>> decls;
  for (const auto& base : order) {
    const auto& [dir, hi] = buses[base];
    decls.emplace_back(
        base,
        hi < 0 ? dir + " " + base : fmt::format("{} [{}:0] {}", dir, hi, base));
  }
  return decls;
}

void RamGen::splitPeriphery()
{
  std::set<uint> periphery;  // by id: odb objects have no order
  for (dbInst* inst : periphery_insts_) {
    periphery.insert(inst->getId());
  }
  auto ref = [](dbNet* n) {
    return n->getBTerms().begin() != n->getBTerms().end()
               ? std::string((*n->getBTerms().begin())->getName())
               : "\\" + n->getName() + " ";
  };
  std::vector<std::pair<std::string, odb::dbIoType>> ports;
  for (dbBTerm* t : block_->getBTerms()) {
    ports.emplace_back(t->getName(), t->getIoType());
  }
  std::vector<std::string> wires;
  std::vector<dbBTerm*> dropped;
  std::vector<std::pair<dbNet*, odb::dbIoType>> shared;
  for (dbNet* n : block_->getNets()) {
    if (n->getSigType() == odb::dbSigType::POWER
        || n->getSigType() == odb::dbSigType::GROUND) {
      continue;
    }
    bool in_periphery = false;
    bool in_core = false;
    bool periphery_drives = false;
    for (odb::dbITerm* it : n->getITerms()) {
      const bool p = periphery.count(it->getInst()->getId()) > 0;
      in_periphery |= p;
      in_core |= !p;
      periphery_drives |= p && it->getIoType() == odb::dbIoType::OUTPUT;
    }
    if (n->getBTerms().begin() != n->getBTerms().end()) {
      if (!in_core) {
        for (dbBTerm* t : n->getBTerms()) {
          dropped.push_back(t);
        }
      }
      continue;
    }
    if (in_periphery) {
      wires.push_back(ref(n));
    }
    if (in_periphery && in_core) {
      shared.emplace_back(
          n, periphery_drives ? odb::dbIoType::INPUT : odb::dbIoType::OUTPUT);
    }
  }
  // the wrapper: the periphery's cells, before they go
  std::ostringstream body;
  for (dbInst* inst : periphery_insts_) {
    std::map<std::string, std::string> nets;
    std::vector<std::pair<std::string, std::string>> conns;
    for (odb::dbITerm* it : inst->getITerms()) {
      dbNet* n = it->getNet();
      if (n == nullptr || n->getSigType() == odb::dbSigType::POWER
          || n->getSigType() == odb::dbSigType::GROUND) {
        continue;
      }
      nets[it->getMTerm()->getName()] = ref(n);
      conns.emplace_back(it->getMTerm()->getName(), ref(n));
    }
    // a logic cell, one output with a function, as an assign
    auto liberty = network_->libertyCell(network_->dbToSta(inst->getMaster()));
    sta::LibertyPort* out = nullptr;
    int outputs = 0;
    if (liberty && !liberty->hasSequentials() && !liberty->isClockGate()) {
      std::unique_ptr<sta::ConcreteCellPortIterator> it(
          liberty->portIterator());
      while (it->hasNext()) {
        auto port = it->next()->libertyPort();
        if (port && port->direction()->isAnyOutput()) {
          ++outputs;
          out = port;
        }
      }
    }
    if (outputs == 1 && out->function() && !out->direction()->isTristate()
        && nets.count(out->name())) {
      auto expr = verilogExpr(out->function(), nets);
      if (!expr) {
        logger_->error(RAM,
                       101,
                       "-periphery verilog: {} ({}) has an input with no net, "
                       "or a function that can't be written as Verilog.",
                       inst->getName(),
                       inst->getMaster()->getName());
      }
      body << "  assign " << nets[out->name()] << " = " << *expr << ";  // "
           << inst->getName() << "\n";
      continue;
    }
    body << "  " << inst->getMaster()->getName() << " \\" << inst->getName()
         << " (";
    for (size_t i = 0; i < conns.size(); ++i) {
      body << (i ? ", " : "") << "." << conns[i].first << "(" << conns[i].second
           << ")";
    }
    body << ");\n";
  }
  // the core: its own ports kept, the shared nets new ones, named as a
  // Verilog and liberty identifier (a net's name has dots and brackets)
  std::map<std::string, std::string> shared_ref;  // core port -> wrapper net
  for (auto& [n, io] : shared) {
    std::string name = n->getName();
    for (char& c : name) {
      if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
        c = '_';
      }
    }
    while (
        block_->findBTerm(name.c_str()) != nullptr
        || (name != n->getName() && block_->findNet(name.c_str()) != nullptr)) {
      name += "_";
    }
    shared_ref[name] = ref(n);
    // a port's net goes by the port's name, as the netlist writes it
    n->rename(name.c_str());
    dbBTerm* t = dbBTerm::create(n, name.c_str());
    t->setIoType(io);
    t->setSigType(odb::dbSigType::SIGNAL);
  }
  // the pin placer constrains what is left of the pin lists
  std::set<uint> gone;  // by id: odb objects have no order
  for (dbBTerm* t : dropped) {
    gone.insert(t->getId());
  }
  auto forget = [&gone](std::vector<dbBTerm*>& terms) {
    std::erase_if(terms,
                  [&gone](dbBTerm* t) { return gone.count(t->getId()) > 0; });
  };
  forget(data_inputs_);
  for (auto& terms : addr_inputs_) {
    forget(terms);
  }
  for (auto& terms : q_outputs_) {
    forget(terms);
  }
  for (dbBTerm* t : dropped) {
    odb::dbBTerm::destroy(t);
  }
  for (dbInst* inst : periphery_insts_) {
    dbInst::destroy(inst);
  }
  periphery_insts_.clear();
  std::vector<dbNet*> empty;
  for (dbNet* n : block_->getNets()) {
    if (n->getITerms().empty() && n->getBTerms().empty()) {
      empty.push_back(n);
    }
  }
  for (dbNet* n : empty) {
    dbNet::destroy(n);
  }
  std::vector<std::pair<std::string, odb::dbIoType>> core_ports;
  for (dbBTerm* t : block_->getBTerms()) {
    core_ports.emplace_back(t->getName(), t->getIoType());
  }
  std::ofstream out(wrapper_verilog_);
  if (!out) {
    logger_->error(RAM, 86, "Unable to open file {}", wrapper_verilog_);
  }
  const auto decls = verilogPorts(ports);
  out << "// generate_ram's periphery around " << block_->getName()
      << ", as Verilog\nmodule " << ram_name_ << " (\n";
  for (size_t i = 0; i < decls.size(); ++i) {
    out << "  " << decls[i].second << (i + 1 < decls.size() ? ",\n" : "\n");
  }
  out << ");\n";
  for (const auto& w : wires) {
    out << "  wire " << w << ";\n";
  }
  out << body.str();
  // the core by its ports: a bus of the module's own, a shared net bit by
  // bit, by its name
  const auto core_decls = verilogPorts(core_ports);
  out << "  " << block_->getName() << " core (";
  for (size_t i = 0; i < core_decls.size(); ++i) {
    const std::string& p = core_decls[i].first;
    auto net = shared_ref.find(p);
    out << (i ? ", " : "") << "." << p << "("
        << (net == shared_ref.end() ? p : net->second) << ")";
  }
  out << ");\nendmodule\n";
}

// -periphery unplaced: the periphery left for a parent to place and the
// outline cut to what stays, the placed core, so a parent sizing its
// floorplan by utilization charges the core and nothing else. Each pin
// goes where its connections land, the centroid of the cells on its net
// (those of the periphery too, while they are still placed), on the pin
// layer's tracks inside the outline: these pins are for a macro
// placer to orient the core by, never routed.
void RamGen::unplacePeriphery()
{
  const size_t periphery_count = periphery_insts_.size();
  if (periphery_verilog_) {
    splitPeriphery();
  }
  odb::dbTechLayer* layer = db_->getTech()->findLayer(pin_layer_.c_str());
  if (layer == nullptr) {
    logger_->error(RAM,
                   75,
                   "-periphery unplaced needs -pin_layer, a routing layer; "
                   "{} is not one.",
                   pin_layer_);
  }
  // the pins' centres, before anything moves
  std::vector<std::pair<dbBTerm*, odb::Point>> pins;
  for (dbBTerm* term : block_->getBTerms()) {
    dbNet* net = term->getNet();
    if (net == nullptr) {
      continue;
    }
    int64_t sx = 0;
    int64_t sy = 0;
    int n = 0;
    for (odb::dbITerm* it : net->getITerms()) {
      const odb::Rect b = it->getInst()->getBBox()->getBox();
      sx += b.xCenter();
      sy += b.yCenter();
      ++n;
    }
    if (n > 0) {
      pins.emplace_back(term, odb::Point(sx / n, sy / n));
    }
  }
  for (odb::dbInst* inst : periphery_insts_) {
    inst->setPlacementStatus(odb::dbPlacementStatus::UNPLACED);
  }
  // the placed core's bounding box, and a row above it: a parent whose
  // rows are of the other parity there moves the core up a row
  odb::Rect core;
  core.mergeInit();
  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->getPlacementStatus().isPlaced()) {
      core.merge(inst->getBBox()->getBox());
    }
  }
  odb::dbRow* first = *block_->getRows().begin();
  const int row_h = first->getBBox().dy();
  odb::dbSite* site = first->getSite();
  const int site_w = site->getWidth();
  const int dx = core.xMin();
  const int dy = core.yMin();
  const int first_row = dy / row_h;
  const int num_rows = core.dy() / row_h + 1;
  const int width = core.dx();
  const int height = num_rows * row_h;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->getPlacementStatus().isPlaced()) {
      const odb::Point at = inst->getLocation();
      const odb::dbPlacementStatus status = inst->getPlacementStatus();
      // a FIRM cell's origin is fixed (ODB-0359)
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      inst->setLocation(at.x() - dx, at.y() - dy);
      inst->setPlacementStatus(status);
    }
  }
  std::vector<odb::dbRow*> rows(block_->getRows().begin(),
                                block_->getRows().end());
  for (odb::dbRow* row : rows) {
    odb::dbRow::destroy(row);
  }
  for (int r = 0; r < num_rows; ++r) {
    dbRow::create(block_,
                  fmt::format("RAM_ROW{}", r).c_str(),
                  site,
                  0,
                  r * row_h,
                  (first_row + r) % 2 == 0 ? odb::dbOrientType::R0
                                           : odb::dbOrientType::MX,
                  odb::dbRowDir::HORIZONTAL,
                  width / site_w,
                  site_w);
  }
  block_->setDieArea(odb::Rect(0, 0, width, height));
  block_->setCoreArea(block_->computeCoreArea());
  // each pin on the layer's tracks, inside the outline
  const int pitch = std::max(1, static_cast<int>(layer->getPitch()));
  const int offset = layer->getOffset();
  const int half = std::max(1, static_cast<int>(layer->getWidth()) / 2);
  auto on_track = [&](int v, int hi) {
    int t = offset + std::max(0, (v - offset + pitch / 2) / pitch) * pitch;
    while (t > hi - half && t - pitch >= half) {
      t -= pitch;
    }
    while (t < half) {
      t += pitch;
    }
    return t;
  };
  // the tracks run in the layer's preferred direction: across it a pin
  // sits on a track, along it on the manufacturing grid
  odb::dbTech* tech = db_->getTech();
  const int grid
      = tech->hasManufacturingGrid() ? tech->getManufacturingGrid() : 1;
  auto on_grid = [&](int v, int hi) {
    int t = std::clamp(v, half, std::max(half, hi - half));
    t -= t % grid;
    while (t < half) {
      t += grid;
    }
    return t;
  };
  const bool horizontal
      = layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
  for (auto& [term, at] : pins) {
    const int x = horizontal ? on_grid(at.x() - dx, width)
                             : on_track(at.x() - dx, width);
    const int y = horizontal ? on_track(at.y() - dy, height)
                             : on_grid(at.y() - dy, height);
    odb::dbBPin* pin = odb::dbBPin::create(term);
    odb::dbBox::create(pin, layer, x - half, y - half, x + half, y + half);
    pin->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  }
  logger_->info(RAM,
                76,
                "-periphery {}: {} periphery instances {}; the core is {:.3f} "
                "x {:.3f} um.",
                periphery_verilog_ ? "verilog" : "unplaced",
                periphery_count,
                periphery_verilog_ ? "written to " + wrapper_verilog_
                                   : std::string("left unplaced"),
                block_->dbuToMicrons(width),
                block_->dbuToMicrons(height));
}

void RamGen::setBehavioralVerilogFilename(const std::string& filename)
{
  behavioral_verilog_filename_ = filename;
}

void RamGen::writeBehavioralVerilog(const std::string& filename,
                                    const int slices_per_word,
                                    const int mask_size,
                                    const int num_words,
                                    const int rw_ports,
                                    const int r_ports,
                                    const int w_ports)
{
  if (filename.empty()) {
    return;
  }

  const int word_size_bit = slices_per_word * mask_size;
  const int address_width
      = (num_words <= 1) ? 1 : std::ceil(std::log2(num_words));

  std::string module_name = ram_name_;

  int r_total = rw_ports + r_ports;
  const int num_write = rw_ports + w_ports;
  // a write port's bus: D, we, addr_w, or with more than one D0, we0, ...
  auto wbus = [num_write](const char* base, int w) {
    return num_write == 1 ? std::string(base) : fmt::format("{}{}", base, w);
  };
  // Build port list
  std::string port_list = "\n  clk";
  // -reset_value: the reset port, at the storage flop's polarity
  const std::string reset_name
      = reset_value_.empty() ? "" : (reset_active_low_ ? "rst_n" : "rst");
  if (!reset_name.empty()) {
    port_list += ",\n  " + reset_name;
  }
  for (int w = 0; w < num_write; ++w) {
    port_list += ",\n  " + wbus("D", w);
  }
  for (int i = 0; i < r_total; i++) {
    if (r_total == 1) {
      port_list += ",\n  Q";
    } else {
      port_list += fmt::format(",\n  Q{}", i);
    }
  }
  // address ports
  if (rw_ports > 0) {
    port_list += ",\n  addr_rw";
  } else {
    for (int w = 0; w < w_ports; ++w) {
      port_list += ",\n  " + wbus("addr_w", w);
    }
  }
  for (int i = 0; i < r_ports; i++) {
    port_list += fmt::format(",\n  addr_r{}", i);
  }
  // banked read ports: an address and a data output per bank
  const int bank_bits = std::max(
      1, static_cast<int>(std::ceil(std::log2(std::max(words_per_bank_, 1)))));
  for (int k = 0; k < rb_ports_; ++k) {
    for (int b = 0; b < banks_; ++b) {
      port_list += fmt::format(",\n  addr_rb{}_b{}", k, b);
    }
  }
  for (int w = 0; w < num_write; ++w) {
    port_list += ",\n  " + wbus("we", w);
  }

  // Build output declarations
  std::string output_declaration;
  for (int i = 0; i < r_total; i++) {
    if (r_total == 1) {
      output_declaration
          += fmt::format("  output reg [{}:0] Q;\n", word_size_bit - 1);
    } else {
      output_declaration
          += fmt::format("  output reg [{}:0] Q{};\n", word_size_bit - 1, i);
    }
  }

  // Build address declarations
  std::string addr_declarations;
  if (rw_ports > 0) {
    addr_declarations
        += fmt::format("  input [{}:0] addr_rw;\n", address_width - 1);
  } else {
    for (int w = 0; w < w_ports; ++w) {
      addr_declarations += fmt::format(
          "  input [{}:0] {};\n", address_width - 1, wbus("addr_w", w));
    }
  }
  for (int i = 0; i < r_ports; i++) {
    addr_declarations
        += fmt::format("  input [{}:0] addr_r{};\n", address_width - 1, i);
  }

  // Build read port logic
  std::string read_port_logic;
  for (int k = 0; k < rb_ports_; ++k) {
    for (int b = 0; b < banks_; ++b) {
      port_list += fmt::format(",\n  Qrb{}_b{}", k, b);
      output_declaration += fmt::format(
          "  output reg [{}:0] Qrb{}_b{};\n", word_size_bit - 1, k, b);
      addr_declarations += fmt::format(
          "  input [{}:0] addr_rb{}_b{};\n", bank_bits - 1, k, b);
      // the word at index addr in bank b
      const std::string word
          = bank_interleaved_
                ? fmt::format("addr_rb{}_b{} * {} + {}", k, b, banks_, b)
                : fmt::format("{} + addr_rb{}_b{}", b * words_per_bank_, k, b);
      const std::string value
          = zero_word_ >= 0 ? fmt::format("({}) == {} ? {}'d0 : mem[{}]",
                                          word,
                                          zero_word_,
                                          word_size_bit,
                                          word)
                            : fmt::format("mem[{}]", word);
      read_port_logic += fmt::format(R"(  always @(*) begin
    Qrb{}_b{} = {};
  end

)",
                                     k,
                                     b,
                                     value);
    }
  }
  for (int i = 0; i < r_total; i++) {
    std::string port_name = (r_total == 1) ? "Q" : fmt::format("Q{}", i);
    std::string addr_name;
    if (i == 0 && rw_ports > 0) {
      addr_name = "addr_rw";
    } else if (rw_ports > 0) {
      addr_name = fmt::format("addr_r{}", i - 1);
    } else {
      addr_name = fmt::format("addr_r{}", i);
    }
    if (i > 0) {
      read_port_logic += "\n";
    }
    if (zero_word_ >= 0) {
      read_port_logic += fmt::format(R"(  always @(*) begin
    {} = {} == {} ? {}'d0 : mem[{}];
  end
)",
                                     port_name,
                                     addr_name,
                                     zero_word_,
                                     word_size_bit,
                                     addr_name);
    } else {
      read_port_logic += fmt::format(R"(  always @(*) begin
    {} = mem[{}];
  end
)",
                                     port_name,
                                     addr_name);
    }
  }

  // -register_read_address 1: every read address registered, the reads from the
  // registered ones (addr_r0 -> addr_r0_q)
  if (register_read_address_ > 0) {
    std::string registers;
    auto registered = [&](const std::string& name, int width) {
      registers += fmt::format(
          "  reg [{}:0] {}_q;\n  always @(posedge clk) {}_q <= {};\n",
          width - 1,
          name,
          name,
          name);
      // whole words only: addr_r1 is not part of addr_r10
      std::string out;
      size_t at = 0;
      while (true) {
        size_t hit = read_port_logic.find(name, at);
        if (hit == std::string::npos) {
          out += read_port_logic.substr(at);
          break;
        }
        const size_t end = hit + name.size();
        const bool whole
            = end >= read_port_logic.size()
              || !(
                  std::isalnum(static_cast<unsigned char>(read_port_logic[end]))
                  || read_port_logic[end] == '_');
        out += read_port_logic.substr(at, end - at);
        if (whole) {
          out += "_q";
        }
        at = end;
      }
      read_port_logic = out;
    };
    for (int i = 0; i < r_ports; ++i) {
      registered(fmt::format("addr_r{}", i), address_width);
    }
    for (int k = 0; k < rb_ports_; ++k) {
      for (int b = 0; b < banks_; ++b) {
        registered(fmt::format("addr_rb{}_b{}", k, b), bank_bits);
      }
    }
    read_port_logic = registers + "\n" + read_port_logic;
  }

  // Write logic, the ports in order: where two write the same word in a
  // cycle, the last one's data is kept. With -write_priority or each
  // port also writes the data of every earlier port writing the same
  // slice, so the last one writes the OR of them all.
  std::string data_declarations;
  std::string enable_declarations;
  std::string write_logic;
  for (int w = 0; w < num_write; ++w) {
    const std::string write_addr
        = rw_ports > 0 ? std::string("addr_rw") : wbus("addr_w", w);
    std::string write_data
        = fmt::format("{}[i*{} +:{}]", wbus("D", w), mask_size, mask_size);
    for (int v = 0; write_or_ && v < w; ++v) {
      write_data += fmt::format(
          "\n          | ({{{}{{{}[i] && {} == {}}}}} & {}[i*{} +:{}])",
          mask_size,
          wbus("we", v),
          wbus("addr_w", v),
          write_addr,
          wbus("D", v),
          mask_size,
          mask_size);
    }
    data_declarations
        += fmt::format("  input [{}:0] {};\n", word_size_bit - 1, wbus("D", w));
    enable_declarations += fmt::format(
        "  input [{}:0] {};\n", slices_per_word - 1, wbus("we", w));
    write_logic += fmt::format(
        R"(    for (i = 0; i < {}; i = i + 1) begin
      if ({}[i]{}) begin
        mem[{}][i*{} +:{}] <= {};
      end
    end
)",
        slices_per_word,
        wbus("we", w),
        zero_word_ >= 0 ? fmt::format(" && {} != {}", write_addr, zero_word_)
                        : std::string(),
        write_addr,
        mask_size,
        mask_size,
        write_data);
  }

  std::string reset_declaration;
  std::string always_head = "  always @(posedge clk) begin\n";
  if (!reset_name.empty()) {
    std::string reset_word;  // most significant bit first
    for (int bit = word_size_bit - 1; bit >= 0; --bit) {
      reset_word += reset_bits_.at(bit) ? '1' : '0';
    }
    reset_declaration = fmt::format("  input {};\n", reset_name);
    always_head = fmt::format(
        "  always @(posedge clk or {}edge {}) begin\n"
        "    if ({}{}) begin\n"
        "      for (i = 0; i < {}; i = i + 1) begin\n"
        "        mem[i] <= {}'b{};\n"
        "      end\n"
        "    end else begin\n",
        reset_active_low_ ? "neg" : "pos",
        reset_name,
        reset_active_low_ ? "!" : "",
        reset_name,
        num_words,
        word_size_bit,
        reset_word);
    write_logic += "    end\n";
  }

  std::string verilog_code = fmt::format(R"(module {} ({}
);
  input clk;
{}{}{}{}{}
  // memory array declaration
  reg [{}:0] mem[0:{}];

  // write logic
  integer i;
{}{}  end

  // read logic
{}
endmodule
)",
                                         module_name,
                                         port_list,
                                         reset_declaration,
                                         data_declarations,
                                         output_declaration,
                                         addr_declarations,
                                         enable_declarations,
                                         word_size_bit - 1,
                                         num_words - 1,
                                         always_head,
                                         write_logic,
                                         read_port_logic);

  std::ofstream vf(filename);
  if (!vf.is_open()) {
    logger_->error(RAM, 23, "Unable to open file {}", filename);
  }

  vf << verilog_code;
  vf.close();
  logger_->info(RAM, 24, "Behavioral Verilog written for {}", module_name);
}

}  // namespace ram
