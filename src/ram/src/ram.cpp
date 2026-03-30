// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ram/ram.h"

#include <array>
#include <cmath>
#include <fstream>
#include <functional>
#include <limits>
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
                                      vector<odb::dbNet*>& data_output)
{
  auto bit_cell = std::make_unique<Cell>();

  auto storage_net = makeNet(prefix, "storage");

  makeInst(bit_cell.get(),
           prefix,
           "bit",
           storage_cell_,
           {{storage_cell_->findMTerm("CLK") ? "CLK" : "GATE", clock},
            {"D", data_input},
            {"Q", storage_net}});

  for (int read_port = 0; read_port < read_ports; ++read_port) {
    makeInst(bit_cell.get(),
             prefix,
             fmt::format("obuf{}", read_port),
             tristate_cell_,
             {{"A", storage_net},
              {"TE_B", select[read_port]},
              {"Z", data_output[read_port]}});
  }

  return bit_cell;
}

void RamGen::makeSlice(const int slice_idx,
                       const int mask_size,
                       const int row_idx,
                       const int word_idx,
                       const int read_ports,
                       const int column_mux_ratio,
                       dbNet* clock,
                       dbNet* write_enable,
                       dbNet* word_select,
                       const vector<dbNet*>& selects,
                       const vector<dbNet*>& data_input,
                       const vector<vector<dbNet*>>& data_output)
{
  const int start_bit_idx = slice_idx * mask_size;
  std::string prefix = fmt::format("storage_{}_{}", row_idx, start_bit_idx);
  vector<dbNet*> select_b_nets(selects.size());
  for (int i = 0; i < selects.size(); ++i) {
    select_b_nets[i] = makeNet(prefix, fmt::format("select{}_b", i));
  }

  auto gclock_net = makeNet(prefix, "gclock");
  auto we0_net = makeNet(prefix, "we0");

  for (int local_bit = 0; local_bit < mask_size; ++local_bit) {
    auto name = fmt::format("{}.bit{}", prefix, start_bit_idx + local_bit);
    vector<dbNet*> outs(read_ports);
    for (int read_port = 0; read_port < read_ports; ++read_port) {
      outs[read_port] = data_output[read_port][local_bit];
    }

    int bit_col = slice_idx * (mask_size * column_mux_ratio + column_mux_ratio)
                  + local_bit * column_mux_ratio + word_idx;
    ram_grid_.addCell(makeBit(name,
                              read_ports,
                              gclock_net,
                              select_b_nets,
                              data_input[local_bit],
                              outs),
                      bit_col);
  }

  auto sel_cell = std::make_unique<Cell>();
  // Make clock gate
  makeInst(sel_cell.get(),
           prefix,
           "cg",
           clock_gate_cell_,
           {{"CLK", clock}, {"GATE", we0_net}, {"GCLK", gclock_net}});

  // Write path: this net is row_select AND with word_select so clock gate only
  // fires for the addressed word within the row. Read path handled by AOI mux.
  // word_select is nullptr when column_mux_ratio=1 (no mux needed).
  dbNet* write_sel = selects[0];
  if (word_select) {
    write_sel = makeNet(prefix, "write_sel");
    makeInst(sel_cell.get(),
             prefix,
             "word_and",
             and2_cell_,
             {{"A", selects[0]}, {"B", word_select}, {"X", write_sel}});
  }

  // Make clock and
  // this AND gate needs to be fed a net created by a decoder
  // adding any net will automatically connect with any port
  makeInst(sel_cell.get(),
           prefix,
           "gcand",
           and2_cell_,
           {{"A", write_sel}, {"B", write_enable}, {"X", we0_net}});

  // Make select inverters
  for (int i = 0; i < selects.size(); ++i) {
    makeInst(sel_cell.get(),
             prefix,
             fmt::format("select_inv_{}", i),
             inv_cell_,
             {{"A", selects[i]}, {"Y", select_b_nets[i]}});
  }
  int sel_col = slice_idx * (mask_size * column_mux_ratio + column_mux_ratio)
                + mask_size * column_mux_ratio + word_idx;
  ram_grid_.addCell(std::move(sel_cell), sel_col);
}

