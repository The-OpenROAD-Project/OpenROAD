/*
BSD 3-Clause License

Copyright (c) 2020, The Regents of the University of Minnesota

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <map>

#include "odb/db.h"
#include "utl/Logger.h"

namespace psm {

using odb::dbInst;
using odb::Point;
using NodeIdx = int;

struct NodeEnclosure
{
  int neg_x = 0;
  int pos_x = 0;
  int neg_y = 0;
  int pos_y = 0;

  int dx() { return neg_x + pos_x; };
  int dy() { return neg_y + pos_y; };
};

//! Node class which stores the properties of the node of the PDN
class Node
{
 public:
  Node(const Point& loc, int layer);
  //! Get the layer number of the node
  int getLayerNum() const;
  //! Get the location of the node
  Point getLoc() const;
  //! Get location of the node in G matrix
  NodeIdx getGLoc() const;
  //! Get location of the node in G matrix
  void setGLoc(NodeIdx loc);
  //! Function to print node details
  void print(utl::Logger* logger) const;
  //! Function to set the current value at a particular node
  void setCurrent(double t_current);
  //! Function to get the value of current at a node
  double getCurrent() const;
  //! Function to add the current source
  void addCurrentSrc(double t_current);
  //! Function to set the value of the voltage source
  void setVoltage(double t_voltage);
  //! Function to get the value of the voltage source
  double getVoltage() const;

  bool getConnected() const;

  void setConnected();

  bool hasInstances() const;

  void setEnclosure(NodeEnclosure encl);

  NodeEnclosure getEnclosure() const;

  const std::vector<dbInst*>& getInstances() const;

  void addInstance(dbInst* inst);

 private:
  int layer_{-1};
  Point loc_;
  NodeIdx node_loc_{0};
  double current_src_{0.0};
  double voltage_{0.0};
  bool connected_{false};
  NodeEnclosure encl_{0, 0, 0, 0};
  std::vector<dbInst*> connected_instances_;
};
}  // namespace psm
