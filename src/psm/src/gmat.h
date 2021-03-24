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

#ifndef __IRSOLVER_GMAT_
#define __IRSOLVER_GMAT_
#include "node.h"
#include "opendb/db.h"
#include "utl/Logger.h"

//! Global variable which holds the G matrix.
/*!
 * Three dimensions with layer number,x location, and y location.
 * Holds a pointer to the node of the power grid.
 */
namespace psm {
typedef std::map<int, std::map<int, Node*>> NodeMap;

//! G matrix class
/*!
 * Class to store the G matrix. Contains the member functions for all node
 * related operations.
 *
 */
class GMat
{
 public:
  //! Constructor for creating the G matrix
  GMat(int t_num_layers, utl::Logger* logger)
      : m_num_layers(t_num_layers), m_layer_maps(t_num_layers + 1, NodeMap())
  {  // as it start from 0 and everywhere we use layer
    m_logger = logger;
  }
  //! Destructor of the G matrix
  ~GMat()
  {
    while (!m_G_mat_nodes.empty()) {
      delete m_G_mat_nodes.back();
      m_G_mat_nodes.pop_back();
    }
  }
  //! Function to return a pointer to the node with a index
  Node* GetNode(NodeIdx t_node);
  //! Function to return a pointer to the node with the x, y, and layer number
  Node* GetNode(int t_x, int t_y, int t_l, bool t_nearest = false);
  //! Function to set attributes of the node with index and node pointer
  void SetNode(NodeIdx t_node_loc, Node* t_node);
  //! Function to create a node
  Node* SetNode(int t_x, int t_y, int t_layer, BBox t_bBox);
  //! Function to insert a node into the matrix
  void InsertNode(Node* t_node);
  //! Function that prints the G matrix for debug purposes
  void Print();
  //! Function to add the conductance value between two nodes
  void SetConductance(Node* t_node1, Node* t_node2, double t_cond);
  //! Function to initialize the sparse dok matrix
  void InitializeGmatDok(int t_numC4);
  //! Function that returns the number of nodes in the G matrix
  NodeIdx GetNumNodes();
  //! Function to return a pointer to the G matrix
  CscMatrix* GetGMat();
  //! Function to return a pointer to the A matrix
  CscMatrix* GetAMat();
  //! Function to get the conductance of the strip of the power grid
  void GenerateStripeConductance(int                        t_l,
                                 odb::dbTechLayerDir::Value layer_dir,
                                 int                        t_x_min,
                                 int                        t_x_max,
                                 int                        t_y_min,
                                 int                        t_y_max,
                                 double                     t_rho);
  //! Function to get location of vias to the redistribution layer
  std::vector<Node*> GetRDLNodes(int                        t_l,
                                 odb::dbTechLayerDir::Value layer_dir,
                                 int                        t_x_min,
                                 int                        t_x_max,
                                 int                        t_y_min,
                                 int                        t_y_max);
  //! Function to add the voltage source based on C4 bump location
  void AddC4Bump(int t_loc, int t_C4Num);
  //! Function which generates the compressed sparse column matrix
  bool GenerateCSCMatrix();
  //! Function which generates the compressed sparse column matrix for A
  bool GenerateACSCMatrix();
  //! Function to return a vector which contains a  pointer to all the nodes
  std::vector<Node*> GetAllNodes();

  //! Function to return a pointer to the G matrix in DOK format
  DokMatrix* GetGMatDOK();

 private:
  //! Pointer to the logger
  utl::Logger* m_logger;
  //! Number of nodes in G matrix
  NodeIdx m_n_nodes{0};
  //! Number of metal layers in PDN stack
  int m_num_layers;
  //! Dictionary of keys for G matrix
  DokMatrix m_G_mat_dok;
  //! Compressed sparse column matrix for superLU
  CscMatrix m_G_mat_csc;
  //! Dictionary of keys for A matrix
  DokMatrix m_A_mat_dok;
  //! Compressed sparse column matrix for A
  CscMatrix m_A_mat_csc;
  //! Vector of pointers to all nodes in the G matrix
  std::vector<Node*> m_G_mat_nodes;
  //! Vector of maps to all nodes
  std::vector<NodeMap> m_layer_maps;
  //! Function to get the conductance value at a row and column of the matrix
  double GetConductance(NodeIdx t_row, NodeIdx t_col);
  //! Function to add a conductance value at the specified location of the
  //! matrix
  void UpdateConductance(NodeIdx t_row, NodeIdx t_col, double t_cond);
  //! Function to find the nearest node to a particular location
  Node* NearestYNode(NodeMap::iterator x_itr, int t_y);
  //! Function to find conductivity of a stripe based on width,length, and pitch
  double GetConductivity(double width, double length, double rho);
};
}  // namespace psm
#endif
