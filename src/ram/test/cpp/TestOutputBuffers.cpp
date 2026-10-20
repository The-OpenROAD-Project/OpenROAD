// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "ram/ram.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "tst/fixture.h"
#include "tst/logger_checks.h"

namespace ram {
namespace {

// Evaluate the generated connectivity, not the behavioral Verilog. The only
// state models are Nangate45 DFF_X1/DLH_X1 and Sky130 dfxtp; combinational and
// tristate logic comes from Liberty. Enables are stable across clock edges.
class RamCircuit
{
 public:
  RamCircuit(odb::dbBlock* block, sta::dbNetwork* network)
      : block_(block), network_(network)
  {
    for (auto term : block_->getBTerms()) {
      if (term->getIoType() == odb::dbIoType::INPUT) {
        inputs_[terminalNet(term->getName(), odb::dbIoType::INPUT)] = false;
      }
    }
    for (auto inst : block_->getInsts()) {
      const auto name = inst->getMaster()->getName();
      has_latches_ |= name == "DLH_X1";
      if (name == "DFF_X1" || name == "DLH_X1"
          || name == "sky130_fd_sc_hd__dfxtp_1") {
        state_[inst] = false;
      }
    }
  }

  void drive(const std::string& name, bool value)
  {
    inputs_[terminalNet(name, odb::dbIoType::INPUT)] = value;
    cache_.clear();
    settleLatches();
  }

  void address(const std::string& prefix, int word)
  {
    for (int bit = 0;; ++bit) {
      const auto name = prefix + "[" + std::to_string(bit) + "]";
      if (!block_->findBTerm(name.c_str())) {
        break;
      }
      drive(name, (word >> bit) & 1);
    }
  }

  void clock()
  {
    drive("clk", true);
    auto next = state_;
    for (const auto& [inst, value] : state_) {
      if (inst->getMaster()->getName() == "DLH_X1") {
        continue;
      }
      const char* clock_pin
          = inst->getMaster()->getName() == "DFF_X1" ? "CK" : "CLK";
      auto clock = pinNet(inst, clock_pin);
      auto data = pinNet(inst, "D");
      if (read(clock)) {
        next[inst] = read(data);
      }
    }
    state_ = std::move(next);
    cache_.clear();
    drive("clk", false);
  }

  bool output(const std::string& name)
  {
    return read(terminalNet(name, odb::dbIoType::OUTPUT));
  }

 private:
  odb::dbNet* terminalNet(const std::string& name, odb::dbIoType direction)
  {
    const std::string role
        = direction == odb::dbIoType::INPUT ? "input" : "output";
    auto term = block_->findBTerm(name.c_str());
    if (!term) {
      throw std::runtime_error("Missing " + role + " terminal " + name);
    }
    if (term->getIoType() != direction) {
      throw std::runtime_error("Wrong direction for " + role + " terminal "
                               + name);
    }
    auto net = term->getNet();
    if (!net) {
      throw std::runtime_error("Unconnected " + role + " terminal " + name);
    }
    return net;
  }

  odb::dbNet* pinNet(odb::dbInst* inst, const std::string& name)
  {
    const auto label = inst->getName() + "." + name;
    auto pin = inst->findITerm(name.c_str());
    if (!pin) {
      throw std::runtime_error("Missing pin " + label);
    }
    auto net = pin->getNet();
    if (!net) {
      throw std::runtime_error("Unconnected pin " + label);
    }
    return net;
  }

  void settleLatches()
  {
    if (!has_latches_) {
      return;
    }
    // Resolve both phases of the generated latch RAM until all open latches
    // agree with their inputs. Closed latches keep their previous state.
    for (size_t iteration = 0; iteration <= state_.size(); ++iteration) {
      bool changed = false;
      auto next = state_;
      for (const auto& [inst, value] : state_) {
        if (inst->getMaster()->getName() != "DLH_X1") {
          continue;
        }
        auto gate = pinNet(inst, "G");
        auto input = pinNet(inst, "D");
        if (read(gate)) {
          const bool data = read(input);
          next[inst] = data;
          changed |= data != value;
        }
      }
      state_ = std::move(next);
      cache_.clear();
      if (!changed) {
        return;
      }
    }
    throw std::runtime_error("Latch RAM did not settle");
  }

  bool clockGate(odb::dbInst* inst)
  {
    const bool nangate = inst->findITerm("CK") != nullptr;
    auto clock = pinNet(inst, nangate ? "CK" : "CLK");
    auto enable = pinNet(inst, nangate ? "E" : "GATE");
    return read(clock) && read(enable);
  }

