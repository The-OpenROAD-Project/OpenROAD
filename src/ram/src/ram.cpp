#include "ram/ram.h"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <bit>
#include "sta/FuncExpr.hh"
#include "db_sta/dbNetwork.hh"
//#include "def/defwWriter.hpp"


namespace ram {

using odb::dbBlock;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbMaster;
using odb::dbNet;
using sta::FuncExpr;
using sta::LibertyCell;
using sta::PortDirection;

//----------------------------------------------
// RamGen Implementation
//----------------------------------------------

RamGen::RamGen()
  : db_(nullptr),
    block_(nullptr),
    network_(nullptr),
    logger_(nullptr),
    storage_cell_(nullptr),
    tristate_cell_(nullptr),
    inv_cell_(nullptr),
    and2_cell_(nullptr),
    clock_gate_cell_(nullptr),
    mux2_cell_(nullptr)
{
  // Initialize row configuration with default values
  row_config_.rows_count = 32;
  row_config_.site_width = 0.46;  // Default for SKY130
  row_config_.site_height = 2.72; // Default for SKY130
  row_config_.tap_distance = 20;  // Default for SKY130
  row_config_.tap_cell_pattern = "sky130_fd_sc_hd__tap_"; // Default pattern
  row_config_.filler_cell_patterns = {
    "sky130_fd_sc_hd__fill_",
    "sky130_fd_sc_hd__decap_"
  };
}

void RamGen::init(odb::dbDatabase* db, sta::dbNetwork* network, Logger* logger)
{
  db_ = db;
  network_ = network;
  logger_ = logger;
  
  // Validate database is available
  if (!db_) {
    logger_->error(utl::RAM, 1500, "No database provided during initialization");
    return;
  }
  
  logger_->info(utl::RAM, 1501, "RAM generator initialized with database");
}

bool RamGen::validateDatabase(const char* caller_name)
{
  if (!db_) {
    logger_->error(utl::RAM, 1510, "{}: No database available", caller_name);
    return false;
  }
  
  if (!network_) {
    logger_->warn(utl::RAM, 1511, "{}: No network available, using simplified cell matching", caller_name);
    // Continue, we'll use name-based matching instead
  }
  
  if (!logger_) {
    // Can't log this error since logger is null!
    return false;
  }
  
  return true;
}



void RamGen::setupTechnologyCells()
{
  // Guard against invalid database
  if (!validateDatabase("setupTechnologyCells")) {
    return;
  }

  logger_->info(utl::RAM, 710, "Finding technology cells in libraries");
  
  // Skip if already set
  if (storage_cell_ && tristate_cell_ && inv_cell_ && and2_cell_) {
    logger_->info(utl::RAM, 700, "Using provided technology cells");
    return;
  }
  
  // Function to find a master with specific characteristics - safer version
  auto findCell = [&](const char* name, odb::dbMaster** target) {
    if (*target) return; // Already set
    
    // Direct name search with enhanced Sky130 patterns
    auto directSearch = [&]() -> odb::dbMaster* {
      if (strcmp(name, "storage") == 0) {
        const char* common_names[] = {
          "dff", "dlxtp", "dfrtp", "dfxtp", "sdfxtp", "sdfrtp", "latch"
        };
        for (const char* pattern : common_names) {
          for (auto lib : db_->getLibs()) {
            if (!lib) continue; // Safety check
            for (auto master : lib->getMasters()) {
              if (!master) continue; // Safety check
              std::string masterName = master->getName();
              if (masterName.find(pattern) != std::string::npos) {
                return master;
              }
            }
          }
        }
      }
      else if (strcmp(name, "tristate") == 0) {
        const char* common_names[] = {
          "einvn", "einvp", "tbuf", "tri"
        };
        for (const char* pattern : common_names) {
          for (auto lib : db_->getLibs()) {
            if (!lib) continue;
            for (auto master : lib->getMasters()) {
              if (!master) continue;
              std::string masterName = master->getName();
              if (masterName.find(pattern) != std::string::npos) {
                return master;
              }
            }
          }
        }
      }
      else if (strcmp(name, "inverter") == 0) {
        const char* common_names[] = {
          "inv", "not", "clkinv"
        };
        for (const char* pattern : common_names) {
          for (auto lib : db_->getLibs()) {
            if (!lib) continue;
            for (auto master : lib->getMasters()) {
              if (!master) continue;
              std::string masterName = master->getName();
              if (masterName.find(pattern) != std::string::npos) {
                return master;
              }
            }
          }
        }
      }
      else if (strcmp(name, "and2") == 0) {
        // Enhanced Sky130-specific AND2 patterns
        for (auto lib : db_->getLibs()) {
          if (!lib) continue;
          for (auto master : lib->getMasters()) {
            if (!master) continue;
            std::string masterName = master->getName();
            if (masterName.find("sky130_fd_sc_hd__and2_") != std::string::npos ||
                masterName.find("sky130_fd_sc_hd__and2b_") != std::string::npos) {
              return master;
            }
          }
        }
      }
      else if (strcmp(name, "clock_gate") == 0) {
        // Enhanced Sky130-specific clock gate patterns
        for (auto lib : db_->getLibs()) {
          if (!lib) continue;
          for (auto master : lib->getMasters()) {
            if (!master) continue;
            std::string masterName = master->getName();
            if (masterName.find("sky130_fd_sc_hd__dlclkp_") != std::string::npos ||
                masterName.find("sky130_fd_sc_hd__clkbuf_") != std::string::npos) {
              return master;
            }
          }
        }
      }
      else if (strcmp(name, "mux2") == 0) {
        // Enhanced Sky130-specific MUX2 patterns
        for (auto lib : db_->getLibs()) {
          if (!lib) continue;
          for (auto master : lib->getMasters()) {
            if (!master) continue;
            std::string masterName = master->getName();
            if (masterName.find("sky130_fd_sc_hd__mux2_") != std::string::npos ||
                masterName.find("sky130_fd_sc_hd__mux2i_") != std::string::npos ||
                masterName.find("sky130_fd_sc_hd__a21oi_") != std::string::npos) {
              return master;
            }
          }
        }
      }
      return nullptr;
    };
    
    // Try direct name search first
    odb::dbMaster* best = directSearch();
    
    // If not found and network is available, try liberty-based search
    if (!best && network_) {
      dbMaster* best_master = nullptr;
      float best_area = std::numeric_limits<float>::max();
  
      for (auto lib : db_->getLibs()) {
        if (!lib) continue;
        
        for (auto master : lib->getMasters()) {
          if (!master) continue;
          
          try {
            auto cell = network_->dbToSta(master);
            if (!cell) continue;
  
            auto liberty = network_->libertyCell(cell);
            if (!liberty) continue;
  
            // Enhanced Liberty matchers from GitHub implementation
            bool matches = false;
            
            if (strcmp(name, "storage") == 0) {
              // Enhanced storage cell matcher
              matches = liberty->hasSequentials();
              
              // Optional: More precise check for specific port
              if (matches) {
                sta::LibertyCellPortIterator port_iter(liberty);
                matches = false; // Reset to find exact port match
                while (port_iter.hasNext()) {
                  auto port = port_iter.next();
                  if (port->direction()->isOutput() && 
                      std::strcmp(port->name(), "Q") == 0) {
                    matches = true;
                    break;
                  }
                }
              }
            } 
            else if (strcmp(name, "tristate") == 0) {
              // Enhanced tristate matcher from GitHub
              sta::LibertyCellPortIterator port_iter(liberty);
              while (port_iter.hasNext()) {
                auto port = port_iter.next();
                if (port->direction()->isTristate() && 
                    std::strcmp(port->name(), "Z") == 0) {
                  matches = true;
                  break;
                }
              }
            }
            else if (strcmp(name, "inverter") == 0) {
              // Enhanced inverter matcher from GitHub
              matches = liberty->isInverter() && 
                       liberty->findLibertyPort("Y") != nullptr &&
                       liberty->findLibertyPort("Y")->direction()->isOutput();
            }
            else if (strcmp(name, "and2") == 0) {
              // Enhanced AND2 matcher from GitHub
              sta::LibertyCellPortIterator port_iter(liberty);
              sta::LibertyPort* out_port = nullptr;
              bool reject = false;
              
              // Find output port
              while (port_iter.hasNext()) {
                auto port = port_iter.next();
                if (port->direction()->isAnyOutput()) {
                  if (!out_port) {
                    out_port = port;
                  } else {
                    reject = true; // More than one output port
                    break;
                  }
                }
              }
              
              if (!reject && out_port && out_port->function()) {
                auto func = out_port->function();
                matches = func->op() == sta::FuncExpr::op_and &&
                         func->left()->op() == sta::FuncExpr::op_port &&
                         func->right()->op() == sta::FuncExpr::op_port;
              }
            }
            else if (strcmp(name, "mux2") == 0) {
              // Enhanced MUX2 matcher - look for OR of ANDs structure
              sta::LibertyCellPortIterator port_iter(liberty);
              sta::LibertyPort* out_port = nullptr;
              bool reject = false;
              
              // Find output port
              while (port_iter.hasNext()) {
                auto port = port_iter.next();
                if (port->direction()->isAnyOutput()) {
                  if (!out_port) {
                    out_port = port;
                  } else {
                    reject = true; // More than one output port
                    break;
                  }
                }
              }
              
              if (!reject && out_port && out_port->function()) {
                auto func = out_port->function();
                matches = func->op() == sta::FuncExpr::op_or &&
                         func->left()->op() == sta::FuncExpr::op_and &&
                         func->right()->op() == sta::FuncExpr::op_and;
              }
            }
            
            // Select the smallest area cell that matches
            if (matches) {
              float area = liberty->area();
              if (area < best_area) {
                best_area = area;
                best_master = master;
              }
            }
          } catch (...) {
            // Safely handle Liberty exceptions
            continue;
          }
        }
      }
      
      best = best_master;
    }
  
    if (best) {
      *target = best;
      masters_[name] = best;
      logger_->info(utl::RAM, 720, "Found {} cell: {}", name, best->getName());
    } else {
      logger_->warn(utl::RAM, 730, "Could not find {} cell", name);
    }
  };
  
  // Find all required cell types
  findCell("storage", &storage_cell_);
  findCell("tristate", &tristate_cell_);
  findCell("inverter", &inv_cell_);
  findCell("and2", &and2_cell_);
  findCell("clock_gate", &clock_gate_cell_);
  findCell("mux2", &mux2_cell_);
  
  // Relaxed validation as suggested in second snippet
  if (!storage_cell_ || !tristate_cell_ || !inv_cell_) {
    logger_->error(utl::RAM, 8940, "Missing essential technology cells");
  } else {
    // Log warnings but continue if secondary cells are missing
    if (!and2_cell_) logger_->warn(utl::RAM, 8941, "AND2 cell not found, some functionality may be limited");
    if (!clock_gate_cell_) logger_->warn(utl::RAM, 8942, "Clock gate cell not found, using workaround");
    if (!mux2_cell_) logger_->warn(utl::RAM, 8943, "MUX2 cell not found, using workaround");
  }
}















//----------------------------------------------
// Original generate() method for backward compatibility
//----------------------------------------------
void RamGen::generate(int bytes_per_word,
  int word_count,
  int read_ports,
  odb::dbMaster* storage_cell,
  odb::dbMaster* tristate_cell,
  odb::dbMaster* inv_cell)
{
// Store the cell masters if provided
storage_cell_ = storage_cell;
tristate_cell_ = tristate_cell;
inv_cell_ = inv_cell;

// Call setupTechnologyCells to find additional cells or validate provided ones
setupTechnologyCells();

// Validate all required cells are found
if (!storage_cell_ || !tristate_cell_ || !inv_cell_ || !and2_cell_) {
logger_->error(utl::RAM, 750, "Missing required technology cells");
return;
}

// Validate configuration
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

// Create or get block
odb::dbChip* chip = db_->getChip();
if (!chip) {
chip = odb::dbChip::create(db_);
}
block_ = chip->getBlock();
if (!block_) {
std::string block_name = fmt::format("RAM_{}x{}x{}", word_count, bytes_per_word, 8);
block_ = odb::dbBlock::create(chip, block_name.c_str());
}

// For simplicity, delegate to the new implementation
MemoryConfig config(bytes_per_word, word_count, read_ports, RAM_1RW);
generateDFFRAM(bytes_per_word, word_count, read_ports, RAM_1RW, "", false);
}

//----------------------------------------------
// Enhanced method with DFFRAM-like capabilities
//----------------------------------------------
void RamGen::generateDFFRAM(int bytes_per_word,
  int word_count,
  int read_ports,
  MemoryType mem_type,
  const std::string& output_def,
  bool optimize_layout)
{
logger_->info(utl::RAM, 500, "Generating DFFRAM-style memory: {} words x {} bytes, {} type",
word_count, bytes_per_word, getMemoryTypeName(mem_type));

// Validate database is available
if (!validateDatabase("generateDFFRAM")) {
return;
}

// Create chip and block if needed
odb::dbChip* chip = db_->getChip();
if (!chip) {
chip = odb::dbChip::create(db_);
}

// Create a new block with appropriate name
std::string block_name = fmt::format("RAM_{}x{}x{}_{}", 
            word_count, bytes_per_word, 8, 
            getMemoryTypeName(mem_type));
block_ = odb::dbBlock::create(chip, block_name.c_str());

// Configure the memory
MemoryConfig config(bytes_per_word, word_count, read_ports, mem_type);

// Initialize or validate technology cells
setupTechnologyCells();

// Check if we have all necessary masters
if (!storage_cell_ || !tristate_cell_ || !inv_cell_ || !and2_cell_) {
logger_->error(utl::RAM, 740, "Missing essential technology cells");
return;
}

// Create the hierarchy builder
HierarchyBuilder builder(block_, config, logger_, network_);

// Pass the masters we already found
builder.setMasters(storage_cell_, tristate_cell_, inv_cell_, 
and2_cell_, clock_gate_cell_, mux2_cell_);

// Build the memory hierarchy
auto hierarchy = builder.buildHierarchy();

// Create the layout builder, passing any available cell masters for taps/fillers
LayoutBuilder layoutBuilder(block_, config, row_config_, logger_);

// Build the layout
layoutBuilder.buildLayout(std::move(hierarchy));

// Optimize the layout if requested
if (optimize_layout) {
layoutBuilder.optimizeLayout();
}

// Write DEF file if requested
if (!output_def.empty()) {
if (layoutBuilder.writeToDefFile(output_def)) {
logger_->info(utl::RAM, 510, "Wrote DEF file to {}", output_def);
} else {
logger_->error(utl::RAM, 511, "Failed to write DEF file to {}", output_def);
}
}

logger_->info(utl::RAM, 520, "DFFRAM-style memory generation completed successfully");
}

void RamGen::generateRegisterFile(int word_count,
        int word_width,
        const std::string& output_def)
{
logger_->info(utl::RAM, 600, "Generating register file: {} words x {} bits",
word_count, word_width);

// Initialize technology cells
setupTechnologyCells();

// Check if we have all necessary masters
if (!storage_cell_ || !tristate_cell_ || !inv_cell_ || !and2_cell_) {
logger_->error(utl::RAM, 506, "Missing essential technology cells");
return;
}

// Calculate bytes per word (rounded up)
int bytes_per_word = (word_width + 7) / 8;

// Use the DFFRAM-style generator with 2R1W memory type
generateDFFRAM(bytes_per_word, word_count, 2, RAM_2R1W, output_def, true);

logger_->info(utl::RAM, 610, "Register file generation completed successfully");
}

//----------------------------------------------
// Helper methods
//----------------------------------------------
const char* RamGen::getMemoryTypeName(MemoryType type) const
{
  switch (type) {
    case RAM_1RW: return "1RW";
    case RAM_1RW1R: return "1RW1R";
    case RAM_2R1W: return "2R1W";
    default: return "Unknown";
  }
}


//----------------------------------------------
// Write DEF file
//----------------------------------------------
bool RamGen::writeDefFile(const std::string& filename) const
{
  logger_->info(utl::RAM, 811, "Writing DEF file to {}", filename);

  if (!block_) {
    logger_->error(utl::RAM, 800, "No block available to write DEF");
    return false;
  }
  
  logger_->warn(utl::RAM, 942, "DEF writing is disabled in this build");
  return false;
}

//----------------------------------------------
// LayoutBuilder Implementation
//----------------------------------------------

// In ram.cpp
LayoutBuilder::LayoutBuilder(odb::dbBlock* block,
  const MemoryConfig& config, 
  const RowConfig& row_config, 
  Logger* logger,
  odb::dbMaster* tap_cell,
  const std::vector<odb::dbMaster*>& filler_cells)
: block_(block),
config_(config),
row_config_(row_config),
logger_(logger),
layout_(std::make_unique<Layout>()),
provided_tap_cell_(tap_cell),
provided_filler_cells_(filler_cells)
{
// Database validation check on construction
validateDatabase();
}



bool LayoutBuilder::validateDatabase() const
{
  if (!block_) {
    logger_->error(utl::RAM, 1120, "No block provided to LayoutBuilder");
    return false;
  }
  
  if (!block_->getDb()) {
    logger_->error(utl::RAM, 1121, "No database available in provided block");
    return false;
  }
  
  return true;
}


void LayoutBuilder::buildLayout(std::unique_ptr<Element> hierarchy)
{
  // Check database availability first
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 901, "Cannot build layout - database not available");
    return;
  }

  logger_->info(utl::RAM, 900, "Building layout for memory with {} words",
               config_.getWordCount());
  
  // Add the hierarchy to the layout
  layout_->addElement(std::move(hierarchy));
  
  // Setup rows
  if (!setupRows()) {
    logger_->error(utl::RAM, 902, "Failed to set up rows, aborting layout build");
    return;
  }
  
  // Place memory components
  placeBitCells();
  placeDecoders();
  placeControlLogic();
  
  // Fill empty spaces
  fillEmptySpaces();
  
  logger_->info(utl::RAM, 910, "Layout building completed");
}


