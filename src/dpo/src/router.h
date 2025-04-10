// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <vector>

#include "rectangle.h"

namespace dpo {
class Node;
}

namespace dpo {

class RoutingParams
{
 public:
  class EdgeAdjust
  {
   public:
    int irow_ = -1;
    int icol_ = -1;
    int ilayer_ = -1;
    int jrow_ = -1;
    int jcol_ = -1;
    int jlayer_ = -1;
    double rcap_ = 0.0;

    EdgeAdjust() = default;

    EdgeAdjust(int irow,
               int icol,
               int ilayer,
               int jrow,
               int jcol,
               int jlayer,
               double rcap)
    {
      init(irow, icol, ilayer, jrow, jcol, jlayer, rcap);
    }
    EdgeAdjust(const EdgeAdjust& other)
    {
      init(other.irow_,
           other.icol_,
           other.ilayer_,
           other.jrow_,
           other.jcol_,
           other.jlayer_,
           other.rcap_);
    }
    EdgeAdjust& operator=(const EdgeAdjust& other)
    {
      if (this != &other) {
        init(other.irow_,
             other.icol_,
             other.ilayer_,
             other.jrow_,
             other.jcol_,
             other.jlayer_,
             other.rcap_);
      }
      return *this;
    }

    void init(int irow,
              int icol,
              int ilayer,
              int jrow,
              int jcol,
              int jlayer,
              double rcap)
    {
      irow_ = irow;
      icol_ = icol;
      ilayer_ = ilayer;
      jrow_ = jrow;
      jcol_ = jcol;
      jlayer_ = jlayer;
      rcap_ = rcap;
    }
  };

 public:
  RoutingParams()
  {
    v_capacity_.clear();
    h_capacity_.clear();
    wire_width_.clear();
    wire_spacing_.clear();
    via_spacing_.clear();

    edge_adjusts_.clear();
  }

  void postProcess();

  // Get spacing between two objects.
  double get_spacing(int layer,
                     double xmin1,
                     double xmax1,
                     double ymin1,
                     double ymax1,
                     double xmin2,
                     double xmax2,
                     double ymin2,
                     double ymax2);
  double get_spacing(int layer, double width, double parallel);
  double get_maximum_spacing(int layer);

 public:
  int grid_x_ = 0;
  int grid_y_ = 0;
  int num_layers_ = 0;

  int default_layer_ = 1;

  double origin_x_ = 0;
  double origin_y_ = 0;

  std::vector<double> v_capacity_;
  std::vector<double> h_capacity_;
  std::vector<double> wire_width_;
  std::vector<double> wire_spacing_;
  std::vector<double> via_spacing_;
  std::vector<int> layer_dir_;

  double tile_size_x_ = 0;
  double tile_size_y_ = 0;

  double blockage_porosity_ = 0;

  int num_route_blockages_ = 0;

  // Stuff for edge adjustements (ICCAD12).
  int num_edge_adjusts_ = 0;
  std::vector<EdgeAdjust> edge_adjusts_;

  // Map for routing blockages...  We have the name of the node and a vector of
  // layers with which it interferes...
  std::map<Node*, std::vector<unsigned>*> blockage_;

  // Other blockages which are simply specified by rectangles on a layer...
  std::vector<std::vector<Rectangle>> layerBlockages_;

  // Added to get information from LEF/DEF...
  double Xlowerbound_ = 0;
  double Xupperbound_ = 0;
  double Ylowerbound_ = 0;
  double Yupperbound_ = 0;
  double XpitchGcd_ = 0;
  double YpitchGcd_ = 0;
  int hasObs_ = 0;
  std::vector<std::vector<std::vector<unsigned>>> obs_;

  // Stuff for routing rules...  These vectors should all be the same length...
  int numRules_ = 0;
  std::vector<std::vector<double>> ruleWidths_;
  std::vector<std::vector<double>> ruleSpacings_;

  // Stuff for spacing tables...  Only one spacing table per layer...
  std::vector<std::vector<double>> spacingTableWidth_;
  std::vector<std::vector<double>> spacingTableLength_;
  std::vector<std::vector<std::vector<double>>> spacingTable_;
};

}  // namespace dpo