  bool eval(const sta::FuncExpr* expr, odb::dbInst* inst)
  {
    if (!expr) {
      throw std::runtime_error("Missing Liberty function for "
                               + inst->getName());
    }
    switch (expr->op()) {
      case sta::FuncExpr::Op::port:
        return read(pinNet(inst, expr->port()->name()));
      case sta::FuncExpr::Op::not_:
        return !eval(expr->left(), inst);
      case sta::FuncExpr::Op::and_:
        return eval(expr->left(), inst) && eval(expr->right(), inst);
      case sta::FuncExpr::Op::or_:
        return eval(expr->left(), inst) || eval(expr->right(), inst);
      case sta::FuncExpr::Op::xor_:
        return eval(expr->left(), inst) != eval(expr->right(), inst);
      case sta::FuncExpr::Op::one:
        return true;
      case sta::FuncExpr::Op::zero:
        return false;
      default:
        throw std::runtime_error("Unsupported Liberty function");
    }
  }

  bool read(odb::dbNet* net)
  {
    if (const auto it = inputs_.find(net); it != inputs_.end()) {
      return it->second;
    }
    if (const auto it = cache_.find(net); it != cache_.end()) {
      return it->second;
    }
    int drivers = 0;
    bool value = false;
    for (auto term : net->getITerms()) {
      if (term->getIoType() != odb::dbIoType::OUTPUT) {
        continue;
      }
      auto inst = term->getInst();
      auto sta_cell = network_->dbToSta(inst->getMaster());
      auto liberty = sta_cell ? network_->libertyCell(sta_cell) : nullptr;
      if (!liberty) {
        throw std::runtime_error("Missing Liberty cell for " + inst->getName()
                                 + " (" + inst->getMaster()->getName() + ")");
      }
      auto port = liberty->findLibertyPort(term->getMTerm()->getConstName());
      if (!port) {
        throw std::runtime_error("Missing Liberty port " + inst->getName() + "."
                                 + term->getMTerm()->getName());
      }
      // OpenSTA has already inverted Liberty's three_state disable function.
      if (port->tristateEnable() && !eval(port->tristateEnable(), inst)) {
        continue;
      }
      ++drivers;
      if (const auto it = state_.find(inst); it != state_.end()) {
        value = it->second;
      } else if (liberty->isClockGate()) {
        // Model the gate's latched enable with inputs held stable at the edge.
        value = clockGate(inst);
      } else {
        value = eval(port->function(), inst);
      }
    }
    if (drivers != 1) {
      throw std::runtime_error("Expected one active driver, found "
                               + std::to_string(drivers) + " on "
                               + net->getName());
    }
    cache_[net] = value;
    return value;
  }

