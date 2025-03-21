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
  // Store the cell masters
  storage_cell_ = storage_cell;
  tristate_cell_ = tristate_cell;
  inv_cell_ = inv_cell;
  
  // Call setupTechnologyCells to find additional cells
  setupTechnologyCells();
  
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

  // Create nets and instances using the original approach
  // This is a simplified implementation for backward compatibility
  logger_->info(utl::RAM, 400, "Generating RAM with {} words, {} bytes per word, {} read ports",
               word_count, bytes_per_word, read_ports);
  
  // For demonstration, delegate to the new implementation
  // In practice, you might want to keep the original implementation here
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
  
  // Initialize the technology cells
  setupTechnologyCells();
  
  // Create the hierarchy builder
  HierarchyBuilder builder(block_, config, logger_, network_);
  
  // Build the memory hierarchy
  auto hierarchy = builder.buildHierarchy();
  
  // Create the layout builder
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

//----------------------------------------------
// Register file generation
//----------------------------------------------
void RamGen::generateRegisterFile(int word_count,
                                 int word_width,
                                 const std::string& output_def)
{
  logger_->info(utl::RAM, 600, "Generating register file: {} words x {} bits",
               word_count, word_width);
  
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
// Setup technology cells
//----------------------------------------------
void RamGen::setupTechnologyCells()
{
  // If technology cells are already provided, use them
  if (storage_cell_ && tristate_cell_ && inv_cell_) {
    logger_->info(utl::RAM, 700, "Using provided technology cells");
    return;
  }
  
  // Otherwise, find them in the libraries
  logger_->info(utl::RAM, 710, "Finding technology cells in libraries");
  
  // Function to find a master with specific characteristics
  auto findCell = [&](const std::function<bool(sta::LibertyPort*)>& match, 
                     const char* name, odb::dbMaster** target) {
    if (*target) return; // Already set
    
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

    if (best) {
      *target = best;
      masters_[name] = best;
      logger_->info(utl::RAM, 720, "Found {} cell: {}", name, best->getName());
    } else {
      logger_->warn(utl::RAM, 730, "Could not find {} cell", name);
    }
  };
  
  // Find storage cell (DFF)
  findCell([](sta::LibertyPort* port) {
    return port->libertyCell()->hasSequentials()
        && std::strcmp(port->name(), "Q") == 0;
  }, "storage", &storage_cell_);
  
  // Find tristate buffer
  findCell([](sta::LibertyPort* port) {
    return port->direction()->isTristate()
        && std::strcmp(port->name(), "Z") == 0;
  }, "tristate", &tristate_cell_);
  
  // Find inverter
  findCell([](sta::LibertyPort* port) {
    return port->libertyCell()->isInverter()
        && port->direction()->isOutput()
        && std::strcmp(port->name(), "Y") == 0;
  }, "inverter", &inv_cell_);
  
  // Find AND2 gate
  findCell([](sta::LibertyPort* port) {
    auto func = port->function();
    return func
        && func->op() == sta::FuncExpr::op_and
        && func->left()->op() == sta::FuncExpr::op_port
        && func->right()->op() == sta::FuncExpr::op_port;
  }, "and2", &and2_cell_);
  
  // Find clock gate
  findCell([](sta::LibertyPort* port) {
    return port->libertyCell()->isClockGate();
  }, "clock_gate", &clock_gate_cell_);
  
  // Find 2:1 mux
  findCell([](sta::LibertyPort* port) {
    auto func = port->function();
    return func
        && func->op() == sta::FuncExpr::op_or
        && func->left()->op() == sta::FuncExpr::op_and
        && func->right()->op() == sta::FuncExpr::op_and;
  }, "mux2", &mux2_cell_);
  
  // Validate essential cells are found
  if (!storage_cell_ || !tristate_cell_ || !inv_cell_ || !and2_cell_) {
    logger_->error(utl::RAM, 740, "Missing essential technology cells");
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
  
  /* logger_->info(utl::RAM, 810, "Writing DEF file to {}", filename);
  
  FILE* f = fopen(filename.c_str(), "w");
  if (!f) {
    logger_->error(utl::RAM, 801, "Could not open file {} for writing", filename);
    return false;
  }
  
  bool result = block_->writeDef(f, 5.8, 1000.0, false, false, false);
  fclose(f); */
  
  logger_->warn(utl::RAM, 942, "DEF writing is disabled in this build");
  return false;
}

//----------------------------------------------
// LayoutBuilder Implementation
//----------------------------------------------

LayoutBuilder::LayoutBuilder(odb::dbBlock* block, const MemoryConfig& config, 
                           const RowConfig& row_config, Logger* logger)
  : block_(block),
    config_(config),
    row_config_(row_config),
    logger_(logger),
    layout_(std::make_unique<Layout>())
{
}

void LayoutBuilder::buildLayout(std::unique_ptr<Element> hierarchy)
{
  logger_->info(utl::RAM, 900, "Building layout for memory with {} words",
               config_.getWordCount());
  
  // Add the hierarchy to the layout
  layout_->addElement(std::move(hierarchy));
  
  // Setup rows
  setupRows();
  
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
  logger_->info(utl::RAM, 920, "Optimizing layout");
  
  // Let the layout optimize itself
  layout_->optimizeLayout(block_, row_config_);
  
  logger_->info(utl::RAM, 930, "Layout optimization completed");
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

  /* 
  // OPTION 2: If you need DEF writing and can find the right API, uncomment and use:
  // This would use the odb::defout::writeDefFile function if available
  // bool result = odb::defout::writeDefFile(block_, filename.c_str(), 5, 8);
  
  // OR try to use the ord namespace if that's where the function is defined
  // bool result = ord::write_def(block_, filename.c_str(), 5.8);
  
  if (result) {
    logger_->info(utl::RAM, 961, "Successfully wrote DEF file to {}", filename);
  } else {
    logger_->error(utl::RAM, 941, "Failed to write DEF file to {}", filename);
  }
  
  return result;
  */
}

void LayoutBuilder::setupRows()
{
  logger_->info(utl::RAM, 970, "Setting up {} rows for memory layout", row_config_.rows_count);
  
  // Find a site to use
  odb::dbSite* site = nullptr;
  for (auto lib : block_->getDb()->getLibs()) {
    auto sites = lib->getSites();
    if (!sites.empty()) {
      // Use iterator instead of indexing
      site = *sites.begin();
      break;
    }
  }
  
  if (!site) {
    logger_->error(utl::RAM, 980, "No site found in libraries");
    return;
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
}

void LayoutBuilder::placeBitCells()
{
  logger_->info(utl::RAM, 1000, "Placing bit cells");
  
  // Find all bit cells in the layout
  auto bit_cells = layout_->findElementsByType(Element::BIT);
  
  logger_->info(utl::RAM, 1010, "Found {} bit cells to place", bit_cells.size());
  
  // Let the layout handle the bit cell placement
  layout_->placeBitCells();
}

void LayoutBuilder::placeDecoders()
{
  logger_->info(utl::RAM, 1020, "Placing decoders");
  
  // Find all decoders in the layout
  auto decoders = layout_->findElementsByType(Element::DECODER);
  
  logger_->info(utl::RAM, 1030, "Found {} decoders to place", decoders.size());
  
  // Let the layout handle the decoder placement
  layout_->placeDecoders();
}

void LayoutBuilder::placeControlLogic()
{
  logger_->info(utl::RAM, 1040, "Placing control logic");
  
  // Let the layout handle the control logic placement
  layout_->placeControlLogic();
}

void LayoutBuilder::fillEmptySpaces()
{
  logger_->info(utl::RAM, 1050, "Filling empty spaces with filler cells");
  
  // Find tap cell
  auto tap_cell = findTapCell();
  
  // Find filler cells
  auto filler_cells = findFillerCells();
  
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

std::vector<odb::dbMaster*> LayoutBuilder::findFillerCells()
{
  std::vector<odb::dbMaster*> filler_cells;
  
  // Find filler cells based on patterns
  for (const auto& pattern : row_config_.filler_cell_patterns) {
    for (auto lib : block_->getDb()->getLibs()) {
      for (auto master : lib->getMasters()) {
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

odb::dbMaster* LayoutBuilder::findTapCell()
{
  // Find tap cell based on pattern
  for (auto lib : block_->getDb()->getLibs()) {
    for (auto master : lib->getMasters()) {
      std::string name = master->getName();
      if (name.find(row_config_.tap_cell_pattern) != std::string::npos) {
        logger_->info(utl::RAM, 1100, "Found tap cell: {}", name);
        return master;
      }
    }
  }
  
  logger_->warn(utl::RAM, 1110, "No tap cell found matching pattern {}", 
               row_config_.tap_cell_pattern);
  return nullptr;
}

} // namespace ram