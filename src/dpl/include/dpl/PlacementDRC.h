#pragma once
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Coordinates.h"

namespace odb {
class dbTech;
class dbOrientType;
}  // namespace odb

namespace dpl {
class Grid;
class Node;

struct EdgeSpacingEntry
{
  EdgeSpacingEntry(const int spc_in,
                   const bool is_exact_in,
                   const bool except_abutted_in)
      : spc(spc_in), is_exact(is_exact_in), except_abutted(except_abutted_in)
  {
  }
  bool operator<(const EdgeSpacingEntry& rhs) const { return spc < rhs.spc; }
  int spc;
  bool is_exact;
  bool except_abutted;
};

class PlacementDRC
{
 public:
  // Constructor
  PlacementDRC(Grid* grid, odb::dbTech* tech);
  bool checkEdgeSpacing(const Node* cell) const;
  // Check edge spacing for a cell at a given location and orientation
  bool checkEdgeSpacing(const Node* cell,
                        GridX x,
                        GridY y,
                        const odb::dbOrientType& orient) const;

  int getEdgeTypeIdx(const std::string& edge_type) const;
  bool hasCellEdgeSpacingTable() const;
  int getMaxSpacing(int edge_type_idx) const;

 private:
  // Member variables
  Grid* grid_{nullptr};  // Pointer to the grid for placement
  std::unordered_map<std::string, int> edge_types_indices_;
  std::vector<std::vector<EdgeSpacingEntry>>
      edge_spacing_table_;  // LEF58_CELLEDGESPACINGTABLE between edge type
                            // pairs [from_idx][to_idx]

  // Helper functions
  DbuX gridToDbu(GridX grid_x, DbuX site_width) const;
  void makeCellEdgeSpacingTable(odb::dbTech* tech);
};

}  // namespace dpl