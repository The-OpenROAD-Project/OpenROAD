// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"

namespace grt {

class Net;

// For iterm pins this is the edge on a macro/pad cell that the pin lies on.
// For bterm pins this is the edge on block that the pin lies on.
// It is none for standard cells.
enum class PinEdge
{
  north,
  south,
  east,
  west,
  none
};

class Pin
{
 public:
  Pin(odb::dbITerm* iterm,
      const odb::Point& position,
      const std::vector<odb::dbTechLayer*>& layers,
      const std::map<odb::dbTechLayer*, std::vector<odb::Rect>>&
          boxes_per_layer,
      bool connected_to_pad_or_macro);
  Pin(odb::dbBTerm* bterm,
      const odb::Point& position,
      const std::vector<odb::dbTechLayer*>& layers,
      const std::map<odb::dbTechLayer*, std::vector<odb::Rect>>&
          boxes_per_layer,
      const odb::Point& die_center);

  odb::dbITerm* getITerm() const;
  odb::dbBTerm* getBTerm() const;
  std::string getName() const;
  const odb::Point& getPosition() const { return position_; }
  const std::vector<int>& getLayers() const { return layers_; }
  int getNumLayers() const { return layers_.size(); }
  int getConnectionLayer() const { return connection_layer_; }
  void setConnectionLayer(int layer) { connection_layer_ = layer; }
  PinEdge getEdge() const { return edge_; }
  const std::map<int, std::vector<odb::Rect>>& getBoxes() const
  {
    return boxes_per_layer_;
  }
  bool isPort() const { return is_port_; }
  bool isConnectedToPadOrMacro() const { return connected_to_pad_or_macro_; }
  const odb::Point& getOnGridPosition() const { return on_grid_position_; }
  void setOnGridPosition(odb::Point on_grid_pos)
  {
    on_grid_position_ = on_grid_pos;
  }
  bool isDriver();
  odb::Point getPositionNearInstEdge(const odb::Rect& pin_box,
                                     const odb::Point& rect_middle) const;
  bool isCorePin() const;

 private:
  void determineEdge(const odb::Rect& bounds,
                     const std::vector<odb::dbTechLayer*>& layers);

  union
  {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
  };
  odb::Point position_;
  odb::Point on_grid_position_;
  std::vector<int> layers_;
  int connection_layer_;
  PinEdge edge_;
  std::map<int, std::vector<odb::Rect>> boxes_per_layer_;
  bool is_port_;
  bool connected_to_pad_or_macro_;
};

}  // namespace grt
