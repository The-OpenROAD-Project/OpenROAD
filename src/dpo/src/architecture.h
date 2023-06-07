///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

#pragma once

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include <map>
#include <string>
#include <vector>

#include "rectangle.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class Network;
class Node;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class Architecture
{
  // This class represents information about the layout area.

 public:
  class Row;
  class Spacing;
  class Region;

  ~Architecture();

  const std::vector<Architecture::Row*>& getRows() const { return rows_; }
  int getNumRows() const { return (int) rows_.size(); }
  Architecture::Row* getRow(int r) const { return rows_[r]; }
  Architecture::Row* createAndAddRow();

  const std::vector<Architecture::Region*>& getRegions() const
  {
    return regions_;
  }
  int getNumRegions() const { return (int) regions_.size(); }
  Architecture::Region* getRegion(int r) const { return regions_[r]; }
  Architecture::Region* createAndAddRegion();

  bool isSingleHeightCell(const Node* ndi) const;
  bool isMultiHeightCell(const Node* ndi) const;

  int getCellHeightInRows(const Node* ndi) const;

  int postProcess(Network* network);
  int find_closest_row(int y);

  void clear_edge_type();
  void init_edge_type();
  int add_edge_type(const char* name);

  int getMinX() const { return xmin_; }
  int getMaxX() const { return xmax_; }
  int getMinY() const { return ymin_; }
  int getMaxY() const { return ymax_; }

  int getWidth() const { return xmax_ - xmin_; }
  int getHeight() const { return ymax_ - ymin_; }

  void setMinX(int xmin) { xmin_ = xmin; }
  void setMaxX(int xmax) { xmax_ = xmax; }
  void setMinY(int ymin) { ymin_ = ymin; }
  void setMaxY(int ymax) { ymax_ = ymax; }

  bool powerCompatible(const Node* ndi, const Row* row, bool& flip) const;

  // Using tables...
  void setUseSpacingTable(bool val = true) { useSpacingTable_ = val; }
  bool getUseSpacingTable() const { return useSpacingTable_; }
  void clearSpacingTable();
  int getCellSpacingUsingTable(int firstEdge, int secondEdge) const;
  void addCellSpacingUsingTable(int firstEdge, int secondEdge, int sep);

  const std::vector<Spacing*>& getCellSpacings() const { return cellSpacings_; }

  const std::vector<std::pair<std::string, int>>& getEdgeTypes() const
  {
    return edgeTypes_;
  }

  // Using padding...
  void setUsePadding(bool val = true) { usePadding_ = val; }
  bool getUsePadding() const { return usePadding_; }
  void addCellPadding(Node* ndi, int leftPadding, int rightPadding);
  bool getCellPadding(const Node* ndi,
                      int& leftPadding,
                      int& rightPadding) const;

  int getCellSpacing(const Node* leftNode, const Node* rightNode) const;

 private:
  // Boundary around rows.
  int xmin_ = std::numeric_limits<int>::max();
  int xmax_ = std::numeric_limits<int>::lowest();
  int ymin_ = std::numeric_limits<int>::max();
  int ymax_ = std::numeric_limits<int>::lowest();

  // Rows...
  std::vector<Row*> rows_;

  // Regions...
  std::vector<Region*> regions_;

  // Spacing tables...
  bool useSpacingTable_ = false;
  std::vector<std::pair<std::string, int>> edgeTypes_;
  std::vector<Spacing*> cellSpacings_;

  // Padding...
  bool usePadding_ = false;
  std::map<int, std::pair<int, int>> cellPaddings_;  // Padding to left,right.
};

class Architecture::Spacing
{
 public:
  Spacing(int i1, int i2, int sep);

  int getFirstEdge() const { return i1_; }
  int getSecondEdge() const { return i2_; }
  int getSeparation() const { return sep_; }

 private:
  int i1_;
  int i2_;
  int sep_;
};

class Architecture::Row
{
 public:
  enum PowerType
  {
    Power_UNK,
    Power_VDD,
    Power_VSS
  };

  void setId(int id) { id_ = id; }
  int getId() const { return id_; }

  void setOrient(unsigned orient) { siteOrient_ = orient; }
  unsigned getOrient() const { return siteOrient_; }

  void setSymmetry(unsigned sym) { siteSymmetry_ = sym; }
  unsigned getSymmetry() const { return siteSymmetry_; }

  void setBottom(int bottom) { rowLoc_ = bottom; }
  void setHeight(int height) { rowHeight_ = height; }
  void setSubRowOrigin(int origin) { subRowOrigin_ = origin; }
  void setSiteSpacing(int spacing) { siteSpacing_ = spacing; }
  void setSiteWidth(int width) { siteWidth_ = width; }
  void setNumSites(int nsites) { numSites_ = nsites; }

  int getBottom() const { return rowLoc_; }
  int getTop() const { return rowLoc_ + rowHeight_; }
  int getLeft() const { return subRowOrigin_; }
  int getRight() const { return subRowOrigin_ + numSites_ * siteSpacing_; }
  int getHeight() const { return rowHeight_; }
  int getSiteWidth() const { return siteWidth_; }
  int getSiteSpacing() const { return siteSpacing_; }
  int getNumSites() const { return numSites_; }

  void setTopPower(int pwr) { powerTop_ = pwr; }
  int getTopPower() const { return powerTop_; }

  void setBottomPower(int pwr) { powerBot_ = pwr; }
  int getBottomPower() const { return powerBot_; }

  double getCenterY() const { return rowLoc_ + 0.5 * rowHeight_; }

 private:
  int id_ = -1;           // Every row  needs an id...  Filled in after sorting.
  int rowLoc_ = 0;        // Y-location of the row.
  int rowHeight_ = 0;     // Height of the row.
  int subRowOrigin_ = 0;  // Starting X location (xmin) of the row.
  int siteSpacing_ = 0;   // Spacing between sites in the row. XXX: Likely
                          // assumed to be the same as the width...
  int siteWidth_ = 0;     // Width of sites in the row.
  int numSites_ = 0;      // Number of sites...  Ending X location (xmax) is =
                          // subRowOrigin_ + numSites_ * siteSpacing_;
  unsigned siteOrient_ = 0;    // Orientation of sites in the row.
  unsigned siteSymmetry_ = 0;  // Symmetry of sites in the row.  Symmetry allows
                               // for certain orientations...
  // Voltages at the top and bottom of the row.
  int powerTop_ = Power_UNK;
  int powerBot_ = Power_UNK;
};

class Architecture::Region
{
 public:
  int getId() const { return id_; }
  void setId(int id) { id_ = id; }

  int getMinX() const { return xmin_; }
  int getMaxX() const { return xmax_; }
  int getMinY() const { return ymin_; }
  int getMaxY() const { return ymax_; }

  void setMinX(int xmin) { xmin_ = xmin; }
  void setMaxX(int xmax) { xmax_ = xmax; }
  void setMinY(int ymin) { ymin_ = ymin; }
  void setMaxY(int ymax) { ymax_ = ymax; }

  void addRect(Rectangle_i& rect) { rects_.push_back(rect); }
  const std::vector<Rectangle_i>& getRects() const { return rects_; }

 private:
  // Id for the region.
  int id_ = -1;

  // Box around all sub-rectangles.
  int xmin_ = std::numeric_limits<int>::max();
  int ymin_ = std::numeric_limits<int>::max();
  int xmax_ = std::numeric_limits<int>::lowest();
  int ymax_ = std::numeric_limits<int>::lowest();

  // Sub-rectangles forming the rectilinear region.
  std::vector<Rectangle_i> rects_;
};

}  // namespace dpo