  odb::dbBlock* block_;
  sta::dbNetwork* network_;
  std::unordered_map<odb::dbNet*, bool> inputs_;
  std::unordered_map<odb::dbNet*, bool> cache_;
  std::unordered_map<odb::dbInst*, bool> state_;
  bool has_latches_ = false;
};

template <typename Action>
void expectModelError(const Action& action, const std::string& message)
{
  try {
    action();
    ADD_FAILURE() << "Expected model error: " << message;
  } catch (const std::runtime_error& error) {
    EXPECT_EQ(std::string(error.what()), message);
  }
}

struct Configuration
{
  int words;
  int bits;
  int mask;
  int mux;
  int rw;
  int reads;
  const char* buffer;
  bool latch = false;
};

struct PhysicalPin
{
  const char* name;
  odb::dbIoType direction;
  odb::dbSigType type = odb::dbSigType::SIGNAL;
};

class OutputBufferTest : public tst::Fixture,
                         public testing::WithParamInterface<Configuration>
{
 protected:
  OutputBufferTest()
  {
    loadTechAndLib("ng45", "ng45", "_main/test/Nangate45/Nangate45.lef");
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    generator_ = std::make_unique<RamGen>(sta_->getDbNetwork(),
                                          db_.get(),
                                          &logger_,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr);
  }

  void generate(const Configuration& config)
  {
    generator_->generate(config.mask,
                         config.bits,
                         config.words,
                         config.rw,
                         config.reads,
                         1 - config.rw,
                         config.mux,
                         config.latch,
                         db_->findMaster(config.latch ? "DLH_X1" : "DFF_X1"),
                         nullptr,
                         nullptr,
                         nullptr,
                         0,
                         config.buffer);
  }

  odb::dbMaster* makePhysicalBuffer(const std::vector<PhysicalPin>& pins
                                    = {{"A", odb::dbIoType::INPUT},
                                       {"Z", odb::dbIoType::OUTPUT}},
                                    bool with_site = true)
  {
    auto original = db_->findMaster("BUF_X4");
    auto lib = original->getLib();
    const auto name
        = "physical_buffer_" + std::to_string(lib->getMasters().size());
    auto master = odb::dbMaster::create(lib, name.c_str());
    master->setType(odb::dbMasterType::CORE);
    master->setWidth(original->getWidth());
    master->setHeight(original->getHeight());
    if (with_site) {
      master->setSite(original->getSite());
    }
    // Keep valid Liberty data while independently varying the physical view.
    master->staSetCell(original->staCell());
    odb::dbMTerm::create(
        master, "VDD", odb::dbIoType::INOUT, odb::dbSigType::POWER);
    odb::dbMTerm::create(
        master, "VSS", odb::dbIoType::INOUT, odb::dbSigType::GROUND);
    for (const auto& pin : pins) {
      odb::dbMTerm::create(master, pin.name, pin.direction, pin.type);
    }
    master->setFrozen();
    return master;
  }

  odb::dbMaster* makeStorageMaster(bool with_site = true)
  {
    auto original = db_->findMaster("DFF_X1");
    auto master = odb::dbMaster::create(original->getLib(), "physical_storage");
    master->setType(odb::dbMasterType::CORE);
    master->setWidth(original->getWidth());
    master->setHeight(original->getHeight());
    master->staSetCell(original->staCell());
    if (with_site) {
      master->setSite(original->getSite());
    }
    for (auto pin : original->getMTerms()) {
      odb::dbMTerm::create(
          master, pin->getConstName(), pin->getIoType(), pin->getSigType());
    }
    master->setFrozen();
    return master;
  }

  odb::dbMaster* makeModelFlop(const std::vector<PhysicalPin>& pins)
  {
    auto original = db_->findMaster("DFF_X1");
    auto lib = odb::dbLib::create(db_.get(), "model", db_->getTech());
    auto master = odb::dbMaster::create(lib, "DFF_X1");
    master->setWidth(original->getWidth());
    master->setHeight(original->getHeight());
    master->setSite(original->getSite());
    master->staSetCell(original->staCell());
    for (const auto& pin : pins) {
      odb::dbMTerm::create(master, pin.name, pin.direction, pin.type);
    }
    master->setFrozen();
    return master;
  }

  void expectRejectedStorage(odb::dbMaster* master, const char* buffer)
  {
    ASSERT_EQ(db_->getChip(), nullptr);
    EXPECT_LOGGER_ERROR((generator_->generate(1,
                                              1,
                                              2,
                                              1,
                                              0,
                                              0,
                                              1,
                                              false,
                                              master,
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              0,
                                              buffer)),
                        "RAM-0065");
    EXPECT_EQ(db_->getChip(), nullptr);
  }

  void expectRejectedBuffer(odb::dbMaster* master, const char* message)
  {
    const auto name = master->getName();
    EXPECT_LOGGER_ERROR((generate({2, 1, 1, 1, 1, 0, name.c_str()})), message);
    ASSERT_EQ(db_->getChip(), nullptr);
  }

  void checkPlacement(odb::dbBlock* block)
  {
    const auto die = block->getDieArea();
    auto site = (*block->getRows().begin())->getSite();
    std::map<int, std::vector<odb::Rect>> rows;
    for (auto inst : block->getInsts()) {
      const auto box = inst->getBBox()->getBox();
      EXPECT_TRUE(inst->isPlaced()) << inst->getName();
      EXPECT_GE(box.xMin(), die.xMin()) << inst->getName();
      EXPECT_GE(box.yMin(), die.yMin()) << inst->getName();
      EXPECT_LE(box.xMax(), die.xMax()) << inst->getName();
      EXPECT_LE(box.yMax(), die.yMax()) << inst->getName();
      EXPECT_EQ(box.xMin() % site->getWidth(), 0);
      EXPECT_EQ(box.yMin() % site->getHeight(), 0);
      rows[box.yMin()].push_back(box);
    }
    for (auto& [y, boxes] : rows) {
      std::ranges::sort(boxes, [](const auto& a, const auto& b) {
        return a.xMin() < b.xMin();
      });
      for (int i = 1; i < boxes.size(); ++i) {
        EXPECT_LE(boxes[i - 1].xMax(), boxes[i].xMin()) << "row " << y;
      }
    }
  }

  std::unique_ptr<RamGen> generator_;
};

TEST_P(OutputBufferTest, TopologyPlacementAndMaskedMemory)
{
  const auto config = GetParam();
  generate(config);
  auto block = db_->getChip()->getBlock();
  const int ports = config.rw + config.reads;
  const bool buffered = std::string(config.buffer) != "none";
  int buffer_count = 0;
  for (auto inst : block->getInsts()) {
    buffer_count += inst->getName().starts_with("output_buffer.");
  }
  ASSERT_EQ(buffer_count, buffered ? ports * config.bits : 0);
  for (int port = 0; port < ports; ++port) {
    for (int bit = 0; bit < config.bits; ++bit) {
      const auto suffix
          = "p" + std::to_string(port) + "_b" + std::to_string(bit);
      auto buffer = block->findInst(("output_buffer." + suffix).c_str());
      const auto out_name = (ports == 1 ? "Q" : "Q" + std::to_string(port))
                            + "[" + std::to_string(bit) + "]";
      auto output_net = block->findBTerm(out_name.c_str())->getNet();
      if (buffered) {
        ASSERT_NE(buffer, nullptr);
        EXPECT_EQ(output_net->getITerms().size(), 1);
        EXPECT_EQ(buffer->findITerm("Z")->getNet(), output_net);
        auto internal = buffer->findITerm("A")->getNet();
        EXPECT_NE(internal, output_net);
        EXPECT_EQ(internal->getITermCount(),
                  (config.mux == 1 ? config.words : 1) + 1);
        EXPECT_TRUE(internal->getBTerms().empty());
        EXPECT_EQ(internal, block->findNet(("read." + suffix).c_str()));
        auto input = block->findInst(
            ("buffer.in[" + std::to_string(bit) + "]").c_str());
        EXPECT_EQ(buffer->getBBox()->yMin(), input->getBBox()->yMin());
        if (std::string(config.buffer).empty()) {
          EXPECT_EQ(buffer->getMaster(), input->getMaster());
        } else {
          EXPECT_EQ(buffer->getMaster()->getName(), config.buffer);
        }
      } else {
        EXPECT_EQ(buffer, nullptr);
        EXPECT_EQ(block->findNet(("read." + suffix).c_str()), nullptr);
        EXPECT_EQ(output_net->getITerms().size(),
                  config.mux == 1 ? config.words : 1);
      }
    }
  }
  if (config.mux > 1) {
    for (int row = 0; row < config.words / config.mux; ++row) {
      for (int port = 0; port < ports; ++port) {
        const auto name = "row" + std::to_string(row) + ".select"
                          + std::to_string(port) + "_b";
        auto select = block->findNet(name.c_str());
        ASSERT_NE(select, nullptr);
        int drivers = 0;
        for (auto term : select->getITerms()) {
          drivers += term->getIoType() == odb::dbIoType::OUTPUT;
        }
        EXPECT_EQ(drivers, 1) << name;
        EXPECT_EQ(select->getITermCount(), config.bits * config.mux + 1)
            << name;
      }
    }
  }

  checkPlacement(block);

  RamCircuit circuit(block, sta_->getDbNetwork());
  std::mt19937_64 random(9392);
  std::vector<uint64_t> memory(config.words, 0);
  const auto write_address = config.rw ? "addr_rw" : "addr_w";
  // First initialize every word, then alternate write masks and verify that
  // disabled slices retain their data. Scan every read port after each pass.
  for (int pass = 0; pass < 3; ++pass) {
    for (int word = 0; word < config.words; ++word) {
      circuit.address(write_address, word);
      const auto data = random();
      for (int bit = 0; bit < config.bits; ++bit) {
        circuit.drive("D[" + std::to_string(bit) + "]", (data >> bit) & 1);
        const int slice = bit / config.mask;
        const bool enabled = pass == 0 || (slice + pass) % 2 == 0;
        circuit.drive("we[" + std::to_string(slice) + "]", enabled);
        if (enabled) {
          const auto mask = uint64_t{1} << bit;
          memory[word] = (memory[word] & ~mask) | (data & mask);
        }
      }
      circuit.clock();
    }
    for (int word = 0; word < config.words; ++word) {
      for (int port = 0; port < ports; ++port) {
        const auto address = port < config.rw
                                 ? "addr_rw"
                                 : "addr_r" + std::to_string(port - config.rw);
        const int read_word = (word + port) % config.words;
        circuit.address(address, read_word);
        for (int bit = 0; bit < config.bits; ++bit) {
          const auto output = (ports == 1 ? "Q" : "Q" + std::to_string(port))
                              + "[" + std::to_string(bit) + "]";
          EXPECT_EQ(circuit.output(output), (memory[read_word] >> bit) & 1)
              << "pass=" << pass << " word=" << read_word << " bit=" << bit
              << " port=" << port;
        }
      }
    }
  }
}

std::vector<Configuration> configurations()
{
  std::vector<Configuration> result;
  for (const auto buffer : {"", "BUF_X4", "none"}) {
    for (const auto config :
         std::vector<Configuration>{{2, 1, 1, 1, 1, 0, buffer},
                                    {7, 7, 1, 1, 0, 2, buffer},
                                    {8, 8, 2, 2, 1, 0, buffer},
                                    {16, 5, 1, 4, 1, 0, buffer},
                                    {8, 8, 2, 1, 1, 0, buffer, true},
                                    {31, 13, 1, 1, 1, 1, buffer},
                                    {64, 64, 8, 1, 0, 2, buffer}}) {
      result.push_back(config);
    }
  }
  return result;
}

INSTANTIATE_TEST_SUITE_P(Dimensions,
                         OutputBufferTest,
                         testing::ValuesIn(configurations()));

class MultiportOutputBufferTest : public OutputBufferTest
{
};

TEST_P(MultiportOutputBufferTest, ReadMuxOutputIsolation)
{
  // Test this stage independently of the pre-existing low-address decoder
  // limitations in multiport column-mux configurations.
  const auto config = GetParam();
  generate(config);
  auto block = db_->getChip()->getBlock();
  std::set<uint32_t> read_nets;
  const int ports = config.rw + config.reads;
  int count = 0;
  for (auto inst : block->getInsts()) {
    count += inst->getName().starts_with("output_buffer.");
  }
  EXPECT_EQ(count, ports * config.bits);
  for (int port = 0; port < ports; ++port) {
    for (int bit = 0; bit < config.bits; ++bit) {
      const auto suffix
          = "p" + std::to_string(port) + "_b" + std::to_string(bit);
      auto buffer = block->findInst(("output_buffer." + suffix).c_str());
      ASSERT_NE(buffer, nullptr);
      EXPECT_TRUE(buffer->isPlaced());
      EXPECT_EQ(buffer->getMaster()->getName(), "BUF_X4");
      auto read_net = block->findNet(("read." + suffix).c_str());
      ASSERT_NE(read_net, nullptr);
      EXPECT_TRUE(read_nets.insert(read_net->getId()).second);
      EXPECT_EQ(read_net->getITermCount(), 2);
      EXPECT_EQ(read_net->getBTermCount(), 0);
      EXPECT_EQ(buffer->findITerm("A")->getNet(), read_net);
      const auto mux_name = "mux_slice0_bit" + std::to_string(bit) + "_p"
                            + std::to_string(port)
                            + (config.mux == 2 ? ".inv" : ".s2_aoi");
      auto mux = block->findInst(mux_name.c_str());
      ASSERT_NE(mux, nullptr);
      EXPECT_EQ(mux->findITerm("ZN")->getNet(), read_net);
      const auto output_name
          = "Q" + std::to_string(port) + "[" + std::to_string(bit) + "]";
      auto output = block->findBTerm(output_name.c_str())->getNet();
      EXPECT_EQ(output->getITermCount(), 1);
      EXPECT_EQ(buffer->findITerm("Z")->getNet(), output);
      EXPECT_NE(output, read_net);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    ColumnMux,
    MultiportOutputBufferTest,
    testing::Values(Configuration{16, 4, 4, 2, 1, 1, "BUF_X4"},
                    Configuration{16, 4, 4, 4, 0, 2, "BUF_X4"}));

TEST_F(OutputBufferTest, SimulatorReportsMissingOutputTerminal)
{
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "missing_output");
  tst::Fixture::makeBTerm(block, "D");
  RamCircuit circuit(block, sta_->getDbNetwork());
  expectModelError([&] { circuit.output("Q"); }, "Missing output terminal Q");
  expectModelError([&] { circuit.output("D"); },
                   "Wrong direction for output terminal D");
  expectModelError([&] { circuit.drive("absent", false); },
                   "Missing input terminal absent");
}

TEST_F(OutputBufferTest, SimulatorReportsDisconnectedTerminals)
{
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "disconnected_terminals");
  auto input = tst::Fixture::makeBTerm(block, "D");
  auto output
      = tst::Fixture::makeBTerm(block, "Q", {.io_type = odb::dbIoType::OUTPUT});
  RamCircuit circuit(block, sta_->getDbNetwork());
  output->disconnect();
  input->disconnect();
  expectModelError([&] { circuit.output("Q"); },
                   "Unconnected output terminal Q");
  expectModelError([&] { circuit.drive("D", false); },
                   "Unconnected input terminal D");
}

TEST_F(OutputBufferTest, SimulatorReportsMissingClockPin)
{
  auto master = makeModelFlop(
      {{"D", odb::dbIoType::INPUT}, {"Q", odb::dbIoType::OUTPUT}});
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "missing_clock");
  tst::Fixture::makeBTerm(block, "clk");
  tst::Fixture::makeBTerm(block, "D");
  tst::Fixture::makeInst(block, master, "ff", {.iterms = {{"D", "D"}}});
  RamCircuit circuit(block, sta_->getDbNetwork());
  expectModelError([&] { circuit.clock(); }, "Missing pin ff.CK");
}

TEST_F(OutputBufferTest, SimulatorReportsMissingDataPin)
{
  auto master = makeModelFlop(
      {{"CK", odb::dbIoType::INPUT}, {"Q", odb::dbIoType::OUTPUT}});
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "missing_data");
  tst::Fixture::makeBTerm(block, "clk");
  tst::Fixture::makeInst(block, master, "ff", {.iterms = {{"clk", "CK"}}});
  RamCircuit circuit(block, sta_->getDbNetwork());
  expectModelError([&] { circuit.clock(); }, "Missing pin ff.D");
}

