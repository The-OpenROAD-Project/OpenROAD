#include "ram/ram.h"
#include "layout.h"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <bit>
#include "sta/FuncExpr.hh"
#include <cmath>
#include "db_sta/dbNetwork.hh"

namespace ram {

using odb::dbBlock;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbMaster;
using odb::dbNet;
using sta::FuncExpr;
using sta::LibertyCell;
using sta::PortDirection;

RamGen::RamGen()
  : db_(nullptr),
    block_(nullptr),
    network_(nullptr),
    logger_(nullptr),
    storage_cell_(nullptr),
    tristate_cell_(nullptr),
    inv_cell_(nullptr),
    and2_cell_(nullptr),
    clock_gate_cell_(nullptr)
{
}


void RamGen::init(odb::dbDatabase* db, sta::dbNetwork* network, Logger* logger)
{
  db_ = db;
  network_ = network;
  logger_ = logger;
}

//----------------------------------------------
// findMaster and findMasters
//----------------------------------------------
odb::dbMaster* RamGen::findMaster(
    const std::function<bool(sta::LibertyPort*)>& match,
    const char* name)
{
  dbMaster* best = nullptr;
  float best_area = std::numeric_limits<float>::max();

  for (auto lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      auto cell = network_->dbToSta(master);
      if (!cell) continue;

      auto liberty = network_->libertyCell(cell);
      if (!liberty) continue;

      sta::LibertyCellPortIterator port_iter(liberty);
      sta::LibertyPort* out_port = nullptr;
      bool reject = false;

      while (port_iter.hasNext()) {
        auto port = port_iter.next();
        if (port->direction()->isAnyOutput()) {
          if (!out_port) {
            out_port = port;
          } else {
            reject = true;
            break;
          }
        }
      }

      if (!reject && out_port && match(out_port)) {
        if (liberty->area() < best_area) {
          best_area = liberty->area();
          best = master;
        }
      }
    }
  }

  if (!best)
    logger_->error(utl::RAM, 900, "Could not find {}", name);
  else
    logger_->info(utl::RAM, 916, "Selected {}: {}", name, best->getName());
  return best;
}

void RamGen::findMasters()
{
  auto find = [&](const std::function<bool(sta::LibertyPort*)>& match,
                  const char* name, odb::dbMaster** target) {
    if (!*target) *target = findMaster(match, name);
  };

  // Inverter
  find([](sta::LibertyPort* port) {
    return port->libertyCell()->isInverter()
        && port->direction()->isOutput()
        && std::strcmp(port->name(), "Y") == 0;
  }, "inverter", &inv_cell_);

  // tri-state
  find([](sta::LibertyPort* port) {
    return port->direction()->isTristate()
        && std::strcmp(port->name(), "Z") == 0;
  }, "tristate", &tristate_cell_);

  // and2
  find([](sta::LibertyPort* port) {
    auto func = port->function();
    return func
        && func->op() == sta::FuncExpr::op_and
        && func->left()->op() == sta::FuncExpr::op_port
        && func->right()->op() == sta::FuncExpr::op_port;
  }, "and2", &and2_cell_);

  // storage
  find([](sta::LibertyPort* port) {
    return port->libertyCell()->hasSequentials()
        && std::strcmp(port->name(), "Q") == 0;
  }, "storage", &storage_cell_);

  // clock_gate
  find([](sta::LibertyPort* port) {
    return port->libertyCell()->isClockGate();
  }, "clock_gate", &clock_gate_cell_);
}

//----------------------------------------------
// makeNet, makeInst, makeBTerm
//----------------------------------------------
odb::dbNet* RamGen::makeNet(const std::string& prefix, const std::string& name)
{
  std::string full = prefix.empty() ? name : fmt::format("{}.{}", prefix, name);
  return odb::dbNet::create(block_, full.c_str());
}