void LayoutBuilder::optimizeLayout()
{
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 921, "Cannot optimize layout - database not available");
    return;
  }

  logger_->info(utl::RAM, 920, "Optimizing layout");
  
  // Let the layout optimize itself
  layout_->optimizeLayout(block_, row_config_);
  
  logger_->info(utl::RAM, 930, "Layout optimization completed");
}

// Updated to return success/failure
bool LayoutBuilder::setupRows()
{
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 971, "Cannot set up rows - database not available");
    return false;
  }

  logger_->info(utl::RAM, 970, "Setting up {} rows for memory layout", row_config_.rows_count);
  
  // Find a site to use
  odb::dbSite* site = nullptr;
  for (auto lib : block_->getDb()->getLibs()) {
    if (!lib) continue; // Safety check
    
    auto sites = lib->getSites();
    if (!sites.empty()) {
      // Use iterator instead of indexing
      site = *sites.begin();
      break;
    }
  }
  
  if (!site) {
    logger_->error(utl::RAM, 980, "No site found in libraries");
    return false;
  }
  
  // Calculate site width and height
  double site_width = site->getWidth();
  double site_height = site->getHeight();
  
  // Calculate die width based on expected memory size
  int total_bits = config_.getTotalBits();
  int estimated_sites_per_row = std::sqrt(total_bits * 4); // Rough estimation
  
  // Create rows
  std::vector<Row> rows;
  int die_width = estimated_sites_per_row * site_width;
  int sites_per_row = die_width / site_width;
  
  for (int i = 0; i < row_config_.rows_count; i++) {
    odb::Point origin(0, i * site_height);
    rows.emplace_back(i, origin, site, sites_per_row);
  }
  
  // Set the rows in the layout
  layout_->setRows(std::move(rows));
  
  logger_->info(utl::RAM, 990, "Created {} rows with {} sites per row",
               row_config_.rows_count, sites_per_row);
  return true;
}

