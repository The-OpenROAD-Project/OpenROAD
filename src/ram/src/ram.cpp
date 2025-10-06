// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ram/ram.h"

#include <array>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "layout.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
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

RamGen::RamGen(sta::dbNetwork* network, odb::dbDatabase* db, Logger* logger)
    : network_(network), db_(db), logger_(logger)
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
                          const int byte_number,
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

  int first_byte = byte_number * 9;
  for (int bit = first_byte; bit < first_byte + 8; ++bit) {
    auto name = fmt::format("{}.bit{}", prefix, bit);
    vector<dbNet*> outs;
    outs.reserve(read_ports);
    for (int read_port = 0; read_port < read_ports; ++read_port) {
      outs.push_back(data_output[read_port][bit]->getNet());
    }

    ram_grid.addCell(
        makeCellBit(
            name, read_ports, gclock_net, select_b_nets, data_input[bit], outs),
        bit);
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

  ram_grid.addCell(std::move(sel_cell), (byte_number * 9) + 8);
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

  for (int i = 0; i < layers; ++i) {
    auto input_net = makeNet(prefix, fmt::format("layer_in{}", i));
    // sets up first AND gate, closest to byte's select + write enable gate
    if (i == 0 && i == layers - 1) {
      makeCellInst(
          word_cell.get(),
          prefix,
          fmt::format("and_layer{}", i),
          and2_cell_,
          {{"A", addr_nets[i]}, {"B", addr_nets[i + 1]}, {"X", selects[0]}});
      prev_net = input_net;
    } else if (i == 0) {
      makeCellInst(word_cell.get(),
                   prefix,
                   fmt::format("and_layer{}", i),
                   and2_cell_,
                   {{"A", addr_nets[i]}, {"B", input_net}, {"X", selects[0]}});
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

void RamGen::generate(const int bytes_per_word,
                      const int word_count,
                      const int read_ports,
                      dbMaster* storage_cell,
                      dbMaster* tristate_cell,
                      dbMaster* inv_cell)
{
  const int bits_per_word = bytes_per_word * 8;
  const std::string ram_name
      = fmt::format("RAM{}x{}", word_count, bits_per_word);

  logger_->info(RAM, 3, "Generating {}", ram_name);

  storage_cell_ = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_ = inv_cell;
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
  vector<dbBTerm*> addr(num_inputs, nullptr);
  for (int i = 0; i < num_inputs; ++i) {
    addr[i] = makeBTerm(fmt::format("addr[{}]", i), dbIoType::INPUT);
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
    for (int input = 0; input < num_inputs; ++input) {  // start at right most
                                                        // bit
      if (word_num % 2 == 0) {
        // places inverted address for each input
        decoder_input_nets[word][input] = inv_addr[input];
      } else {  // puts original input in invert nets
        decoder_input_nets[word][input] = addr[input]->getNet();
      }
      word_num /= 2;
    }
  }

  vector<dbNet*> decoder_output_nets;

  for (int col = 0; col < bytes_per_word; ++col) {
    array<dbBTerm*, 8> D_bTerms;  // array for b-term for external inputs
    array<dbNet*, 8> D_nets;      // net for buffers
    for (int bit = 0; bit < 8; ++bit) {
      D_bTerms[bit]
          = makeBTerm(fmt::format("D[{}]", bit + col * 8), dbIoType::INPUT);
      D_nets[bit] = makeNet(fmt::format("D_nets[{}]", bit + col * 8), "net");
    }

    vector<array<dbBTerm*, 8>> Q;
    // if readports == 1, only have Q outputs
    if (read_ports == 1) {
      array<dbBTerm*, 8> q_bTerms;
      for (int bit = 0; bit < 8; ++bit) {
        auto out_name = fmt::format("Q[{}]", bit + col * 8);
        q_bTerms[bit] = makeBTerm(out_name, dbIoType::OUTPUT);
      }
      Q.push_back(q_bTerms);
    } else {
      for (int read_port = 0; read_port < read_ports; ++read_port) {
        array<dbBTerm*, 8> q_bTerms;
        for (int bit = 0; bit < 8; ++bit) {
          auto out_name = fmt::format("Q{}[{}]", read_port, bit + col * 8);
          q_bTerms[bit] = makeBTerm(out_name, dbIoType::OUTPUT);
        }
        Q.push_back(q_bTerms);
      }
    }

    for (int row = 0; row < word_count; ++row) {
      auto cell_name = fmt::format("storage_{}_{}", row, col);
      if (word_count == 2) {
        decoder_output_nets.clear();
        decoder_output_nets.push_back(row == 0 ? inv_addr[0]
                                               : addr[0]->getNet());
      } else {
        decoder_output_nets = selectNets(cell_name, read_ports);
      }

      makeCellByte(ram_grid,
                   col,
                   cell_name,
                   read_ports,
                   clock->getNet(),
                   write_enable[col]->getNet(),
                   decoder_output_nets,
                   D_nets,
                   Q);
      auto decoder_name = fmt::format("decoder_{}_{}", row, col);
      auto decoder_and_cell = makeDecoder(decoder_name,
                                          word_count,
                                          read_ports,
                                          decoder_output_nets,
                                          decoder_input_nets[row]);

      ram_grid.addCell(std::move(decoder_and_cell), (bytes_per_word * 9));
    }

    for (int bit = 0; bit < 8; ++bit) {
      auto buffer_cell = std::make_unique<Cell>();
      makeCellInst(buffer_cell.get(),
                   "buffer",
                   fmt::format("in[{}]", bit),
                   buffer_cell_,
                   {{"A", D_bTerms[bit]->getNet()}, {"X", D_nets[bit]}});
      ram_grid.addCell(std::move(buffer_cell), bit);
    }
  }

  auto cell_inv_layout = std::make_unique<Layout>(odb::vertical);
  // check for AND gate, specific case for 2 words
  if (num_inputs > 1) {
    for (int i = num_inputs - 1; i >= 0; --i) {
      auto inv_cell = std::make_unique<Cell>();
      makeCellInst(inv_cell.get(),
                   "decoder",
                   fmt::format("inv_{}", i),
                   inv_cell_,
                   {{"A", addr[i]->getNet()}, {"Y", inv_addr[i]}});
      cell_inv_layout->addCell(std::move(inv_cell));
      for (int filler_count = 0; filler_count < num_inputs - 1;
           ++filler_count) {
        cell_inv_layout->addCell(nullptr);
      }
    }
  } else {
    auto inv_cell = std::make_unique<Cell>();
    makeCellInst(inv_cell.get(),
                 "decoder",
                 fmt::format("inv_{}", 0),
                 inv_cell_,
                 {{"A", addr[0]->getNet()}, {"Y", inv_addr[0]}});
    cell_inv_layout->addCell(std::move(inv_cell));
  }

  ram_grid.addLayout(std::move(cell_inv_layout));

  auto ram_origin(odb::Point(0, 0));

  ram_grid.setOrigin(ram_origin);
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
}

}  // namespace ram
