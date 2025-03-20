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

RamGen::RamGen() :
    db_(nullptr),
    block_(nullptr),
    network_(nullptr),
    logger_(nullptr),
    storage_cell_(nullptr),
    tristate_cell_(nullptr),
    inv_cell_(nullptr),
    and2_cell_(nullptr),
    clock_gate_cell_(nullptr) {}

void RamGen::init(odb::dbDatabase* db, sta::dbNetwork* network, Logger* logger) {
  db_ = db;
  network_ = network;
  logger_ = logger;
}

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

      LibertyCell* liberty = network_->libertyCell(cell);
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

  if (!best) logger_->error(utl::RAM, 900, "Could not find {}", name);
  else logger_->info(utl::RAM, 916, "Selected {}: {}", name, best->getName());
  return best;
}

void RamGen::findMasters() {
  auto find = [&](const std::function<bool(sta::LibertyPort*)>& match, 
                 const char* name, odb::dbMaster** target) {
    if (!*target) *target = findMaster(match, name);
  };

  find([](sta::LibertyPort* port) { 
    return port->libertyCell()->isInverter() &&
           port->direction()->isOutput() &&
           std::strcmp(port->name(), "Y") == 0;
  }, "inverter", &inv_cell_);

  find([](sta::LibertyPort* port) {
    return port->direction()->isTristate() &&
           std::strcmp(port->name(), "Z") == 0;
  }, "tristate", &tristate_cell_);

  find([](sta::LibertyPort* port) {
    auto func = port->function();
    return func && 
           func->op() == sta::FuncExpr::op_and &&
           func->left()->op() == sta::FuncExpr::op_port &&
           func->right()->op() == sta::FuncExpr::op_port;
  }, "and2", &and2_cell_);

  find([](sta::LibertyPort* port) {
    return port->libertyCell()->hasSequentials() &&
           std::strcmp(port->name(), "Q") == 0;
  }, "storage", &storage_cell_);

  find([](sta::LibertyPort* port) {
    return port->libertyCell()->isClockGate();
  }, "clock_gate", &clock_gate_cell_);
}

odb::dbNet* RamGen::makeNet(const std::string& prefix, const std::string& name) {
  std::string full_name = prefix.empty() ? name : fmt::format("{}.{}", prefix, name);
  return odb::dbNet::create(block_, full_name.c_str());
}

odb::dbInst* RamGen::makeInst(Layout* layout,
                             const std::string& prefix,
                             const std::string& name,
                             odb::dbMaster* master,
                             const std::vector<std::pair<std::string, odb::dbNet*>>& connections) {
  std::string inst_name = prefix.empty() ? name : fmt::format("{}.{}", prefix, name);
  odb::dbInst* inst = odb::dbInst::create(block_, master, inst_name.c_str());

  for (const auto& [term_name, net] : connections) {
    odb::dbMTerm* mterm = master->findMTerm(term_name.c_str());
    if (!mterm) logger_->error(utl::RAM, 901, "Term {} not found", term_name);
    inst->getITerm(mterm)->connect(net);
  }

  layout->addElement(std::make_unique<Element>(inst));
  return inst;
}

odb::dbNet* RamGen::makeBTerm(const std::string& name) {
  odb::dbNet* net = block_->findNet(name.c_str());
  if (!net) {
    net = odb::dbNet::create(block_, name.c_str());
    odb::dbBTerm::create(net, name.c_str());
  }
  return net;
}