// Method for LayoutBuilder::writeToDefFile
bool LayoutBuilder::writeToDefFile(const std::string& filename) const
{
  logger_->info(utl::RAM, 940, "Writing layout to DEF file: {}", filename);
  
  if (!block_) {
    logger_->error(utl::RAM, 802, "No block available to write DEF");
    return false;
  }

  // OPTION 1: Comment out DEF writing functionality entirely
  logger_->warn(utl::RAM, 949, "DEF writing is disabled in this build");
  return false;
}



// Other placement methods remain mostly the same, just add database validation
void LayoutBuilder::placeBitCells()
{
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 1001, "Cannot place bit cells - database not available");
    return;
  }

  logger_->info(utl::RAM, 1000, "Placing bit cells");
  
  // Find all bit cells in the layout
  auto bit_cells = layout_->findElementsByType(Element::BIT);
  
  logger_->info(utl::RAM, 1010, "Found {} bit cells to place", bit_cells.size());
  
  // Let the layout handle the bit cell placement
  layout_->placeBitCells();
}

void LayoutBuilder::placeDecoders()
{
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 1021, "Cannot place decoders - database not available");
    return;
  }

  logger_->info(utl::RAM, 1020, "Placing decoders");
  
  // Find all decoders in the layout
  auto decoders = layout_->findElementsByType(Element::DECODER);
  
  logger_->info(utl::RAM, 1030, "Found {} decoders to place", decoders.size());
  
  // Let the layout handle the decoder placement
  layout_->placeDecoders();
}