void RamGen::makeWord(const int slices_per_word,
                      const int mask_size,
                      const int row_idx,
                      const int word_idx,
                      const int read_ports,
                      const int column_mux_ratio,
                      dbNet* clock,
                      dbNet* word_select,
                      vector<dbBTerm*>& write_enable,
                      const vector<dbNet*>& selects,
                      const vector<dbNet*>& data_input,
                      const vector<vector<dbNet*>>& data_output)
{
  for (int slice = 0; slice < slices_per_word; ++slice) {
    int start_idx = slice * mask_size;

    vector<dbNet*> slice_inputs(data_input.begin() + start_idx,
                                data_input.begin() + start_idx + mask_size);
    std::vector<std::vector<odb::dbNet*>> slice_outputs;
    slice_outputs.reserve(read_ports);
    for (int port = 0; port < read_ports; ++port) {
      const auto& port_outputs = data_output[port];
      slice_outputs.emplace_back(port_outputs.begin() + start_idx,
                                 port_outputs.begin() + start_idx + mask_size);
    }

    makeSlice(slice,
              mask_size,
              row_idx,
              word_idx,
              read_ports,
              column_mux_ratio,
              clock,
              write_enable[slice]->getNet(),
              word_select,
              selects,
              slice_inputs,
              slice_outputs);
  }
}

// mux made out of AOI22 cells used for col/mux feature. currently support only
// col_mux_ratio = 1,2,4 NOTE: ratio =1 mean no muxing so this function is not
// called in that case called once per bit by generate(), it will mux between
// every corresponding pair of bits of column_mux_ratio words
std::unique_ptr<Cell> RamGen::makeColMux(const std::string& prefix,
                                         const int column_mux_ratio,
                                         const vector<dbNet*>& word_q_nets,
                                         const vector<dbNet*>& word_sel_nets,
                                         dbNet* q_out_net)
{
  // NOTE: word_sel_nets must be in strict alternating 'ascending' order of
  // select signal i.e. [S0', S0, S1', S1, S2', S2, etc]
  auto mux_cell = std::make_unique<Cell>();
  const int num_stages = static_cast<int>(std::log2(column_mux_ratio));
  const bool need_final_inv = (num_stages % 2 == 1);

  // Stage 1: one AOI22 per pair of word bits determined by column_mux_ratio
  int num_pairs = column_mux_ratio / 2;
  vector<dbNet*> prev_nets(num_pairs);

  for (int i = 0; i < num_pairs; ++i) {
    prev_nets[i] = dbNet::create(
        block_, fmt::format("{}_aoi_s1_g{}", prefix, i).c_str());

    // AOI boolean function:  Y = (S0'I0 + S0I1)' = AOI22(S0', I0, S0, I1)
    // AOI has inverted output so need an inverter if num_stage is odd
    makeInst(
        mux_cell.get(),
        prefix,
        fmt::format("aoi_s1_g{}", i),
        aoi22_cell_,
        {{aoi22_in_a1_, word_sel_nets[0]},        // S0'
         {aoi22_in_a2_, word_q_nets[i * 2]},      // I0 = bit from word i*2
         {aoi22_in_b1_, word_sel_nets[1]},        // S0
         {aoi22_in_b2_, word_q_nets[i * 2 + 1]},  // I1 = bit from word i*2+1
         {aoi22_out_, prev_nets[i]}});            // output to next stage
  }

  // Stages 2 to N: each stage halves the number of signals
  // sel index increases by 2 per stage (Sn' at sel_idx, Sn at sel_idx+1)
  for (int stage = 1; stage < num_stages; ++stage) {
    int num_out = static_cast<int>(prev_nets.size()) / 2;
    vector<dbNet*> curr_nets(num_out);
    int sel_idx = stage * 2;
    bool is_last_stage = (stage == num_stages - 1);

    for (int i = 0; i < num_out; ++i) {
      // if this is the last stage and no INV needed, drive Q directly
      // else create intermediate net for next stage or for final inverter
      if (is_last_stage && !need_final_inv) {
        curr_nets[i] = q_out_net;
      } else {
        curr_nets[i] = dbNet::create(
            block_,
            fmt::format("{}_aoi_s{}_g{}", prefix, stage + 1, i).c_str());
      }

      makeInst(mux_cell.get(),
               prefix,
               fmt::format("aoi_s{}_g{}", stage + 1, i),
               aoi22_cell_,
               {{aoi22_in_a1_, word_sel_nets[sel_idx]},
                {aoi22_in_a2_, prev_nets[i * 2]},
                {aoi22_in_b1_, word_sel_nets[sel_idx + 1]},
                {aoi22_in_b2_, prev_nets[i * 2 + 1]},
                {aoi22_out_, curr_nets[i]}});
    }
    prev_nets = curr_nets;  // output net of current stage becomes input net of
                            // next stage
  }

  // Final stage/output:
  // If "odd" stages (ratio = 2,8 meaning num_stages = 1,3): output inverted,
  // add inverter If "even" stages (ratio = 4 meaning num_stages = 2): Q driven
  // directly by last AOI22 output, no inverter needed
  if (need_final_inv) {
    makeInst(mux_cell.get(),
             prefix,
             "mux_inv",
             inv_cell_,
             {{"A", prev_nets[0]}, {"Y", q_out_net}});
  }

  return mux_cell;
}