std::unique_ptr<Element> RamGen::make_decoder(const std::string& prefix,
                                             int address_bits,
                                             const std::vector<odb::dbNet*>& inputs) {
  if (static_cast<int>(inputs.size()) != address_bits) {
    logger_->error(utl::RAM, 1000, "Decoder {}: Requires {} bits, got {}", 
                  prefix, address_bits, inputs.size());
    return nullptr;
  }

  if (!inv_cell_ || !and2_cell_) {
    logger_->error(utl::RAM, 1015, "Missing cells for decoder {}", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  std::vector<odb::dbNet*> inverted(inputs.size());

  for (size_t i = 0; i < inputs.size(); ++i) {
    inverted[i] = makeNet(prefix, fmt::format("A{}_bar", i));
    makeInst(layout.get(), prefix, fmt::format("inv{}", i), inv_cell_,
            {{"A", inputs[i]}, {"Y", inverted[i]}});
  }

  const int outputs = 1 << address_bits;
  for (int i = 0; i < outputs; ++i) {
    std::vector<odb::dbNet*> terms;
    for (size_t bit = 0; bit < static_cast<size_t>(address_bits); ++bit) {
      terms.push_back((i & (1 << bit)) ? inputs[bit] : inverted[bit]);
    }

    odb::dbNet* current_net = terms[0];
    for (size_t term_idx = 1; term_idx < terms.size(); ++term_idx) {
      odb::dbNet* new_net = makeNet(prefix, fmt::format("and_{}_{}", i, term_idx));
      makeInst(layout.get(), prefix, fmt::format("and_{}_{}", i, term_idx), and2_cell_,
              {{"A", current_net}, {"B", terms[term_idx]}, {"X", new_net}});
      current_net = new_net;
    }

    odb::dbNet* out_net = makeNet(prefix, fmt::format("dec_out{}", i));
    makeInst(layout.get(), prefix, fmt::format("buf{}", i), inv_cell_,
            {{"A", current_net}, {"Y", out_net}});
  }

  return std::make_unique<Element>(std::move(layout));
}

std::unique_ptr<Element> RamGen::make_bit(const std::string& prefix,
                                        int read_ports,
                                        odb::dbNet* clock,
                                        const std::vector<odb::dbNet*>& select,
                                        odb::dbNet* data_input,
                                        std::vector<odb::dbNet*>& data_output) {
  if (select.size() < static_cast<size_t>(read_ports)) {
    logger_->error(utl::RAM, 870, "Bit {}: Select vector too small ({} < {})", 
                  prefix, select.size(), read_ports);
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  odb::dbNet* storage_net = makeNet(prefix, "storage");

  makeInst(layout.get(), prefix, "dff", storage_cell_,
          {{"GATE", clock}, {"D", data_input}, {"Q", storage_net}});

  for (int rp = 0; rp < read_ports; ++rp) {
    makeInst(layout.get(), prefix, fmt::format("tbuf{}", rp), tristate_cell_,
            {{"A", storage_net}, {"TE_B", select[rp]}, {"Z", data_output[rp]}});
  }

  return std::make_unique<Element>(std::move(layout));
}

std::unique_ptr<Element> RamGen::make_byte(
    const std::string& prefix,
    int read_ports,
    odb::dbNet* clock,
    odb::dbNet* write_enable,
    const std::vector<odb::dbNet*>& selects,
    const std::array<odb::dbNet*, 8>& data_input,
    const std::vector<std::array<odb::dbNet*, 8>>& data_output) {
  if (selects.empty()) {
    logger_->error(utl::RAM, 1020, "Byte {}: No select signals", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);

  // Generate inverted select signals for each read port
  std::vector<odb::dbNet*> select_b_nets;
  select_b_nets.reserve(read_ports);
  for (int i = 0; i < read_ports; ++i) {
    odb::dbNet* select_b = makeNet(prefix, fmt::format("select{}_b", i));
    makeInst(layout.get(), prefix, fmt::format("inv{}", i), inv_cell_,
            {{"A", selects[0]}, {"Y", select_b}});
    select_b_nets.push_back(select_b);
  }

  // Clock gating logic
  odb::dbNet* clock_b = makeNet(prefix, "clock_b");
  odb::dbNet* gclk = makeNet(prefix, "gclk");
  odb::dbNet* we_gated = makeNet(prefix, "we_gated");

  makeInst(layout.get(), prefix, "clock_inv", inv_cell_,
          {{"A", clock}, {"Y", clock_b}});
  makeInst(layout.get(), prefix, "gcand", and2_cell_,
          {{"A", selects[0]}, {"B", write_enable}, {"X", we_gated}});
  makeInst(layout.get(), prefix, "cg", clock_gate_cell_,
          {{"CLK", clock_b}, {"GATE", we_gated}, {"GCLK", gclk}});

  // Create bits with proper select signals
  for (int bit = 0; bit < 8; ++bit) {
    std::vector<odb::dbNet*> outputs;
    for (int rp = 0; rp < read_ports; ++rp) {
      outputs.push_back(data_output[rp][bit]);
    }
    
    layout->addElement(
      make_bit(fmt::format("{}.bit{}", prefix, bit), read_ports, gclk,
              select_b_nets, data_input[bit], outputs)
    );
  }

  return std::make_unique<Element>(std::move(layout));
}

odb::dbInst* RamGen::createBufferInstance(odb::dbNet* net) {
    // Use the inverter cell as a buffer (simple passthrough)
    auto master = inv_cell_;
    auto inst = odb::dbInst::create(block_, master, fmt::format("buf_{}", net->getName()).c_str());
    
    // Find the terminals for "A" and "Y"
    auto mtermA = master->findMTerm("A");
    auto mtermY = master->findMTerm("Y");
    
    // Use getIndex() instead of getOrder()
    inst->getITerm(mtermA->getIndex())->connect(net);
    inst->getITerm(mtermY->getIndex())->connect(net);
    
    return inst;
}



void RamGen::generate(int bytes_per_word,
                     int word_count,
                     int read_ports,
                     odb::dbMaster* storage_cell,
                     odb::dbMaster* tristate_cell,
                     odb::dbMaster* inv_cell) {
  // Validate input parameters
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

  // Calculate address configuration
  const int row_address_bits = static_cast<int>(std::log2(word_count));
  const int col_address_bits = static_cast<int>(std::log2(bytes_per_word));
  const int total_address_bits = row_address_bits + col_address_bits;
  const int columns = 1 << col_address_bits;

  if (total_address_bits > 32) {
    logger_->error(utl::RAM, 696, "Total address bits exceed 32");
    return;
  }

  // Initialize database
  odb::dbChip* chip = db_->getChip();
  if (!chip) chip = odb::dbChip::create(db_);
  block_ = chip->getBlock();
  if (!block_) {
    block_ = odb::dbBlock::create(chip, 
      fmt::format("RAM_{}x{}x{}", word_count, bytes_per_word, 8).c_str());
  }

  // Initialize technology cells
  storage_cell_ = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_ = inv_cell;
  findMasters();

  // Validate critical cells
  if (!storage_cell_ || !tristate_cell_ || !inv_cell_ || !and2_cell_) {
    logger_->error(utl::RAM, 1016, "Missing required technology cells");
    return;
  }

  // Create address signals
  std::vector<odb::dbNet*> address(total_address_bits);
  for (int i = 0; i < total_address_bits; ++i) {
    address[i] = makeBTerm(fmt::format("A{}", i));
  }

  // Split address into components
  std::vector<odb::dbNet*> row_address(
    address.begin(), 
    address.begin() + row_address_bits
  );
  std::vector<odb::dbNet*> col_address(
    address.begin() + row_address_bits,
    address.end()
  );

  if (row_address.size() != static_cast<size_t>(row_address_bits) ||
      col_address.size() != static_cast<size_t>(col_address_bits)) {
    logger_->error(utl::RAM, 444, "Address split mismatch");
    return;
  }

  // Create main horizontal layout
  Layout main_layout(odb::horizontal);

  // Build row decoder
  /* auto row_decoder = make_decoder("row_dec", row_address_bits, row_address);
  if (!row_decoder) {
    logger_->error(utl::RAM, 1557, "Failed to create row decoder");
    return;
  }

  // Conditionally build column decoder
  std::unique_ptr<Element> col_decoder;
  if (col_address_bits > 0) {
    col_decoder = make_decoder("col_dec", col_address_bits, col_address);
    if (!col_decoder) {
      logger_->error(utl::RAM, 1227, "Failed to create column decoder");
      return;
    }
  } */

  // Create vertical decoder group (left side)
  /* auto decoder_group = std::make_unique<Layout>(odb::vertical);
  decoder_group->addElement(std::move(row_decoder));
  if (col_address_bits > 0) {
    decoder_group->addElement(std::move(col_decoder));
  }
  main_layout.addElement(std::make_unique<Element>(std::move(decoder_group)));

  // Create horizontal memory array (right side)
  auto memory_array = std::make_unique<Layout>(odb::horizontal); */


// Build decoders
  auto row_decoder = make_decoder("row_dec", row_address_bits, row_address);
  if (!row_decoder) {
    logger_->error(utl::RAM, 1017, "Failed to create row decoder");
    return;
  }

  std::unique_ptr<Element> col_decoder;
  if (col_address_bits > 0) {
    col_decoder = make_decoder("col_dec", col_address_bits, col_address);
    if (!col_decoder) {
      logger_->error(utl::RAM, 197, "Failed to create column decoder");
      return;
    }
  }

  // Create decoder grid matching memory columns
  auto decoder_group = std::make_unique<Layout>(odb::horizontal);
  
  // Add column decoder outputs as individual columns
  if (col_address_bits > 0) {
    for (int col = 0; col < columns; ++col) {
      auto decoder_col = std::make_unique<Layout>(odb::vertical);
      
      // Add column select signal with valid buffer instance
      odb::dbNet* col_sel = block_->findNet(fmt::format("col_dec.dec_out{}", col).c_str());
      if (col_sel) {
        auto net_holder = std::make_unique<Layout>(odb::vertical);
        // Create actual buffer instance instead of dummy
        odb::dbInst* buffer_inst = createBufferInstance(col_sel);
        net_holder->addElement(std::make_unique<Element>(buffer_inst));
        decoder_col->addElement(std::make_unique<Element>(std::move(net_holder)));
      }

      // Add row decoder to first column
      if (col == 0) {
        decoder_col->addElement(std::move(row_decoder));
      }

      decoder_group->addElement(std::make_unique<Element>(std::move(decoder_col)));
    }
  } else {
    // Single column case with proper element wrapping
    auto decoder_col = std::make_unique<Layout>(odb::vertical);
    decoder_col->addElement(std::move(row_decoder));
    decoder_group->addElement(std::make_unique<Element>(std::move(decoder_col)));
  }

  main_layout.addElement(std::make_unique<Element>(std::move(decoder_group)));

  // Create memory array
  auto memory_array = std::make_unique<Layout>(odb::horizontal);

  // Create write enable signals
  std::vector<odb::dbNet*> we(columns);
  for (int i = 0; i < columns; ++i) {
    we[i] = makeBTerm(fmt::format("WE{}", i));
  }

  // Create data signals
  std::vector<std::array<odb::dbNet*, 8>> data_input(bytes_per_word);
  std::vector<std::vector<std::array<odb::dbNet*, 8>>> data_output(read_ports);
  for (int rp = 0; rp < read_ports; ++rp) {
    data_output[rp].resize(bytes_per_word);
    for (int byte = 0; byte < bytes_per_word; ++byte) {
      for (int bit = 0; bit < 8; ++bit) {
        data_output[rp][byte][bit] = makeBTerm(
          fmt::format("Do{}_b{}_b{}", rp, byte, bit));
      }
    }
  }

  // Create clock signal
  odb::dbNet* clk = makeBTerm("CLK");

  // Build columns with net validation
  for (int col = 0; col < columns; ++col) {
    odb::dbNet* col_sel = nullptr;
    if (col_address_bits > 0) {
      col_sel = block_->findNet(fmt::format("col_dec.dec_out{}", col).c_str());
      if (!col_sel) {
        logger_->error(utl::RAM, 1018, "Column select {} not found", col);
        continue;
      }
    } else {
      // Handle single-column case
      col_sel = makeBTerm("const_enable");
    }

    auto column = std::make_unique<Layout>(odb::vertical);
    for (int row = 0; row < word_count; ++row) {
      odb::dbNet* row_sel = block_->findNet(fmt::format("row_dec.dec_out{}", row).c_str());
      if (!row_sel) {
        logger_->error(utl::RAM, 1019, "Row select {} not found", row);
        continue;
      }

      std::array<odb::dbNet*, 8> din;
      std::vector<std::array<odb::dbNet*, 8>> dout(read_ports);
      for (int b = 0; b < 8; ++b) {
        din[b] = makeBTerm(fmt::format("Din_col{}_b{}_row{}", col, b, row));
        for (int rp = 0; rp < read_ports; ++rp) {
          dout[rp][b] = data_output[rp][col][b];
        }
      }

      column->addElement(
        make_byte(fmt::format("col{}.row{}", col, row), read_ports, clk,
                 we[col], {row_sel, col_sel}, din, dout)
      );
    }
    memory_array->addElement(std::make_unique<Element>(std::move(column)));
  }

  // Final assembly
  main_layout.addElement(std::make_unique<Element>(std::move(memory_array)));
  main_layout.position(odb::Point(0, 0));
}
} // namespace ram