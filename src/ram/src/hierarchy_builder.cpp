#include "ram/ram.h"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include "db_sta/dbNetwork.hh"
#include "sta/FuncExpr.hh" // Add this for proper FuncExpr definition


namespace ram {

//----------------------------------------------
// HierarchyBuilder implementation
//----------------------------------------------

HierarchyBuilder::HierarchyBuilder(odb::dbBlock* block, const MemoryConfig& config, Logger* logger, sta::dbNetwork* network)
  : block_(block),
    config_(config),
    logger_(logger),
    network_(network),
    masters_are_set_(false)
{
  // Don't automatically find masters - wait for setMasters or buildHierarchy
}


void HierarchyBuilder::setMasters(odb::dbMaster* storage_cell,
  odb::dbMaster* tristate_cell,
  odb::dbMaster* inv_cell,
  odb::dbMaster* and2_cell,
  odb::dbMaster* clock_gate_cell,
  odb::dbMaster* mux2_cell)
{
storage_cell_ = storage_cell;
tristate_cell_ = tristate_cell;
inv_cell_ = inv_cell;
and2_cell_ = and2_cell;
clock_gate_cell_ = clock_gate_cell;
mux2_cell_ = mux2_cell;

// Cache them for faster lookup
if (storage_cell_) masters_cache_["storage"] = storage_cell_;
if (tristate_cell_) masters_cache_["tristate"] = tristate_cell_;
if (inv_cell_) masters_cache_["inverter"] = inv_cell_;
if (and2_cell_) masters_cache_["and2"] = and2_cell_;
if (clock_gate_cell_) masters_cache_["clock_gate"] = clock_gate_cell_;
if (mux2_cell_) masters_cache_["mux2"] = mux2_cell_;

masters_are_set_ = true;
}








std::unique_ptr<Element> HierarchyBuilder::buildHierarchy()
{

  // More validation before starting
  if (!block_) {
    logger_->error(utl::RAM, 1201, "Cannot build hierarchy - block is null");
    return nullptr;
  }


  // If masters weren't set, try to find them now
  if (!masters_are_set_) {
    logger_->info(utl::RAM, 1287, "Masters not set - attempting to find them");
    findMasters();
  }
  
  // Validate essential masters were found
  if (!storage_cell_ || !tristate_cell_ || !inv_cell_ || !and2_cell_) {
    logger_->error(utl::RAM, 1250, "Missing essential technology cells");
    return nullptr;
  }

  logger_->info(utl::RAM, 1200, "Building memory hierarchy for {} words x {} bytes, type: {}",
               config_.getWordCount(), config_.getBytesPerWord(),
               config_.getType() == RAM_1RW ? "1RW" : 
               config_.getType() == RAM_1RW1R ? "1RW1R" : "2R1W");
  
  // Create clock, enable and other global nets
  odb::dbNet* clk = createBTermNet("CLK");
  if (!clk) {
    logger_->error(utl::RAM, 1202, "Failed to create clock net");
    return nullptr;
  }

  
  // Create data input nets
  int data_width = config_.getBytesPerWord() * 8;
  std::array<odb::dbNet*, 32> data_in{};
  for (int i = 0; i < data_width && i < 32; ++i) {
    data_in[i] = createBTermNet(fmt::format("Di{}", i));
    if (!data_in[i]) {
      logger_->error(utl::RAM, 1203, "Failed to create data input net Di{}", i);
      return nullptr;
    }
  }
  
  // Create address nets
  std::vector<odb::dbNet*> address;
  int addr_bits = config_.getAddressBits();
  for (int i = 0; i < addr_bits; ++i) {
    auto addr_net = createBTermNet(fmt::format("A{}", i));
    if (!addr_net) {
      logger_->error(utl::RAM, 1204, "Failed to create address net A{}", i);
      return nullptr;
    }
    address.push_back(addr_net);
  }
  
  // Create data output nets
  int read_ports = config_.getReadPorts();
  std::vector<std::array<odb::dbNet*, 32>> data_out(read_ports);
  for (int rp = 0; rp < read_ports; ++rp) {
    for (int i = 0; i < data_width && i < 32; ++i) {
      data_out[rp][i] = createBTermNet(fmt::format("Do{}_{}", rp, i));
      if (!data_out[rp][i]) {
        logger_->error(utl::RAM, 1205, "Failed to create data output net Do{}_{}", rp, i);
        return nullptr;
      }
    }
  }
  
  // Create write enable nets
  std::vector<odb::dbNet*> we_nets(config_.getWordCount());
  for (int w = 0; w < config_.getWordCount(); ++w) {
    we_nets[w] = createBTermNet(fmt::format("WE{}", w));
    if (!we_nets[w]) {
      logger_->error(utl::RAM, 1206, "Failed to create write enable net WE{}", w);
      return nullptr;
    }
  }
  
  // If 1RW1R, create additional read enable net
  odb::dbNet* read_enable = nullptr;
  if (config_.getType() == RAM_1RW1R) {
    read_enable = createBTermNet("RE");
    if (!read_enable) {
      logger_->error(utl::RAM, 1207, "Failed to create read enable net");
      return nullptr;
    }
  }
  
  // Build the appropriate memory hierarchy based on size
  std::unique_ptr<Element> top_element;
  
  if (config_.getWordCount() <= 8) {
    // Small memory using RAM8
    top_element = createRAM8("ram8", config_.getReadPorts(), 
                            clk, we_nets, address, data_in, data_out);
    if (!top_element) {
      logger_->error(utl::RAM, 1208, "Failed to create RAM8 element");
      return nullptr;
    }
  }
  else if (config_.getWordCount() <= 32) {
    // Medium memory using RAM32
    top_element = createRAM32("ram32", config_.getReadPorts(), 
                             clk, we_nets, address, data_in, data_out);
    if (!top_element) {
      logger_->error(utl::RAM, 1209, "Failed to create RAM32 element");
      return nullptr;
    }
  }
  // Other size options...
  
  // If 1RW1R, add read port
  if (config_.getType() == RAM_1RW1R && read_enable) {
    auto read_port = createReadPort("read_port", clk, read_enable, address);
    if (!read_port) {
      logger_->error(utl::RAM, 179, "Failed to create read port element");
      return nullptr;
    }
    
    // Add read port to top-level hierarchy
    if (top_element) {
      top_element->addChild(std::move(read_port));
    }
  }
  
  logger_->info(utl::RAM, 1210, "Memory hierarchy built successfully");
  return top_element;
}



void HierarchyBuilder::findMasters()
{
  logger_->info(utl::RAM, 1220, "Finding cell masters");

  // Safer version that avoids database access issues
  auto findCellMatch = [&](const char* name, odb::dbMaster** target) {
    if (*target) return; // Already set
    
    // Check if we have a database and block
    if (!block_) {
      logger_->error(utl::RAM, 1280, "No block available for cell lookup");
      return;
    }
    
    odb::dbDatabase* db = block_->getDb();
    if (!db) {
      logger_->error(utl::RAM, 1281, "No database available for cell lookup");
      return;
    }
    
    // Try a simpler name-based approach
    for (auto lib : db->getLibs()) {
      if (!lib) continue; // Skip null libraries
      
      for (auto master : lib->getMasters()) {
        if (!master) continue; // Skip null masters
        
        std::string masterName = master->getName();
        
        // Simple pattern matching based on name
        bool matches = false;
        
        if (strcmp(name, "storage") == 0) {
          matches = (masterName.find("dff") != std::string::npos || 
                    masterName.find("latch") != std::string::npos);
        } 
        else if (strcmp(name, "tristate") == 0) {
          matches = (masterName.find("einv") != std::string::npos || 
                    masterName.find("tri") != std::string::npos);
        }
        else if (strcmp(name, "inverter") == 0) {
          matches = (masterName.find("inv") != std::string::npos);
        }
        else if (strcmp(name, "and2") == 0) {
          matches = (masterName.find("and") != std::string::npos);
        }
        
        if (matches) {
          *target = master;
          masters_cache_[name] = master;
          logger_->info(utl::RAM, 1296, "Found {} cell: {}", name, master->getName());
          return;
        }
      }
    }
    
    logger_->warn(utl::RAM, 1297, "Could not find {} cell", name);
  };
  
  // Find essential cells
  findCellMatch("storage", &storage_cell_);
  findCellMatch("tristate", &tristate_cell_);
  findCellMatch("inverter", &inv_cell_);
  findCellMatch("and2", &and2_cell_);
  findCellMatch("clock_gate", &clock_gate_cell_);
  findCellMatch("mux2", &mux2_cell_);
}

odb::dbMaster* HierarchyBuilder::findMaster(const std::function<bool(sta::LibertyPort*)>& match, const char* name)
{
  // First check if the master is in our cache
  auto it = masters_cache_.find(name);
  if (it != masters_cache_.end()) {
    return it->second;
  }
  
  // Validate database access
  if (!block_) {
    logger_->error(utl::RAM, 1225, "No block available for findMaster");
    return nullptr;
  }
  
  odb::dbDatabase* db = block_->getDb();
  if (!db) {
    logger_->error(utl::RAM, 1226, "No database available for findMaster");
    return nullptr;
  }

  // Find the best matching master
  odb::dbMaster* best = nullptr;
  float best_area = std::numeric_limits<float>::max();

  // Try Liberty-based matching if network is available
  if (network_) {
    for (auto lib : db->getLibs()) {
      if (!lib) continue; // Safety check
      
      for (auto master : lib->getMasters()) {
        if (!master) continue; // Safety check
        
        try {
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
        } catch (...) {
          // Catch any Liberty-related exceptions and continue
          continue;
        }
      }
    }
  } else {
    // Simple name-based matching as fallback when network is unavailable
    logger_->warn(utl::RAM, 1227, "Using name-based matching for {} (no network available)", name);
    
    // Define name patterns for various cell types
    std::vector<std::string> patterns;
    if (strcmp(name, "storage") == 0) {
      patterns = {"dff", "latch", "dlxtp", "dfxtp"};
    } else if (strcmp(name, "tristate") == 0) {
      patterns = {"einv", "tri", "tbuf"};
    } else if (strcmp(name, "inverter") == 0) {
      patterns = {"inv", "not"};
    } else if (strcmp(name, "and2") == 0) {
      patterns = {"and2", "and_2"};
    } else if (strcmp(name, "clock_gate") == 0) {
      patterns = {"clkgate", "dlclkp"};
    }
    
    // Search for matching cells
    for (auto lib : db->getLibs()) {
      if (!lib) continue;
      
      for (auto master : lib->getMasters()) {
        if (!master) continue;
        
        std::string masterName = master->getName();
        for (const auto& pattern : patterns) {
          if (masterName.find(pattern) != std::string::npos) {
            float area = master->getWidth() * master->getHeight();
            if (area < best_area) {
              best_area = area;
              best = master;
            }
            break;
          }
        }
      }
    }
  }

  // Cache and return the result
  if (best) {
    masters_cache_[name] = best;
    logger_->info(utl::RAM, 1228, "Found {} cell: {}", name, best->getName());
  } else {
    logger_->warn(utl::RAM, 1229, "Could not find {} cell", name);
  }
  
  return best;
}

odb::dbMaster* HierarchyBuilder::getMaster(const std::string& name)
{
  // Check if we already found this master
  auto it = masters_cache_.find(name);
  if (it != masters_cache_.end()) {
    return it->second;
  }
  
  // Validate database access
  if (!block_) {
    logger_->error(utl::RAM, 1235, "No block available for getMaster");
    return nullptr;
  }
  
  odb::dbDatabase* db = block_->getDb();
  if (!db) {
    logger_->error(utl::RAM, 1236, "No database available for getMaster");
    return nullptr;
  }
  
  // Look up the master in the libraries
  for (auto lib : db->getLibs()) {
    if (!lib) continue; // Safety check
    
    odb::dbMaster* master = lib->findMaster(name.c_str());
    if (master) {
      masters_cache_[name] = master;
      return master;
    }
  }
  
  logger_->warn(utl::RAM, 1237, "Master '{}' not found in any library", name);
  return nullptr;
}

odb::dbNet* HierarchyBuilder::createNet(const std::string& name)
{
  // Check if net already exists in cache
  auto it = nets_cache_.find(name);
  if (it != nets_cache_.end()) {
    return it->second;
  }
  
  // Create a new net
  auto net = odb::dbNet::create(block_, name.c_str());
  nets_cache_[name] = net;
  return net;
}

odb::dbNet* HierarchyBuilder::createBTermNet(const std::string& name)
{
  // First validate block and database
  if (!block_) {
    logger_->error(utl::RAM, 1305, "Cannot create BTerm - block is null");
    return nullptr;
  }


  // Check if net already exists in cache
  auto it = nets_cache_.find(name);
  if (it != nets_cache_.end()) {
    logger_->info(utl::RAM, 1306, "Reusing existing net for {}", name);
    return it->second;
  }

  // Check if net already exists in block before creating
  odb::dbNet* existing_net = block_->findNet(name.c_str());
  if (existing_net) {
    logger_->info(utl::RAM, 1307, "Using existing block net for {}", name);
    nets_cache_[name] = existing_net;
    return existing_net;
  }

  try {
    // Create a new net with a block terminal
    odb::dbNet* net = odb::dbNet::create(block_, name.c_str());
    if (!net) {
      logger_->error(utl::RAM, 1308, "Failed to create net {}", name);
      return nullptr;
    }
    
    // Create the BTerm
    odb::dbBTerm* bterm = odb::dbBTerm::create(net, name.c_str());
    if (!bterm) {
      // Clean up if BTerm creation fails
      odb::dbNet::destroy(net);
      logger_->error(utl::RAM, 1309, "Failed to create BTerm {}", name);
      return nullptr;
    }
    
    // Cache the new net
    nets_cache_[name] = net;
    return net;
  }
  catch (const std::exception& e) {
    logger_->error(utl::RAM, 1310, "Exception creating net {}: {}", name, e.what());
    return nullptr;
  }
  catch (...) {
    logger_->error(utl::RAM, 1311, "Unknown exception creating net {}", name);
    return nullptr;
  }
  
}

odb::dbInst* HierarchyBuilder::createInst(const std::string& name, odb::dbMaster* master,
                                         const std::vector<std::pair<std::string, odb::dbNet*>>& connections)
{
  // Create a new instance
  auto inst = odb::dbInst::create(block_, master, name.c_str());
  
  // Connect pins to nets
  for (const auto& [term_name, net] : connections) {
    auto mterm = master->findMTerm(term_name.c_str());
    if (!mterm) {
      logger_->error(utl::RAM, 1260, "Term {} not found on master {}", 
                    term_name, master->getName());
      continue;
    }
    inst->getITerm(mterm)->connect(net);
  }
  
  return inst;
}

//----------------------------------------------
// Building Block Methods Implementation
//----------------------------------------------

std::unique_ptr<Element> HierarchyBuilder::createBit(const std::string& prefix, 
                                                    int read_ports,
                                                    odb::dbNet* clock,
                                                    odb::dbNet* data_in,
                                                    const std::vector<odb::dbNet*>& select,
                                                    std::vector<odb::dbNet*>& data_out)
{
  logger_->info(utl::RAM, 1300, "Creating bit cell {}", prefix);
  
  if ((int)select.size() < read_ports) {
    logger_->error(utl::RAM, 521, "Bit {}: Select vector too small ({} < {})",
                  prefix, select.size(), read_ports);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto storage_net = createNet(fmt::format("{}.storage", prefix));

  // Create storage element (DFF)
  auto dff_inst = createInst(fmt::format("{}.dff", prefix), storage_cell_,
                           {{"GATE", clock}, {"D", data_in}, {"Q", storage_net}});
  
  // Create Element with BIT type
  std::unique_ptr<Element> bit_elem = std::make_unique<Element>(dff_inst, Element::BIT);

  // Create tri-state buffers for each read port
  for (int rp = 0; rp < read_ports; ++rp) {
    auto tbuf_inst = createInst(fmt::format("{}.tbuf{}", prefix, rp), tristate_cell_,
                              {{"A", storage_net}, {"TE_B", select[rp]}, {"Z", data_out[rp]}});
    bit_elem->addChild(std::make_unique<Element>(tbuf_inst));
  }

  return bit_elem;
}


std::unique_ptr<Element> HierarchyBuilder::createByte(const std::string& prefix,
                                                     int read_ports,
                                                     odb::dbNet* clock,
                                                     odb::dbNet* write_enable,
                                                     const std::vector<odb::dbNet*>& selects,
                                                     const std::array<odb::dbNet*, 8>& data_input,
                                                     const std::vector<std::array<odb::dbNet*, 8>>& data_output)
{
  logger_->info(utl::RAM, 1320, "Creating byte {}", prefix);
  
  if (selects.empty()) {
    logger_->error(utl::RAM, 1330, "Byte {}: No select signals", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto byte_elem = std::make_unique<Element>(std::move(layout), Element::BYTE);

  // Invert the select signals for tristate control
  std::vector<odb::dbNet*> select_b_nets;
  select_b_nets.reserve(read_ports);
  for (int i = 0; i < read_ports; ++i) {
    auto select_b = createNet(fmt::format("{}.select{}_b", prefix, i));
    auto inv_inst = createInst(fmt::format("{}.inv{}", prefix, i), inv_cell_,
                             {{"A", selects[0]}, {"Y", select_b}});
    byte_elem->addChild(std::make_unique<Element>(inv_inst));
    select_b_nets.push_back(select_b);
  }

  // Clock gating
  auto clock_b = createNet(fmt::format("{}.clock_b", prefix));
  auto gclk = createNet(fmt::format("{}.gclk", prefix));
  auto we_gated = createNet(fmt::format("{}.we_gated", prefix));

  // Create clock inverter
  auto clk_inv_inst = createInst(fmt::format("{}.clock_inv", prefix), inv_cell_,
                               {{"A", clock}, {"Y", clock_b}});
  byte_elem->addChild(std::make_unique<Element>(clk_inv_inst));

  // Create write enable AND gate
  auto cg_and_inst = createInst(fmt::format("{}.cgand", prefix), and2_cell_,
                              {{"A", selects[0]}, {"B", write_enable}, {"X", we_gated}});
  byte_elem->addChild(std::make_unique<Element>(cg_and_inst));

  // MODIFIED: Create clock gate based on the actual cell type found
  std::string cell_name = clock_gate_cell_->getName();
  odb::dbInst* clk_gate_inst = nullptr;
  
  if (cell_name.find("clkbuf") != std::string::npos) {
    // For Sky130 clock buffers (A->X)
    logger_->info(utl::RAM, 1331, "Using clkbuf as clock gate with we_gated->A, X->gclk");
    clk_gate_inst = createInst(fmt::format("{}.cg", prefix), clock_gate_cell_,
                             {{"A", we_gated}, {"X", gclk}});
  } 
  else if (cell_name.find("dlclkp") != std::string::npos) {
    // For Sky130 latch-based clock gates
    logger_->info(utl::RAM, 1332, "Using dlclkp as clock gate");
    clk_gate_inst = createInst(fmt::format("{}.cg", prefix), clock_gate_cell_,
                             {{"CLK", clock_b}, {"GATE", we_gated}, {"GCLK", gclk}});
  }
  else {
    // Generic fallback - identify input/output pins
    logger_->info(utl::RAM, 1333, "Using generic approach for clock gate: {}", cell_name);
    
    // Find input and output pins
    std::string in_pin = "A";   // Default input pin name
    std::string out_pin = "X";  // Default output pin name
    
    // Check if the master actually has these pins
    bool found_in = false;
    bool found_out = false;
    
    for (auto mterm : clock_gate_cell_->getMTerms()) {
      std::string term_name = mterm->getName();
      if (mterm->getIoType() == odb::dbIoType::INPUT) {
        in_pin = term_name;
        found_in = true;
      }
      else if (mterm->getIoType() == odb::dbIoType::OUTPUT) {
        out_pin = term_name;
        found_out = true;
      }
    }
    
    if (found_in && found_out) {
      logger_->info(utl::RAM, 1334, "Found clock gate pins: in={}, out={}", in_pin, out_pin);
      clk_gate_inst = createInst(fmt::format("{}.cg", prefix), clock_gate_cell_,
                               {{in_pin, we_gated}, {out_pin, gclk}});
    }
    else {
      // Last resort: AND gate as clock gate
      logger_->warn(utl::RAM, 1335, "Using AND2 as clock gate fallback");
      clk_gate_inst = createInst(fmt::format("{}.cg", prefix), and2_cell_,
                               {{"A", clock_b}, {"B", we_gated}, {"X", gclk}});
    }
  }
  
  if (clk_gate_inst) {
    byte_elem->addChild(std::make_unique<Element>(clk_gate_inst));
  }
  else {
    logger_->error(utl::RAM, 1336, "Failed to create clock gate");
    return nullptr;
  }

  // Create 8 bits
  for (int bit = 0; bit < 8; ++bit) {
    std::vector<odb::dbNet*> bit_outputs;
    bit_outputs.reserve(read_ports);
    for (int rp = 0; rp < read_ports; ++rp) {
      bit_outputs.push_back(data_output[rp][bit]);
    }
    
    // Create bit cell
    auto bit_elem = createBit(fmt::format("{}.bit{}", prefix, bit),
                             read_ports, gclk,
                             data_input[bit], select_b_nets, bit_outputs);
    
    if (bit_elem) {
      byte_elem->addChild(std::move(bit_elem));
    }
  }

  return byte_elem;
}

std::unique_ptr<Element> HierarchyBuilder::createWord(const std::string& prefix,
                                                     int read_ports,
                                                     odb::dbNet* clock,
                                                     const std::vector<odb::dbNet*>& we_per_byte,
                                                     odb::dbNet* sel,
                                                     const std::array<odb::dbNet*, 32>& data_input,
                                                     const std::vector<std::array<odb::dbNet*, 32>>& data_output)
{
  logger_->info(utl::RAM, 1340, "Creating word {}", prefix);
  
  if (we_per_byte.size() < 4) {
    logger_->error(utl::RAM, 1350, "Word {}: not enough WE signals for 4 bytes", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto word_elem = std::make_unique<Element>(std::move(layout), Element::WORD);

  // Create clock buffer
  auto clkbuf_net = createNet(fmt::format("{}.clkbuf_out", prefix));
  auto clkbuf_inst = createInst(fmt::format("{}.clkbuf", prefix), inv_cell_,
                              {{"A", clock}, {"Y", clkbuf_net}});
  word_elem->addChild(std::make_unique<Element>(clkbuf_inst));

  // Create select buffers for each read port
  std::vector<odb::dbNet*> selbuf_nets;
  selbuf_nets.reserve(read_ports);
  for (int rp = 0; rp < read_ports; ++rp) {
    auto selbuf_net = createNet(fmt::format("{}.selbuf{}_out", prefix, rp));
    auto selbuf_inst = createInst(fmt::format("{}.selbuf{}", prefix, rp), inv_cell_,
                                {{"A", sel}, {"Y", selbuf_net}});
    word_elem->addChild(std::make_unique<Element>(selbuf_inst));
    selbuf_nets.push_back(selbuf_net);
  }

  // Create 4 bytes
  for (int b = 0; b < 4; ++b) {
    // Extract 8 data inputs for this byte
    std::array<odb::dbNet*, 8> di;
    for (int i = 0; i < 8; ++i) {
      di[i] = data_input[b * 8 + i];
    }
    
    // Extract data outputs for this byte
    std::vector<std::array<odb::dbNet*, 8>> do_arrays(read_ports);
    for (int rp = 0; rp < read_ports; ++rp) {
      for (int i = 0; i < 8; ++i) {
        do_arrays[rp][i] = data_output[rp][b * 8 + i];
      }
    }
    
    // Create byte
    auto byte_elem = createByte(fmt::format("{}.byte{}", prefix, b),
                               read_ports, clkbuf_net,
                               we_per_byte[b], selbuf_nets, di, do_arrays);
    
    if (byte_elem) {
      word_elem->addChild(std::move(byte_elem));
    }
  }

  return word_elem;
}

std::unique_ptr<Element> HierarchyBuilder::createDecoder(const std::string& prefix,
                                                        int address_bits,
                                                        const std::vector<odb::dbNet*>& inputs)
{
  logger_->info(utl::RAM, 1360, "Creating decoder {}", prefix);
  
  if ((int)inputs.size() != address_bits) {
    logger_->error(utl::RAM, 1370, "Decoder {}: need {} bits, got {}",
                  prefix, address_bits, inputs.size());
    return nullptr;
  }
  
  if (!inv_cell_ || !and2_cell_) {
    logger_->error(utl::RAM, 1380, "Missing cells for decoder {}", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto decoder_elem = std::make_unique<Element>(std::move(layout), Element::DECODER);
  
  // Create inverted address signals
  std::vector<odb::dbNet*> inverted(address_bits);
  for (int i = 0; i < address_bits; ++i) {
    inverted[i] = createNet(fmt::format("{}.A{}_bar", prefix, i));
    auto inv_inst = createInst(fmt::format("{}.inv{}", prefix, i),
                             inv_cell_,
                             {{"A", inputs[i]}, {"Y", inverted[i]}});
    decoder_elem->addChild(std::make_unique<Element>(inv_inst));
  }

  // Create decoder outputs (2^address_bits)
  int outputs = 1 << address_bits;
  for (int o = 0; o < outputs; ++o) {
    // For each output, determine which inputs (or inverted inputs) to use
    std::vector<odb::dbNet*> terms;
    for (int bit = 0; bit < address_bits; ++bit) {
      bool is_one = (o & (1 << bit)) != 0;
      terms.push_back(is_one ? inputs[bit] : inverted[bit]);
    }

    // Create AND gate tree
    odb::dbNet* current_net = terms[0];
    for (size_t t = 1; t < terms.size(); ++t) {
      auto new_net = createNet(fmt::format("{}.and_{}_{}", prefix, o, t));
      auto and_inst = createInst(fmt::format("{}.and_{}_{}", prefix, o, t),
                               and2_cell_,
                               {{"A", current_net}, {"B", terms[t]}, {"X", new_net}});
      decoder_elem->addChild(std::make_unique<Element>(and_inst));
      current_net = new_net;
    }

    // Create output buffer
    auto out_net = createNet(fmt::format("{}.dec_out{}", prefix, o));
    auto buf_inst = createInst(fmt::format("{}.buf{}", prefix, o),
                             inv_cell_,
                             {{"A", current_net}, {"Y", out_net}});
    decoder_elem->addChild(std::make_unique<Element>(buf_inst));
  }

  return decoder_elem;
}

std::unique_ptr<Element> HierarchyBuilder::createRAM8(const std::string& prefix,
                                                     int read_ports,
                                                     odb::dbNet* clock,
                                                     const std::vector<odb::dbNet*>& we_per_word,
                                                     const std::vector<odb::dbNet*>& addr3bit,
                                                     const std::array<odb::dbNet*, 32>& data_input,
                                                     const std::vector<std::array<odb::dbNet*, 32>>& data_output)
{
  logger_->info(utl::RAM, 1390, "Creating RAM8 {}", prefix);
  
  if (we_per_word.size() < 8) {
    logger_->error(utl::RAM, 1400, "RAM8 {}: need 8 WE signals", prefix);
    return nullptr;
  }
  
  if (addr3bit.size() != 3) {
    logger_->error(utl::RAM, 1410, "RAM8 {}: need exactly 3 address bits", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto ram8_elem = std::make_unique<Element>(std::move(layout), Element::RAM8);

  // Create 3-to-8 decoder
  auto decoder = createDecoder(fmt::format("{}.dec", prefix), 3, addr3bit);
  if (decoder) {
    ram8_elem->addChild(std::move(decoder));
  } else {
    logger_->error(utl::RAM, 1420, "RAM8 {}: Decoder creation failed", prefix);
    return nullptr;
  }

  // Create 8 words
  for (int w = 0; w < 8; ++w) {
    // Get decoder output for this word
    auto sel_w = block_->findNet(fmt::format("{}.dec.dec_out{}", prefix, w).c_str());
    if (!sel_w) {
      logger_->warn(utl::RAM, 1430, "RAM8 {}: No net for dec_out{}", prefix, w);
      continue;
    }
    
    // Create write enable signals for the 4 bytes in this word
    std::vector<odb::dbNet*> we_4(4, we_per_word[w]);
    
    // Create word
    auto word_elem = createWord(fmt::format("{}.word{}", prefix, w),
                              read_ports, clock,
                              we_4, sel_w, data_input, data_output);
    
    if (word_elem) {
      ram8_elem->addChild(std::move(word_elem));
    }
  }

  return ram8_elem;
}

std::unique_ptr<Element> HierarchyBuilder::createRAM32(const std::string& prefix,
                                                      int read_ports,
                                                      odb::dbNet* clock,
                                                      const std::vector<odb::dbNet*>& we_32,
                                                      const std::vector<odb::dbNet*>& addr5bit,
                                                      const std::array<odb::dbNet*, 32>& data_input,
                                                      const std::vector<std::array<odb::dbNet*, 32>>& data_output)
{
  logger_->info(utl::RAM, 1440, "Creating RAM32 {}", prefix);
  
  if (we_32.size() < 32) {
    logger_->error(utl::RAM, 1450, "RAM32 {}: need 32 WEs total", prefix);
    return nullptr;
  }
  
  if (addr5bit.size() < 5) {
    logger_->error(utl::RAM, 1460, "RAM32 {}: need 5 address bits", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto ram32_elem = std::make_unique<Element>(std::move(layout), Element::RAM32);

  // Use top 2 bits for selecting between 4 RAM8 blocks
  std::vector<odb::dbNet*> top_addr2(addr5bit.begin(), addr5bit.begin() + 2);
  
  // Create 2-to-4 decoder
  auto decoder = createDecoder(fmt::format("{}.dec2x4", prefix), 2, top_addr2);
  if (decoder) {
    ram32_elem->addChild(std::move(decoder));
  } else {
    logger_->error(utl::RAM, 1470, "RAM32 {}: 2x4 decoder creation failed", prefix);
    return nullptr;
  }

  // Use lower 3 bits for addressing within each RAM8 block
  std::vector<odb::dbNet*> sub_addr3(addr5bit.begin() + 2, addr5bit.end());

  // Create 4 RAM8 blocks
  for (int sub = 0; sub < 4; ++sub) {
    // Get decoder output for this RAM8 block
    auto sub_sel = block_->findNet(fmt::format("{}.dec2x4.dec_out{}", prefix, sub).c_str());
    if (!sub_sel) {
      logger_->warn(utl::RAM, 1480, "RAM32 {}: No net for dec2x4.dec_out{}", prefix, sub);
      continue;
    }
    
    // Extract 8 WE signals for this RAM8 block
    std::vector<odb::dbNet*> we_subblock;
    for (int w = sub * 8; w < (sub * 8) + 8; w++) {
      we_subblock.push_back(we_32[w]);
    }
    
    // Create RAM8 block
    auto ram8_elem = createRAM8(fmt::format("{}.ram8_{}", prefix, sub),
                               read_ports, clock,
                               we_subblock, sub_addr3,
                               data_input, data_output);
    
    if (ram8_elem) {
      ram32_elem->addChild(std::move(ram8_elem));
    }
  }

  return ram32_elem;
}

std::unique_ptr<Element> HierarchyBuilder::createRAM128(const std::string& prefix,
                                                       int read_ports,
                                                       odb::dbNet* clock,
                                                       const std::vector<odb::dbNet*>& we_128,
                                                       const std::vector<odb::dbNet*>& addr7bit,
                                                       const std::array<odb::dbNet*, 32>& data_input,
                                                       const std::vector<std::array<odb::dbNet*, 32>>& data_output)
{
  logger_->info(utl::RAM, 1490, "Creating RAM128 {}", prefix);
  
  if (we_128.size() < 128) {
    logger_->error(utl::RAM, 1777, "RAM128 {}: need 128 WEs total", prefix);
    return nullptr;
  }
  
  if (addr7bit.size() < 7) {
    logger_->error(utl::RAM, 1110, "RAM128 {}: need 7 address bits", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto ram128_elem = std::make_unique<Element>(std::move(layout), Element::RAM128);

  // Use top 2 bits for selecting between 4 RAM32 blocks
  std::vector<odb::dbNet*> top_addr2(addr7bit.begin(), addr7bit.begin() + 2);
  
  // Create 2-to-4 decoder
  auto decoder = createDecoder(fmt::format("{}.dec2x4", prefix), 2, top_addr2);
  if (decoder) {
    ram128_elem->addChild(std::move(decoder));
  } else {
    logger_->error(utl::RAM, 1520, "RAM128 {}: 2x4 decoder creation failed", prefix);
    return nullptr;
  }

  // Use lower 5 bits for addressing within each RAM32 block
  std::vector<odb::dbNet*> sub_addr5(addr7bit.begin() + 2, addr7bit.end());

  // Create 4 RAM32 blocks
  for (int sub = 0; sub < 4; ++sub) {
    // Get decoder output for this RAM32 block
    auto sub_sel = block_->findNet(fmt::format("{}.dec2x4.dec_out{}", prefix, sub).c_str());
    if (!sub_sel) {
      logger_->warn(utl::RAM, 1530, "RAM128 {}: No net for dec2x4.dec_out{}", prefix, sub);
      continue;
    }
    
    // Extract 32 WE signals for this RAM32 block
    std::vector<odb::dbNet*> we_subblock;
    for (int w = sub * 32; w < (sub * 32) + 32; w++) {
      we_subblock.push_back(we_128[w]);
    }
    
    // Create RAM32 block
    auto ram32_elem = createRAM32(fmt::format("{}.ram32_{}", prefix, sub),
                                read_ports, clock,
                                we_subblock, sub_addr5,
                                data_input, data_output);
    
    if (ram32_elem) {
      ram128_elem->addChild(std::move(ram32_elem));
    }
  }

  return ram128_elem;
}

std::unique_ptr<Element> HierarchyBuilder::createRAM512(const std::string& prefix,
                                                       int read_ports,
                                                       odb::dbNet* clock,
                                                       const std::vector<odb::dbNet*>& we_512,
                                                       const std::vector<odb::dbNet*>& addr9bit,
                                                       const std::array<odb::dbNet*, 32>& data_input,
                                                       const std::vector<std::array<odb::dbNet*, 32>>& data_output)
{
  logger_->info(utl::RAM, 1540, "Creating RAM512 {}", prefix);
  
  if (we_512.size() < 512) {
    logger_->error(utl::RAM, 1550, "RAM512 {}: need 512 WEs total", prefix);
    return nullptr;
  }
  
  if (addr9bit.size() < 9) {
    logger_->error(utl::RAM, 1560, "RAM512 {}: need 9 address bits", prefix);
    return nullptr;
  }

  auto layout = std::make_unique<Layout>(odb::horizontal);
  auto ram512_elem = std::make_unique<Element>(std::move(layout), Element::RAM512);

  // Use top 2 bits for selecting between 4 RAM128 blocks
  std::vector<odb::dbNet*> top_addr2(addr9bit.begin(), addr9bit.begin() + 2);
  
  // Create 2-to-4 decoder
  auto decoder = createDecoder(fmt::format("{}.dec2x4", prefix), 2, top_addr2);
  if (decoder) {
    ram512_elem->addChild(std::move(decoder));
  } else {
    logger_->error(utl::RAM, 1570, "RAM512 {}: 2x4 decoder creation failed", prefix);
    return nullptr;
  }

  // Use lower 7 bits for addressing within each RAM128 block
  std::vector<odb::dbNet*> sub_addr7(addr9bit.begin() + 2, addr9bit.end());

  // Create 4 RAM128 blocks
  for (int sub = 0; sub < 4; ++sub) {
    // Get decoder output for this RAM128 block
    auto sub_sel = block_->findNet(fmt::format("{}.dec2x4.dec_out{}", prefix, sub).c_str());
    if (!sub_sel) {
      logger_->warn(utl::RAM, 1580, "RAM512 {}: No net for dec2x4.dec_out{}", prefix, sub);
      continue;
    }
    
    // Extract 128 WE signals for this RAM128 block
    std::vector<odb::dbNet*> we_subblock;
    for (int w = sub * 128; w < (sub * 128) + 128; w++) {
      we_subblock.push_back(we_512[w]);
    }
    
    // Create RAM128 block
    auto ram128_elem = createRAM128(fmt::format("{}.ram128_{}", prefix, sub),
                                  read_ports, clock,
                                  we_subblock, sub_addr7,
                                  data_input, data_output);
    
    if (ram128_elem) {
      ram512_elem->addChild(std::move(ram128_elem));
    }
  }

  return ram512_elem;
}

std::unique_ptr<Element> HierarchyBuilder::createReadPort(const std::string& prefix,
                                                         odb::dbNet* clock,
                                                         odb::dbNet* read_enable,
                                                         const std::vector<odb::dbNet*>& addr)
{
  logger_->info(utl::RAM, 1590, "Creating read port {}", prefix);
  
  // For 1RW1R memories, this creates an additional read port
  auto layout = std::make_unique<Layout>(odb::horizontal);
  
  // Create read port structure
  // This would involve:
  // 1. Address decoders
  // 2. Read enable logic
  // 3. Read sense amplifiers or output staging
  
  // For now, we'll create a simple placeholder structure
  auto read_en_buf = createNet(fmt::format("{}.read_en_buf", prefix));
  auto read_en_inst = createInst(fmt::format("{}.read_en_buf", prefix), inv_cell_,
                               {{"A", read_enable}, {"Y", read_en_buf}});
  
  auto read_port_elem = std::make_unique<Element>(read_en_inst);
  
  // In a real implementation, we'd create the full read port circuitry
  
  return read_port_elem;
}

} // namespace ram