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
#include <limits>
#include <map>
#include <memory>
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

RamGen::TreeEnd RamGen::planReadTree(const std::string& prefix,
                                     const int word_idx,
                                     const int bit,
                                     const vector<int>& rows,
                                     const int lo,
                                     const int hi,
                                     const vector<dbNet*>& selects,
                                     dbNet* out)
{
  auto add = [&](const int row,
                 dbMaster* master,
                 const std::string& name,
                 const vector<std::pair<std::string, dbNet*>>& conns) {
    tree_nodes_[{row, word_idx, bit}].push_back({master, prefix, name, conns});
  };
  // the zero word's term, where the trees read inverted data: its own
  // select, the inverted form of a stored 0 (the zero word is in the
  // tree only then)
  auto q = [&](const int row) {
    return row == zero_word_ ? selects[row]
                             : storage_nets_.at({row, word_idx, bit});
  };
  auto net = [&](dbNet* given) {
    return given ? given : makeNet(prefix, fmt::format("t{}_{}", lo, hi));
  };
  const int n = hi - lo;
  if (n == 1) {
    const int r = rows[lo];
    dbNet* y = net(out);
    add(r,
        nand2_cell_,
        fmt::format("nand_r{}", r),
        {{nand2_ports_[{PortRoleType::DataIn, 0}], selects[r]},
         {nand2_ports_[{PortRoleType::DataIn, 1}], q(r)},
         {nand2_ports_[{PortRoleType::DataOut, 0}], y}});
    return {y, true};
  }
  if (n == 2) {
    const int r0 = rows[lo];
    const int r1 = rows[lo + 1];
    dbNet* y = net(out);
    add(r0,
        aoi22_cell_,
        fmt::format("aoi_r{}", r0),
        {{aoi22_in_a1_, selects[r0]},
         {aoi22_in_a2_, q(r0)},
         {aoi22_in_b1_, selects[r1]},
         {aoi22_in_b2_, q(r1)},
         {aoi22_out_, y}});
    return {y, true};
  }
  int half = 1;
  while (half * 2 < n) {
    half *= 2;
  }
  const int mid = lo + half;
  const int host = rows[mid - 1];
  const TreeEnd a
      = planReadTree(prefix, word_idx, bit, rows, lo, mid, selects, nullptr);
  TreeEnd b
      = planReadTree(prefix, word_idx, bit, rows, mid, hi, selects, nullptr);
  if (b.inverted != a.inverted) {
    dbNet* nb = makeNet(prefix, fmt::format("t{}_{}_inv", mid, hi));
    add(rows[mid],
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
  if (row_idx == zero_word_) {
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
      ram_grid_.addCell(
          std::move(cell),
          slice_idx * (mask_size * column_mux_ratio + column_mux_ratio)
              + local_bit * column_mux_ratio + word_idx);
    }
    ram_grid_.addCell(
        nullptr,
        slice_idx * (mask_size * column_mux_ratio + column_mux_ratio)
            + mask_size * column_mux_ratio + word_idx);
    return;
  }

  int r_total = rw_ports + r_ports;
  vector<dbNet*> select_b_nets(r_total);
  if (and_or_read_) {
    // the AND-OR trees read the true selects
  } else if (column_mux_ratio == 1) {
    for (int i = 0; i < r_total; ++i) {
      select_b_nets[i] = makeNet(prefix, fmt::format("select{}_b", i));
    }
  } else {
    select_b_nets = shared_select_b_nets;
  }

  auto gclock_net = makeNet(prefix, "gclock");
  auto we_net = makeNet(prefix, "we");
  auto sel_cell = std::make_unique<Cell>();

  // -reset_value: one tie per word slice holds the flops' unused pins off
  dbNet* reset_off = nullptr;
  if (reset_port_) {
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
  // last port wins, as in the behavioral model. The clock gate opens for
  // any of them.
  vector<dbNet*> take(num_write);
  if (num_write > 1) {
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

  for (int local_bit = 0; local_bit < mask_size; ++local_bit) {
    auto name = fmt::format("{}.bit{}", prefix, start_bit_idx + local_bit);
    vector<dbNet*> outs(r_total);
    for (int r_port = 0; r_port < r_total; ++r_port) {
      outs[r_port] = data_output[r_port][local_bit];
    }

    dbNet* d = data_input[0][local_bit];
    if (num_write > 1) {
      d = makeNet(name, "d");
    }
    const int bit = start_bit_idx + local_bit;
    const int reset_bit
        = reset_port_
              ? (bit < 64 ? static_cast<int>((reset_bits_ >> bit) & 1) : 0)
              : -1;
    auto bit_cell = makeBit(
        name,
        and_or_read_ ? 0 : r_total,
        gclock_net,
        select_b_nets,
        d,
        outs,
        and_or_read_ ? storage_nets_.at({row_idx, word_idx, bit}) : nullptr,
        reset_bit,
        reset_off);
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
                         ? d
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
          dbNet* sum = last ? d : makeNet(name, fmt::format("d_or{}", w / 2));
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

    int bit_col = slice_idx * (mask_size * column_mux_ratio + column_mux_ratio)
                  + local_bit * column_mux_ratio + word_idx;
    ram_grid_.addCell(std::move(bit_cell), bit_col);
  }

  // Make clock gate
  makeInst(sel_cell.get(),
           prefix,
           "cg",
           clock_gate_cell_,
           {{clock_gate_ports_[{PortRoleType::Clock, 0}], clock},
            {clock_gate_ports_[{PortRoleType::DataIn, 0}], we_net},
            {clock_gate_ports_[{PortRoleType::DataOut, 0}], gclock_net}});

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
  if (num_write == 1) {
    makeInst(sel_cell.get(),
             prefix,
             "gcand",
             and2_cell_,
             {{and2_ports_[{PortRoleType::DataIn, 0}], write_sel},
              {and2_ports_[{PortRoleType::DataIn, 1}], write_enable[0]},
              {and2_ports_[{PortRoleType::DataOut, 0}], we_net}});
  }

  // Make select inverters
  if (create_select_inv && !and_or_read_) {
    for (int i = 0; i < r_total; ++i) {
      makeInst(sel_cell.get(),
               prefix,
               fmt::format("select_inv_{}", i),
               inv_cell_,
               {{inv_ports_[{PortRoleType::DataIn, 0}], selects[num_write + i]},
                {inv_ports_[{PortRoleType::DataOut, 0}], select_b_nets[i]}});
    }
  }

  int sel_col = slice_idx * (mask_size * column_mux_ratio + column_mux_ratio)
                + mask_size * column_mux_ratio + word_idx;
  ram_grid_.addCell(std::move(sel_cell), sel_col);
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
      slice_enables.push_back(write_enable[w][slice]->getNet());
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
  }
  ram_grid_.addLayout(std::move(decoder_layout));
}

void RamGen::makeBufferColumn(const std::string& prefix,
                              const int num_rows,
                              const std::vector<dbNet*>& decoder_output_nets,
                              const std::vector<dbNet*>& select_nets)
{
  auto buffer_layout = std::make_unique<Layout>(odb::vertical);
  for (int word = 0; word < num_rows; ++word) {
    auto buf_cell = std::make_unique<Cell>();
    makeInst(
        buf_cell.get(),
        prefix,
        fmt::format("sel_buf_{}", word),
        buffer_cell_,
        {{buffer_ports_[{PortRoleType::DataIn, 0}], decoder_output_nets[word]},
         {buffer_ports_[{PortRoleType::DataOut, 0}], select_nets[word]}});
    buffer_layout->addCell(std::move(buf_cell));
  }
  ram_grid_.addLayout(std::move(buffer_layout));
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

  // number of inverters per bit group (one per port in this column)
  // fillers evenly space each bit group within the column
  int fillers_per_group = (num_rows / num_row_bits) - ports_in_col;
  // ensures we don't go negative for small rams
  fillers_per_group = std::max(0, fillers_per_group);

  for (int bit = num_row_bits - 1; bit >= 0; --bit) {
    for (int p = start_port; p < end_port; ++p) {
      auto inv_grid_cell = std::make_unique<Cell>();
      makeInst(inv_grid_cell.get(),
               fmt::format("decoder_p{}", p),
               fmt::format("inv_p{}_a{}", p, bit),
               inv_cell_,
               {{inv_ports_[{PortRoleType::DataIn, 0}],
                 addr_inputs_[p][num_word_bits + bit]->getNet()},
                {inv_ports_[{PortRoleType::DataOut, 0}],
                 inv_addr_nets[p][num_word_bits + bit]}});
      cell_inv_layout->addCell(std::move(inv_grid_cell));
    }
    // add fillers between bit groups
    for (int f = 0; f < fillers_per_group; ++f) {
      cell_inv_layout->addCell(nullptr);
    }
  }

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
    const bool required)
{
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
      while (port_iter->hasNext()) {
        auto lib_port = port_iter->next();
        auto dir = lib_port->direction();
        if (dir->isAnyOutput()) {
          if (!out) {
            out = lib_port;
          } else {
            out = nullptr;  // no multi-output gates
            break;
          }
        }
      }

      if (!out || !match(out->libertyPort())) {
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
  tie_off_cell_ = findMaster(
      [off_high](sta::LibertyPort* port) {
        auto f = port->function();
        return f && port->direction()->isOutput()
               && f->op()
                      == (off_high ? sta::FuncExpr::Op::one
                                   : sta::FuncExpr::Op::zero);
      },
      off_high ? "tie high" : "tie low");
  tie_off_pin_ = buildPortMap(tie_off_cell_)[{PortRoleType::DataOut, 0}];

  try {
    reset_bits_ = std::stoull(reset_value_, nullptr, 0);
  } catch (const std::exception&) {
    logger_->error(RAM, 64, "-reset_value {} is not a number.", reset_value_);
  }
}

void RamGen::findMasters()
{
  if (!inv_cell_) {
    inv_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          return port->libertyCell()->isInverter();
        },
        "inverter");
  }
  inv_ports_ = buildPortMap(inv_cell_);

  // The read path: tristate bitlines where the library has a tristate
  // cell, AND-OR trees where it has none, or as -read_style says.
  if (read_style_ == "and_or") {
    and_or_read_ = true;
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

  if (!and2_cell_) {
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
        "and2");
  }
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
            // with a reset, a flop with both an asynchronous clear and
            // preset, so any bit can take either value
            if (reset) {
              bool clear_preset = false;
              for (const auto& seq : port->libertyCell()->sequentials()) {
                clear_preset |= seq.isRegister() && seq.clear() && seq.preset();
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
          "storage");
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

  if (!clock_gate_cell_) {
    clock_gate_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          return port->libertyCell()->isClockGate();
        },
        "clock gate");
  }
  clock_gate_ports_ = buildPortMap(clock_gate_cell_);

  // for input buffers
  if (!buffer_cell_) {
    buffer_cell_ = findMaster(
        [](sta::LibertyPort* port) { return port->libertyCell()->isBuffer(); },
        "buffer");
  }
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
  if (!aoi22_cell_) {
    // AOI22 truth table: Y = !(A1&A2 | B1&B2)
    std::array<std::string, 4> in;
    aoi22_cell_ = findMaster4(0x0777, "aoi22", in, aoi22_out_);
    aoi22_in_a1_ = in[0];
    aoi22_in_a2_ = in[1];
    aoi22_in_b1_ = in[2];
    aoi22_in_b2_ = in[3];
  }

  // The AND-OR read path's tree: NAND2 and NOR2 levels over AOI22 leaves.
  if (and_or_read_) {
    // by truth table: asap7 writes its NAND2 as (!A) + (!B)
    if (!nand2_cell_) {
      nand2_cell_ = findMaster(
          [](sta::LibertyPort* port) { return twoInputTable(port) == 0x7; },
          "nand2");
    }
    nand2_ports_ = buildPortMap(nand2_cell_);
    if (!nor2_cell_) {
      nor2_cell_ = findMaster(
          [](sta::LibertyPort* port) { return twoInputTable(port) == 0x1; },
          "nor2");
    }
    nor2_ports_ = buildPortMap(nor2_cell_);
  }

  // More than one write port: the write data mux, an AO22 per pair of
  // ports, and an OR2 for the write enables and across pairs.
  if (num_write_ > 1) {
    if (!ao22_cell_) {
      // AO22 truth table: Y = A1&A2 | B1&B2
      ao22_cell_ = findMaster4(
          0xF888, "ao22", ao22_in_, ao22_out_, /*required=*/false);
    }
    if (!or2_cell_) {
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
          "or2");
    }
    or2_ports_ = buildPortMap(or2_cell_);
  }
}

