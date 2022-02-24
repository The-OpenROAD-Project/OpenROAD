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
#include <stdio.h>
#include <iostream>
#include <limits>
#include <map>
#include <vector>
#include "orientation.h"
#include "rectangle.h"
#include "symmetry.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class RoutingParams;
class Architecture;
class Network;
class Node;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class Architecture {
  // This class represents information about the layout area. 

 public:
  class Row;
  class Spacing;
  class Region;

 public:
  Architecture();
  virtual ~Architecture();

  std::vector<Architecture::Row*>& getRows() { return m_rows; }
  int getNumRows() const { return (int)m_rows.size(); }
  Architecture::Row* getRow(int r) const { return m_rows[r]; }
  Architecture::Row* createAndAddRow();

  std::vector<Architecture::Region*>& getRegions() { return m_regions; }
  int getNumRegions() const { return (int)m_regions.size(); }
  Architecture::Region* getRegion(int r) const { return m_regions[r]; }
  Architecture::Region* createAndAddRegion();

  bool isSingleHeightCell(Node* ndi) const;
  bool isMultiHeightCell(Node* ndi) const;

  int getCellHeightInRows(Node* ndi) const;

  int postProcess( Network* network );
  int find_closest_row(int y);

  void clear_edge_type();
  void init_edge_type();
  int add_edge_type(const char* name);

  int getMinX() const { return m_xmin; }
  int getMaxX() const { return m_xmax; }
  int getMinY() const { return m_ymin; }
  int getMaxY() const { return m_ymax; }

  int getWidth() const { return m_xmax - m_xmin; }
  int getHeight() const { return m_ymax - m_ymin; }

  void setMinX(int xmin) { m_xmin = xmin; }
  void setMaxX(int xmax) { m_xmax = xmax; }
  void setMinY(int ymin) { m_ymin = ymin; }
  void setMaxY(int ymax) { m_ymax = ymax; }

  bool power_compatible(Node* ndi, Row* row, bool& flip);

  // Using tables...
  void setUseSpacingTable(bool val = true) { m_useSpacingTable = val; }
  bool getUseSpacingTable() const { return m_useSpacingTable; }
  void clearSpacingTable();
  int getCellSpacingUsingTable(int firstEdge, int secondEdge);
  void addCellSpacingUsingTable(int firstEdge, int secondEdge, int sep);
  std::vector<Architecture::Spacing*>& getCellSpacings() { 
    return m_cellSpacings;
  }
  std::vector<std::pair<char*,int> >& getEdgeTypes() { 
    return m_edgeTypes;
  }

  // Using padding...
  void setUsePadding(bool val = true) { m_usePadding = val; }
  bool getUsePadding() const { return m_usePadding; }
  void addCellPadding(Node* ndi, int leftPadding, int rightPadding);
  bool getCellPadding(Node* ndi, int& leftPadding, int& rightPadding);

  int getCellSpacing(Node* leftNode, Node* rightNode);

 protected:
  // Boundary around rows.
  int m_xmin;
  int m_xmax;
  int m_ymin;
  int m_ymax;

  // Rows...
  std::vector<Row*> m_rows;

  // Regions...
  std::vector<Region*> m_regions;

  // Spacing tables...
  bool m_useSpacingTable;
  std::vector<std::pair<char*, int> > m_edgeTypes;
  std::vector<Spacing*> m_cellSpacings;

  // Padding...
  bool m_usePadding;
  std::map<int, std::pair<int,int> >
      m_cellPaddings;  // Padding to left,right.
};

class Architecture::Spacing {
 public:
  Spacing(int i1, int i2, int sep);

  int getFirstEdge() const { return m_i1; }
  int getSecondEdge() const { return m_i2; }
  int getSeparation() const { return m_sep; }

 protected:
  int m_i1;
  int m_i2;
  int m_sep;
};

const unsigned RowPower_UNK = 0x00000001;
const unsigned RowPower_VDD = 0x00000002;
const unsigned RowPower_VSS = 0x00000004;

