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
                      odb::dbMaster* inv_cell)
{
  //---------------------------------------------------------------------------
  // 0) Validate input parameters
  //---------------------------------------------------------------------------
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

  // Number of row bits, column bits, total
  const int row_address_bits = static_cast<int>(std::log2(word_count));
  const int col_address_bits = static_cast<int>(std::log2(bytes_per_word));
  const int total_address_bits = row_address_bits + col_address_bits;
  if (total_address_bits > 32) {
    logger_->error(utl::RAM, 696, "Total address bits exceed 32");
    return;
  }

  //---------------------------------------------------------------------------
  // 1) Initialize database: chip and block
  //---------------------------------------------------------------------------
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    chip = odb::dbChip::create(db_);
  }
  block_ = chip->getBlock();
  if (!block_) {
    block_ = odb::dbBlock::create(
        chip,
        fmt::format("RAM_{}x{}x{}", word_count, bytes_per_word, 8).c_str());
  }

  //---------------------------------------------------------------------------
  // 2) Prepare technology cells & check
  //---------------------------------------------------------------------------
  storage_cell_  = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_      = inv_cell;
  findMasters();
  if (!storage_cell_ || !tristate_cell_ || !inv_cell_ || !and2_cell_) {
    logger_->error(utl::RAM, 1016, "Missing required technology cells");
    return;
  }

  //---------------------------------------------------------------------------
  // 3) Create address nets
  //---------------------------------------------------------------------------
  const int columns = 1 << col_address_bits;
  std::vector<odb::dbNet*> address(total_address_bits);
  for (int i = 0; i < total_address_bits; ++i) {
    address[i] = makeBTerm(fmt::format("A{}", i));
  }
  // Split row vs col
  std::vector<odb::dbNet*> row_address(address.begin(),
                                       address.begin() + row_address_bits);
  std::vector<odb::dbNet*> col_address(address.begin() + row_address_bits,
                                       address.end());

  //---------------------------------------------------------------------------
  // 4) Build row & column decoders (the netlists)
  //    We will physically place their gates shortly.
  //---------------------------------------------------------------------------
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

  //---------------------------------------------------------------------------
  // 5) Create WE signals, data signals, clock
  //---------------------------------------------------------------------------
  std::vector<odb::dbNet*> we(columns);
  for (int i = 0; i < columns; ++i) {
    we[i] = makeBTerm(fmt::format("WE{}", i));
  }
  std::vector<std::array<odb::dbNet*, 8>> data_input(bytes_per_word);
  std::vector<std::vector<std::array<odb::dbNet*, 8>>> data_output(read_ports);
  for (int rp = 0; rp < read_ports; ++rp) {
    data_output[rp].resize(bytes_per_word);
    for (int byte = 0; byte < bytes_per_word; ++byte) {
      for (int bit = 0; bit < 8; ++bit) {
        data_output[rp][byte][bit]
            = makeBTerm(fmt::format("Do{}_b{}_b{}", rp, byte, bit));
      }
    }
  }
  odb::dbNet* clk = makeBTerm("CLK");

  //---------------------------------------------------------------------------
  // 6) Place the final row-decoder gates in a vertical column,
  //    so each row_dec.dec_out<N> gate is aligned with row N.
  //---------------------------------------------------------------------------
  // We'll do this by building a "vertical" Layout for the row-dec logic:
  auto rowDecLogicLayout = std::make_unique<Layout>(odb::vertical);

  // In the simplest approach, we assume there are "word_count" final gates
  // in the row decoder that produce row_dec.dec_out0..row_dec.dec_outN-1.
  // We'll gather them by name.  For each row "r", we find e.g. "row_dec.inv<r>"
  // or "row_dec.dec_out<r>" instance.  We'll place them in one vertical stack.
  for (int r = 0; r < word_count; ++r) {
    // Typically, the final gate might be "row_dec.inv<r>" or "row_dec.andN<r>".
    // Let's suppose it's named "row_dec.inv<r>": you can adapt if your netlist
    // naming differs.  Alternatively, we can look up the net "row_dec.dec_out<r>"
    // and find which gate drives it.
    const std::string gate_name = fmt::format("row_dec.inv{}", r);
    odb::dbInst* gate_inst = block_->findInst(gate_name.c_str());
    if (!gate_inst) {
      // If we can't find it, we might log a warning or skip
      logger_->warn(utl::RAM, 555, "No final row dec gate named {}", gate_name);
      continue;
    }
    // Place this gate in a horizontal sub-layout if you prefer,
    // or place it directly as an Element:
    auto gateElem = std::make_unique<Element>(gate_inst);
    rowDecLogicLayout->addElement(std::move(gateElem));
  }

  //---------------------------------------------------------------------------
  // 7) Place the final column-decoder gates in a vertical column as well
  //---------------------------------------------------------------------------
  // Only if col_address_bits>0
  auto colDecLogicLayout = std::make_unique<Layout>(odb::vertical);
  if (col_address_bits > 0) {
    // Suppose the final gates are named "col_dec.invX" for columns 0..(columns-1)
    for (int c = 0; c < columns; ++c) {
      const std::string gate_name = fmt::format("col_dec.inv{}", c);
      odb::dbInst* gate_inst = block_->findInst(gate_name.c_str());
      if (!gate_inst) {
        logger_->warn(utl::RAM, 556, "No final col dec gate named {}", gate_name);
        continue;
      }
      auto gateElem = std::make_unique<Element>(gate_inst);
      colDecLogicLayout->addElement(std::move(gateElem));
    }
  }

  //---------------------------------------------------------------------------
  // 8) Build a "memory array" that lines up with each row's row_dec output
  //    We do a single vertical layout, each row = horizontal layout
  //    [ row_dec gate area or buffer? | memory columns ... ]
  //---------------------------------------------------------------------------
  auto memoryArrayLayout = std::make_unique<Layout>(odb::vertical);

  for (int r = 0; r < word_count; ++r) {
    // Horizontal layout: row-dec's net, then memory columns
    auto rowLayout = std::make_unique<Layout>(odb::horizontal);

    // (a) We do NOT buffer row_sel again; we just find the net row_dec.dec_out<r>.
    odb::dbNet* row_sel = block_->findNet(fmt::format("row_dec.dec_out{}", r).c_str());
    if (!row_sel) {
      logger_->warn(utl::RAM, 557, "No net row_dec.dec_out{} found", r);
      continue;
    }

    // If you still want a small buffer for each row, you can do it here:
    // e.g., dbInst* rowBuf = createBufferInstance(row_sel);
    // rowLayout->addElement(std::make_unique<Element>(rowBuf));

    // (b) place memory columns
    auto rowMemCols = std::make_unique<Layout>(odb::horizontal);
    for (int c = 0; c < columns; ++c) {
      // find col_sel net
      odb::dbNet* col_sel = nullptr;
      if (col_address_bits > 0) {
        col_sel = block_->findNet(fmt::format("col_dec.dec_out{}", c).c_str());
        if (!col_sel) {
          logger_->warn(utl::RAM, 558, "No net col_dec.dec_out{} found", c);
          continue;
        }
      } else {
        col_sel = makeBTerm("const_col_enable");
      }

      // Data input for this cell
      std::array<odb::dbNet*, 8> din;
      for (int bit = 0; bit < 8; ++bit) {
        din[bit] = makeBTerm(fmt::format("Din_col{}_b{}_row{}", c, bit, r));
      }

      // Data outputs for each read port
      std::vector<std::array<odb::dbNet*, 8>> dout(read_ports);
      for (int rp = 0; rp < read_ports; ++rp) {
        dout[rp] = data_output[rp][c];
      }

      // Create the memory cell
      auto memCell = make_byte(fmt::format("cell_r{}_c{}", r, c),
                               read_ports,
                               clk,
                               we[c],
                               std::vector<odb::dbNet*>{row_sel, col_sel},
                               din,
                               dout);
      if (memCell) {
        rowMemCols->addElement(std::move(memCell));
      }
    }

    rowLayout->addElement(std::make_unique<Element>(std::move(rowMemCols)));
    memoryArrayLayout->addElement(std::make_unique<Element>(std::move(rowLayout)));
  }

  //---------------------------------------------------------------------------
  // 9) Top-level: a horizontal layout containing:
  //    [ rowDecLogicLayout | colDecLogicLayout | memoryArrayLayout ]
  //    So everything is side-by-side, but you can adapt to put them
  //    in separate columns or arrange them differently as needed.
  //---------------------------------------------------------------------------
  auto topLayout = std::make_unique<Layout>(odb::horizontal);

  // Place row-dec logic on the far left
  topLayout->addElement(std::make_unique<Element>(std::move(rowDecLogicLayout)));

  // Next, place col-dec logic (if present) to the right
  if (col_address_bits > 0) {
    topLayout->addElement(std::make_unique<Element>(std::move(colDecLogicLayout)));
  }

  // Finally, place the memory array
  topLayout->addElement(std::make_unique<Element>(std::move(memoryArrayLayout)));

  // Position the entire design at (0,0)
  topLayout->position(odb::Point(0,0));
}


} // namespace ram