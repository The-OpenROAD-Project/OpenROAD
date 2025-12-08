// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "GeoTypes.h"
#include "odb/db.h"

namespace grt {

struct LayerRange
{
  int min_layer;
  int max_layer;
};

class CUGRPin
{
 public:
  CUGRPin(int index,
          odb::dbITerm* iterm,
          const std::vector<BoxOnLayer>& pin_shapes);
  CUGRPin(int index,
          odb::dbBTerm* bterm,
          const std::vector<BoxOnLayer>& pin_shapes);

  int getIndex() const { return index_; }
  odb::dbITerm* getITerm() const;
  odb::dbBTerm* getBTerm() const;
  std::vector<BoxOnLayer> getPinShapes() const { return pin_shapes_; }
  bool isPort() const { return is_port_; }
  std::string getName() const;

 private:
  union
  {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
  };
  std::vector<BoxOnLayer> pin_shapes_;
  const int index_;
  const bool is_port_;
};

class CUGRNet
{
 public:
  CUGRNet(int index,
          odb::dbNet* db_net,
          const std::vector<CUGRPin>& pins,
          LayerRange layer_range);
  int getIndex() const { return index_; }
  odb::dbNet* getDbNet() const { return db_net_; }
  std::vector<CUGRPin> getPins() const { return pins_; }
  int getNumPins() const { return pins_.size(); }
  std::string getName() const { return db_net_->getName(); }
  LayerRange getLayerRange() const { return layer_range_; }

 private:
  const int index_;
  odb::dbNet* db_net_;
  std::vector<CUGRPin> pins_;
  LayerRange layer_range_;
};

}  // namespace grt
