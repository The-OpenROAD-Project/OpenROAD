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
#include <map>
#include <vector>
#include "rectangle.h"

namespace dpo {
class Node;
}

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class RoutingParams {
 public:
  class EdgeAdjust {
   public:
    int m_irow;
    int m_icol;
    int m_ilayer;
    int m_jrow;
    int m_jcol;
    int m_jlayer;
    double m_rcap;

    EdgeAdjust()
        : m_irow(-1),
          m_icol(-1),
          m_ilayer(-1),
          m_jrow(-1),
          m_jcol(-1),
          m_jlayer(-1),
          m_rcap(0.0) {}
    EdgeAdjust(int irow, int icol, int ilayer, int jrow, int jcol, int jlayer,
               double rcap) {
      init(irow, icol, ilayer, jrow, jcol, jlayer, rcap);
    }
    EdgeAdjust(const EdgeAdjust& other) {
      init(other.m_irow, other.m_icol, other.m_ilayer, other.m_jrow,
           other.m_jcol, other.m_jlayer, other.m_rcap);
    }
    EdgeAdjust& operator=(const EdgeAdjust& other) {
      if (this != &other) {
        init(other.m_irow, other.m_icol, other.m_ilayer, other.m_jrow,
             other.m_jcol, other.m_jlayer, other.m_rcap);
      }
      return *this;
    }
    virtual ~EdgeAdjust() {}

    void init(int irow, int icol, int ilayer, int jrow, int jcol, int jlayer,
              double rcap) {
      m_irow = irow;
      m_icol = icol;
      m_ilayer = ilayer;
      m_jrow = jrow;
      m_jcol = jcol;
      m_jlayer = jlayer;
      m_rcap = rcap;
    }
  };

 public:
  RoutingParams()
      : m_grid_x(0),
        m_grid_y(0),
        m_num_layers(0),
        m_default_layer(1),
        m_origin_x(0),
        m_origin_y(0),
        m_tile_size_x(0),
        m_tile_size_y(0),
        m_blockage_porosity(0.0),
        m_num_ni_terminals(0),
        m_num_route_blockages(0),
        m_num_edge_adjusts(0),
        m_Xlowerbound(0.0),
        m_Xupperbound(0.0),
        m_Ylowerbound(0.0),
        m_Yupperbound(0.0),
        m_XpitchGcd(0.0),
        m_YpitchGcd(0.0),
        m_hasObs(0),
        m_numRules(0) {
    m_v_capacity.erase(m_v_capacity.begin(), m_v_capacity.end());
    m_h_capacity.erase(m_h_capacity.begin(), m_h_capacity.end());
    m_wire_width.erase(m_wire_width.begin(), m_wire_width.end());
    m_wire_spacing.erase(m_wire_spacing.begin(), m_wire_spacing.end());
    m_via_spacing.erase(m_via_spacing.begin(), m_via_spacing.end());

    m_edge_adjusts.erase(m_edge_adjusts.begin(), m_edge_adjusts.end());
  }
  virtual ~RoutingParams() { ; }

  void postProcess();

  // Get spacing between two objects.
  double get_spacing(int layer, double xmin1, double xmax1, double ymin1,
                     double ymax1, double xmin2, double xmax2, double ymin2,
                     double ymax2);
  double get_spacing(int layer, double width, double parallel);
  double get_maximum_spacing(int layer);

 public:
  int m_grid_x;
  int m_grid_y;
  int m_num_layers;

  int m_default_layer;

  double m_origin_x;
  double m_origin_y;

  std::vector<double> m_v_capacity;
  std::vector<double> m_h_capacity;
  std::vector<double> m_wire_width;
  std::vector<double> m_wire_spacing;
  std::vector<double> m_via_spacing;
  std::vector<int> m_layer_dir;

  double m_tile_size_x;
  double m_tile_size_y;

  double m_blockage_porosity;

  int m_num_ni_terminals;
  int m_num_route_blockages;

  // Stuff for edge adjustements (ICCAD12).
  int m_num_edge_adjusts;
  std::vector<EdgeAdjust> m_edge_adjusts;

  // Map for routing blockages...  We have the name of the node and a vector of
  // layers with which it interferes...
  std::map<Node*, std::vector<unsigned>*> m_blockage;

  // Other blockages which are simply specified by rectangles on a layer...
  std::vector<std::vector<Rectangle> > m_layerBlockages;

  // Added to get information from LEF/DEF...
  double m_Xlowerbound;
  double m_Xupperbound;
  double m_Ylowerbound;
  double m_Yupperbound;
  double m_XpitchGcd;
  double m_YpitchGcd;
  int m_hasObs;
  std::vector<std::vector<std::vector<unsigned> > > m_obs;

  // Stuff for routing rules...  These vectors should all be the same length...
  int m_numRules;
  std::vector<std::vector<double> > m_ruleWidths;
  std::vector<std::vector<double> > m_ruleSpacings;

  // Stuff for spacing tables...  Only one spacing table per layer...
  std::vector<std::vector<double> > m_spacingTableWidth;
  std::vector<std::vector<double> > m_spacingTableLength;
  std::vector<std::vector<std::vector<double> > > m_spacingTable;
};

}  // namespace dpo