class Architecture::Row {
 public:
  Row();
  virtual ~Row();

  void setId(int id) { m_id = id; }
  int getId() const { return m_id; }

  void setOrient(unsigned orient) { m_siteOrient = orient; }
  unsigned getOrient() const { return m_siteOrient; }

  void setSymmetry(unsigned sym) { m_siteSymmetry = sym; }
  unsigned getSymmetry() const { return m_siteSymmetry; }

  void setBottom(int bottom) { m_rowLoc = bottom; }
  void setHeight(int height) { m_rowHeight = height; }
  void setSubRowOrigin(int origin) { m_subRowOrigin = origin; }
  void setSiteSpacing(int spacing) { m_siteSpacing = spacing; }
  void setSiteWidth(int width) { m_siteWidth = width; }
  void setNumSites(int nsites) { m_numSites = nsites; }

  int getBottom() const { return m_rowLoc; }
  int getTop() const { return m_rowLoc+m_rowHeight; }
  int getLeft() const { return m_subRowOrigin; }
  int getRight() const { 
    return m_subRowOrigin+m_numSites*m_siteSpacing; 
  }
  int getHeight() const { return m_rowHeight; }
  int getSiteWidth() const { return m_siteWidth; }
  int getSiteSpacing() const { return m_siteSpacing; }
  int getNumSites() const { return m_numSites; }

  void setTopPower(int pwr) { m_powerTop = pwr; }
  int getTopPower() const { return m_powerTop; }

  void setBottomPower(int pwr) { m_powerBot = pwr; }
  int getBottomPower() const { return m_powerBot; }

  double getCenterY() const { return m_rowLoc+0.5*m_rowHeight; } 

 public:
  struct compareRowBottom {
    bool operator()(Architecture::Row* p, Architecture::Row* q) const {
      return p->getBottom() < q->getBottom();
    }
    bool operator()(Architecture::Row*& s, double i) const {
      return s->getBottom() < i;
    }
    bool operator()(double i, Architecture::Row*& s) const {
      return i < s->getBottom();
    }
  };

 protected:
  int m_id;              // Every row  needs an id...  Filled in after sorting.
  int m_rowLoc;          // Y-location of the row.
  int m_rowHeight;       // Height of the row.
  int m_subRowOrigin;    // Starting X location (xmin) of the row.
  int m_siteSpacing;     // Spacing between sites in the row. XXX: Likely
                         // assumed to be the same as the width...
  int m_siteWidth;       // Width of sites in the row.
  int m_numSites;        // Number of sites...  Ending X location (xmax) is =
                         // m_subRowOrigin + m_numSites * m_siteSpacing;
  unsigned m_siteOrient;    // Orientation of sites in the row.
  unsigned m_siteSymmetry;  // Symmetry of sites in the row.  Symmetry allows
                            // for certain orientations...
  // Voltages at the top and bottom of the row.
  int m_powerTop;
  int m_powerBot;
};

class Architecture::Region {
 public:
  Region();
  virtual ~Region();

  int getId() const { return m_id; }
  void setId(int id) { m_id = id; }

  int getMinX() const { return m_xmin; }
  int getMaxX() const { return m_xmax; }
  int getMinY() const { return m_ymin; }
  int getMaxY() const { return m_ymax; }

  void setMinX(int xmin) { m_xmin = xmin; }
  void setMaxX(int xmax) { m_xmax = xmax; }
  void setMinY(int ymin) { m_ymin = ymin; }
  void setMaxY(int ymax) { m_ymax = ymax; }

  void addRect(Rectangle_i& rect) { m_rects.push_back(rect); }
  const std::vector<Rectangle_i>& getRects() const { return  m_rects; }

 protected:
  // Id for the region.
  int m_id;  

  // Box around all sub-rectangles.
  int m_xmin;
  int m_ymin;
  int m_xmax;
  int m_ymax;

  // Sub-rectangles forming the rectilinear region.
  std::vector<Rectangle_i> m_rects;
};

}  // namespace dpo