std::unique_ptr<Layout> RamGen::generateTapColumn(const int num_words,
                                                  const int tapcell_col)
{
  auto tapcell_layout = std::make_unique<Layout>(odb::vertical);
  for (int i = 0; i <= num_words; ++i) {
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

std::unique_ptr<Cell> RamGen::makeDecoder(
    const std::string& prefix,
    const int num_word,
    const int read_ports,
    const std::vector<odb::dbNet*>& selects,
    const std::vector<odb::dbNet*>& addr_nets)
{
  auto word_cell = std::make_unique<Cell>();

  // can make this an AND gate layer method
  // places appropriate number of AND gates for each word

  // calculates number of and gate layers needed
  int layers = std::log2(num_word) - 1;

  dbNet* prev_net = nullptr;  // net to store previous and gate output
  dbNet* decoder_out_net = makeNet(prefix, "decoder_out");

  for (int i = 0; i < layers; ++i) {
    auto input_net = makeNet(prefix, fmt::format("layer_in{}", i));
    // sets up first AND gate, closest to slice's select + write enable gate
    if (i == 0 && i == layers - 1) {
      makeInst(word_cell.get(),
               prefix,
               fmt::format("and_layer{}", i),
               and2_cell_,
               {{"A", addr_nets[i]},
                {"B", addr_nets[i + 1]},
                {"X", decoder_out_net}});
      prev_net = input_net;
    } else if (i == 0) {
      makeInst(word_cell.get(),
               prefix,
               fmt::format("and_layer{}", i),
               and2_cell_,
               {{"A", addr_nets[i]}, {"B", input_net}, {"X", decoder_out_net}});
      prev_net = input_net;
    } else if (i == layers - 1) {  // last AND gate layer
      makeInst(word_cell.get(),
               prefix,
               fmt::format("and_layer{}", i),
               and2_cell_,
               {{"A", addr_nets[i]}, {"B", addr_nets[i + 1]}, {"X", prev_net}});
      prev_net = input_net;
    } else {  // middle AND gate layers
      makeInst(word_cell.get(),
               prefix,
               fmt::format("and_layer{}", i),
               and2_cell_,
               {{"A", addr_nets[i]}, {"B", input_net}, {"X", prev_net}});
      prev_net = input_net;
    }
  }

  for (int port = 0; port < read_ports; ++port) {
    makeInst(word_cell.get(),
             prefix,
             fmt::format("buf_port{}", port),
             buffer_cell_,
             {{"A", decoder_out_net}, {"X", selects[port]}});
  }

  return word_cell;
}

std::vector<dbNet*> RamGen::selectNets(const std::string& prefix,
                                       const int read_ports)
{
  std::vector<dbNet*> select_nets(read_ports);
  for (int i = 0; i < read_ports; ++i) {
    select_nets[i] = makeNet(prefix, fmt::format("decoder{}", i));
  }
  return select_nets;
}

dbMaster* RamGen::findMaster(
    const std::function<bool(sta::LibertyPort*)>& match,
    const char* name)
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

      auto port_iter = liberty->portIterator();

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

      delete port_iter;
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
    logger_->error(RAM, 10, "Can't find {} cell", name);
  }
  logger_->info(RAM, 16, "Selected {} cell {}", name, best->getName());
  return best;
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

  if (!tristate_cell_) {
    tristate_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          if (!port->direction()->isTristate()) {
            return false;
          }
          auto function = port->function();
          return function->op() != sta::FuncExpr::Op::not_;
        },
        "tristate");
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

  if (!storage_cell_) {
    // FIXME
    storage_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          if (!port->direction()->isOutput()) {
            return false;
          }
          auto function = port->function();
          return function && function->op() == sta::FuncExpr::Op::and_
                 && function->left()->op() == sta::FuncExpr::Op::port
                 && function->right()->op() == sta::FuncExpr::Op::port;
        },
        "storage");
  }

  if (!clock_gate_cell_) {
    clock_gate_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          return port->libertyCell()->isClockGate();
        },
        "clock gate");
  }
  // for input buffers
  if (!buffer_cell_) {
    buffer_cell_ = findMaster(
        [](sta::LibertyPort* port) { return port->libertyCell()->isBuffer(); },
        "buffer");
  }

  // aoi cells used for column mux functionality when column_mux_ratio > 1
  if (!aoi22_cell_) {
    auto isNotPort = [](const sta::FuncExpr* e) {
      return e && e->op() == sta::FuncExpr::Op::not_ && e->left()
             && e->left()->op() == sta::FuncExpr::Op::port;
    };
    auto isAndOfNots = [&isNotPort](const sta::FuncExpr* e) {
      return e && e->op() == sta::FuncExpr::Op::and_ && isNotPort(e->left())
             && isNotPort(e->right());
    };
    aoi22_cell_ = findMaster(
        [&isNotPort, &isAndOfNots](sta::LibertyPort* port) {
          if (!port->direction()->isOutput()) {
            return false;
          }
          auto f = port->function();
          if (!f) {
            return false;
          }
          if (f->op() == sta::FuncExpr::Op::not_) {
            auto inner = f->left();
            if (!inner || inner->op() != sta::FuncExpr::Op::or_) {
              return false;
            }
            auto L = inner->left();
            auto R = inner->right();
            return L && L->op() == sta::FuncExpr::Op::and_ && L->left()
                   && L->left()->op() == sta::FuncExpr::Op::port && L->right()
                   && L->right()->op() == sta::FuncExpr::Op::port && R
                   && R->op() == sta::FuncExpr::Op::and_ && R->left()
                   && R->left()->op() == sta::FuncExpr::Op::port && R->right()
                   && R->right()->op() == sta::FuncExpr::Op::port;
          }
          if (f->op() == sta::FuncExpr::Op::or_) {
            auto l1 = f->left();
            auto t4 = f->right();
            if (!isAndOfNots(t4) || !l1 || l1->op() != sta::FuncExpr::Op::or_) {
              return false;
            }
            auto l2 = l1->left();
            auto t3 = l1->right();
            if (!isAndOfNots(t3) || !l2 || l2->op() != sta::FuncExpr::Op::or_) {
              return false;
            }
            return isAndOfNots(l2->left()) && isAndOfNots(l2->right());
          }
          return false;
        },
        "aoi22");

    // extract port names from the liberty function tree
    auto cell = network_->libertyCell(network_->dbToSta(aoi22_cell_));
    auto port_iter = cell->portIterator();
    while (port_iter->hasNext()) {
      auto p = static_cast<sta::ConcretePort*>(port_iter->next());
      if (p->direction()->isAnyOutput()) {
        auto f = p->libertyPort()->function();
        if (f->op() == sta::FuncExpr::Op::not_) {
          // Compact form: not_(or_(and_(A1,A2), and_(B1,B2)))
          auto or_expr = f->left();
          auto and_a = or_expr->left();
          auto and_b = or_expr->right();
          aoi22_in_a1_ = and_a->left()->port()->name();
          aoi22_in_a2_ = and_a->right()->port()->name();
          aoi22_in_b1_ = and_b->left()->port()->name();
          aoi22_in_b2_ = and_b->right()->port()->name();
        } else {
          // SOP expanded form used by sky130hd:
          // or_(or_(or_(and_(not_(A1),not_(B1)), and_(not_(A1),not_(B2))),
          //     and_(not_(A2),not_(B1))),
          //     and_(not_(A2),not_(B2)))
          // Port names are inside not_() wrappers, requiring an extra
          // ->left() to reach the actual port compared to the compact form.
          // e.g. and_(not_(A1), not_(B1))->left()->left()->port() gives A1
          // NOTE: this branch is PDK-specific to sky130hd's liberty
          // representation. If adding support for other PDKs, verify their
          // AOI22 function expression form and extend this extraction
          // accordingly.
          auto t1 = f->left()->left()->left();
          auto t2 = f->left()->left()->right();
          auto t3 = f->left()->right();
          aoi22_in_a1_ = t1->left()->left()->port()->name();
          aoi22_in_a2_ = t3->left()->left()->port()->name();
          aoi22_in_b1_ = t1->right()->left()->port()->name();
          aoi22_in_b2_ = t2->right()->left()->port()->name();
        }
        aoi22_out_ = p->name();
        break;
      }
    }
    delete port_iter;
  }
}