TEST_F(OutputBufferTest, SimulatorReportsDisconnectedPin)
{
  auto master = makePhysicalBuffer();
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "disconnected_pin");
  tst::Fixture::makeBTerm(block, "Q", {.io_type = odb::dbIoType::OUTPUT});
  tst::Fixture::makeInst(block, master, "buffer", {.iterms = {{"Q", "Z"}}});
  RamCircuit circuit(block, sta_->getDbNetwork());
  expectModelError([&] { circuit.output("Q"); }, "Unconnected pin buffer.A");
}

TEST_F(OutputBufferTest, SimulatorReportsMissingFunctionPin)
{
  auto master = makePhysicalBuffer({{"Z", odb::dbIoType::OUTPUT}});
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "missing_function_pin");
  tst::Fixture::makeBTerm(block, "Q", {.io_type = odb::dbIoType::OUTPUT});
  tst::Fixture::makeInst(block, master, "buffer", {.iterms = {{"Q", "Z"}}});
  RamCircuit circuit(block, sta_->getDbNetwork());
  expectModelError([&] { circuit.output("Q"); }, "Missing pin buffer.A");
}

TEST_F(OutputBufferTest, SimulatorReportsMissingLibertyCell)
{
  auto master = makePhysicalBuffer();
  master->staSetCell(nullptr);
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "missing_liberty_cell");
  tst::Fixture::makeBTerm(block, "Q", {.io_type = odb::dbIoType::OUTPUT});
  tst::Fixture::makeInst(block, master, "buffer", {.iterms = {{"Q", "Z"}}});
  RamCircuit circuit(block, sta_->getDbNetwork());
  expectModelError(
      [&] { circuit.output("Q"); },
      "Missing Liberty cell for buffer (" + master->getName() + ")");
}