void LayoutBuilder::placeControlLogic()
{
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 1041, "Cannot place control logic - database not available");
    return;
  }

  logger_->info(utl::RAM, 1040, "Placing control logic");
  
  // Let the layout handle the control logic placement
  layout_->placeControlLogic();
}

// Updated to use provided masters first
void LayoutBuilder::fillEmptySpaces()
{
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 1051, "Cannot fill empty spaces - database not available");
    return;
  }

  logger_->info(utl::RAM, 1050, "Filling empty spaces with filler cells");
  
  // Use provided tap cell or find one if not provided
  odb::dbMaster* tap_cell = provided_tap_cell_;
  if (!tap_cell) {
    logger_->info(utl::RAM, 1052, "No tap cell provided, attempting to find one");
    tap_cell = findTapCell();
  } else {
    logger_->info(utl::RAM, 1053, "Using provided tap cell: {}", tap_cell->getName());
  }
  
  // Use provided filler cells or find them if not provided
  std::vector<odb::dbMaster*> filler_cells = provided_filler_cells_;
  if (filler_cells.empty()) {
    logger_->info(utl::RAM, 1054, "No filler cells provided, attempting to find some");
    filler_cells = findFillerCells();
  } else {
    logger_->info(utl::RAM, 1055, "Using {} provided filler cells", filler_cells.size());
  }
  
  if (!tap_cell) {
    logger_->warn(utl::RAM, 1060, "No tap cell found, skipping tap insertion");
  } else {
    // Add tap cells
    layout_->fillWithTapCells(block_, tap_cell, row_config_.tap_distance);
  }
  
  if (filler_cells.empty()) {
    logger_->warn(utl::RAM, 1070, "No filler cells found, skipping fill");
    return;
  }
  
  // Fill rows with filler cells
  const auto& rows = layout_->getRows();
  Row::fillRows(const_cast<std::vector<Row>&>(rows), 0, rows.size(), block_, filler_cells);
  
  logger_->info(utl::RAM, 1080, "Filled {} rows with filler cells", rows.size());
}

