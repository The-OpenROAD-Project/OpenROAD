// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Coordinates.h"
#include "dpl/Opendp.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
namespace dpl {
class Group;
class Node;
class Padding;
class Network;

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

  void postProcess(Network* network);
  int find_closest_row(DbuY y);

  DbuX getMinX() const { return xmin_; }
  DbuX getMaxX() const { return xmax_; }
  DbuY getMinY() const { return ymin_; }
  DbuY getMaxY() const { return ymax_; }

  DbuX getWidth() const { return xmax_ - xmin_; }
  DbuY getHeight() const { return ymax_ - ymin_; }

  void setMinX(DbuX xmin) { xmin_ = xmin; }
  void setMaxX(DbuX xmax) { xmax_ = xmax; }
  void setMinY(DbuY ymin) { ymin_ = ymin; }
  void setMaxY(DbuY ymax) { ymax_ = ymax; }

  bool powerCompatible(const Node* ndi, const Row* row, bool& flip) const;

  // Using padding...
  void setPadding(dpl::Padding* padding) { padding_ = padding; }
  void setSiteWidth(const DbuX& site_width) { site_width_ = site_width; }
  void setUsePadding(bool val = true) { usePadding_ = val; }
  bool getUsePadding() const { return usePadding_; }
  bool getCellPadding(const Node* ndi,
                      int& leftPadding,
                      int& rightPadding) const;
  bool getCellPadding(const Node* ndi,
                      DbuX& leftPadding,
                      DbuX& rightPadding) const;
  void flipCellPadding(const Node* ndi);
  int getCellSpacing(const Node* leftNode, const Node* rightNode) const;
  void clear();

 private:
  // Boundary around rows.
  DbuX xmin_ = std::numeric_limits<DbuX>::max();
  DbuX xmax_ = std::numeric_limits<DbuX>::lowest();
  DbuY ymin_ = std::numeric_limits<DbuY>::max();
  DbuY ymax_ = std::numeric_limits<DbuY>::lowest();

  // Rows...
  std::vector<Row*> rows_;

  // Regions...
  std::vector<Group*> regions_;

  // Padding...
  bool usePadding_{false};
  dpl::Padding* padding_{nullptr};
  DbuX site_width_{0};
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

  void setBottom(DbuY bottom) { rowLoc_ = bottom; }
  void setHeight(DbuY height) { rowHeight_ = height; }
  void setSubRowOrigin(DbuX origin) { subRowOrigin_ = origin; }
  void setSiteSpacing(DbuX spacing) { siteSpacing_ = spacing; }
  void setSiteWidth(DbuX width) { siteWidth_ = width; }
  void setNumSites(int nsites) { numSites_ = nsites; }

  DbuY getBottom() const { return rowLoc_; }
  DbuY getTop() const { return rowLoc_ + rowHeight_; }
  DbuX getLeft() const { return subRowOrigin_; }
  DbuX getRight() const { return subRowOrigin_ + numSites_ * siteSpacing_.v; }
  DbuY getHeight() const { return rowHeight_; }
  DbuX getSiteWidth() const { return siteWidth_; }
  DbuX getSiteSpacing() const { return siteSpacing_; }
  int getNumSites() const { return numSites_; }

  void setTopPower(int pwr) { powerTop_ = pwr; }
  int getTopPower() const { return powerTop_; }

  void setBottomPower(int pwr) { powerBot_ = pwr; }
  int getBottomPower() const { return powerBot_; }

 private:
  int id_ = -1;           // Every row  needs an id...  Filled in after sorting.
  DbuY rowLoc_{0};        // Y-location of the row.
  DbuY rowHeight_{0};     // Height of the row.
  DbuX subRowOrigin_{0};  // Starting X location (xmin) of the row.
  DbuX siteSpacing_{0};   // Spacing between sites in the row. XXX: Likely
                          // assumed to be the same as the width...
  DbuX siteWidth_{0};     // Width of sites in the row.
  int numSites_{0};       // Number of sites...  Ending X location (xmax) is =
                          // subRowOrigin_ + numSites_ * siteSpacing_;
  odb::dbOrientType siteOrient_;  // Orientation of sites in the row.
  unsigned siteSymmetry_ = 0;  // Symmetry of sites in the row.  Symmetry allows
                               // for certain orientations...
  // Voltages at the top and bottom of the row.
  int powerTop_ = Power_UNK;
  int powerBot_ = Power_UNK;
};

}  // namespace dpl
