/////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "ram/ram.h"

#include "db_sta/dbNetwork.hh"
#include "layout.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace ram {

using odb::dbBlock;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbMaster;
using odb::dbNet;

using utl::RAM;

using std::vector;
using std::array;

////////////////////////////////////////////////////////////////

RamGen::RamGen() : db_(nullptr), logger_(nullptr)
{
}

void RamGen::init(odb::dbDatabase* db, sta::dbNetwork* network, Logger* logger)
{
  db_ = db;
  network_ = network;
  logger_ = logger;
}

dbInst* RamGen::makeInst(
    Layout* layout,
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
          RAM, 9, "term {} of cell {} not found.", name, master->getName());
    }
    auto iterm = inst->getITerm(mterm);
    iterm->connect(net);
  }

  layout->addElement(std::make_unique<Element>(inst));
  return inst;
}

dbNet* RamGen::makeNet(const std::string& prefix, const std::string& name)
{
  const auto net_name = fmt::format("{}.{}", prefix, name);
  return dbNet::create(block_, net_name.c_str());
}

dbNet* RamGen::makeBTerm(const std::string& name)
{
  auto net = dbNet::create(block_, name.c_str());
  dbBTerm::create(net, name.c_str());
  return net;
}

std::unique_ptr<Element> RamGen::make_bit(const std::string& prefix,
                                          const int read_ports,
                                          dbNet* clock,
                                          vector<odb::dbNet*>& select,
                                          dbNet* data_input,
                                          vector<odb::dbNet*>& data_output)
{
  auto layout = std::make_unique<Layout>(odb::horizontal);

  auto storage_net = makeNet(prefix, "storage");

  // Make Storage latch
  makeInst(layout.get(),
           prefix,
           "bit",
           storage_cell_,
           {{"GATE", clock}, {"D", data_input}, {"Q", storage_net}});

  // Make ouput tristate driver(s) for read port(s)
  for (int read_port = 0; read_port < read_ports; ++read_port) {
    makeInst(layout.get(),
             prefix,
             fmt::format("obuf{}", read_port),
             tristate_cell_,
             {{"A", storage_net},
              {"TE_B", select[read_port]},
              {"Z", data_output[read_port]}});
  }

  return std::make_unique<Element>(std::move(layout));
}

std::unique_ptr<Element> RamGen::make_byte(
    const std::string& prefix,
    const int read_ports,
    dbNet* clock,
    dbNet* write_enable,
    const vector<dbNet*>& selects,
    const array<dbNet*,8>& data_input,
    const vector<array<dbNet*, 8>>& data_output)
{
  auto layout = std::make_unique<Layout>(odb::horizontal);

  vector<dbNet*> select_b_nets(selects.size());
  for (int i = 0; i < selects.size(); ++i) {
    select_b_nets[i] = makeNet(prefix, fmt::format("select{}_b", i));
  }

  auto clock_b_net = makeNet(prefix, "clock_b");
  auto gclock_net = makeNet(prefix, "gclock");
  auto we0_net = makeNet(prefix, "we0");

  for (int bit = 0; bit < 8; ++bit) {
    auto name = fmt::format("{}.bit{}", prefix, bit);
    vector<dbNet*> outs;
    for (int read_port = 0; read_port < read_ports; ++read_port) {
      outs.push_back(data_output[read_port][bit]);
    }
    layout->addElement(make_bit(
        name, read_ports, gclock_net, select_b_nets, data_input[bit], outs));
  }

  // Make clock gate
  makeInst(layout.get(),
           prefix,
           "cg",
           clock_gate_cell_,
           {{"CLK", clock_b_net}, {"GATE", we0_net}, {"GCLK", gclock_net}});

  // Make clock and
  makeInst(layout.get(),
           prefix,
           "gcand",
           and2_cell_,
           {{"A", selects[0]}, {"B", write_enable}, {"X", we0_net}});

  // Make select inverters
  for (int i = 0; i < selects.size(); ++i) {
    makeInst(layout.get(),
             prefix,
             fmt::format("select_inv_{}", i),
             inv_cell_,
             {{"A", selects[i]}, {"Y", select_b_nets[i]}});
  }

  // Make clock inverter
  makeInst(layout.get(),
           prefix,
           "clock_inv",
           inv_cell_,
           {{"A", clock}, {"Y", clock_b_net}});

  return std::make_unique<Element>(std::move(layout));
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
        [this](sta::LibertyPort* port) {
          return port->libertyCell()->isInverter();
        },
        "inverter");
  }

  if (!tristate_cell_) {
    tristate_cell_ = findMaster(
        [this](sta::LibertyPort* port) {
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
        [this](sta::LibertyPort* port) {
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
        [this](sta::LibertyPort* port) {
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
        [this](sta::LibertyPort* port) {
          return port->libertyCell()->isClockGate();
        },
        "clock gate");
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
  findMasters();

  auto chip = db_->getChip();
  if (!chip) {
    chip = odb::dbChip::create(db_);
  }

  block_ = chip->getBlock();
  if (!block_) {
    block_ = odb::dbBlock::create(chip, ram_name.c_str());
  }

  Layout layout(odb::horizontal);

  auto clock = makeBTerm("clock");

  vector<dbNet*> write_enable(bytes_per_word, nullptr);
  for (int byte = 0; byte < bytes_per_word; ++byte) {
    auto in_name = fmt::format("write_enable[{}]", byte);
    write_enable[byte] = makeBTerm(in_name);
  }

  vector<dbNet*> select(read_ports, nullptr);
  for (int port = 0; port < read_ports; ++port) {
    select[port] = makeBTerm(fmt::format("select{}", port));
  }

  for (int col = 0; col < bytes_per_word; ++col) {
    array<dbNet*, 8> Di0;
    for (int bit = 0; bit < 8; ++bit) {
      Di0[bit] = makeBTerm(fmt::format("Di0[{}]", bit + col * 8));
    }

    vector<array<dbNet*, 8>> Do;
    for (int read_port = 0; read_port < read_ports; ++read_port) {
      array<dbNet*,8> d;
      for (int bit = 0; bit < 8; ++bit) {
        auto out_name = fmt::format("Do{}[{}]", read_port, bit + col * 8);
        d[bit] = makeBTerm(out_name);
      }
      Do.push_back(d);
    }

    auto column = std::make_unique<Layout>(odb::vertical);
    for (int row = 0; row < word_count; ++row) {
      auto name = fmt::format("storage_{}_{}", row, col);
      column->addElement(make_byte(name,
                                   read_ports,
                                   clock,
                                   write_enable[col],
                                   select,
                                   Di0,
                                   Do));
    }
    layout.addElement(std::make_unique<Element>(std::move(column)));
  }
  layout.position(odb::Point(0, 0));
}

}  // namespace ram