TEST_F(OutputBufferTest, SimulatorReportsMissingLibertyPort)
{
  auto master = makePhysicalBuffer({{"UNKNOWN", odb::dbIoType::OUTPUT}});
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "missing_liberty_port");
  tst::Fixture::makeBTerm(block, "Q", {.io_type = odb::dbIoType::OUTPUT});
  tst::Fixture::makeInst(
      block, master, "buffer", {.iterms = {{"Q", "UNKNOWN"}}});
  RamCircuit circuit(block, sta_->getDbNetwork());
  expectModelError([&] { circuit.output("Q"); },
                   "Missing Liberty port buffer.UNKNOWN");
}

TEST_F(OutputBufferTest, SimulatorResolvesActiveLowTristateBus)
{
  auto chip = odb::dbChip::create(db_.get(), db_->getTech());
  auto block = odb::dbBlock::create(chip, "tristate_bus");
  for (const auto name : {"D0", "D1", "E0", "E1"}) {
    tst::Fixture::makeBTerm(block, name);
  }
  tst::Fixture::makeBTerm(block, "Q", {.io_type = odb::dbIoType::OUTPUT});
  auto buffer = db_->findMaster("TBUF_X1");
  tst::Fixture::makeInst(
      block, buffer, "t0", {.iterms = {{"D0", "A"}, {"E0", "EN"}, {"Q", "Z"}}});
  tst::Fixture::makeInst(
      block, buffer, "t1", {.iterms = {{"D1", "A"}, {"E1", "EN"}, {"Q", "Z"}}});
  RamCircuit circuit(block, sta_->getDbNetwork());
  circuit.drive("D0", false);
  circuit.drive("D1", true);
  circuit.drive("E0", true);
  circuit.drive("E1", false);
  EXPECT_TRUE(circuit.output("Q"));
  circuit.drive("E0", false);
  circuit.drive("E1", true);
  EXPECT_FALSE(circuit.output("Q"));
  circuit.drive("E0", true);
  EXPECT_THROW(circuit.output("Q"), std::runtime_error);
  circuit.drive("E0", false);
  circuit.drive("E1", false);
  EXPECT_THROW(circuit.output("Q"), std::runtime_error);
}

