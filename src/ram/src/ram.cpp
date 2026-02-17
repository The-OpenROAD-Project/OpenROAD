// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ram/ram.h"

#include <array>
#include <cmath>
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
#include "layout.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/isotropy.h"
#include "ord/OpenRoad.hh"
#include "pdn/PdnGen.hh"
#include "ppl/IOPlacer.h"
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
               ppl::IOPlacer* ioPlacer,
               dpl::Opendp* opendp,
               grt::GlobalRouter* global_router,
               drt::TritonRoute* detailed_router)

    : network_(network),
      db_(db),
      logger_(logger),
      pdngen_(pdngen),
      ioPlacer_(ioPlacer),
      opendp_(opendp),
      global_router_(global_router),
      detailed_router_(detailed_router)
{
}

dbInst* RamGen::makeCellInst(
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

std::unique_ptr<Cell> RamGen::makeCellBit(const std::string& prefix,
                                          const int read_ports,
                                          dbNet* clock,
                                          vector<odb::dbNet*>& select,
                                          dbNet* data_input,
                                          vector<odb::dbNet*>& data_output)
{
  auto bit_cell = std::make_unique<Cell>();

  auto storage_net = makeNet(prefix, "storage");

  makeCellInst(bit_cell.get(),
               prefix,
               "bit",
               storage_cell_,
               {{"GATE", clock}, {"D", data_input}, {"Q", storage_net}});

  for (int read_port = 0; read_port < read_ports; ++read_port) {
    makeCellInst(bit_cell.get(),
                 prefix,
                 fmt::format("obuf{}", read_port),
                 tristate_cell_,
                 {{"A", storage_net},
                  {"TE_B", select[read_port]},
                  {"Z", data_output[read_port]}});
  }

  return bit_cell;
}

void RamGen::makeCellByte(Grid& ram_grid,
                          const int byte_idx,
                          const std::string& prefix,
                          const int read_ports,
                          dbNet* clock,
                          dbNet* write_enable,
                          const vector<dbNet*>& selects,
                          const array<dbNet*, 8>& data_input,
                          const vector<array<dbBTerm*, 8>>& data_output)
{
  vector<dbNet*> select_b_nets(selects.size());
  for (int i = 0; i < selects.size(); ++i) {
    select_b_nets[i] = makeNet(prefix, fmt::format("select{}_b", i));
  }

  auto gclock_net = makeNet(prefix, "gclock");
  auto we0_net = makeNet(prefix, "we0");

  // For naming bits: 0, 8, 16,...
  const int logical_bit_base = byte_idx * 8;

  // For placement taking into acount select bit of each byte: 0, 9, 18, 27...
  const int physical_col_base = byte_idx * 9;

  for (int local_bit = 0; local_bit < 8; ++local_bit) {
    // For naming
    const int global_logical_bit_idx = logical_bit_base + local_bit;

    // For placement
    const int physical_col_idx = physical_col_base + local_bit;

    auto name = fmt::format("{}.bit{}", prefix, global_logical_bit_idx);
    vector<dbNet*> outs;
    outs.reserve(read_ports);
    for (int read_port = 0; read_port < read_ports; ++read_port) {
      outs.push_back(data_output[read_port][local_bit]->getNet());
    }
    ram_grid.addCell(makeCellBit(name,
                                 read_ports,
                                 gclock_net,
                                 select_b_nets,
                                 data_input[local_bit],
                                 outs),
                     physical_col_idx);
  }

  auto sel_cell = std::make_unique<Cell>();
  // Make clock gate
  makeCellInst(sel_cell.get(),
               prefix,
               "cg",
               clock_gate_cell_,
               {{"CLK", clock}, {"GATE", we0_net}, {"GCLK", gclock_net}});

  // Make clock and
  // this AND gate needs to be fed a net created by a decoder
  // adding any net will automatically connect with any port
  makeCellInst(sel_cell.get(),
               prefix,
               "gcand",
               and2_cell_,
               {{"A", selects[0]}, {"B", write_enable}, {"X", we0_net}});

  // Make select inverters
  for (int i = 0; i < selects.size(); ++i) {
    makeCellInst(sel_cell.get(),
                 prefix,
                 fmt::format("select_inv_{}", i),
                 inv_cell_,
                 {{"A", selects[i]}, {"Y", select_b_nets[i]}});
  }

  ram_grid.addCell(std::move(sel_cell), (byte_idx * 9) + 8);
}

std::unique_ptr<Layout> RamGen::generateTapColumn(const int word_count,
                                                  const int tapcell_col)
{
  auto tapcell_layout = std::make_unique<Layout>(odb::vertical);
  for (int i = 0; i <= word_count; ++i) {
    auto tapcell_cell = std::make_unique<Cell>();
    makeCellInst(tapcell_cell.get(),
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
    // sets up first AND gate, closest to byte's select + write enable gate
    if (i == 0 && i == layers - 1) {
      makeCellInst(word_cell.get(),
                   prefix,
                   fmt::format("and_layer{}", i),
                   and2_cell_,
                   {{"A", addr_nets[i]},
                    {"B", addr_nets[i + 1]},
                    {"X", decoder_out_net}});
      prev_net = input_net;
    } else if (i == 0) {
      makeCellInst(
          word_cell.get(),
          prefix,
          fmt::format("and_layer{}", i),
          and2_cell_,
          {{"A", addr_nets[i]}, {"B", input_net}, {"X", decoder_out_net}});
      prev_net = input_net;
    } else if (i == layers - 1) {  // last AND gate layer
      makeCellInst(
          word_cell.get(),
          prefix,
          fmt::format("and_layer{}", i),
          and2_cell_,
          {{"A", addr_nets[i]}, {"B", addr_nets[i + 1]}, {"X", prev_net}});
      prev_net = input_net;
    } else {  // middle AND gate layers
      makeCellInst(word_cell.get(),
                   prefix,
                   fmt::format("and_layer{}", i),
                   and2_cell_,
                   {{"A", addr_nets[i]}, {"B", input_net}, {"X", prev_net}});
      prev_net = input_net;
    }
  }

  for (int port = 0; port < read_ports; ++port) {
    makeCellInst(word_cell.get(),
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
          return function->op() != sta::FuncExpr::op_not;
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
          return function && function->op() == sta::FuncExpr::op_and
                 && function->left()->op() == sta::FuncExpr::op_port
                 && function->right()->op() == sta::FuncExpr::op_port;
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
          return function && function->op() == sta::FuncExpr::op_and
                 && function->left()->op() == sta::FuncExpr::op_port
                 && function->right()->op() == sta::FuncExpr::op_port;
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
  ioPlacer_->addHorLayer(pin_tech->findLayer(hor_name));
  ioPlacer_->addVerLayer(pin_tech->findLayer(ver_name));
  ioPlacer_->runHungarianMatching();
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

void RamGen::generate(const int bytes_per_word,
                      const int word_count,
                      const int read_ports,
                      dbMaster* storage_cell,
                      dbMaster* tristate_cell,
                      dbMaster* inv_cell,
                      dbMaster* tapcell,
                      int max_tap_dist)
{
  const int bits_per_word = bytes_per_word * 8;
  const std::string ram_name
      = fmt::format("RAM{}x{}", word_count, bits_per_word);

  logger_->info(RAM, 3, "Generating {}", ram_name);

  storage_cell_ = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_ = inv_cell;
  tapcell_ = tapcell;
  and2_cell_ = nullptr;
  clock_gate_cell_ = nullptr;
  buffer_cell_ = nullptr;
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

  // 9 columns for 8 bits per word plus
  // cell for WE AND gate/inverter
  // extra column is for decoder cells
  int col_cell_count = bytes_per_word * 9;
  Grid ram_grid(odb::horizontal, col_cell_count + 1);

  auto clock = makeBTerm("clk", dbIoType::INPUT);

  vector<dbBTerm*> write_enable(bytes_per_word, nullptr);
  for (int byte = 0; byte < bytes_per_word; ++byte) {
    auto in_name = fmt::format("we[{}]", byte);
    write_enable[byte] = makeBTerm(in_name, dbIoType::INPUT);
  }

  // input bterms
  int num_inputs = std::ceil(std::log2(word_count));
  for (int i = 0; i < num_inputs; ++i) {
    addr_inputs_.push_back(
        makeBTerm(fmt::format("addr[{}]", i), dbIoType::INPUT));
  }

  // vector of nets storing inverter nets
  vector<dbNet*> inv_addr(num_inputs);
  for (int i = 0; i < num_inputs; ++i) {
    inv_addr[i] = makeNet("inv", fmt::format("addr[{}]", i));
  }

  // decoder_layer nets
  vector<vector<dbNet*>> decoder_input_nets(word_count,
                                            vector<dbNet*>(num_inputs));
  for (int word = 0; word < word_count; ++word) {
    int word_num = word;
    // start at right most bit
    for (int input = 0; input < num_inputs; ++input) {
      if (word_num % 2 == 0) {
        // places inverted address for each input
        decoder_input_nets[word][input] = inv_addr[input];
      } else {  // puts original input in invert nets
        decoder_input_nets[word][input] = addr_inputs_[input]->getNet();
      }
      word_num /= 2;
    }
  }

  // word decoder signals to have one deccoder per word, shared between all
  // bytes of a word
  vector<vector<dbNet*>> word_decoder_nets(word_count);

  for (int row = 0; row < word_count; ++row) {
    auto decoder_name = fmt::format("decoder_{}", row);

    if (word_count == 2) {
      dbNet* addr_net = (row == 0 ? inv_addr[0] : addr_inputs_[0]->getNet());
      for (int i = 0; i < read_ports; ++i) {
        word_decoder_nets[row].push_back(addr_net);
      }
    } else {
      word_decoder_nets[row] = selectNets(decoder_name, read_ports);

      auto decoder_and_cell = makeDecoder(decoder_name,
                                          word_count,
                                          read_ports,
                                          word_decoder_nets[row],
                                          decoder_input_nets[row]);

      ram_grid.addCell(std::move(decoder_and_cell), col_cell_count);
    }
  }

  // create bytes within a word, shared decoder net for each word
  for (int col = 0; col < bytes_per_word; ++col) {
    array<dbNet*, 8> D_nets;  // net for buffers
    for (int bit = 0; bit < 8; ++bit) {
      data_inputs_.push_back(
          makeBTerm(fmt::format("D[{}]", bit + col * 8), dbIoType::INPUT));
      D_nets[bit] = makeNet(fmt::format("D_nets[{}]", bit + col * 8), "net");
    }

    // if readports == 1, only have Q outputs
    if (read_ports == 1) {
      array<dbBTerm*, 8> q_bTerms;
      for (int bit = 0; bit < 8; ++bit) {
        auto out_name = fmt::format("Q[{}]", bit + col * 8);
        q_bTerms[bit] = makeBTerm(out_name, dbIoType::OUTPUT);
      }
      q_outputs_.push_back(q_bTerms);
    } else {
      for (int read_port = 0; read_port < read_ports; ++read_port) {
        array<dbBTerm*, 8> q_bTerms;
        for (int bit = 0; bit < 8; ++bit) {
          auto out_name = fmt::format("Q{}[{}]", read_port, bit + col * 8);
          q_bTerms[bit] = makeBTerm(out_name, dbIoType::OUTPUT);
        }
        q_outputs_.push_back(q_bTerms);
      }
    }

    for (int row = 0; row < word_count; ++row) {
      auto cell_name = fmt::format("storage_{}_{}", row, col);

      makeCellByte(ram_grid,
                   col,
                   cell_name,
                   read_ports,
                   clock->getNet(),
                   write_enable[col]->getNet(),
                   word_decoder_nets[row],
                   D_nets,
                   q_outputs_);
    }

    for (int bit = 0; bit < 8; ++bit) {
      auto buffer_grid_cell = std::make_unique<Cell>();
      makeCellInst(buffer_grid_cell.get(),
                   "buffer",
                   fmt::format("in[{}]", bit + col * 8),
                   buffer_cell_,
                   {{"A", data_inputs_[bit]->getNet()}, {"X", D_nets[bit]}});
      ram_grid.addCell(std::move(buffer_grid_cell), col * 9 + bit);
    }
  }

  auto cell_inv_layout = std::make_unique<Layout>(odb::vertical);
  // check for AND gate, specific case for 2 words
  if (num_inputs > 1) {
    for (int i = num_inputs - 1; i >= 0; --i) {
      auto inv_grid_cell = std::make_unique<Cell>();
      makeCellInst(inv_grid_cell.get(),
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
    makeCellInst(inv_grid_cell.get(),
                 "decoder",
                 fmt::format("inv_{}", 0),
                 inv_cell_,
                 {{"A", addr_inputs_[0]->getNet()}, {"Y", inv_addr[0]}});
    cell_inv_layout->addCell(std::move(inv_grid_cell));
  }

  ram_grid.addLayout(std::move(cell_inv_layout));

  auto ram_origin(odb::Point(0, 0));

  ram_grid.setOrigin(ram_origin);
  ram_grid.gridInit();

  if (tapcell_) {
    // max tap distance specified is greater than the length of ram
    if (ram_grid.getRowWidth() <= max_tap_dist) {
      auto tapcell_layout = generateTapColumn(word_count, 0);
      ram_grid.insertLayout(std::move(tapcell_layout), 0);
    } else {
      // needed this calculation so first cells have right distance
      int nearest_tap
          = (max_tap_dist / ram_grid.getWidth()) * ram_grid.getLayoutWidth(0);
      int tapcell_count = 0;
      // iterates through each of the columns
      for (int col = 0; col < ram_grid.numLayouts(); ++col) {
        if (nearest_tap + ram_grid.getLayoutWidth(col) >= max_tap_dist) {
          // if the nearest_tap is too far, generate tap column
          auto tapcell_layout = generateTapColumn(word_count, tapcell_count);
          ram_grid.insertLayout(std::move(tapcell_layout), col);
          ++col;  // col adjustment after insertion
          nearest_tap = 0;
          ++tapcell_count;
        }
        nearest_tap += ram_grid.getLayoutWidth(col);
      }
      // check for last column in the grid
      if (nearest_tap >= max_tap_dist) {
        auto tapcell_layout = generateTapColumn(word_count, tapcell_count);
        ram_grid.addLayout(std::move(tapcell_layout));
      }
    }
  }

  ram_grid.gridInit();

  auto db_libs = db_->getLibs().begin();
  auto db_sites = *(db_libs->getSites().begin());
  auto sites_width = db_sites->getWidth();

  int num_sites = ram_grid.getRowWidth() / db_sites->getWidth();
  for (int i = 0; i <= word_count; ++i) {  // extra for the layer of buffers
    auto row_name = fmt::format("RAM_ROW{}", i);
    auto y_coord = i * ram_grid.getHeight();
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

  ram_grid.placeGrid();

  int max_y_coord = ram_grid.getHeight() * (word_count + 1);
  int max_x_coord = ram_grid.getRowWidth();

  block_->setDieArea(odb::Rect(0, 0, max_x_coord, max_y_coord));
  block_->setCoreArea(block_->computeCoreArea());
}

}  // namespace ram