// Updated with database validation
std::vector<odb::dbMaster*> LayoutBuilder::findFillerCells()
{
  std::vector<odb::dbMaster*> filler_cells;
  
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 1091, "Cannot find filler cells - database not available");
    return filler_cells;
  }
  
  // Find filler cells based on patterns
  for (const auto& pattern : row_config_.filler_cell_patterns) {
    for (auto lib : block_->getDb()->getLibs()) {
      if (!lib) continue; // Safety check
      
      for (auto master : lib->getMasters()) {
        if (!master) continue; // Safety check
        
        std::string name = master->getName();
        if (name.find(pattern) != std::string::npos) {
          filler_cells.push_back(master);
        }
      }
    }
  }
  
  // Sort by width (increasing)
  std::sort(filler_cells.begin(), filler_cells.end(),
           [](odb::dbMaster* a, odb::dbMaster* b) {
             return a->getWidth() < b->getWidth();
           });
  
  logger_->info(utl::RAM, 1090, "Found {} filler cells", filler_cells.size());
  return filler_cells;
}

// Updated with database validation
odb::dbMaster* LayoutBuilder::findTapCell()
{
  if (!validateDatabase()) {
    logger_->error(utl::RAM, 1101, "Cannot find tap cell - database not available");
    return nullptr;
  }
  
  // Find tap cell based on pattern
  for (auto lib : block_->getDb()->getLibs()) {
    if (!lib) continue; // Safety check
    
    for (auto master : lib->getMasters()) {
      if (!master) continue; // Safety check
      
      std::string name = master->getName();
      if (name.find(row_config_.tap_cell_pattern) != std::string::npos) {
        logger_->info(utl::RAM, 1100, "Found tap cell: {}", name);
        return master;
      }
    }
  }
  
  logger_->warn(utl::RAM, 1111, "No tap cell found matching pattern {}", 
               row_config_.tap_cell_pattern);
  return nullptr;
}

} // namespace ram