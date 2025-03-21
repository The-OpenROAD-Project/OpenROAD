
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
#pragma once
#include <memory>
#include <vector>
#include <map>
#include <string>
#include "odb/db.h"

namespace ram {
class Element;
class Layout;

// Enhanced row class inspired by DFFRAM's Row implementation
class Row {
public:
  Row(int ordinal, odb::Point origin, odb::dbSite* site, int sites_count);
  
  // Place an instance on this row
  void place(odb::dbInst* inst);
  
  // Fill row with filler cells
  void fill(odb::dbBlock* block, odb::dbMaster* filler_cell);
  
  // Get properties
  int getOrdinal() const { return ordinal_; }
  odb::Point getCurrentPosition() const { return current_position_; }
  int getSiteCount() const { return sites_count_; }
  bool isFull() const { return current_sites_ >= sites_count_; }
  
  // Add a tap cell at the current position
  void addTap(odb::dbBlock* block, odb::dbMaster* tap_cell);
  
private:
  int ordinal_;                      // Row number
  odb::Point origin_;                // Starting position
  odb::Point current_position_;      // Current placement position
  odb::dbSite* site_;                // Site template
  int site_width_;                   // Width of a single site
  int sites_count_;                  // Total sites in row
  int current_sites_;                // Sites used so far
  int cell_counter_;                 // Counter for unique cell naming
  int since_last_tap_;               // Sites since last tap cell
};

// Enhanced element class for building blocks
class Element {
 public:
  Element() = delete;

  explicit Element(odb::dbInst* inst);
  explicit Element(std::unique_ptr<Layout> layout);
  
  // Position this element at the given origin
  odb::Rect position(odb::Point origin);
  
  // Get the instance or layout
  odb::dbInst* getInstance() const { return inst_; }
  Layout* getLayout() const { return layout_.get(); }
  
  // Get element dimensions
  int getWidth() const;
  int getHeight() const;
  
  // Get element name
  std::string getName() const;

 private:
  odb::dbInst* inst_ = nullptr;
  std::unique_ptr<Layout> layout_;          // Nested layout container
};

// Enhanced layout class with more placement options
class Layout {
 public:
  Layout(odb::Orientation2D orientation = odb::horizontal);
  
  // Add an element to the layout
  void addElement(std::unique_ptr<Element> element);
  
  // Position the layout at the given origin
  odb::Rect position(odb::Point origin);
  
  // DFFRAM-inspired enhancements
  void optimizeLayout();
  void autoArrangeElements();
  void fillWithTapCells(odb::dbBlock* block, odb::dbMaster* tap_cell, int tap_distance);
  void alignElementsInRows();
  
  // Create optimal rows
  std::vector<Row> createRows(odb::dbBlock* block, odb::dbSite* site, int rows_count);
  
  // Get properties
  std::vector<std::unique_ptr<Element>>& getElements() { return elements_; }
  int getEstimatedWidth() const;
  int getEstimatedHeight() const;
  odb::Orientation2D getOrientation() const { return orientation_; }

 private:
  odb::Orientation2D orientation_;
  std::vector<std::unique_ptr<Element>> elements_;
  
  // Cached dimensions
  mutable int estimated_width_ = -1;
  mutable int estimated_height_ = -1;
  
  // Helper methods
  void calculateEstimatedDimensions() const;
};

// New group class for organizing related elements
class ElementGroup {
public:
  ElementGroup(const std::string& name);
  
  void addElement(Element* element);
  void setPlacementPolicy(const std::string& policy);
  
  const std::string& getName() const { return name_; }
  const std::vector<Element*>& getElements() const { return elements_; }
  
private:
  std::string name_;
  std::vector<Element*> elements_;
  std::string placement_policy_;
};

}  // namespace ram