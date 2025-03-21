#include "ram/ram.h"
#include "utl/Logger.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

namespace ram {

//----------------------------------------------
// Element Implementation
//----------------------------------------------

Element::Element(odb::dbInst* inst, ElementType type)
  : inst_(inst), type_(type)
{
}

Element::Element(std::unique_ptr<Layout> layout, ElementType type)
  : layout_(std::move(layout)), type_(type)
{
}

odb::Rect Element::position(odb::Point origin)
{
  if (inst_) {
    // Place a single instance
    inst_->setLocation(origin.getX(), origin.getY());
    inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    return inst_->getBBox()->getBox();
  }
  else if (layout_) {
    // Place an entire layout
    return layout_->position(origin);
  }
  
  // Should never happen
  return odb::Rect(origin, origin);
}

odb::Rect Element::placeOnRow(Row& row)
{
  // Place this element on the given row
  odb::Point current_pos = row.getCurrentPosition();
  return position(current_pos);
}

int Element::getWidth() const
{
  if (inst_) {
    return inst_->getMaster()->getWidth();
  }
  else if (layout_) {
    return layout_->getEstimatedWidth();
  }
  return 0;
}

int Element::getHeight() const
{
  if (inst_) {
    return inst_->getMaster()->getHeight();
  }
  else if (layout_) {
    return layout_->getEstimatedHeight();
  }
  return 0;
}

std::string Element::getName() const
{
  if (inst_) {
    return inst_->getName();
  }
  else if (layout_) {
    // For layouts, return a descriptive name based on type
    switch (type_) {
      case BIT: return "bit";
      case BYTE: return "byte";
      case WORD: return "word";
      case DECODER: return "decoder";
      case RAM8: return "ram8";
      case RAM32: return "ram32";
      case RAM128: return "ram128";
      case RAM512: return "ram512";
      case REGISTER_FILE: return "register_file";
      default: return "layout";
    }
  }
  return "unknown";
}

void Element::addChild(std::unique_ptr<Element> child)
{
  children_.push_back(std::move(child));
}

//----------------------------------------------
// Layout Implementation
//----------------------------------------------

Layout::Layout(odb::Orientation2D orientation)
  : orientation_(orientation),
    estimated_width_(-1),
    estimated_height_(-1)
{
}

void Layout::addElement(std::unique_ptr<Element> element)
{
  elements_.push_back(std::move(element));
  // Reset estimated dimensions to force recalculation
  estimated_width_ = -1;
  estimated_height_ = -1;
}

odb::Rect Layout::position(odb::Point origin)
{
  odb::Rect bbox(origin, origin);
  odb::Point current_origin = origin;
  
  // Place elements based on orientation
  for (auto& elem : elements_) {
    auto bounds = elem->position(current_origin);
    bbox.merge(bounds);
    
    // Update origin for next element
    if (orientation_ == odb::horizontal) {
      current_origin.setX(bounds.xMax());
    } else {
      current_origin.setY(bounds.yMax());
    }
  }
  
  return bbox;
}

void Layout::optimizeLayout(odb::dbBlock* block, const RowConfig& config)
{
  // This method implements DFFRAM-style optimization
  // Create rows if not already created
  if (rows_.empty()) {
    createAndPlaceRows(block, config);
  }
  
  // Apply the DFFRAM-inspired placement logic
  placeBitCells();
  placeDecoders();
  placeControlLogic();
}

void Layout::createAndPlaceRows(odb::dbBlock* block, const RowConfig& config)
{
  // Find a standard site to use
  odb::dbSite* site = nullptr;
  for (auto lib : block->getDb()->getLibs()) {
    auto sites = lib->getSites();
    if (!sites.empty()) {
      // Use begin() instead of operator[]
      site = *sites.begin();
      break;
    }
  }
  
  if (!site) {
    std::cerr << "Error: No site found in libraries" << std::endl;
    return;
  }
  
  // Calculate site width and height
  double site_width = site->getWidth();
  double site_height = site->getHeight();
  
  // Calculate die width based on expected memory size
  // This is a simple heuristic - in practice, you'd use a more sophisticated approach
  int estimated_width = getEstimatedWidth();
  int estimated_height = getEstimatedHeight();
  
  int sites_per_row = std::ceil(estimated_width / site_width);
  int row_count = std::max(config.rows_count, 
                           (int)std::ceil(estimated_height / site_height));
  
  // Create rows
  rows_.clear();
  for (int i = 0; i < row_count; i++) {
    odb::Point origin(0, i * site_height);
    rows_.emplace_back(i, origin, site, sites_per_row);
  }
}

void Layout::placeBitCells()
{
  // Find all bit cells
  auto bit_cells = findElementsByType(Element::BIT);
  
  if (bit_cells.empty() || rows_.empty()) {
    return;
  }
  
  // Sort bit cells by name for deterministic placement
  std::sort(bit_cells.begin(), bit_cells.end(),
           [](Element* a, Element* b) {
             return a->getName() < b->getName();
           });
  
  // Place bit cells in rows - attempt to create a grid-like pattern
  // for better density, similar to DFFRAM's approach
  int row_idx = 0;
  for (auto* cell : bit_cells) {
    auto& row = rows_[row_idx];
    cell->placeOnRow(row);
    
    // Move to next row
    row_idx = (row_idx + 1) % rows_.size();
  }
}

void Layout::placeDecoders()
{
  // Find all decoders
  auto decoders = findElementsByType(Element::DECODER);
  
  if (decoders.empty() || rows_.empty()) {
    return;
  }
  
  // In DFFRAM, decoders are typically placed on the side
  // For simplicity, we'll place them at the beginning of rows
  int start_row = rows_.size() / 2; // Start in the middle
  for (auto* decoder : decoders) {
    if (start_row < (int)rows_.size()) {
      auto& row = rows_[start_row];
      decoder->placeOnRow(row);
      start_row++;
    }
  }
}

void Layout::placeControlLogic()
{
  // Place control logic (clock gates, enables, etc.)
  // This is typically placed near the periphery
  
  // For simplicity, we'll place them at the end of the first row
  if (rows_.empty()) {
    return;
  }
  
  // Find control elements (anything that's not a bit cell or decoder)
  std::vector<Element*> control_elements;
  for (auto& elem : elements_) {
    if (elem->getType() != Element::BIT && 
        elem->getType() != Element::DECODER) {
      control_elements.push_back(elem.get());
    }
  }
  
  // Place in the first row
  auto& row = rows_[0];
  for (auto* elem : control_elements) {
    elem->placeOnRow(row);
  }
}

void Layout::fillWithTapCells(odb::dbBlock* block, odb::dbMaster* tap_cell, int tap_distance)
{
  // Add tap cells at regular intervals
  // This is similar to DFFRAM's tap cell insertion
  
  for (auto& row : rows_) {
    // Add tap cells as needed
    row.addTap(block, tap_cell);
  }
}

std::vector<Element*> Layout::findElementsByType(Element::ElementType type)
{
  std::vector<Element*> result;
  
  // Find elements of the specified type
  for (auto& elem : elements_) {
    if (elem->getType() == type) {
      result.push_back(elem.get());
    }
    
    // Check children recursively
    if (elem->getLayout()) {
      auto child_elements = elem->getLayout()->findElementsByType(type);
      result.insert(result.end(), child_elements.begin(), child_elements.end());
    }
  }
  
  return result;
}

int Layout::getEstimatedWidth() const
{
  if (estimated_width_ < 0) {
    calculateEstimatedDimensions();
  }
  return estimated_width_;
}

int Layout::getEstimatedHeight() const
{
  if (estimated_height_ < 0) {
    calculateEstimatedDimensions();
  }
  return estimated_height_;
}

void Layout::calculateEstimatedDimensions() const
{
  // Calculate estimated dimensions based on elements
  int total_width = 0;
  int max_height = 0;
  
  if (orientation_ == odb::horizontal) {
    // For horizontal orientation, width is sum of element widths
    for (const auto& elem : elements_) {
      total_width += elem->getWidth();
      max_height = std::max(max_height, elem->getHeight());
    }
  } else {
    // For vertical orientation, height is sum of element heights
    for (const auto& elem : elements_) {
      total_width = std::max(total_width, elem->getWidth());
      max_height += elem->getHeight();
    }
  }
  
  // Add some padding
  total_width = (int)(total_width * 1.1);
  max_height = (int)(max_height * 1.1);
  
  // Update cached values
  const_cast<Layout*>(this)->estimated_width_ = total_width;
  const_cast<Layout*>(this)->estimated_height_ = max_height;
}

//----------------------------------------------
// Row Implementation
//----------------------------------------------

Row::Row(int ordinal, odb::Point origin, odb::dbSite* site, int sites_count)
  : ordinal_(ordinal),
    origin_(origin),
    current_position_(origin),
    site_(site),
    site_width_(site->getWidth()),
    sites_count_(sites_count),
    current_sites_(0),
    cell_counter_(0),
    fill_counter_(0),
    tap_counter_(0),
    since_last_tap_(0)
{
}

void Row::place(odb::dbInst* inst)
{
  // Calculate width in sites
  int width = inst->getMaster()->getWidth();
  int sites = (width + site_width_ - 1) / site_width_;
  
  // Check if the row has enough space
  if (current_sites_ + sites > sites_count_) {
    // Row is full
    return;
  }
  
  // Place the instance
  inst->setLocation(current_position_.getX(), current_position_.getY());
  inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  
  // Update position and site count
  current_position_.setX(current_position_.getX() + width);
  current_sites_ += sites;
  
  // Update tap tracking
  since_last_tap_ += sites;
  cell_counter_++;
}

void Row::addTap(odb::dbBlock* block, odb::dbMaster* tap_cell)
{
  // Check if we need a tap cell
  if (since_last_tap_ < 20) {
    return;
  }
  
  // Create a tap cell
  std::string name = fmt::format("TAP_{}_{}", ordinal_, tap_counter_);
  auto tap = odb::dbInst::create(block, tap_cell, name.c_str());
  
  // Place it
  place(tap);
  
  // Reset tap tracking
  since_last_tap_ = 0;
  tap_counter_++;
}

void Row::fill(odb::dbBlock* block, const std::vector<odb::dbMaster*>& filler_cells)
{
  // Skip if no fillers available
  if (filler_cells.empty()) {
    return;
  }
  
  // Calculate remaining sites
  int remaining_sites = sites_count_ - current_sites_;
  if (remaining_sites <= 0) {
    return;
  }
  
  // Sort filler cells by descending width
  std::vector<std::pair<odb::dbMaster*, int>> sorted_fillers;
  for (auto* filler : filler_cells) {
    int width = filler->getWidth();
    int sites = (width + site_width_ - 1) / site_width_;
    sorted_fillers.emplace_back(filler, sites);
  }
  
  std::sort(sorted_fillers.begin(), sorted_fillers.end(),
           [](const auto& a, const auto& b) {
             return a.second > b.second;
           });
  
  // Fill with largest possible fillers first
  while (remaining_sites > 0) {
    bool placed = false;
    
    for (const auto& [filler, sites] : sorted_fillers) {
      if (sites <= remaining_sites) {
        // Create and place filler
        std::string name = fmt::format("FILLER_{}_{}", ordinal_, fill_counter_);
        auto fill_inst = odb::dbInst::create(block, filler, name.c_str());
        
        place(fill_inst);
        
        // Update tracking
        remaining_sites -= sites;
        fill_counter_++;
        placed = true;
        break;
      }
    }
    
    // If we couldn't place any filler, we're done
    if (!placed) {
      break;
    }
  }
}

void Row::fillRows(std::vector<Row>& rows, int from_index, int to_index,
                  odb::dbBlock* block, const std::vector<odb::dbMaster*>& filler_cells)
{
  // Fill multiple rows with filler cells
  for (int i = from_index; i < to_index && i < (int)rows.size(); i++) {
    rows[i].fill(block, filler_cells);
  }
}

} // namespace ram