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
               {{storage_cell_->findMTerm("CLK") ? "CLK" : "GATE", clock},
                {"D", data_input},
                {"Q", storage_net}});

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
                          const int col_group,
                          const int bytes_per_word_total,
                          const std::string& prefix,
                          const int read_ports,
                          dbNet* clock,
                          dbNet* write_enable,
                          const vector<dbNet*>& selects,
                          dbNet* col_select,
                          const array<dbNet*, 8>& data_input,
                          const vector<array<dbNet*, 8>>& data_output)
{
  auto sel_cell = std::make_unique<Cell>();

  // Write path: AND row_select with col_select so the clock gate only fires
  // for the addressed column group. The read path is handled by the AOI mux
  // in makeColMux, so the tristate enable remains row_select only.
  dbNet* write_sel = selects[0];
  if (col_select) {
    write_sel = makeNet(prefix, "write_sel");
    makeCellInst(sel_cell.get(),
                 prefix,
                 "col_and",
                 and2_cell_,
                 {{"A", selects[0]}, {"B", col_select}, {"X", write_sel}});
  }

  // Tristate enable is row_select only (inverted): the AOI mux downstream
  // selects which column group's tristate bus drives the final Q output.
  vector<dbNet*> select_b_nets(selects.size());
  for (int i = 0; i < (int) selects.size(); ++i) {
    select_b_nets[i] = makeNet(prefix, fmt::format("select{}_b", i));
  }

  auto gclock_net = makeNet(prefix, "gclock");
  auto we0_net = makeNet(prefix, "we0");

  // For naming bits: 0, 8, 16,...
  const int logical_bit_base = byte_idx * 8;

  // Physical column base accounts for which column group this byte is in.
  const int physical_col_base = (col_group * bytes_per_word_total + byte_idx) * 9;

  for (int local_bit = 0; local_bit < 8; ++local_bit) {
    const int global_logical_bit_idx = logical_bit_base + local_bit;
    const int physical_col_idx = physical_col_base + local_bit;

    auto name = fmt::format("{}.bit{}", prefix, global_logical_bit_idx);
    vector<dbNet*> outs;
    outs.reserve(read_ports);
    for (int read_port = 0; read_port < read_ports; ++read_port) {
      outs.push_back(data_output[read_port][local_bit]);
    }
    ram_grid.addCell(makeCellBit(name,
                                 read_ports,
                                 gclock_net,
                                 select_b_nets,
                                 data_input[local_bit],
                                 outs),
                     physical_col_idx);
  }

  // Clock gate
  makeCellInst(sel_cell.get(),
               prefix,
               "cg",
               clock_gate_cell_,
               {{"CLK", clock}, {"GATE", we0_net}, {"GCLK", gclock_net}});

  // WE AND gate: write_sel already combines row+col for the write path
  makeCellInst(sel_cell.get(),
               prefix,
               "gcand",
               and2_cell_,
               {{"A", write_sel}, {"B", write_enable}, {"X", we0_net}});

  // Select inverters: invert row_select only for tristate enable
  for (int i = 0; i < (int) selects.size(); ++i) {
    makeCellInst(sel_cell.get(),
                 prefix,
                 fmt::format("select_inv_{}", i),
                 inv_cell_,
                 {{"A", selects[i]}, {"Y", select_b_nets[i]}});
  }

  ram_grid.addCell(std::move(sel_cell), physical_col_base + 8);
}

