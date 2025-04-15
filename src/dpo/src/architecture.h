// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "dpl/Coordinates.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rectangle.h"
namespace dpl {
class Group;
class Node;

}  // namespace dpl
namespace dpo {

class Network;
using dpl::DbuX;
using dpl::DbuY;
using dpl::Group;
using dpl::Node;

class Architecture
{
  // This class represents information about the layout area.

 public:
  class Row;

  ~Architecture();

  const std::vector<Architecture::Row*>& getRows() const { return rows_; }
  int getNumRows() const { return (int) rows_.size(); }
  Architecture::Row* getRow(int r) const { return rows_[r]; }
  Architecture::Row* createAndAddRow();

  const std::vector<Group*>& getRegions() const { return regions_; }
  int getNumRegions() const { return (int) regions_.size(); }
  Group* getRegion(int r) const { return regions_[r]; }
  Group* createAndAddRegion();

  bool isSingleHeightCell(const Node* ndi) const;
  bool isMultiHeightCell(const Node* ndi) const;

  int getCellHeightInRows(const Node* ndi) const;

  int postProcess(Network* network);
  int find_closest_row(DbuY y);

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

  // Using padding...
  void setUsePadding(bool val = true) { usePadding_ = val; }
  bool getUsePadding() const { return usePadding_; }
  void addCellPadding(Node* ndi, int leftPadding, int rightPadding);
  bool getCellPadding(const Node* ndi,
                      int& leftPadding,
                      int& rightPadding) const;
  void addCellPadding(Node* ndi, DbuX leftPadding, DbuX rightPadding);
  bool getCellPadding(const Node* ndi,
                      DbuX& leftPadding,
                      DbuX& rightPadding) const;

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
  std::vector<Group*> regions_;

  // Padding...
  bool usePadding_ = false;
  std::map<int, std::pair<int, int>> cellPaddings_;  // Padding to left,right.
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

  void setOrient(const odb::dbOrientType& orient) { siteOrient_ = orient; }
  odb::dbOrientType getOrient() const { return siteOrient_; }

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
  odb::dbOrientType siteOrient_;  // Orientation of sites in the row.
  unsigned siteSymmetry_ = 0;  // Symmetry of sites in the row.  Symmetry allows
                               // for certain orientations...
  // Voltages at the top and bottom of the row.
  int powerTop_ = Power_UNK;
  int powerBot_ = Power_UNK;
};

}  // namespace dpo
