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
#include "opendb/db.h"
using odb::dbInst;


typedef std::pair<int, int>         NodeLoc;
typedef std::pair<int, int>         BBox;
typedef int                         NodeIdx;  // TODO temp as it interfaces with SUPERLU
typedef std::pair<NodeIdx, NodeIdx> GMatLoc;

//! Data structure for the Dictionary of Keys Matrix
typedef struct
{
  NodeIdx                   num_rows;
  NodeIdx                   num_cols;
  std::map<GMatLoc, double> values;  // pair < col_num, row_num >
} DokMatrix;

//! Data structure for the Compressed Sparse Column Matrix
typedef struct
{
  NodeIdx              num_rows;
  NodeIdx              num_cols;
  NodeIdx              nnz;
  std::vector<NodeIdx> row_idx;
  std::vector<NodeIdx> col_ptr;
  std::vector<double>  values;
} CscMatrix;

//! Node class which stores the properties of the node of the PDN
class Node
{
 public:
  Node() : m_loc(std::make_pair(0.0, 0.0)), m_bBox(std::make_pair(0.0, 0.0)) {}
  ~Node() {}
  //! Get the layer number of the node
  int     GetLayerNum();
  //! Set the layer number of the node
  void    SetLayerNum(int layer);
  //! Get the location of the node
  NodeLoc GetLoc();
  //! Set the location of the node using x and y coordinates
  void    SetLoc(int x, int y);
  //! Set the location of the node using x,y and layer information
  void    SetLoc(int x, int y, int l);
  //! Get location of the node in G matrix
  NodeIdx GetGLoc();
  //! Get location of the node in G matrix
  void    SetGLoc(NodeIdx loc);
  //! Function to print node details
  void    Print();
  //! Function to set the bounding box of the stripe
  void    SetBbox(int dX, int dY);
  //! Function to get the bounding box of the stripe
  BBox    GetBbox();
  //! Function to update the stripe
  void    UpdateMaxBbox(int dX, int dY);
  //! Function to set the current value at a particular node
  void    SetCurrent(double t_current);
  //! Function to get the value of current at a node
  double  GetCurrent();
  //! Function to add the current source 
  void    AddCurrentSrc(double t_current);
  //! Function to set the value of the voltage source
  void    SetVoltage(double t_voltage);
  //! Function to get the value of the voltage source
  double  GetVoltage();

  bool    GetConnected();

  void    SetConnected();

  bool    HasInstances();

  std::vector<dbInst*> GetInstances();

  void    AddInstance(dbInst* inst);

 private:
  int     m_layer;
  NodeLoc m_loc;  // layer,x,y
  NodeIdx m_node_loc{0};
  BBox    m_bBox;
  double  m_current_src{0.0};
  double  m_voltage{0.0};
  bool    m_connected{false};
  bool    m_has_instances{false};
  std::vector<dbInst*> m_connected_instances;
};
#endif