TEST_F(OutputBufferTest, RejectMissingStorageSiteBeforeCreatingDesign)
{
  auto storage = makeStorageMaster(false);
  for (const auto buffer : {"none", ""}) {
    expectRejectedStorage(storage, buffer);
  }
}

TEST_F(OutputBufferTest, RejectInvalidStorageWidthBeforeCreatingDesign)
{
  auto storage = makeStorageMaster();
  for (const auto width : {0U, storage->getWidth() + 1}) {
    storage->setWidth(width);
    for (const auto buffer : {"none", ""}) {
      expectRejectedStorage(storage, buffer);
    }
  }
}

TEST_F(OutputBufferTest, RejectStorageHeightMismatchBeforeCreatingDesign)
{
  auto storage = makeStorageMaster();
  storage->setHeight(storage->getHeight() * 2);
  for (const auto buffer : {"none", ""}) {
    expectRejectedStorage(storage, buffer);
  }
}

TEST_F(OutputBufferTest, RejectZeroStorageSiteDimensionsBeforeCreatingDesign)
{
  auto storage = makeStorageMaster();
  auto site = storage->getSite();
  const auto width = site->getWidth();
  site->setWidth(0);
  for (const auto buffer : {"none", ""}) {
    expectRejectedStorage(storage, buffer);
  }
  site->setWidth(width);
  site->setHeight(0);
  for (const auto buffer : {"none", ""}) {
    expectRejectedStorage(storage, buffer);
  }
}