odb::dbInst* RamGen::makeInst(Layout* layout,
                              const std::string& prefix,
                              const std::string& name,
                              odb::dbMaster* master,
                              const std::vector<std::pair<std::string, odb::dbNet*>>& connections)
{
  std::string inst_name = prefix.empty() ? name : fmt::format("{}.{}", prefix, name);
  auto inst = odb::dbInst::create(block_, master, inst_name.c_str());

  for (auto& [term_name, net] : connections) {
    auto mterm = master->findMTerm(term_name.c_str());
    if (!mterm) {
      logger_->error(utl::RAM, 901, "Term {} not found", term_name);
    }
    inst->getITerm(mterm)->connect(net);
  }
  layout->addElement(std::make_unique<Element>(inst));
  return inst;
}

odb::dbNet* RamGen::makeBTerm(const std::string& name)
{
  auto net = block_->findNet(name.c_str());
  if (!net) {
    net = odb::dbNet::create(block_, name.c_str());
    odb::dbBTerm::create(net, name.c_str());
  }
  return net;
}

//----------------------------------------------
// make_decoder - general AND2 tree + invert
//----------------------------------------------
std::unique_ptr<Element> RamGen::make_decoder(const std::string& prefix,
                                              int address_bits,
                                              const std::vector<odb::dbNet*>& inputs)
{
  if ((int)inputs.size() != address_bits) {
    logger_->error(utl::RAM, 1000, "Decoder {}: need {} bits, got {}",
                   prefix, address_bits, inputs.size());
    return nullptr;
  }
  if (!inv_cell_ || !and2_cell_) {
    logger_->error(utl::RAM, 1015, "Missing cells for decoder {}", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  std::vector<odb::dbNet*> inverted(address_bits);

  // invert each input
  for (int i = 0; i < address_bits; ++i) {
    inverted[i] = makeNet(prefix, fmt::format("A{}_bar", i));
    makeInst(layout.get(), prefix, fmt::format("inv{}", i),
             inv_cell_,
             {{"A", inputs[i]}, {"Y", inverted[i]}});
  }

  int outputs = 1 << address_bits;
  for (int o = 0; o < outputs; ++o) {
    std::vector<odb::dbNet*> terms;
    for (int bit = 0; bit < address_bits; ++bit) {
      bool is_one = (o & (1 << bit)) != 0;
      terms.push_back(is_one ? inputs[bit] : inverted[bit]);
    }

    odb::dbNet* current_net = terms[0];
    for (size_t t = 1; t < terms.size(); ++t) {
      auto new_net = makeNet(prefix, fmt::format("and_{}_{}", o, t));
      makeInst(layout.get(), prefix, fmt::format("and_{}_{}", o, t),
               and2_cell_,
               {{"A", current_net}, {"B", terms[t]}, {"X", new_net}});
      current_net = new_net;
    }

    // final buffer
    auto out_net = makeNet(prefix, fmt::format("dec_out{}", o));
    makeInst(layout.get(), prefix, fmt::format("buf{}", o),
             inv_cell_,
             {{"A", current_net}, {"Y", out_net}});
  }

  return std::make_unique<Element>(std::move(layout));
}

//----------------------------------------------
// createBufferInstance
//----------------------------------------------
odb::dbInst* RamGen::createBufferInstance(odb::dbNet* net)
{
  auto master = inv_cell_;
  auto inst = odb::dbInst::create(block_, master,
                 fmt::format("buf_{}", net->getName()).c_str());

  auto mtermA = master->findMTerm("A");
  auto mtermY = master->findMTerm("Y");
  inst->getITerm(mtermA->getIndex())->connect(net);
  inst->getITerm(mtermY->getIndex())->connect(net);

  return inst;
}

//----------------------------------------------
// make_bit
//----------------------------------------------
std::unique_ptr<Element> RamGen::make_bit(const std::string& prefix,
                                          int read_ports,
                                          odb::dbNet* clock,
                                          const std::vector<odb::dbNet*>& select,
                                          odb::dbNet* data_input,
                                          std::vector<odb::dbNet*>& data_output)
{
  if ((int)select.size() < read_ports) {
    logger_->error(utl::RAM, 870,
                   "Bit {}: Select vector too small ({} < {})",
                   prefix, select.size(), read_ports);
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto storage_net = makeNet(prefix, "storage");

  // DFF
  makeInst(layout.get(), prefix, "dff", storage_cell_,
           {{"GATE", clock}, {"D", data_input}, {"Q", storage_net}});

  // tri-state for each read port
  for (int rp = 0; rp < read_ports; ++rp) {
    makeInst(layout.get(), prefix, fmt::format("tbuf{}", rp),
             tristate_cell_,
             {{"A", storage_net}, {"TE_B", select[rp]}, {"Z", data_output[rp]}});
  }

  return std::make_unique<Element>(std::move(layout));
}

//----------------------------------------------
// make_byte
//----------------------------------------------
std::unique_ptr<Element> RamGen::make_byte(const std::string& prefix,
                                           int read_ports,
                                           odb::dbNet* clock,
                                           odb::dbNet* write_enable,
                                           const std::vector<odb::dbNet*>& selects,
                                           const std::array<odb::dbNet*, 8>& data_input,
                                           const std::vector<std::array<odb::dbNet*, 8>>& data_output)
{
  if (selects.empty()) {
    logger_->error(utl::RAM, 1020, "Byte {}: No select signals", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);

  // invert the first select for gating
  std::vector<odb::dbNet*> select_b_nets;
  select_b_nets.reserve(read_ports);
  for (int i = 0; i < read_ports; ++i) {
    auto select_b = makeNet(prefix, fmt::format("select{}_b", i));
    makeInst(layout.get(), prefix, fmt::format("inv{}", i),
             inv_cell_,
             {{"A", selects[0]}, {"Y", select_b}});
    select_b_nets.push_back(select_b);
  }

  // clock gating
  auto clock_b = makeNet(prefix, "clock_b");
  auto gclk = makeNet(prefix, "gclk");
  auto we_gated = makeNet(prefix, "we_gated");

  makeInst(layout.get(), prefix, "clock_inv", inv_cell_,
           {{"A", clock}, {"Y", clock_b}});
  makeInst(layout.get(), prefix, "gcand", and2_cell_,
           {{"A", selects[0]}, {"B", write_enable}, {"X", we_gated}});
  makeInst(layout.get(), prefix, "cg", clock_gate_cell_,
           {{"CLK", clock_b}, {"GATE", we_gated}, {"GCLK", gclk}});

  // 8 bits
  for (int bit = 0; bit < 8; ++bit) {
    std::vector<odb::dbNet*> outputs;
    outputs.reserve(read_ports);
    for (int rp = 0; rp < read_ports; ++rp) {
      outputs.push_back(data_output[rp][bit]);
    }
    layout->addElement(
      make_bit(fmt::format("{}.bit{}", prefix, bit),
               read_ports, gclk,
               select_b_nets, data_input[bit], outputs)
    );
  }

  return std::make_unique<Element>(std::move(layout));
}

//----------------------------------------------
// make_word - 32-bit word from 4 bytes
//----------------------------------------------
std::unique_ptr<Element> RamGen::make_word(const std::string& prefix,
                                           int read_ports,
                                           odb::dbNet* clock,
                                           const std::vector<odb::dbNet*>& we_per_byte,
                                           odb::dbNet* sel,
                                           const std::array<odb::dbNet*, 32>& data_input,
                                           const std::vector<std::array<odb::dbNet*, 32>>& data_output)
{
  if (we_per_byte.size() < 4) {
    logger_->error(utl::RAM, 1050,
                   "Word {}: not enough WE signals for 4 bytes", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);

  // Byte3..Byte0
  for (int b = 3; b >= 0; --b) {
    std::array<odb::dbNet*, 8> di;
    for (int i = 0; i < 8; ++i) {
      int idx = b * 8 + i;
      di[i] = data_input[idx];
    }
    std::vector<std::array<odb::dbNet*, 8>> do_arrays(read_ports);
    for (int rp = 0; rp < read_ports; ++rp) {
      for (int i = 0; i < 8; ++i) {
        int idx = b * 8 + i;
        do_arrays[rp][i] = data_output[rp][idx];
      }
    }
    // single SEL for entire byte
    std::vector<odb::dbNet*> selects = {sel};

    layout->addElement(
      make_byte(fmt::format("{}.byte{}", prefix, b),
                read_ports, clock,
                we_per_byte[b], selects, di, do_arrays)
    );
  }

  return std::make_unique<Element>(std::move(layout));
}

//----------------------------------------------
// make_ram8 - 8 words (32bit), 3x8 decoder
//----------------------------------------------
std::unique_ptr<Element> RamGen::make_ram8(const std::string& prefix,
                                           int read_ports,
                                           odb::dbNet* clock,
                                           const std::vector<odb::dbNet*>& we_per_word,
                                           const std::vector<odb::dbNet*>& addr3bit,
                                           const std::array<odb::dbNet*, 32>& data_input,
                                           const std::vector<std::array<odb::dbNet*, 32>>& data_output)
{
  if (we_per_word.size() < 8) {
    logger_->error(utl::RAM, 1070, "RAM8 {}: need 8 WE signals", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);

  // 3->8 decoder
  auto dec_elem = make_decoder(fmt::format("{}.dec", prefix), 3, addr3bit);
  if (!dec_elem) {
    logger_->error(utl::RAM, 821, "RAM8 {}: Decoder creation failed", prefix);
    return nullptr;
  }
  layout->addElement(std::move(dec_elem));

  // For each output => one 32-bit word
  for (int w = 0; w < 8; ++w) {
    auto sel_w = block_->findNet(fmt::format("{}.dec.dec_out{}", prefix, w).c_str());
    if (!sel_w) {
      logger_->warn(utl::RAM, 162, "no net for dec_out{}", w);
      continue;
    }
    std::vector<odb::dbNet*> we_4(4, we_per_word[w]);

    std::array<odb::dbNet*, 32> di;
    for (int i = 0; i < 32; ++i) {
      di[i] = data_input[i];
    }
    std::vector<std::array<odb::dbNet*, 32>> do_4(read_ports);
    for (int rp = 0; rp < read_ports; ++rp) {
      for (int i = 0; i < 32; ++i) {
        do_4[rp][i] = data_output[rp][i];
      }
    }

    layout->addElement(
      make_word(fmt::format("{}.word{}", prefix, w),
                read_ports, clock,
                we_4, sel_w, di, do_4)
    );
  }

  return std::make_unique<Element>(std::move(layout));
}

//----------------------------------------------
// make_ram32 - 4 sub-blocks of ram8 + 2x4 decode
//----------------------------------------------
std::unique_ptr<Element> RamGen::make_ram32(const std::string& prefix,
                                            int read_ports,
                                            odb::dbNet* clock,
                                            const std::vector<odb::dbNet*>& we_32,
                                            const std::vector<odb::dbNet*>& addr5bit,
                                            const std::array<odb::dbNet*, 32>& data_input,
                                            const std::vector<std::array<odb::dbNet*, 32>>& data_output)
{
  if (we_32.size() < 32) {
    logger_->error(utl::RAM, 1090, "RAM32 {}: need 32 WEs total", prefix);
    return nullptr;
  }
  if ((int)addr5bit.size() < 5) {
    logger_->error(utl::RAM, 1091, "RAM32 {}: need 5 address bits", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);

  // 2->4 top-level decode
  std::vector<odb::dbNet*> top_addr2(addr5bit.begin(), addr5bit.begin() + 2);
  auto dec_elem = make_decoder(fmt::format("{}.dec2x4", prefix), 2, top_addr2);
  if (!dec_elem) {
    logger_->error(utl::RAM, 1092, "RAM32 {}: 2x4 dec creation failed", prefix);
    return nullptr;
  }
  layout->addElement(std::move(dec_elem));

  // lower 3 bits
  std::vector<odb::dbNet*> sub_addr3(addr5bit.begin() + 2, addr5bit.end());

  // sub-blocks
  for (int sub = 0; sub < 4; ++sub) {
    auto sub_sel = block_->findNet(fmt::format("{}.dec2x4.dec_out{}", prefix, sub).c_str());
    if (!sub_sel) {
      logger_->warn(utl::RAM, 1093, "No net dec_out{}", sub);
      continue;
    }

    std::vector<odb::dbNet*> we_subblock;
    for (int w = sub*8; w < sub*8 + 8; w++) {
      we_subblock.push_back(we_32[w]);
    }

    layout->addElement(
      make_ram8(fmt::format("{}.ram8_{}", prefix, sub),
                read_ports, clock,
                we_subblock, sub_addr3,
                data_input, data_output)
    );
  }

  return std::make_unique<Element>(std::move(layout));
}

//----------------------------------------------
// NEW: make_ram8_8bit
// builds an 8-word x 8-bit memory with a single 3x8 decoder
// and 8 "byte" sub-blocks
//----------------------------------------------
std::unique_ptr<Element> RamGen::make_ram8_8bit(const std::string& prefix,
                                                int read_ports,
                                                odb::dbNet* clock,
                                                const std::vector<odb::dbNet*>& we_8,
                                                const std::vector<odb::dbNet*>& addr3,
                                                const std::array<odb::dbNet*, 8>& data_input,
                                                const std::vector<std::array<odb::dbNet*, 8>>& data_output)
{
  if (we_8.size() < 8) {
    logger_->error(utl::RAM, 1071, "ram8_8bit: need 8 WE lines");
    return nullptr;
  }
  if ((int)addr3.size() != 3) {
    logger_->error(utl::RAM, 1072, "ram8_8bit: need exactly 3 address lines");
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);

  // 3->8 row decoder
  auto dec_elem = make_decoder(fmt::format("{}.dec", prefix),
                               3, addr3);
  if (!dec_elem) {
    logger_->error(utl::RAM, 1073, "ram8_8bit: 3x8 decoder creation failed");
    return nullptr;
  }
  layout->addElement(std::move(dec_elem));

  // For each dec_outN => 1 Byte
  for (int w = 0; w < 8; ++w) {
    auto sel_w = block_->findNet(fmt::format("{}.dec.dec_out{}", prefix, w).c_str());
    if (!sel_w) {
      logger_->warn(utl::RAM, 1074, "No net dec_out{}", w);
      continue;
    }
    std::vector<odb::dbNet*> selects = {sel_w};

    auto byte_elem = make_byte(fmt::format("{}.byte{}", prefix, w),
                               read_ports, clock,
                               we_8[w], // each word can have a separate WE
                               selects,
                               data_input,
                               data_output);
    if (byte_elem) {
      layout->addElement(std::move(byte_elem));
    }
  }

  return std::make_unique<Element>(std::move(layout));
}

//----------------------------------------------
// Updated generate()
//----------------------------------------------
void RamGen::generate(int bytes_per_word,
                      int word_count,
                      int read_ports,
                      odb::dbMaster* storage_cell,
                      odb::dbMaster* tristate_cell,
                      odb::dbMaster* inv_cell)
{
  // Validate
  if (read_ports <= 0) {
    logger_->error(utl::RAM, 349, "read_ports must be > 0");
    return;
  }
  if (word_count <= 0 || (word_count & (word_count - 1)) != 0) {
    logger_->error(utl::RAM, 458, "word_count must be a positive power of 2");
    return;
  }
  if (bytes_per_word <= 0 || (bytes_per_word & (bytes_per_word - 1)) != 0) {
    logger_->error(utl::RAM, 114, "bytes_per_word must be a positive power of 2");
    return;
  }

  // Setup
  storage_cell_  = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_      = inv_cell;
  findMasters();
  if (!storage_cell_ || !tristate_cell_ || !inv_cell_ || !and2_cell_) {
    logger_->error(utl::RAM, 1016, "Missing required technology cells");
    return;
  }

  // Create or get block
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    chip = odb::dbChip::create(db_);
  }
  block_ = chip->getBlock();
  if (!block_) {
    block_ = odb::dbBlock::create(chip,
      fmt::format("RAM_{}x{}x{}", word_count, bytes_per_word, 8).c_str());
  }

  int row_bits = (int)std::log2(word_count);
  int col_bits = (int)std::log2(bytes_per_word);
  int total_address_bits = row_bits + col_bits;
  if (total_address_bits > 32) {
    logger_->error(utl::RAM, 696, "Total address bits exceed 32");
    return;
  }

  // address nets
  std::vector<odb::dbNet*> address(total_address_bits);
  for (int i = 0; i < total_address_bits; ++i) {
    address[i] = makeBTerm(fmt::format("A{}", i));
  }
  // row vs col
  std::vector<odb::dbNet*> row_address(address.begin(), address.begin() + row_bits);
  std::vector<odb::dbNet*> col_address(address.begin() + row_bits, address.end());

  // data
  int data_width = bytes_per_word * 8;
  // For up to 32 bits
  std::array<odb::dbNet*, 32> data_in{};
  for (int i = 0; i < data_width; ++i) {
    if (i < 32) {
      data_in[i] = makeBTerm(fmt::format("Di{}", i));
    }
  }
  std::vector<std::array<odb::dbNet*, 32>> data_out(read_ports);
  for (int rp = 0; rp < read_ports; ++rp) {
    for (int i = 0; i < data_width; ++i) {
      if (i < 32) {
        data_out[rp][i] = makeBTerm(fmt::format("Do{}_{}", rp, i));
      }
    }
  }

  // WE signals (1 per word)
  std::vector<odb::dbNet*> we_nets(word_count);
  for (int w = 0; w < word_count; ++w) {
    we_nets[w] = makeBTerm(fmt::format("WE{}", w));
  }

  // Clock
  odb::dbNet* clk = makeBTerm("CLK");

  // Final top layout
  auto topLayout = std::make_unique<Layout>(odb::horizontal);

  // Check special "8 words x 8 bits" => call make_ram8_8bit
  if (word_count == 8 && bytes_per_word == 1) {
    logger_->info(utl::RAM, 2501,
                  "Building a specialized RAM8(8bit) with a single 3x8 decoder...");
    // row_address = 3 bits
    if ((int)row_address.size() != 3) {
      logger_->error(utl::RAM, 2502,
                     "Expected 3 row bits for 8-word memory, got {}", row_address.size());
      return;
    }
    // we_nets => 8 lines, data_in => 8 bits
    // read_ports => data_out => do8
    std::array<odb::dbNet*, 8> di;
    for (int i = 0; i < 8; i++) {
      di[i] = data_in[i];
    }
    std::vector<std::array<odb::dbNet*, 8>> do8(read_ports);
    for (int rp = 0; rp < read_ports; ++rp) {
      for (int i = 0; i < 8; i++) {
        do8[rp][i] = data_out[rp][i];
      }
    }

    auto ram8_elem = make_ram8_8bit("ram8_8bit",
                                    read_ports, clk,
                                    we_nets,
                                    row_address,
                                    di, do8);
    if (ram8_elem) {
      topLayout->addElement(std::move(ram8_elem));
    }
  }
  else if (word_count == 8 && bytes_per_word == 4 && data_width == 32) {
    logger_->info(utl::RAM, 2601, "Building a standard RAM8(32bit) using 3x8 dec + 32bit words");
    // The existing "make_ram8" for 32 bits
    if ((int)row_address.size() != 3) {
      logger_->error(utl::RAM, 2602, "Expect 3 row bits for 8 words, got {}", row_address.size());
      return;
    }
    auto ram8_elem = make_ram8("ram8_32bit", read_ports, clk,
                               we_nets, row_address,
                               data_in, data_out);
    if (ram8_elem) {
      topLayout->addElement(std::move(ram8_elem));
    }
  }
  else if (word_count == 32 && bytes_per_word == 4 && data_width == 32) {
    logger_->info(utl::RAM, 2701, "Building a standard RAM32(32bit) with 2x4 top-level dec + 4 sub-blocks");
    // 5 address bits => row_address=?
    auto ram32_elem = make_ram32("ram32", read_ports, clk,
                                 we_nets, address,
                                 data_in, data_out);
    if (ram32_elem) {
      topLayout->addElement(std::move(ram32_elem));
    }
  }
  else {
    // If you prefer a fully generic approach for any row_bits, col_bits, data_width
    // you'd place that code here. For brevity, we show a fallback:
    logger_->warn(utl::RAM, 2022,
                  "General case not fully implemented for word_count={}, bytes_per_word={}, data_width={}. row_bits={}, col_bits={}",
                  word_count, bytes_per_word, data_width,
                  row_address.size(), col_address.size());
  }

  // Position final design
  topLayout->position(odb::Point(0,0));
}

} // namespace ram
