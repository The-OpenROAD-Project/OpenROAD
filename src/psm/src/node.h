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

#ifndef __IRSOLVER_NODE__
#define __IRSOLVER_NODE__

#include <map>

#include "odb/db.h"
#include "utl/Logger.h"
namespace psm {
using odb::dbInst;

using NodeLoc = std::pair<int, int>;
using BBox = std::pair<int, int>;
using NodeIdx = int;  // TODO temp as it interfaces with SUPERLU
using GMatLoc = std::pair<NodeIdx, NodeIdx>;

//! Data structure for the Dictionary of Keys Matrix
struct DokMatrix
{
  NodeIdx num_rows;
  NodeIdx num_cols;
  std::map<GMatLoc, double> values;  // pair < col_num, row_num >
};

//! Data structure for the Compressed Sparse Column Matrix
struct CscMatrix
{
  NodeIdx num_rows;
  NodeIdx num_cols;
  NodeIdx nnz;
  std::vector<NodeIdx> row_idx;
  std::vector<NodeIdx> col_ptr;
  std::vector<double> values;
};

//! Node class which stores the properties of the node of the PDN
class Node
{
 public:
  Node() : loc_(std::make_pair(0.0, 0.0)), bBox_(std::make_pair(0.0, 0.0)) {}
  ~Node() {}
  //! Get the layer number of the node
  int GetLayerNum();
  //! Set the layer number of the node
  void SetLayerNum(int layer);
  //! Get the location of the node
  NodeLoc GetLoc();
  //! Set the location of the node using x and y coordinates
  void SetLoc(int x, int y);
  //! Set the location of the node using x,y and layer information
  void SetLoc(int x, int y, int l);
  //! Get location of the node in G matrix
  NodeIdx GetGLoc();
  //! Get location of the node in G matrix
  void SetGLoc(NodeIdx loc);
  //! Function to print node details
  void Print(utl::Logger* logger);
  //! Function to set the bounding box of the stripe
  void SetBbox(int dX, int dY);
  //! Function to get the bounding box of the stripe
  BBox GetBbox();
  //! Function to update the stripe
  void UpdateMaxBbox(int dX, int dY);
  //! Function to set the current value at a particular node
  void SetCurrent(double t_current);
  //! Function to get the value of current at a node
  double GetCurrent();
  //! Function to add the current source
  void AddCurrentSrc(double t_current);
  //! Function to set the value of the voltage source
  void SetVoltage(double t_voltage);
  //! Function to get the value of the voltage source
  double GetVoltage();

  bool GetConnected();

  void SetConnected();

  bool HasInstances();

  std::vector<dbInst*> GetInstances();

  void AddInstance(dbInst* inst);

 private:
  int layer_;
  NodeLoc loc_;  // layer,x,y
  NodeIdx node_loc_{0};
  BBox bBox_;
  double current_src_{0.0};
  double voltage_{0.0};
  bool connected_{false};
  bool has_instances_{false};
  std::vector<dbInst*> connected_instances_;
};
}  // namespace psm
#endif
