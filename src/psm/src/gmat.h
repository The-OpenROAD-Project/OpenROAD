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

#include "node.h"
#include "odb/db.h"
#include "utl/Logger.h"

//! Global variable which holds the G matrix.
/*!
 * Three dimensions with layer number,x location, and y location.
 * Holds a pointer to the node of the power grid.
 */
namespace psm {
using NodeMap = std::map<int, std::map<int, Node*>>;

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
  GMat(int num_layers, utl::Logger* logger, odb::dbTech* tech);
  //! Destructor of the G matrix
  ~GMat();
  //! Function to return a pointer to the node with a index
  Node* getNode(NodeIdx node);
  bool findLayer(int layer);
  //! Function to return a pointer to the node with the x, y, and layer number
  Node* getNode(int x, int y, int layer, bool nearest);
  //! Call func on all nodes in the region defined
  void foreachNode(int layer,
                   int x_min,
                   int x_max,
                   int y_min,
                   int y_max,
                   const std::function<void(Node*)>& func);

  //! Function to return a vector of pointers to the nodes within an area sorted
  // by direction
  std::map<std::pair<int, int>, Node*> getNodes(
      int layer,
      odb::dbTechLayerDir::Value layer_dir,
      int x_min,
      int x_max,
      int y_min,
      int y_max);
  //! Function to create a node
  Node* setNode(const Point& loc, int layer);
  //! Function to insert a node into the matrix
  void insertNode(Node* node);
  //! Function that prints the G matrix for debug purposes
  void print();
  //! Function to add the conductance value between two nodes
  void setConductance(const Node* node1, const Node* node2, double cond);
  //! Function to initialize the sparse dok matrix
  void initializeGmatDok(int num);
  //! Function that returns the number of nodes in the G matrix
  NodeIdx getNumNodes();
  //! Function to return a pointer to the G matrix
  CscMatrix* getGMat();
  //! Function to return a pointer to the A matrix
  CscMatrix* getAMat();
  //! Function to get the conductance of the strip of the power grid
  void generateStripeConductance(int layer,
                                 odb::dbTechLayerDir::Value layer_dir,
                                 int x_min,
                                 int x_max,
                                 int y_min,
                                 int y_max,
                                 double rho);
  //! Function to get location of vias to the redistribution layer
  std::vector<Node*> getRDLNodes(int layer,
                                 odb::dbTechLayerDir::Value layer_dir,
                                 int x_min,
                                 int x_max,
                                 int y_min,
                                 int y_max);
  //! Function to add the voltage source based on source location
  void addSource(int loc, int source_number);
  //! Function which generates the compressed sparse column matrix
  bool generateCSCMatrix();
  //! Function which generates the compressed sparse column matrix for A
  bool generateACSCMatrix();
  //! Function to return a vector which contains a  pointer to all the nodes
  std::vector<Node*> getAllNodes();
  //! Function to return a pointer to the G matrix in DOK format
  DokMatrix* getGMatDOK();

 private:
  //! Function to get the conductance value at a row and column of the matrix
  double getConductance(NodeIdx row, NodeIdx col);
  //! Function to add a conductance value at the specified location of the
  //! matrix
  void updateConductance(NodeIdx row, NodeIdx col, double cond);
  //! Function to find the nearest node to a particular location
  Node* nearestYNode(NodeMap::const_iterator x_itr, int y);
  //! Function to find conductivity of a stripe based on width,length, and pitch
  double getConductivity(double width, double length, double rho);

  //! Pointer to the logger
  utl::Logger* logger_{nullptr};
  //! Pointer to the logger
  odb::dbTech* tech_{nullptr};
  //! Number of nodes in G matrix
  NodeIdx n_nodes_{0};
  //! Dictionary of keys for G matrix
  DokMatrix G_mat_dok_;
  //! Compressed sparse column matrix for superLU
  CscMatrix G_mat_csc_;
  //! Dictionary of keys for A matrix
  DokMatrix A_mat_dok_;
  //! Compressed sparse column matrix for A
  CscMatrix A_mat_csc_;
  //! Vector of pointers to all nodes in the G matrix
  std::vector<Node*> G_mat_nodes_;
  //! Vector of maps to all nodes
  std::vector<NodeMap> layer_maps_;
};
}  // namespace psm