dbMaster* RamGen::findMaster4(const uint16_t table,
                              const char* name,
                              std::array<std::string, 4>& inputs_out,
                              std::string& output,
                              const bool required)
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

  return findMaster(
      [&](sta::LibertyPort* out_port) -> bool {
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
      },
      name,
      required);
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
  io_placer_->addHorLayer(pin_tech->findLayer(hor_name));
  io_placer_->addVerLayer(pin_tech->findLayer(ver_name));
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
                      const int word_size,
                      const int num_words,
                      const int rw_ports,
                      const int r_ports,
                      const int w_ports,
                      const int column_mux_ratio,
                      const bool use_latch,
                      dbMaster* storage_cell,
                      dbMaster* tristate_cell,
                      dbMaster* inv_cell,
                      dbMaster* tapcell,
                      int max_tap_dist,
                      const std::string& read_style,
                      const int zero_word,
                      const std::string& reset_value)
{
  if (num_words / column_mux_ratio == 1) {
    logger_->error(RAM, 42, "1-row RAMs are not supported.");
  }
  const int slices_per_word = word_size / mask_size;
  const std::string ram_name = fmt::format("RAM{}x{}", num_words, word_size);

  // error checking for column_mux_ratio
  if (column_mux_ratio != 1 && column_mux_ratio != 2 && column_mux_ratio != 4) {
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
  const int num_word_bits = (column_mux_ratio > 1)
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

  // A zero word reads 0 and drops its writes, as a RISC-V x0: its row
  // has no flops, and the read trees leave it out.
  if (zero_word_ >= 0) {
    if (zero_word_ >= num_words) {
      logger_->error(
          RAM, 59, "-zero_word {} is not a word of {}.", zero_word_, num_words);
    }
    if (!and_or_read_ || column_mux_ratio != 1) {
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
    block_ = odb::dbBlock::create(chip, ram_name.c_str());
  }

  // One column per bit plus one select/control column per slice,
  // Each read/write port gets a buffer col and decoder col,
  // plus inverter columns shared across ports
  const int total_ports = rw_ports + r_ports + w_ports;
  const int ports_per_col
      = (num_row_bits > 0) ? (num_rows / num_row_bits) : total_ports;
  const int inv_col_count
      = std::ceil(static_cast<float>(total_ports) / ports_per_col);

  // With column_mux_ratio = 1: One column per bit plus one select/control
  // column per slice, plus one extra decoder column With column_mux_ratio > 1:
  // Each slice has (mask_size * column_mux_ratio) columns (one per bit per word
  // in the row), plus one select/control column per slice (one per word), plus
  // one extra decoder column at far right shared across all row

  // mux layout columns + per-port buffer/decoder columns + inverter columns
  int col_cell_count
      = slices_per_word * (mask_size * column_mux_ratio + column_mux_ratio);
  ram_grid_.setNumLayouts(col_cell_count);

  auto clock = makeBTerm("clk", dbIoType::INPUT);

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
    for (int slice = 0; slice < slices_per_word; ++slice) {
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

  addr_inputs_.resize(total_ports);
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
    } else {
      port_prefix = fmt::format("addr_r{}", r_idx++);
    }
    // NOLINTEND(misc-include-cleaner)

    addr_inputs_[p].resize(num_inputs);
    inv_addr_nets[p].resize(num_inputs);

    for (int i = 0; i < num_inputs; ++i) {
      addr_inputs_[p][i]
          = makeBTerm(fmt::format("{}[{}]", port_prefix, i), dbIoType::INPUT);
      inv_addr_nets[p][i]
          = makeNet(port_prefix, fmt::format("inv_addr[{}]", i));
    }

    // inputs to decoders
    decoder_input_nets[p].assign(num_rows, vector<dbNet*>(num_row_bits));
    for (int row = 0; row < num_rows; ++row) {
      int row_num = row;
      // start at right most bit
      for (int input = 0; input < num_row_bits; ++input) {
        if (row_num % 2 == 0) {
          // places inverted address for each input
          decoder_input_nets[p][row][input]
              = inv_addr_nets[p][num_word_bits + input];
        } else {  // puts original input in invert nets
          decoder_input_nets[p][row][input]
              = addr_inputs_[p][num_word_bits + input]->getNet();
        }
        row_num /= 2;
      }
    }

    // output nets of each port decoder
    decoder_output_nets[p]
        = makeDecoderOutputNets(fmt::format("decoder_p{}", p), num_rows);
    // output nets of each decoder buffers
    // write-capable ports to index 0 since there can only be 1
    if (p < rw_ports) {
      write_select_nets[w_sel_idx] = makeSelectNets(
          fmt::format("rw_sel_p{}", p), num_rows, RamPortType::ReadWrite);
      read_select_nets[r_sel_idx++] = write_select_nets[w_sel_idx];
      ++w_sel_idx;
    } else if (p < rw_ports + w_ports) {
      write_select_nets[w_sel_idx] = makeSelectNets(
          fmt::format("w_sel_p{}", p), num_rows, RamPortType::Write);
      ++w_sel_idx;
    }
    if (p >= rw_ports + w_ports) {
      read_select_nets[r_sel_idx++] = makeSelectNets(
          fmt::format("r_sel_p{}", p), num_rows, RamPortType::Read);
    }
  }

  // start of input/output net creation
  int r_total = rw_ports + r_ports;
  q_outputs_.resize(r_total);
  // One data bus per write port: D, or with more than one D0, D1, ...
  vector<vector<dbBTerm*>> data_inputs(num_write);
  vector<vector<dbNet*>> D_nets(num_write, vector<dbNet*>(word_size));
  for (int bit = 0; bit < word_size; ++bit) {
    for (int w = 0; w < num_write; ++w) {
      const std::string d = num_write == 1 ? "D" : fmt::format("D{}", w);
      data_inputs[w].push_back(
          makeBTerm(fmt::format("{}[{}]", d, bit), dbIoType::INPUT));
      data_inputs_.push_back(data_inputs[w].back());
      D_nets[w][bit] = makeNet(num_write == 1 ? "D_nets" : d + "_nets",
                               fmt::format("b{}", bit));
    }

    // if total read ports == 1, only have Q outputs
    if (r_total == 1) {
      auto out_name = fmt::format("Q[{}]", bit);
      q_outputs_[0].push_back(makeBTerm(out_name, dbIoType::OUTPUT));
    } else {
      for (int port = 0; port < r_total; ++port) {
        auto out_name = fmt::format("Q{}[{}]", port, bit);
        q_outputs_[port].push_back(makeBTerm(out_name, dbIoType::OUTPUT));
      }
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
  if (column_mux_ratio == 1) {
    for (int port = 0; port < r_total; ++port) {
      for (int bit = 0; bit < word_size; ++bit) {
        word_q_nets[0][port][bit] = q_outputs_[port][bit]->getNet();
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
  if (column_mux_ratio == 2) {
    word_sel_nets[0] = inv_addr_nets[0][0];
    word_sel_nets[1] = addr_inputs_[0][0]->getNet();
  } else if (column_mux_ratio == 4) {
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

  // The AND-OR read path: every storage net named ahead, and every
  // bitline's tree planned, so each row's bit cells take their nodes.
  // Where the storage flop's output is QN, or the tree's root is
  // inverted, but not both, an inverter at the bottom row puts it right.
  if (and_or_read_) {
    // The rows in the trees: where the storage output is inverted the
    // final inverter assumes some row's term is the read's, so the zero
    // word stays, its term a 1; otherwise its term would be a 0, and it
    // is left out.
    vector<int> stored_rows;
    for (int row = 0; row < num_rows; ++row) {
      if (row == zero_word_) {
        if (storage_inverted_) {
          stored_rows.push_back(row);
        }
        continue;
      }
      stored_rows.push_back(row);
      for (int w = 0; w < column_mux_ratio; ++w) {
        for (int bit = 0; bit < word_size; ++bit) {
          const std::string prefix = fmt::format(
              "storage_{}_{}_{}", row, w, (bit / mask_size) * mask_size);
          storage_nets_[{row, w, bit}]
              = makeNet(fmt::format("{}.bit{}", prefix, bit), "storage");
        }
      }
    }
    const int stored = static_cast<int>(stored_rows.size());
    const bool flip = treeInverted(0, stored) != storage_inverted_;
    for (int w = 0; w < column_mux_ratio; ++w) {
      for (int port = 0; port < r_total; ++port) {
        for (int bit = 0; bit < word_size; ++bit) {
          const std::string prefix
              = fmt::format("read_w{}_p{}_b{}", w, port, bit);
          dbNet* out = word_q_nets[w][port][bit];
          const TreeEnd root = planReadTree(prefix,
                                            w,
                                            bit,
                                            stored_rows,
                                            0,
                                            stored,
                                            read_select_nets[port],
                                            flip ? nullptr : out);
          if (flip) {
            tree_nodes_[{num_rows - 1, w, bit}].push_back(
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
    if (column_mux_ratio > 1) {
      for (int i = 0; i < r_total; ++i) {
        shared_select_b_nets[i]
            = makeNet(fmt::format("row{}", row), fmt::format("select{}_b", i));
      }
    }

    for (int word_idx = 0; word_idx < column_mux_ratio; ++word_idx) {
      // the write ports' row selects, then the read ports'
      vector<dbNet*> row_selects(num_write + r_total);
      for (int p = 0; p < num_write; ++p) {
        row_selects[p] = write_select_nets[p][row];
      }
      for (int p = 0; p < r_total; ++p) {
        row_selects[num_write + p] = read_select_nets[p][row];
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
               r_ports,
               w_ports,
               word_idx,
               column_mux_ratio,
               clock->getNet(),
               word_sel_nets[word_idx],
               write_enable,
               row_selects,
               shared_select_b_nets,
               (word_idx == 0 || column_mux_ratio == 1),
               use_latch_ ? vector<vector<dbNet*>>{mid_nets} : D_nets,
               word_output_nets);
    }
  }

  // cell placement for mux made of AOI22 (and inverter if needed) for column
  // muxing
  if (column_mux_ratio > 1) {
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

  for (int slice = 0; slice < slices_per_word; ++slice) {
    for (int bit = 0; bit < mask_size; ++bit) {
      int bit_idx = bit + slice * mask_size;
      auto buffer_grid_cell = std::make_unique<Cell>();
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
      int buf_col = slice * (mask_size * column_mux_ratio + column_mux_ratio)
                    + bit * column_mux_ratio;
      ram_grid_.addCell(std::move(buffer_grid_cell), buf_col);
    }
  }

  // indices for creating buffers
  int r_buf_idx = 0;
  int w_buf_idx = 0;
  // append buffer, decoder, and inverter columns per port
  for (int p = 0; p < total_ports; ++p) {
    // buffer column
    // keeping convention of write-capable ports at index 0
    if (p < rw_ports) {
      makeBufferColumn(fmt::format("sel_buf_rw_p{}", p),
                       num_rows,
                       decoder_output_nets[p],
                       write_select_nets[w_buf_idx++]);
      ++r_buf_idx;  // so that next read port doesn't use the rw_select
    } else if (p < rw_ports + w_ports) {
      makeBufferColumn(fmt::format("sel_buf_w_p{}", p),
                       num_rows,
                       decoder_output_nets[p],
                       write_select_nets[w_buf_idx++]);
    }
    if (p >= rw_ports + w_ports) {
      makeBufferColumn(fmt::format("sel_buf_r_p{}", p),
                       num_rows,
                       decoder_output_nets[p],
                       read_select_nets[r_buf_idx++]);
    }

    // decoder column creation
    makeDecoderColumn(fmt::format("decoder_p{}", p),
                      num_rows,
                      decoder_input_nets[p],
                      decoder_output_nets[p]);
  }

  if (num_row_bits > 0) {
    // building out inverter columns
    for (int col = 0; col < inv_col_count; ++col) {
      int start_port = col * ports_per_col;
      int end_port = std::min(start_port + ports_per_col, total_ports);
      ram_grid_.addLayout(makeInverterColumn(num_rows,
                                             num_row_bits,
                                             start_port,
                                             end_port,
                                             num_word_bits,
                                             inv_addr_nets));
    }
  }

  std::unique_ptr<Cell> inv_sel_cell = std::make_unique<Cell>();
  std::vector<std::unique_ptr<Cell>> word_sel_cells(column_mux_ratio);

  if (column_mux_ratio == 2) {
    // place inv_addr[0] inverter in sel column
    makeInst(
        inv_sel_cell.get(),
        "word_sel",
        "inv_addr_0",
        inv_cell_,
        {{inv_ports_[{PortRoleType::DataIn, 0}], addr_inputs_[0][0]->getNet()},
         {inv_ports_[{PortRoleType::DataOut, 0}], inv_addr_nets[0][0]}});
  } else if (column_mux_ratio == 4) {
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
  if (column_mux_ratio == 2) {
    ram_grid_.addCell(std::move(inv_sel_cell), mask_size * column_mux_ratio);
  } else if (column_mux_ratio == 4) {
    for (int c = 0; c < 4; ++c) {
      int sel_col = mask_size * column_mux_ratio + c;
      ram_grid_.addCell(std::move(word_sel_cells[c]), sel_col);
    }
  }

  // adding filler cells to ease congestion for multi-port
  if (column_mux_ratio > 1 && total_ports > 1) {
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
  const int num_rows_grid = num_rows + 1 + (use_latch_ ? 1 : 0);

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

  writeBehavioralVerilog(behavioral_verilog_filename_,
                         slices_per_word,
                         mask_size,
                         num_words,
                         rw_ports,
                         r_ports,
                         w_ports);
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

  std::string module_name = fmt::format("RAM{}x{}", num_words, word_size_bit);

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

  // Write logic, the ports in order: where two write the same word in a
  // cycle, the last one's data is kept.
  std::string data_declarations;
  std::string enable_declarations;
  std::string write_logic;
  for (int w = 0; w < num_write; ++w) {
    const std::string write_addr
        = rw_ports > 0 ? std::string("addr_rw") : wbus("addr_w", w);
    data_declarations
        += fmt::format("  input [{}:0] {};\n", word_size_bit - 1, wbus("D", w));
    enable_declarations += fmt::format(
        "  input [{}:0] {};\n", slices_per_word - 1, wbus("we", w));
    write_logic += fmt::format(
        R"(    for (i = 0; i < {}; i = i + 1) begin
      if ({}[i]{}) begin
        mem[{}][i*{} +:{}] <= {}[i*{} +:{}];
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
        wbus("D", w),
        mask_size,
        mask_size);
  }

  std::string reset_declaration;
  std::string always_head = "  always @(posedge clk) begin\n";
  if (!reset_name.empty()) {
    reset_declaration = fmt::format("  input {};\n", reset_name);
    always_head = fmt::format(
        "  always @(posedge clk or {}edge {}) begin\n"
        "    if ({}{}) begin\n"
        "      for (i = 0; i < {}; i = i + 1) begin\n"
        "        mem[i] <= {}'d{};\n"
        "      end\n"
        "    end else begin\n",
        reset_active_low_ ? "neg" : "pos",
        reset_name,
        reset_active_low_ ? "!" : "",
        reset_name,
        num_words,
        word_size_bit,
        reset_bits_);
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