std::unique_ptr<Cell> RamGen::makeColMux(
    const std::string& prefix,
    const int mux_col_ratio,
    const vector<array<dbNet*, 8>>& col_q_nets,
    const vector<dbNet*>& col_sel_nets,
    const array<dbNet*, 8>& q_out_nets)
{
  auto mux_cell = std::make_unique<Cell>();

  for (int bit = 0; bit < 8; ++bit) {
    auto aoi_lo_net = makeNet(prefix, fmt::format("aoi_lo_bit{}", bit));
    auto inv_in_net = aoi_lo_net;  // default for mux=2; overridden for mux=4

    // First AOI22: NOT((col_sel[0] & col_q[0]) | (col_sel[1] & col_q[1]))
    makeCellInst(mux_cell.get(),
                 prefix,
                 fmt::format("aoi_lo_bit{}", bit),
                 aoi22_cell_,
                 {{aoi22_in_a1_, col_sel_nets[0]},
                  {aoi22_in_a2_, col_q_nets[0][bit]},
                  {aoi22_in_b1_, col_sel_nets[1]},
                  {aoi22_in_b2_, col_q_nets[1][bit]},
                  {aoi22_out_, aoi_lo_net}});

    if (mux_col_ratio == 4) {
      // Second AOI22: NOT((col_sel[2] & col_q[2]) | (col_sel[3] & col_q[3]))
      auto aoi_hi_net = makeNet(prefix, fmt::format("aoi_hi_bit{}", bit));
      makeCellInst(mux_cell.get(),
                   prefix,
                   fmt::format("aoi_hi_bit{}", bit),
                   aoi22_cell_,
                   {{aoi22_in_a1_, col_sel_nets[2]},
                    {aoi22_in_a2_, col_q_nets[2][bit]},
                    {aoi22_in_b1_, col_sel_nets[3]},
                    {aoi22_in_b2_, col_q_nets[3][bit]},
                    {aoi22_out_, aoi_hi_net}});

      // AND2(aoi_lo, aoi_hi): De Morgan gives us the OR of all four AND terms
      // NOT(aoi_lo AND aoi_hi) = (cs0&q0|cs1&q1) OR (cs2&q2|cs3&q3)
      // so we still need to invert — use AND2 then INV below.
      auto and_net = makeNet(prefix, fmt::format("mux_and_bit{}", bit));
      makeCellInst(mux_cell.get(),
                   prefix,
                   fmt::format("mux_and_bit{}", bit),
                   and2_cell_,
                   {{"A", aoi_lo_net}, {"B", aoi_hi_net}, {"X", and_net}});
      inv_in_net = and_net;
    }

    // Final INV to un-invert the AOI output and drive Q
    makeCellInst(mux_cell.get(),
                 prefix,
                 fmt::format("mux_inv_bit{}", bit),
                 inv_cell_,
                 {{"A", inv_in_net}, {"Y", q_out_nets[bit]}});
  }

  return mux_cell;
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

  if (!aoi22_cell_) {
    // AOI22: Y = NOT((A AND B) OR (C AND D))
    // FuncExpr tree: not_ → or_ → [ and_(port, port), and_(port, port) ]
    aoi22_cell_ = findMaster(
        [](sta::LibertyPort* port) {
          if (!port->direction()->isOutput()) {
            return false;
          }
          auto f = port->function();
          if (!f || f->op() != sta::FuncExpr::Op::not_) {
            return false;
          }
          auto inner = f->left();
          if (!inner || inner->op() != sta::FuncExpr::Op::or_) {
            return false;
          }
          auto L = inner->left();
          auto R = inner->right();
          return L && L->op() == sta::FuncExpr::Op::and_
                 && L->left() && L->left()->op() == sta::FuncExpr::Op::port
                 && L->right() && L->right()->op() == sta::FuncExpr::Op::port
                 && R && R->op() == sta::FuncExpr::Op::and_
                 && R->left() && R->left()->op() == sta::FuncExpr::Op::port
                 && R->right() && R->right()->op() == sta::FuncExpr::Op::port;
        },
        "aoi22");

    // Extract the actual port names from the liberty function tree so we can
    // wire up the cell generically regardless of naming convention.
    auto cell = network_->libertyCell(network_->dbToSta(aoi22_cell_));
    auto port_iter = cell->portIterator();
    while (port_iter->hasNext()) {
      auto p = static_cast<sta::ConcretePort*>(port_iter->next());
      if (p->direction()->isAnyOutput()) {
        auto f = p->libertyPort()->function();
        auto or_expr = f->left();
        auto and_a = or_expr->left();
        auto and_b = or_expr->right();
        aoi22_in_a1_ = and_a->left()->port()->name();
        aoi22_in_a2_ = and_a->right()->port()->name();
        aoi22_in_b1_ = and_b->left()->port()->name();
        aoi22_in_b2_ = and_b->right()->port()->name();
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
                      const int mux_col_ratio,
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

  // error checking for read ports != 1 for current version of RamGen, edit
  // later for future changes
  if (read_ports != 1) {
    logger_->error(
        RAM, 25, "The ram generator currently only supports 1 read port.");
    return;
  }

  if (mux_col_ratio != 1 && mux_col_ratio != 2 && mux_col_ratio != 4) {
    logger_->error(RAM, 26, "mux_col_ratio must be 1, 2, or 4.");
    return;
  }
  if (word_count % mux_col_ratio != 0) {
    logger_->error(RAM,
                   27,
                   "word_count ({}) must be divisible by mux_col_ratio ({}).",
                   word_count,
                   mux_col_ratio);
    return;
  }
  if (mux_col_ratio > 1 && word_count / mux_col_ratio < 2) {
    logger_->error(
        RAM,
        28,
        "word_count / mux_col_ratio must be at least 2 (got {}).",
        word_count / mux_col_ratio);
    return;
  }

  // Number of physical rows and address bit split between row and column.
  const int num_rows = word_count / mux_col_ratio;
  const int num_col_bits = (mux_col_ratio > 1)
                               ? static_cast<int>(std::log2(mux_col_ratio))
                               : 0;

  logger_->info(RAM, 3, "Generating {}", ram_name);

  storage_cell_ = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_ = inv_cell;
  tapcell_ = tapcell;
  and2_cell_ = nullptr;
  aoi22_cell_ = nullptr;
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

  // 9 columns per byte (8 bit cells + 1 sel cell), replicated mux_col_ratio
  // times horizontally. Extra column at the end is for row decoder cells.
  int col_cell_count = bytes_per_word * 9 * mux_col_ratio;
  Grid ram_grid(odb::horizontal, col_cell_count + 1);

  auto clock = makeBTerm("clk", dbIoType::INPUT);

  vector<dbBTerm*> write_enable(bytes_per_word, nullptr);
  for (int byte = 0; byte < bytes_per_word; ++byte) {
    auto in_name = fmt::format("we[{}]", byte);
    write_enable[byte] = makeBTerm(in_name, dbIoType::INPUT);
  }

  // Total address bits cover the full word_count.
  // Lower num_col_bits select the physical column group (mux select).
  // Upper bits select the physical row (row decoder).
  int num_inputs = std::ceil(std::log2(word_count));
  const int num_row_bits = num_inputs - num_col_bits;

  for (int i = 0; i < num_inputs; ++i) {
    addr_inputs_.push_back(
        makeBTerm(fmt::format("addr[{}]", i), dbIoType::INPUT));
  }

  // Inverted address nets (one per address bit, covers all bits)
  vector<dbNet*> inv_addr(num_inputs);
  for (int i = 0; i < num_inputs; ++i) {
    inv_addr[i] = makeNet("inv", fmt::format("addr[{}]", i));
  }

  // Column select nets: derived from the lower num_col_bits of address.
  //   mux_col_ratio=1: unused (nullptr throughout)
  //   mux_col_ratio=2: col_sel[0]=~addr[0], col_sel[1]=addr[0]
  //   mux_col_ratio=4: col_sel[c] = AND of addr[1:0] combination for column c
  vector<dbNet*> col_sel_nets(mux_col_ratio, nullptr);
  if (mux_col_ratio == 2) {
    col_sel_nets[0] = inv_addr[0];
    col_sel_nets[1] = addr_inputs_[0]->getNet();
  } else if (mux_col_ratio == 4) {
    for (int c = 0; c < 4; ++c) {
      col_sel_nets[c] = makeNet("col_sel", fmt::format("{}", c));
    }
  }

  // Row decoder input nets: for each physical row, determine whether each
  // upper address bit (addr[num_col_bits..num_inputs-1]) is true or inverted.
  vector<vector<dbNet*>> decoder_input_nets(num_rows,
                                            vector<dbNet*>(num_row_bits));
  for (int row = 0; row < num_rows; ++row) {
    int row_num = row;
    for (int input = 0; input < num_row_bits; ++input) {
      if (row_num % 2 == 0) {
        decoder_input_nets[row][input] = inv_addr[num_col_bits + input];
      } else {
        decoder_input_nets[row][input]
            = addr_inputs_[num_col_bits + input]->getNet();
      }
      row_num /= 2;
    }
  }

  // Row decoder: one select net per physical row, shared across all bytes and
  // column groups in that row.
  vector<vector<dbNet*>> row_decoder_nets(num_rows);

  for (int row = 0; row < num_rows; ++row) {
    auto decoder_name = fmt::format("decoder_{}", row);

    if (num_rows == 2) {
      // Special case: single upper address bit, no AND gates needed.
      dbNet* addr_net = (row == 0 ? inv_addr[num_col_bits]
                                  : addr_inputs_[num_col_bits]->getNet());
      for (int i = 0; i < read_ports; ++i) {
        row_decoder_nets[row].push_back(addr_net);
      }
    } else {
      row_decoder_nets[row] = selectNets(decoder_name, read_ports);

      auto decoder_and_cell = makeDecoder(decoder_name,
                                          num_rows,
                                          read_ports,
                                          row_decoder_nets[row],
                                          decoder_input_nets[row]);

      ram_grid.addCell(std::move(decoder_and_cell), col_cell_count);
    }
  }

  // For mux_col_ratio=4, build the column-select AND gates and place them in
  // the decoder column at the buffer-row position (after all row decoder cells).
  if (mux_col_ratio == 4) {
    // If num_rows==2 the special case left the decoder column empty; pad so
    // the col-select cell lands in the correct buffer-row slot.
    if (num_rows == 2) {
      for (int r = 0; r < num_rows; ++r) {
        ram_grid.addCell(std::unique_ptr<Cell>(), col_cell_count);
      }
    }
    auto col_sel_cell = std::make_unique<Cell>();
    // col c is selected when addr[1:0] == c
    // c=0: ~addr[1] & ~addr[0]
    // c=1: ~addr[1] &  addr[0]
    // c=2:  addr[1] & ~addr[0]
    // c=3:  addr[1] &  addr[0]
    makeCellInst(col_sel_cell.get(),
                 "col_sel",
                 "and_0",
                 and2_cell_,
                 {{"A", inv_addr[1]},
                  {"B", inv_addr[0]},
                  {"X", col_sel_nets[0]}});
    makeCellInst(col_sel_cell.get(),
                 "col_sel",
                 "and_1",
                 and2_cell_,
                 {{"A", inv_addr[1]},
                  {"B", addr_inputs_[0]->getNet()},
                  {"X", col_sel_nets[1]}});
    makeCellInst(col_sel_cell.get(),
                 "col_sel",
                 "and_2",
                 and2_cell_,
                 {{"A", addr_inputs_[1]->getNet()},
                  {"B", inv_addr[0]},
                  {"X", col_sel_nets[2]}});
    makeCellInst(col_sel_cell.get(),
                 "col_sel",
                 "and_3",
                 and2_cell_,
                 {{"A", addr_inputs_[1]->getNet()},
                  {"B", addr_inputs_[0]->getNet()},
                  {"X", col_sel_nets[3]}});
    ram_grid.addCell(std::move(col_sel_cell), col_cell_count);
  }

  // Build the storage array: iterate over logical bytes, then column groups,
  // then physical rows.
  //
  // For mux=1: each bit cell's tristate drives the Q BTerm net directly.
  // For mux>1: each col_group gets its own intermediate tristate bus
  //   (col_q_nets[col_group][bit]). After all col_groups are built, an AOI
  //   mux selects the addressed col_group and drives the actual Q BTerm net.
  for (int col = 0; col < bytes_per_word; ++col) {
    array<dbNet*, 8> D_nets;
    for (int bit = 0; bit < 8; ++bit) {
      data_inputs_.push_back(
          makeBTerm(fmt::format("D[{}]", bit + col * 8), dbIoType::INPUT));
      D_nets[bit] = makeNet(fmt::format("D_nets[{}]", bit + col * 8), "net");
    }

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

    // The Q BTerm net for this byte (read_port 0, which is the only port for
    // now). For mux=1 this is used directly. For mux>1 the AOI mux drives it.
    const int q_idx = (read_ports == 1) ? col : col * read_ports;
    array<dbNet*, 8> q_out_nets;
    for (int bit = 0; bit < 8; ++bit) {
      q_out_nets[bit] = q_outputs_[q_idx][bit]->getNet();
    }

    // Intermediate tristate bus per col_group. For mux=1, just alias Q BTerm
    // nets directly (no intermediate wires, same as before).
    vector<array<dbNet*, 8>> col_q_nets(mux_col_ratio);
    if (mux_col_ratio == 1) {
      col_q_nets[0] = q_out_nets;
    } else {
      for (int cg = 0; cg < mux_col_ratio; ++cg) {
        for (int bit = 0; bit < 8; ++bit) {
          col_q_nets[cg][bit] = makeNet(
              fmt::format("col{}_q[{}]", cg, bit + col * 8), "net");
        }
      }
    }

    for (int col_group = 0; col_group < mux_col_ratio; ++col_group) {
      // data_output for this col_group: one entry per read port.
      vector<array<dbNet*, 8>> col_group_output(read_ports);
      col_group_output[0] = col_q_nets[col_group];

      for (int row = 0; row < num_rows; ++row) {
        auto cell_name = fmt::format("storage_{}_{}_{}", col_group, row, col);

        makeCellByte(ram_grid,
                     col,
                     col_group,
                     bytes_per_word,
                     cell_name,
                     read_ports,
                     clock->getNet(),
                     write_enable[col]->getNet(),
                     row_decoder_nets[row],
                     col_sel_nets[col_group],
                     D_nets,
                     col_group_output);
      }
    }

    // For mux>1, place the AOI column mux in the sel-cell track of col_group=0
    // (track col*9+8). That track has exactly num_rows sel_cells already, so
    // this cell lands in the buffer-row slot.
    if (mux_col_ratio > 1) {
      auto mux_cell = makeColMux(fmt::format("col_mux_{}", col),
                                 mux_col_ratio,
                                 col_q_nets,
                                 col_sel_nets,
                                 q_out_nets);
      ram_grid.addCell(std::move(mux_cell), col * 9 + 8);
    }

    // Input buffers for this logical byte are placed in col_group=0's physical
    // columns (same track indices as before). The D_nets wire is shared across
    // all column groups so the router connects them.
    for (int bit = 0; bit < 8; ++bit) {
      auto buffer_grid_cell = std::make_unique<Cell>();
      makeCellInst(buffer_grid_cell.get(),
                   "buffer",
                   fmt::format("in[{}]", bit + col * 8),
                   buffer_cell_,
                   {{"A", data_inputs_[bit + col * 8]->getNet()},
                    {"X", D_nets[bit]}});
      ram_grid.addCell(std::move(buffer_grid_cell), col * 9 + bit);
    }
  }

  // Address inverter column: one inverter per address bit, compact for mux>1.
  auto cell_inv_layout = std::make_unique<Layout>(odb::vertical);
  if (mux_col_ratio == 1) {
    // Original spaced pattern: each inverter is separated by (num_inputs-1)
    // filler rows so the inverters spread evenly over the array height.
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
  } else {
    // Compact pattern for mux>1: place inverters without spacing and pad the
    // remaining slots with nullptr to stay within the array height.
    for (int i = num_inputs - 1; i >= 0; --i) {
      auto inv_grid_cell = std::make_unique<Cell>();
      makeCellInst(inv_grid_cell.get(),
                   "decoder",
                   fmt::format("inv_{}", i),
                   inv_cell_,
                   {{"A", addr_inputs_[i]->getNet()}, {"Y", inv_addr[i]}});
      cell_inv_layout->addCell(std::move(inv_grid_cell));
    }
    for (int pad = num_inputs; pad <= num_rows; ++pad) {
      cell_inv_layout->addCell(nullptr);
    }
  }

  ram_grid.addLayout(std::move(cell_inv_layout));

  auto ram_origin(odb::Point(0, 0));

  ram_grid.setOrigin(ram_origin);
  ram_grid.gridInit();

  if (tapcell_) {
    if (ram_grid.getRowWidth() <= max_tap_dist) {
      auto tapcell_layout = generateTapColumn(num_rows, 0);
      ram_grid.insertLayout(std::move(tapcell_layout), 0);
    } else {
      int nearest_tap
          = (max_tap_dist / ram_grid.getWidth()) * ram_grid.getLayoutWidth(0);
      int tapcell_count = 0;
      for (int col = 0; col < ram_grid.numLayouts(); ++col) {
        if (nearest_tap + ram_grid.getLayoutWidth(col) >= max_tap_dist) {
          auto tapcell_layout = generateTapColumn(num_rows, tapcell_count);
          ram_grid.insertLayout(std::move(tapcell_layout), col);
          ++col;
          nearest_tap = 0;
          ++tapcell_count;
        }
        nearest_tap += ram_grid.getLayoutWidth(col);
      }
      if (nearest_tap >= max_tap_dist) {
        auto tapcell_layout = generateTapColumn(num_rows, tapcell_count);
        ram_grid.addLayout(std::move(tapcell_layout));
      }
    }
  }

  ram_grid.gridInit();

  auto db_libs = db_->getLibs().begin();
  auto db_sites = *(db_libs->getSites().begin());
  auto sites_width = db_sites->getWidth();

  int num_sites = ram_grid.getRowWidth() / db_sites->getWidth();
  for (int i = 0; i <= num_rows; ++i) {  // extra for the layer of buffers
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

  int max_y_coord = ram_grid.getHeight() * (num_rows + 1);
  int max_x_coord = ram_grid.getRowWidth();

  block_->setDieArea(odb::Rect(0, 0, max_x_coord, max_y_coord));
  block_->setCoreArea(block_->computeCoreArea());

  writeBehavioralVerilog(
      behavioral_verilog_filename_, bytes_per_word, word_count, read_ports);
}

void RamGen::setBehavioralVerilogFilename(const std::string& filename)
{
  behavioral_verilog_filename_ = filename;
}

void RamGen::writeBehavioralVerilog(const std::string& filename,
                                    const int bytes_per_word,
                                    const int word_count,
                                    const int read_ports)
{
  if (filename.empty()) {
    return;
  }

  const int word_size_bit = bytes_per_word * 8;
  const int address_width
      = (word_count <= 1) ? 1 : std::ceil(std::log2(word_count));

  std::string module_name = fmt::format("RAM{}x{}", word_count, word_size_bit);

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
        mem[addr_rw][i*8 +:8] <= D[i*8 +:8];
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
                                         bytes_per_word - 1,
                                         word_size_bit - 1,
                                         word_count - 1,
                                         bytes_per_word,
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
