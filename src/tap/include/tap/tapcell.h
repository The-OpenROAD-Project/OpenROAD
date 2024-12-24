/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/polygon/polygon.hpp>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace ord {
class OpenRoad;
}

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
class dbTech;
class dbBlock;
}  // namespace odb

namespace tap {

struct Options
{
  odb::dbMaster* endcap_master = nullptr;
  odb::dbMaster* tapcell_master = nullptr;
  int dist = -1;    // default = 2um
  int halo_x = -1;  // default = 2um
  int halo_y = -1;  // default = 2um
  int row_min_width = -1;
  odb::dbMaster* cnrcap_nwin_master = nullptr;
  odb::dbMaster* cnrcap_nwout_master = nullptr;
  odb::dbMaster* tap_nwintie_master = nullptr;
  odb::dbMaster* tap_nwin2_master = nullptr;
  odb::dbMaster* tap_nwin3_master = nullptr;
  odb::dbMaster* tap_nwouttie_master = nullptr;
  odb::dbMaster* tap_nwout2_master = nullptr;
  odb::dbMaster* tap_nwout3_master = nullptr;
  odb::dbMaster* incnrcap_nwin_master = nullptr;
  odb::dbMaster* incnrcap_nwout_master = nullptr;
  bool disallow_one_site_gaps = false;

  bool addBoundaryCells() const
  {
    return tap_nwintie_master && tap_nwin2_master && tap_nwin3_master
           && tap_nwouttie_master && tap_nwout2_master && tap_nwout3_master
           && incnrcap_nwin_master && incnrcap_nwout_master;
  }
};

struct EndcapCellOptions
{
  // External facing endcap cells
  odb::dbMaster* left_top_corner = nullptr;
  odb::dbMaster* right_top_corner = nullptr;
  odb::dbMaster* left_bottom_corner = nullptr;
  odb::dbMaster* right_bottom_corner = nullptr;

  // Internal facing endcap cells
  odb::dbMaster* left_top_edge = nullptr;
  odb::dbMaster* right_top_edge = nullptr;
  odb::dbMaster* left_bottom_edge = nullptr;
  odb::dbMaster* right_bottom_edge = nullptr;

  // row/column endcaps
  std::vector<odb::dbMaster*> top_edge;
  std::vector<odb::dbMaster*> bottom_edge;
  odb::dbMaster* left_edge = nullptr;
  odb::dbMaster* right_edge = nullptr;

  std::string prefix = "PHY_";
};

class Tapcell
{
 public:
  Tapcell();
  void init(odb::dbDatabase* db, utl::Logger* logger);
  void setTapPrefix(const std::string& tap_prefix);
  void setEndcapPrefix(const std::string& endcap_prefix);
  void clear();
  void run(const Options& options);
  void cutRows(const Options& options);
  void reset();
  int removeCells(const std::string& prefix);

  void placeEndcaps(const EndcapCellOptions& options);
  void placeTapcells(const Options& options);

 private:
  enum class EdgeType
  {
    Left,
    Top,
    Right,
    Bottom,
    Unknown
  };
  struct Edge
  {
    EdgeType type;
    odb::Point pt0;
    odb::Point pt1;
    bool operator==(const Edge& edge) const;
  };
  enum class CornerType
  {
    OuterBottomLeft,
    OuterTopLeft,
    OuterTopRight,
    OuterBottomRight,
    InnerBottomLeft,
    InnerTopLeft,
    InnerTopRight,
    InnerBottomRight,
    Unknown
  };
  struct PartialOverlap
  {
    bool left = false;
    int x_start_left;
    bool right = false;
    int x_limit_right;
  };
  struct Corner
  {
    CornerType type;
    odb::Point pt;
  };
  using Polygon = boost::polygon::polygon_90_data<int>;
  using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
  using CornerMap = std::map<odb::dbRow*, std::set<odb::dbInst*>>;

  struct InstIndexableGetter
  {
    using result_type = odb::Rect;
    odb::Rect operator()(odb::dbInst* inst) const
    {
      return inst->getBBox()->getBox();
    }
  };
  using InstTree
      = boost::geometry::index::rtree<odb::dbInst*,
                                      boost::geometry::index::quadratic<16>,
                                      InstIndexableGetter>;

  std::vector<odb::dbBox*> findBlockages();
  bool checkSymmetry(odb::dbMaster* master, const odb::dbOrientType& ori);
  odb::dbInst* makeInstance(odb::dbBlock* block,
                            odb::dbMaster* master,
                            const odb::dbOrientType& orientation,
                            int x,
                            int y,
                            const std::string& prefix);
  std::optional<int> findValidLocation(int x,
                                       int width,
                                       const odb::dbOrientType& orient,
                                       const std::set<odb::dbInst*>& row_insts,
                                       int site_width,
                                       int tap_width,
                                       int row_urx,
                                       bool disallow_one_site_gaps);
  bool isOverlapping(int x,
                     int width,
                     const odb::dbOrientType& orient,
                     const std::set<odb::dbInst*>& row_insts);
  int placeTapcells(odb::dbMaster* tapcell_master,
                    int dist,
                    bool disallow_one_site_gaps);
  int placeTapcells(odb::dbMaster* tapcell_master,
                    int dist,
                    odb::dbRow* row,
                    bool is_edge,
                    bool disallow_one_site_gaps,
                    const InstTree& fixed_instances);

  int defaultDistance() const;

  std::vector<Polygon90> getBoundaryAreas() const;
  std::vector<Edge> getBoundaryEdges(const Polygon& area, bool outer) const;
  std::vector<Edge> getBoundaryEdges(const Polygon90& area, bool outer) const;
  std::vector<Corner> getBoundaryCorners(const Polygon90& area,
                                         bool outer) const;

  std::string toString(EdgeType type) const;
  std::string toString(CornerType type) const;

  odb::dbRow* getRow(const Corner& corner, odb::dbSite* site) const;
  std::vector<odb::dbRow*> getRows(const Edge& edge, odb::dbSite* site) const;

  std::pair<int, int> placeEndcaps(const Polygon& area,
                                   bool outer,
                                   const EndcapCellOptions& options);
  std::pair<int, int> placeEndcaps(const Polygon90& area,
                                   bool outer,
                                   const EndcapCellOptions& options);

  CornerMap placeEndcapCorner(const Corner& corner,
                              const EndcapCellOptions& options);
  int placeEndcapEdge(const Edge& edge,
                      const CornerMap& corners,
                      const EndcapCellOptions& options);
  int placeEndcapEdgeHorizontal(const Edge& edge,
                                const CornerMap& corners,
                                const EndcapCellOptions& options);
  int placeEndcapEdgeVertical(const Edge& edge,
                              const CornerMap& corners,
                              const EndcapCellOptions& options);

  EndcapCellOptions correctEndcapOptions(
      const EndcapCellOptions& options) const;

  EndcapCellOptions correctEndcapOptions(const Options& options) const;

  odb::dbMaster* getMasterByType(const odb::dbMasterType& type) const;
  std::set<odb::dbMaster*> findMasterByType(
      const odb::dbMasterType& type) const;
  odb::dbBlock* getBlock() const;

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  int phy_idx_ = 0;
  std::string tap_prefix_;
  std::string endcap_prefix_;
  std::vector<Edge> filled_edges_;
};

}  // namespace tap