TEST_F(OutputBufferTest, RejectMissingMasterBeforeCreatingDesign)
{
  EXPECT_LOGGER_ERROR((generate({2, 1, 1, 1, 1, 0, "missing_buffer"})),
                      "RAM-0060");
  EXPECT_EQ(db_->getChip(), nullptr);
}

TEST_F(OutputBufferTest, RejectInvertingMasterBeforeCreatingDesign)
{
  EXPECT_LOGGER_ERROR((generate({2, 1, 1, 1, 1, 0, "INV_X1"})), "RAM-0061");
  EXPECT_EQ(db_->getChip(), nullptr);
}

TEST_F(OutputBufferTest, RejectIncompatibleHeightBeforeCreatingDesign)
{
  auto buffer = db_->findMaster("BUF_X4");
  buffer->setHeight(buffer->getHeight() * 2);
  EXPECT_LOGGER_ERROR((generate({2, 1, 1, 1, 1, 0, "BUF_X4"})), "RAM-0063");
  EXPECT_EQ(db_->getChip(), nullptr);
}

TEST_F(OutputBufferTest, RejectMissingSiteBeforeCreatingDesign)
{
  auto master = makePhysicalBuffer(
      {{"A", odb::dbIoType::INPUT}, {"Z", odb::dbIoType::OUTPUT}}, false);
  expectRejectedBuffer(master, "RAM-0063");
}

TEST_F(OutputBufferTest, RejectDifferentSiteWithIdenticalDimensions)
{
  auto master = makePhysicalBuffer();
  auto storage_site = db_->findMaster("DFF_X1")->getSite();
  auto site = odb::dbSite::create(master->getLib(), "wrong_site");
  site->setWidth(storage_site->getWidth());
  site->setHeight(storage_site->getHeight());
  master->setSite(site);
  expectRejectedBuffer(master, "RAM-0063");
}

TEST_F(OutputBufferTest, RejectNonCorePhysicalMasters)
{
  for (const auto type : {odb::dbMasterType::BLOCK,
                          odb::dbMasterType::PAD,
                          odb::dbMasterType::COVER,
                          odb::dbMasterType::ENDCAP}) {
    auto master = makePhysicalBuffer();
    master->setType(type);
    expectRejectedBuffer(master, "RAM-0063");
  }
}

TEST_F(OutputBufferTest, RejectMissingPhysicalSignalPins)
{
  expectRejectedBuffer(makePhysicalBuffer({{"A", odb::dbIoType::INPUT}}),
                       "RAM-0064");
  expectRejectedBuffer(makePhysicalBuffer({{"Z", odb::dbIoType::OUTPUT}}),
                       "RAM-0064");
}

TEST_F(OutputBufferTest, RejectExtraPhysicalSignalPin)
{
  expectRejectedBuffer(makePhysicalBuffer({{"A", odb::dbIoType::INPUT},
                                           {"Z", odb::dbIoType::OUTPUT},
                                           {"EXTRA", odb::dbIoType::INPUT}}),
                       "RAM-0064");
}

