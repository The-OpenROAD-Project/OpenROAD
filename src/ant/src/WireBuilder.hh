// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbWireGraph.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace ant {

struct GuidePoint
{
  odb::Point pos;
  odb::dbTechLayer* layer;
  bool operator<(const GuidePoint& pt) const;
};

struct GuideSegment
{
  GuidePoint pt1;
  GuidePoint pt2;
  bool isVia() const { return pt1.pos == pt2.pos; }
  bool operator==(const GuideSegment& segment) const;
};

struct GuidePtPins
{
  std::vector<odb::dbBTerm*> bterms;
  std::vector<odb::dbITerm*> iterms;
  bool connected = false;
};

using GuidePtPinsMap = std::map<GuidePoint, GuidePtPins>;

struct GuideSegmentHash
{
  std::size_t operator()(const GuideSegment& seg) const;
};

class WireBuilder
{
 public:
  WireBuilder(odb::dbDatabase* db, utl::Logger* logger);
  ~WireBuilder();

  void makeNetWiresFromGuides();
  void makeNetWiresFromGuides(const std::vector<odb::dbNet*>& nets);

 private:
  void makeNetWire(odb::dbNet* db_net, int gcell_dimension);
  void addWireTerms(odb::dbNet* db_net,
                    std::vector<GuideSegment>& route,
                    int grid_x,
                    int grid_y,
                    odb::dbTechLayer* tech_layer,
                    GuidePtPinsMap& route_pt_pins,
                    odb::dbWireEncoder& wire_encoder,
                    bool connect_to_segment);
  void makeWireToTerm(odb::dbWireEncoder& wire_encoder,
                      std::vector<GuideSegment>& route,
                      odb::dbTechLayer* tech_layer,
                      odb::dbTechLayer* conn_layer,
                      const std::vector<odb::Rect>& pin_rects,
                      const odb::Point& grid_pt,
                      odb::Point& pin_pt,
                      bool connect_to_segment);
  void makeWire(odb::dbWireEncoder& wire_encoder,
                odb::dbTechLayer* layer,
                const odb::Point& start,
                const odb::Point& end);
  std::vector<GuideSegment> makeWireFromGuides(odb::dbNet* db_net,
                                               GuidePtPinsMap& route_pt_pins,
                                               int gcell_dimension);
  bool checkGuideITermConnection(const GuidePoint& guide_pt,
                                 odb::dbITerm* iterm,
                                 const odb::Point& box_limit,
                                 int gcell_dimension);
  bool checkGuideBTermConnection(const GuidePoint& guide_pt,
                                 odb::dbBTerm* bterm,
                                 const odb::Point& box_limit,
                                 int gcell_dimension);
  void boxToGuideSegment(const odb::Rect& guide_box,
                         odb::dbTechLayer* layer,
                         odb::dbTechLayer* via_layer,
                         std::vector<GuideSegment>& route,
                         std::pair<GuidePoint, GuidePoint>& endpoints,
                         std::pair<odb::Point, odb::Point>& box_limits,
                         int gcell_dimension);
  bool pinOverlapsGSegment(const odb::Point& pin_position,
                           const odb::dbTechLayer* pin_layer,
                           const std::vector<odb::Rect>& pin_rects,
                           const std::vector<GuideSegment>& route);
  void getBTermTopLayerRects(odb::dbBTerm* bterm,
                             std::vector<odb::Rect>& rects,
                             int& top_layer_idx);
  void getITermTopLayerRects(odb::dbITerm* iterm,
                             std::vector<odb::Rect>& rects,
                             int& top_layer_idx);
  bool dbNetIsLocal(odb::dbNet* db_net);

  odb::dbDatabase* db_{nullptr};
  odb::dbBlock* block_{nullptr};
  utl::Logger* logger_{nullptr};
  std::map<odb::dbTechLayer*, odb::dbTechVia*> default_vias_;
};

}  // namespace ant