void RamGen::ramPdngen(const char* power_pin,
                       const char* ground_pin,
                       const char* route_name,
                       int route_width,
                       const char* ver_name,
                       int ver_width,
                       int ver_pitch,
                       const char* hor_name,
                       int hor_width,
                       int hor_pitch)
{
  // need parameters for power and ground nets
  auto power_net = dbNet::create(block_, "VDD");
  auto ground_net = dbNet::create(block_, "VSS");

  power_net->setSpecial();
  power_net->setSigType(odb::dbSigType::POWER);
  ground_net->setSpecial();
  ground_net->setSigType(odb::dbSigType::GROUND);

  // find a way to get the power and ground net names associated with cells used
  block_->addGlobalConnect(nullptr, ".*", power_pin, power_net, true);
  block_->addGlobalConnect(nullptr, ".*", ground_pin, ground_net, true);

  block_->globalConnect(false, false);

  std::string grid_name = "ram_grid";
  pdngen_->setCoreDomain(power_net, nullptr, ground_net, {});
  pdngen_->makeCoreGrid(pdngen_->findDomain("Core"),
                        grid_name,
                        pdn::StartsWith::GROUND,
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
                         pdn::ExtensionMode::BOUNDARY);

  // add_pdn_stripe
  pdngen_->makeStrap(grid,
                     pdn_tech->findLayer(ver_name),
                     ver_width,
                     0,
                     ver_pitch,
                     0,
                     0,
                     false,
                     pdn::StartsWith::GRID,
                     pdn::ExtensionMode::BOUNDARY,
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
                     pdn::StartsWith::GRID,
                     pdn::ExtensionMode::BOUNDARY,
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
  odb::Rect top_constraint = block_->findConstraintRegion(
      odb::Direction2D::North, die_bounds.xMin(), die_bounds.xMax());
  block_->addBTermConstraintByDirection(dbIoType::OUTPUT, top_constraint);

  block_->addBTermsToConstraint(data_inputs_, top_constraint);
  auto pin_tech = block_->getDb()->getTech();
  io_placer_->addHorLayer(pin_tech->findLayer(hor_name));
  io_placer_->addVerLayer(pin_tech->findLayer(ver_name));
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
                      const int column_mux_ratio,
                      const int read_ports,
                      dbMaster* storage_cell,
                      dbMaster* tristate_cell,
                      dbMaster* inv_cell,
                      dbMaster* tapcell,
                      int max_tap_dist)
{
  const int slices_per_word = word_size / mask_size;
  const std::string ram_name = fmt::format("RAM{}x{}", num_words, word_size);

  // error checking for read ports != 1 for current version of RamGen, edit
  // later for future changes
  if (read_ports != 1) {
    logger_->error(
        RAM, 25, "The ram generator currently only supports 1 read port.");
    return;
  }

  // error checking for column_mux_ratio
  if (column_mux_ratio != 1 && column_mux_ratio != 2 && column_mux_ratio != 4) {
    logger_->error(RAM,
                   33,
                   "The ram generator currently only supports column_mux_ratio "
                   "values of 1, 2, or 4.");
  }

  if (num_words % column_mux_ratio != 0) {
    logger_->error(RAM,
                   34,
                   "num_words ({}) must be divisible by column_mux_ratio ({}).",
                   num_words,
                   column_mux_ratio);
  }

  int num_inputs = std::ceil(std::log2(num_words));
  // compute information to support col/mux ratio feature
  int num_rows = num_words / column_mux_ratio;
  // if column mux ratio > 1, then the lower log2(column_mux_ratio) bits are
  // used to select the word within a row
  int num_word_bits = (column_mux_ratio > 1)
                          ? static_cast<int>(std::log2(column_mux_ratio))
                          : 0;
  // the remaining upper bits are used to select the row
  int num_row_bits = num_inputs - num_word_bits;

  logger_->info(RAM, 3, "Generating {}", ram_name);

  storage_cell_ = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_ = inv_cell;
  tapcell_ = tapcell;
  and2_cell_ = nullptr;
  clock_gate_cell_ = nullptr;
  buffer_cell_ = nullptr;
  aoi22_cell_ = nullptr;
  findMasters();

  auto chip = db_->getChip();
  if (!chip) {
    chip = odb::dbChip::create(
        db_, db_->getTech(), ram_name, odb::dbChip::ChipType::DIE);
  }

  block_ = chip->getBlock();
  if (!block_) {
    block_ = odb::dbBlock::create(chip, ram_name.c_str());
  }

  // With column_mux_ratio = 1: One column per bit plus one select/control
  // column per slice, plus one extra decoder column With column_mux_ratio > 1:
  // Each slice has (mask_size * column_mux_ratio) columns (one per bit per word
  // in the row), plus one select/control column per slice (one per word), plus
  // one extra decoder column at far right shared across all row
  int col_cell_count
      = slices_per_word * (mask_size * column_mux_ratio + column_mux_ratio);
  ram_grid_.setNumLayouts(col_cell_count + 1);

  auto clock = makeBTerm("clk", dbIoType::INPUT);

  vector<dbBTerm*> write_enable(slices_per_word, nullptr);
  for (int slice = 0; slice < slices_per_word; ++slice) {
    auto in_name = fmt::format("we[{}]", slice);
    write_enable[slice] = makeBTerm(in_name, dbIoType::INPUT);
  }

  // input bterms
  for (int i = 0; i < num_inputs; ++i) {
    addr_inputs_.push_back(
        makeBTerm(fmt::format("addr[{}]", i), dbIoType::INPUT));
  }

  // vector of nets storing inverter nets
  vector<dbNet*> inv_addr(num_inputs);
  for (int i = 0; i < num_inputs; ++i) {
    inv_addr[i] = makeNet("inv", fmt::format("addr[{}]", i));
  }

  // word select nets: one per word_idx (word within a row) shared between all
  // rows, derived from lower num_word_bits of address inputs used to determine
  // word selection within a row for column muxing. word_sel_nets[word_idx] =
  // HIGH when this word_idx is addressed. word_idx starts at 0 for the leftmost
  // bit of the row, i.e. word 3 on row 1 with column_mux_ratio =2 will have
  // word_idx = 1 nullptr for all entries when column_mux_ratio = 1 (no column
  // muxing)
  vector<dbNet*> word_sel_nets(column_mux_ratio, nullptr);

  if (column_mux_ratio == 2) {
    word_sel_nets[0] = inv_addr[0];
    word_sel_nets[1] = addr_inputs_[0]->getNet();
  } else if (column_mux_ratio == 4) {
    auto word_sel_cell = std::make_unique<Cell>();
    for (int c = 0; c < 4; ++c) {
      word_sel_nets[c] = makeNet("word_sel", fmt::format("{}", c));
    }
    makeInst(word_sel_cell.get(),
             "word_sel",
             "and_0",
             and2_cell_,
             {{"A", inv_addr[1]}, {"B", inv_addr[0]}, {"X", word_sel_nets[0]}});
    makeInst(word_sel_cell.get(),
             "word_sel",
             "and_1",
             and2_cell_,
             {{"A", inv_addr[1]},
              {"B", addr_inputs_[0]->getNet()},
              {"X", word_sel_nets[1]}});
    makeInst(word_sel_cell.get(),
             "word_sel",
             "and_2",
             and2_cell_,
             {{"A", addr_inputs_[1]->getNet()},
              {"B", inv_addr[0]},
              {"X", word_sel_nets[2]}});
    makeInst(word_sel_cell.get(),
             "word_sel",
             "and_3",
             and2_cell_,
             {{"A", addr_inputs_[1]->getNet()},
              {"B", addr_inputs_[0]->getNet()},
              {"X", word_sel_nets[3]}});
    ram_grid_.addCell(std::move(word_sel_cell), col_cell_count);
  }

  // decoder_layer nets
  // for column muxing, deocder input nets uses only the upper address bits
  // (num_row_bits) to determine row
  vector<vector<dbNet*>> decoder_input_nets(num_rows,
                                            vector<dbNet*>(num_row_bits));
  for (int row = 0; row < num_rows; ++row) {
    int row_num = row;
    // start at right most bit
    for (int input = 0; input < num_row_bits; ++input) {
      if (row_num % 2 == 0) {
        // places inverted address for each input
        decoder_input_nets[row][input] = inv_addr[num_word_bits + input];
      } else {  // puts original input in invert nets
        decoder_input_nets[row][input]
            = addr_inputs_[num_word_bits + input]->getNet();
      }
      row_num /= 2;
    }
  }

  // word decoder signals to have one deccoder per word, shared between all
  // slices of a word
  vector<vector<dbNet*>> word_decoder_nets(num_words);

  for (int row = 0; row < num_rows; ++row) {
    auto decoder_name = fmt::format("decoder_{}", row);

    if (num_rows == 2) {
      dbNet* addr_net = (row == 0 ? inv_addr[num_word_bits]
                                  : addr_inputs_[num_word_bits]->getNet());
      for (int i = 0; i < read_ports; ++i) {
        word_decoder_nets[row].push_back(addr_net);
      }
    } else {
      word_decoder_nets[row] = selectNets(decoder_name, read_ports);

      auto decoder_and_cell = makeDecoder(decoder_name,
                                          num_rows,
                                          read_ports,
                                          word_decoder_nets[row],
                                          decoder_input_nets[row]);

      ram_grid_.addCell(std::move(decoder_and_cell), col_cell_count);
    }
  }

  // start of input/output net creation
  q_outputs_.resize(read_ports);
  vector<dbNet*> D_nets(word_size);
  for (int bit = 0; bit < word_size; ++bit) {
    data_inputs_.push_back(
        makeBTerm(fmt::format("D[{}]", bit), dbIoType::INPUT));
    D_nets[bit] = makeNet(fmt::format("D_nets[{}]", bit), "net");

    // if readports == 1, only have Q outputs
    if (read_ports == 1) {
      auto out_name = fmt::format("Q[{}]", bit);
      q_outputs_[0].push_back(makeBTerm(out_name, dbIoType::OUTPUT));
    } else {
      for (int port = 0; port < read_ports; ++port) {
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
  vector<vector<dbNet*>> word_q_nets(column_mux_ratio,
                                     vector<dbNet*>(word_size));
  if (column_mux_ratio == 1) {
    for (int bit = 0; bit < word_size; ++bit) {
      word_q_nets[0][bit] = q_outputs_[0][bit]->getNet();
    }
  } else {
    for (int w = 0; w < column_mux_ratio; ++w) {
      for (int bit = 0; bit < word_size; ++bit) {
        word_q_nets[w][bit]
            = makeNet(fmt::format("word{}_q", w), fmt::format("[{}]", bit));
      }
    }
  }

  for (int row = 0; row < num_rows; ++row) {
    for (int word_idx = 0; word_idx < column_mux_ratio; ++word_idx) {
      vector<vector<dbNet*>> word_output_nets(read_ports);
      for (int port = 0; port < read_ports; ++port) {
        for (int bit = 0; bit < word_size; ++bit) {
          word_output_nets[port].push_back(word_q_nets[word_idx][bit]);
        }
      }

      makeWord(slices_per_word,
               mask_size,
               row,
               word_idx,
               read_ports,
               column_mux_ratio,
               clock->getNet(),
               word_sel_nets[word_idx],
               write_enable,
               word_decoder_nets[row],
               D_nets,
               word_output_nets);
    }
  }

  if (column_mux_ratio > 1) {
    // build select nets once to be used for every bit and every slice
    // NOTE: must be in exact alternating format: [S0', S0, S1', S1, ...]
    vector<dbNet*> mux_word_sel_nets;
    for (int bit_idx = 0; bit_idx < num_word_bits; ++bit_idx) {
      mux_word_sel_nets.push_back(inv_addr[bit_idx]);
      mux_word_sel_nets.push_back(addr_inputs_[bit_idx]->getNet());
    }

    for (int slice = 0; slice < slices_per_word; ++slice) {
      for (int bit = 0; bit < mask_size; ++bit) {
        const int global_bit = slice * mask_size + bit;
        vector<dbNet*> bit_word_q_nets(column_mux_ratio);
        for (int word_idx = 0; word_idx < column_mux_ratio; ++word_idx) {
          bit_word_q_nets[word_idx] = word_q_nets[word_idx][global_bit];
        }
        int mux_col = slice * (mask_size * column_mux_ratio + column_mux_ratio)
                      + bit * column_mux_ratio;
        ram_grid_.addCell(
            makeColMux(fmt::format("mux_slice{}_bit{}", slice, bit),
                       column_mux_ratio,
                       bit_word_q_nets,
                       mux_word_sel_nets,
                       q_outputs_[0][global_bit]->getNet()),
            mux_col);
      }
    }
  }

  for (int slice = 0; slice < slices_per_word; ++slice) {
    for (int bit = 0; bit < mask_size; ++bit) {
      int bit_idx = bit + slice * mask_size;
      auto buffer_grid_cell = std::make_unique<Cell>();
      makeInst(
          buffer_grid_cell.get(),
          "buffer",
          fmt::format("in[{}]", bit_idx),
          buffer_cell_,
          {{"A", data_inputs_[bit_idx]->getNet()}, {"X", D_nets[bit_idx]}});
      ram_grid_.addCell(std::move(buffer_grid_cell), bit_idx + slice);
    }
  }

  auto cell_inv_layout = std::make_unique<Layout>(odb::vertical);
  // check for AND gate, specific case for 2 words
  if (num_inputs > 1) {
    for (int i = num_inputs - 1; i >= 0; --i) {
      auto inv_grid_cell = std::make_unique<Cell>();
      makeInst(inv_grid_cell.get(),
               "decoder",
               fmt::format("inv_{}", i),
               inv_cell_,
               {{"A", addr_inputs_[i]->getNet()}, {"Y", inv_addr[i]}});
      cell_inv_layout->addCell(std::move(inv_grid_cell));
      for (int filler_count = 0; filler_count < num_inputs - 1;
           ++filler_count) {
        cell_inv_layout->addCell(nullptr);
      }
    }
  } else {
    auto inv_grid_cell = std::make_unique<Cell>();
    makeInst(inv_grid_cell.get(),
             "decoder",
             fmt::format("inv_{}", 0),
             inv_cell_,
             {{"A", addr_inputs_[0]->getNet()}, {"Y", inv_addr[0]}});
    cell_inv_layout->addCell(std::move(inv_grid_cell));
  }

  ram_grid_.addLayout(std::move(cell_inv_layout));

  auto ram_origin(odb::Point(0, 0));

  ram_grid_.setOrigin(ram_origin);
  ram_grid_.gridInit();

  if (tapcell_) {
    // max tap distance specified is greater than the length of ram
    if (ram_grid_.getRowWidth() <= max_tap_dist) {
      auto tapcell_layout = generateTapColumn(num_rows, 0);
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
          auto tapcell_layout = generateTapColumn(num_rows, tapcell_count);
          ram_grid_.insertLayout(std::move(tapcell_layout), col);
          ++col;  // col adjustment after insertion
          nearest_tap = 0;
          ++tapcell_count;
        }
        nearest_tap += ram_grid_.getLayoutWidth(col);
      }
      // check for last column in the grid
      if (nearest_tap >= max_tap_dist) {
        auto tapcell_layout = generateTapColumn(num_rows, tapcell_count);
        ram_grid_.addLayout(std::move(tapcell_layout));
      }
    }
  }

  ram_grid_.gridInit();

  auto db_libs = db_->getLibs().begin();
  auto db_sites = *(db_libs->getSites().begin());
  auto sites_width = db_sites->getWidth();

  int num_sites = ram_grid_.getRowWidth() / db_sites->getWidth();

  // One extra row at the top for placing input buffers
  const int num_rows_grid = num_rows + 1;
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
                         read_ports);
}

void RamGen::setBehavioralVerilogFilename(const std::string& filename)
{
  behavioral_verilog_filename_ = filename;
}

void RamGen::writeBehavioralVerilog(const std::string& filename,
                                    const int slices_per_word,
                                    const int mask_size,
                                    const int num_words,
                                    const int read_ports)
{
  if (filename.empty()) {
    return;
  }

  const int word_size_bit = slices_per_word * mask_size;
  const int address_width
      = (num_words <= 1) ? 1 : std::ceil(std::log2(num_words));

  std::string module_name = fmt::format("RAM{}x{}", num_words, word_size_bit);

  // Build port list
  std::string port_list = "\n  clk,\n  D";
  for (int i = 0; i < read_ports; i++) {
    if (read_ports == 1) {
      port_list += ",\n  Q";
    } else {
      port_list += fmt::format(",\n  Q{}", i);
    }
  }
  port_list += ",\n  addr_rw";
  for (int i = 1; i < read_ports; i++) {
    port_list += fmt::format(",\n  addr_r{}", i);
  }
  port_list += ",\n  we";

  // Build output declarations
  std::string output_declaration;
  for (int i = 0; i < read_ports; i++) {
    if (read_ports == 1) {
      output_declaration
          += fmt::format("  output reg [{}:0] Q;\n", word_size_bit - 1);
    } else {
      output_declaration
          += fmt::format("  output reg [{}:0] Q{};\n", word_size_bit - 1, i);
    }
  }

  // Build address declarations
  std::string addr_declarations;
  addr_declarations
      += fmt::format("  input [{}:0] addr_rw;\n", address_width - 1);
  for (int i = 1; i < read_ports; i++) {
    addr_declarations
        += fmt::format("  input [{}:0] addr_r{};\n", address_width - 1, i);
  }

  // Build read port logic
  std::string read_port_logic;
  for (int i = 0; i < read_ports; i++) {
    std::string port_name = (read_ports == 1) ? "Q" : fmt::format("Q{}", i);
    std::string addr_name = (i == 0) ? "addr_rw" : fmt::format("addr_r{}", i);
    if (i > 0) {
      read_port_logic += "\n";
    }
    read_port_logic += fmt::format(R"(  always @(*) begin
    {} = mem[{}];
  end
)",
                                   port_name,
                                   addr_name);
  }

  std::string verilog_code = fmt::format(R"(module {} ({}
);
  input clk;
  input [{}:0] D;
{}{}  input [{}:0] we;

  // memory array declaration
  reg [{}:0] mem[0:{}];

  // write logic
  integer i;
  always @(posedge clk) begin
    for (i = 0; i < {}; i = i + 1) begin
      if (we[i]) begin
        mem[addr_rw][i*{} +:{}] <= D[i*{} +:{}];
      end
    end
  end

  // read logic
{}
endmodule
)",
                                         module_name,
                                         port_list,
                                         word_size_bit - 1,
                                         output_declaration,
                                         addr_declarations,
                                         slices_per_word - 1,
                                         word_size_bit - 1,
                                         num_words - 1,
                                         slices_per_word,
                                         mask_size,
                                         mask_size,
                                         mask_size,
                                         mask_size,
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