TEST_F(OutputBufferTest, RejectWrongPhysicalDirections)
{
  for (const auto direction :
       {odb::dbIoType::INPUT, odb::dbIoType::OUTPUT, odb::dbIoType::INOUT}) {
    if (direction != odb::dbIoType::INPUT) {
      expectRejectedBuffer(
          makePhysicalBuffer({{"A", direction}, {"Z", odb::dbIoType::OUTPUT}}),
          "RAM-0064");
    }
    if (direction != odb::dbIoType::OUTPUT) {
      expectRejectedBuffer(
          makePhysicalBuffer({{"A", odb::dbIoType::INPUT}, {"Z", direction}}),
          "RAM-0064");
    }
  }
}

TEST_F(OutputBufferTest, RejectSupplyTypesOnPhysicalSignalPins)
{
  for (const auto type : {odb::dbSigType::POWER, odb::dbSigType::GROUND}) {
    expectRejectedBuffer(makePhysicalBuffer({{"A", odb::dbIoType::INPUT, type},
                                             {"Z", odb::dbIoType::OUTPUT}}),
                         "RAM-0064");
    expectRejectedBuffer(
        makePhysicalBuffer(
            {{"A", odb::dbIoType::INPUT}, {"Z", odb::dbIoType::OUTPUT, type}}),
        "RAM-0064");
  }
}

class Sky130OutputBufferTest : public tst::Fixture
{
 protected:
  Sky130OutputBufferTest()
  {
    const std::string prefix = "_main/src/ram/test/sky130hd/";
    auto sites = loadTechAndLib("sky130", "sites", prefix + "sky130hd.tlef");
    loadLibaryLef(
        sites->getTech(), "sky130", prefix + "sky130_fd_sc_hd_merged.lef");
    readLiberty(prefix + "sky130_fd_sc_hd__tt_025C_1v80.lib");
  }
};

TEST_F(Sky130OutputBufferTest, DefaultBuffersAndMemoryBehavior)
{
  RamGen generator(sta_->getDbNetwork(),
                   db_.get(),
                   &logger_,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr);
  auto storage = db_->findMaster("sky130_fd_sc_hd__dfxtp_1");
  ASSERT_NE(storage, nullptr);
  ASSERT_NE(storage->getSite(), nullptr);
  generator.generate(
      2, 8, 8, 1, 0, 0, 1, false, storage, nullptr, nullptr, nullptr, 0);
  auto block = db_->getChip()->getBlock();
  auto row_site = (*block->getRows().begin())->getSite();
  EXPECT_EQ(row_site, storage->getSite());
  int count = 0;
  for (int bit = 0; bit < 8; ++bit) {
    const auto suffix = "p0_b" + std::to_string(bit);
    auto buffer = block->findInst(("output_buffer." + suffix).c_str());
    ASSERT_NE(buffer, nullptr);
    ++count;
    auto input
        = block->findInst(("buffer.in[" + std::to_string(bit) + "]").c_str());
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(buffer->getMaster(), input->getMaster());
    EXPECT_EQ(buffer->getMaster()->getSite(), row_site);
    EXPECT_EQ(buffer->getMaster()->getType(), odb::dbMasterType::CORE);
    auto internal = block->findNet(("read." + suffix).c_str());
    auto output = block->findBTerm(("Q[" + std::to_string(bit) + "]").c_str())
                      ->getNet();
    EXPECT_EQ(internal->getITermCount(), 9);
    EXPECT_EQ(output->getITermCount(), 1);
    EXPECT_TRUE(internal->getBTerms().empty());
    EXPECT_EQ((*output->getITerms().begin())->getInst(), buffer);
    const auto box = buffer->getBBox()->getBox();
    EXPECT_EQ(box.yMin(), input->getBBox()->yMin());
    EXPECT_EQ(box.xMin() % row_site->getWidth(), 0);
    EXPECT_LE(box.xMax(), block->getDieArea().xMax());
    EXPECT_LE(box.yMax(), block->getDieArea().yMax());
  }
  EXPECT_EQ(count, 8);

  RamCircuit circuit(block, sta_->getDbNetwork());
  for (int word = 0; word < 8; ++word) {
    circuit.address("addr_rw", word);
    for (int bit = 0; bit < 8; ++bit) {
      circuit.drive("D[" + std::to_string(bit) + "]", ((word * 37) >> bit) & 1);
    }
    for (int slice = 0; slice < 4; ++slice) {
      circuit.drive("we[" + std::to_string(slice) + "]", true);
    }
    circuit.clock();
  }
  for (int word = 0; word < 8; ++word) {
    circuit.address("addr_rw", word);
    for (int bit = 0; bit < 8; ++bit) {
      EXPECT_EQ(circuit.output("Q[" + std::to_string(bit) + "]"),
                ((word * 37) >> bit) & 1)
          << "word=" << word << " bit=" << bit;
    }
  }
}

}  // namespace
}  // namespace ram
