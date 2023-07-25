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

#include "gmat.h"

#include <iostream>
#include <vector>

#include "node.h"

namespace psm {
using std::make_pair;
using std::map;
using std::pair;
using std::vector;

//! Constructor for creating the G matrix
GMat::GMat(int num_layers, utl::Logger* logger, odb::dbTech* tech)
    : layer_maps_(num_layers + 1, NodeMap())
{
  // as it start from 0 and everywhere we use layer
  logger_ = logger;
  tech_ = tech;
}

//! Destructor of the G matrix
GMat::~GMat()
{
  while (!G_mat_nodes_.empty()) {
    delete G_mat_nodes_.back();
    G_mat_nodes_.pop_back();
  }
}

//! Function to return a pointer to the node with a index
/*!
     \param node Node index number
     \return Pointer to the node in the matrix
*/
Node* GMat::getNode(NodeIdx node)
{
  if (0 <= node && n_nodes_ > node) {
    return G_mat_nodes_[node];
  }
  return nullptr;
}

//! Function to return a vector of nodes in an area
void GMat::foreachNode(int layer,
                       int x_min,
                       int x_max,
                       int y_min,
                       int y_max,
                       const std::function<void(Node*)>& func)
{
  NodeMap& layer_map = layer_maps_[layer];

  for (auto x_itr = layer_map.lower_bound(x_min);
       x_itr != layer_map.end() && x_itr->first <= x_max;
       ++x_itr) {
    const map<int, Node*>& y_itr_map = x_itr->second;
    for (auto y_map_itr = y_itr_map.lower_bound(y_min);
         y_map_itr != y_itr_map.end() && y_map_itr->first <= y_max;
         ++y_map_itr) {
      func(y_map_itr->second);
    }
  }
}

//! Function to return a vector of pointers to the nodes within an area sorted
// by direction
map<pair<int, int>, Node*> GMat::getNodes(int layer,
                                          odb::dbTechLayerDir::Value layer_dir,
                                          int x_min,
                                          int x_max,
                                          int y_min,
                                          int y_max)
{
  NodeMap& layer_map = layer_maps_[layer];
  if (x_min > x_max || y_min > y_max) {
    logger_->warn(utl::PSM,
                  80,
                  "Creating stripe condunctance with invalid inputs. Min and "
                  "max values for X or Y are interchanged.");
  }
  map<pair<int, int>, Node*> node_map;
  // Also check one node before and after to see if it has an overlapping
  // enclosure
  // Start iterating from the first value in the map that is lower than x_min
  auto x_strt = layer_map.lower_bound(x_min);
  if (x_strt != layer_map.begin()) {
    --x_strt;
  }

  // End iteration on the first value in the map that is larger than x_max
  auto x_end = layer_map.upper_bound(x_max);
  if (x_end != layer_map.end()) {
    x_end++;
  }

  for (auto x_itr = x_strt; x_itr != x_end; ++x_itr) {
    const map<int, Node*> y_itr_map = x_itr->second;
    // Start iterating from the first value in the map that is lower than y_min
    auto y_strt = y_itr_map.lower_bound(y_min);
    if (y_strt != y_itr_map.begin()) {
      y_strt--;
    }

    // End iteration on the first value in the map that is larger than y_max
    auto y_end = y_itr_map.upper_bound(y_max);
    if (y_end != y_itr_map.end()) {
      y_end++;
    }

    for (auto y_itr = y_strt; y_itr != y_end; ++y_itr) {
      auto encl = (y_itr->second)->getEnclosure();
      if (layer_dir == odb::dbTechLayerDir::Value::HORIZONTAL) {
        // Skip if x+enclosure and y is not within bounds
        if (((x_itr->first + encl.pos_x) < x_min)
            || ((x_itr->first - encl.neg_x) > x_max) || (y_itr->first < y_min)
            || (y_itr->first > y_max)) {
          continue;
        }
        node_map.insert(
            make_pair(make_pair(x_itr->first, y_itr->first), y_itr->second));
      } else {  // vertical
        // Skip if y+enclosure and x is not within bounds
        if (((y_itr->first + encl.pos_y) < y_min)
            || ((y_itr->first - encl.neg_y) > y_max) || (x_itr->first < x_min)
            || (x_itr->first > x_max)) {
          continue;
        }
        node_map.insert(
            make_pair(make_pair(y_itr->first, x_itr->first), y_itr->second));
      }
    }
  }
  return node_map;
}

//! Function to check if the layer_map exists for a layer
bool GMat::findLayer(int layer)
{
  if (layer > layer_maps_.size() || layer <= 0) {
    return false;
  }
  const NodeMap& layer_map = layer_maps_[layer];
  return !layer_map.empty();
}

//! Function to return a pointer to the node with a index
/*!
     \param x x location coordinate
     \param y y location coordinate
     \param layer layer number
     \return Pointer to the node in the matrix
*/
Node* GMat::getNode(int x, int y, int layer, bool nearest /*=false*/)
{
  if (!findLayer(layer)) {
    logger_->error(utl::PSM,
                   45,
                   "Layer {} contains no grid nodes.",
                   tech_->findRoutingLayer(layer)->getName());
  }
  const NodeMap& layer_map = layer_maps_[layer];
  if (nearest == false) {
    const auto x_itr = layer_map.find(x);
    if (x_itr != layer_map.end()) {
      const auto y_itr = x_itr->second.find(y);
      if (y_itr != x_itr->second.end()) {
        return y_itr->second;
      }
      logger_->error(utl::PSM, 46, "Node location lookup error for y.");
    } else {
      logger_->error(utl::PSM, 47, "Node location lookup error for x.");
    }
  } else {
    auto x_itr = layer_map.lower_bound(x);
    if (layer_map.size() == 1) {
      x_itr = layer_map.begin();
    } else if (x_itr == layer_map.end()) {
      x_itr = prev(x_itr);
    }
    Node* node = nearestYNode(x_itr, y);
    Point node_loc = node->getLoc();
    int dist = abs(node_loc.getX() - x) + abs(node_loc.getY() - y);
    // Searching a bounding box of all nodes nearby to see if a closer one
    // exists.
    auto keep_nearest = [&](Node* new_node) {
      const Point node_loc = new_node->getLoc();
      const int new_dist = abs(node_loc.getX() - x) + abs(node_loc.getY() - y);
      if (new_dist < dist) {
        dist = new_dist;
        node = new_node;
      }
    };
    foreachNode(layer,
                x - dist,  // xmin
                x + dist,  // xmax
                y - dist,  // ymin
                y + dist,
                keep_nearest);  // ymax
    return node;
  }
}

//! Function to add a node to the matrix
/*!
 * Directly updates the G node vector
     \param node Location to insert the node
     \return nothing
*/
void GMat::insertNode(Node* node)
{
  node->setGLoc(n_nodes_);
  int layer = node->getLayerNum();
  Point nodeLoc = node->getLoc();
  NodeMap& layer_map = layer_maps_[layer];
  layer_map[nodeLoc.getX()][nodeLoc.getY()] = node;
  G_mat_nodes_.push_back(node);
  n_nodes_++;
}

//! Function to create a node
/*!
     \param loc node location
     \param layer layer number
     \param bBox layer number
     \return Pointer to the created node
*/
Node* GMat::setNode(const Point& loc, int layer)
{
  NodeMap& layer_map = layer_maps_[layer];
  if (layer_map.empty()) {
    Node* node = new Node(loc, layer);
    insertNode(node);
    return node;
  }
  NodeMap::iterator x_itr = layer_map.find(loc.getX());
  if (x_itr != layer_map.end()) {
    map<int, Node*>::iterator y_itr = x_itr->second.find(loc.getY());
    if (y_itr != x_itr->second.end()) {
      Node* node = y_itr->second;
      return node;
    }
    Node* node = new Node(loc, layer);
    insertNode(node);
    return node;
  }
  Node* node = new Node(loc, layer);
  insertNode(node);
  return node;
}

//! Function to print the G matrix
void GMat::print()
{
  logger_->info(utl::PSM, 48, "Printing GMat obj, with {} nodes.", n_nodes_);
  for (NodeIdx i = 0; i < n_nodes_; i++) {
    Node* node_ptr = G_mat_nodes_[i];
    if (node_ptr != nullptr) {
      node_ptr->print(logger_);
    }
  }
}

//! Function to set conductance values in the G matrix
/*!
 * Directly updates the G matrix
     \param node1 Node pointer 1
     \param node2 Node pointer 2
     \param cond conductance value to be added between node 1 and 2
     \return nothing
*/
void GMat::setConductance(const Node* node1,
                          const Node* node2,
                          const double cond)
{
  NodeIdx node1_r = node1->getGLoc();
  NodeIdx node2_r = node2->getGLoc();
  double node11_cond = getConductance(node1_r, node1_r);
  double node22_cond = getConductance(node2_r, node2_r);
  double node12_cond = getConductance(node1_r, node2_r);
  double node21_cond = getConductance(node2_r, node1_r);
  // Only perform an update if the conductance is higher in case of overlaps.
  // Higher conductance implies larger width.
  // node12_cond is only set for 1 pair of nodes and is negative.
  // Since there are multiple metal segments over the same area in the same
  // layer
  if ((cond + node12_cond) > 0) {
    updateConductance(node1_r, node1_r, node11_cond + cond + node12_cond);
    updateConductance(node2_r, node2_r, node22_cond + cond + node12_cond);
    updateConductance(node1_r, node2_r, -cond);
    updateConductance(node2_r, node1_r, -cond);
  } else {  // to add connection even if there is not change in conductance
    updateConductance(node1_r, node1_r, node11_cond);
    updateConductance(node2_r, node2_r, node22_cond);
    updateConductance(node1_r, node2_r, node12_cond);
    updateConductance(node2_r, node1_r, node21_cond);
  }
}

//! Function to initialize the Dictionary of keys (DOK) matrix
/*! Based on the size of the G matrix
 * initialize the number of rows and columns
 */
void GMat::initializeGmatDok(int num)
{
  if (n_nodes_ <= 0) {
    logger_->error(utl::PSM, 49, "No nodes in object, initialization stopped.");
  } else {
    G_mat_dok_.num_cols = n_nodes_ + num;
    G_mat_dok_.num_rows = n_nodes_ + num;
    A_mat_dok_.num_cols = n_nodes_ + num;
    A_mat_dok_.num_rows = n_nodes_ + num;
  }
}

//! Function that returns the number of nodes in the G matrix
NodeIdx GMat::getNumNodes()
{  // debug
  return n_nodes_;
}

//! Function to return a pointer to the G matrix in CSC format
CscMatrix* GMat::getGMat()
{  // Nodes debug
  return &G_mat_csc_;
}

//! Function to return a pointer to the A matrix in CSC format
CscMatrix* GMat::getAMat()
{  // Nodes debug
  return &A_mat_csc_;
}

//! Function to return a pointer to the G matrix in DOK format
DokMatrix* GMat::getGMatDOK()
{  // Nodes debug
  return &G_mat_dok_;
}

//! Function that gets the value of the conductance of the stripe and
// updates the G matrix
/*!
 * Directly updates the G matrix
     \param layer Layer number
     \param layer_dir  Direction of the layer
     \param x_min Lower left x location
     \param x_max Upper right x location
     \param y_min Lower left y location
     \param y_max Upper right y location
     \return nothing
*/
void GMat::generateStripeConductance(int layer,
                                     odb::dbTechLayerDir::Value layer_dir,
                                     int x_min,
                                     int x_max,
                                     int y_min,
                                     int y_max,
                                     double rho)
{
  if (x_min > x_max || y_min > y_max) {
    logger_->warn(utl::PSM,
                  50,
                  "Creating stripe condunctance with invalid inputs. Min and "
                  "max values for X or Y are interchanged.");
  }
  auto node_map = getNodes(layer, layer_dir, x_min, x_max, y_min, y_max);
  int i = 0;
  pair<pair<int, int>, Node*> node_prev;
  for (auto& node_itr : node_map) {
    if (i == 0) {
      i = 1;
    } else {
      Node* node1 = node_itr.second;
      Node* node2 = node_prev.second;
      int width;
      if (layer_dir == odb::dbTechLayerDir::Value::HORIZONTAL) {
        width = y_max - y_min;
      } else {
        width = x_max - x_min;
      }
      int length = (node_itr.first).first - (node_prev.first).first;
      if (length == 0) {
        length = (node_itr.first).second - (node_prev.first).second;
      }
      double cond = getConductivity(width, length, rho);
      setConductance(node1, node2, cond);
    }
    node_prev = node_itr;
  }
}
//! Function that gets the locations of nodes to connect RDL vias
/*!
 * Directly updates the G matrix
     \param layer Layer number
     \param layer_dir  Direction of the layer
     \param x_min Lower left x location
     \param x_max Upper right x location
     \param y_min Lower left y location
     \param y_max Upper right y location
     \return std::vector<Node*>
*/
vector<Node*> GMat::getRDLNodes(int layer,
                                odb::dbTechLayerDir::Value layer_dir,
                                int x_min,
                                int x_max,
                                int y_min,
                                int y_max)
{
  vector<Node*> RDLNodes;
  NodeMap& layer_map = layer_maps_[layer];
  Point node_loc;
  Node* node1;
  Node* node2;
  if (layer_dir == odb::dbTechLayerDir::Value::HORIZONTAL) {
    int y_loc = (y_min + y_max) / 2;
    node1 = getNode(x_min, y_loc, layer, true);
    node_loc = node1->getLoc();
    int x1 = node_loc.getX();
    node2 = getNode(x_max, y_loc, layer, true);
    node_loc = node2->getLoc();
    int x2 = node_loc.getX();
    map<int, Node*> y_map = layer_map.at(x1);
    map<int, Node*>::iterator y_itr;
    for (y_itr = y_map.lower_bound(y_min);
         y_itr->first <= y_max && y_itr != y_map.end();
         ++y_itr) {
      node1 = y_itr->second;
      RDLNodes.push_back(node1);
    }
    y_map = layer_map.at(x2);
    for (y_itr = y_map.lower_bound(y_min);
         y_itr->first <= y_max && y_itr != y_map.end();
         ++y_itr) {
      node2 = y_itr->second;
      RDLNodes.push_back(node2);
    }
  } else {
    int x_loc = (x_min + x_max) / 2;
    node1 = getNode(x_loc, y_min, layer, true);
    node_loc = node1->getLoc();
    int y1 = node_loc.getY();
    node2 = getNode(x_loc, y_max, layer, true);
    node_loc = node2->getLoc();
    int y2 = node_loc.getY();
    NodeMap::iterator x_itr;
    for (x_itr = layer_map.lower_bound(x_min);
         x_itr != layer_map.end() && x_itr->first <= x_max;
         ++x_itr) {
      map<int, Node*>::iterator y_iter;
      y_iter = (x_itr->second).find(y1);
      if (y_iter != (x_itr->second).end()) {
        RDLNodes.push_back(y_iter->second);
      }
      y_iter = (x_itr->second).find(y2);
      if (y_iter != (x_itr->second).end()) {
        RDLNodes.push_back(y_iter->second);
      }
    }
  }
  return RDLNodes;
}

//! Function which add the values in the G matrix for the
// voltage sources in MNA
/*!
 * Directly updates the G matrix
     \param loc Location of the source
     \param source_number  source number
     \return nothing
*/
void GMat::addSource(int loc, int source_number)
{
  updateConductance(loc, source_number + n_nodes_, 1);
  updateConductance(source_number + n_nodes_, loc, 1);
}

//! Function which converts the DOK matrix into CSC format in a sparse method
/*!
 */
bool GMat::generateCSCMatrix()
{
  G_mat_csc_.num_cols = G_mat_dok_.num_cols;
  G_mat_csc_.num_rows = G_mat_dok_.num_rows;
  G_mat_csc_.nnz = 0;

  for (NodeIdx col = 0; col < G_mat_csc_.num_cols; ++col) {
    G_mat_csc_.col_ptr.push_back(G_mat_csc_.nnz);
    map<GMatLoc, double>::iterator it;
    map<GMatLoc, double>::iterator it_lower
        = G_mat_dok_.values.lower_bound(make_pair(col, 0));
    map<GMatLoc, double>::iterator it_upper
        = G_mat_dok_.values.upper_bound(make_pair(col, G_mat_csc_.num_rows));
    for (it = it_lower; it != it_upper; ++it) {
      G_mat_csc_.values.push_back(it->second);           // push back value
      G_mat_csc_.row_idx.push_back((it->first).second);  // push back row idx
      G_mat_csc_.nnz++;
    }
  }
  G_mat_csc_.col_ptr.push_back(G_mat_csc_.nnz);
  return true;
}
bool GMat::generateACSCMatrix()
{
  A_mat_csc_.num_cols = A_mat_dok_.num_cols;
  A_mat_csc_.num_rows = A_mat_dok_.num_rows;
  A_mat_csc_.nnz = 0;

  for (NodeIdx col = 0; col < A_mat_csc_.num_cols; ++col) {
    A_mat_csc_.col_ptr.push_back(A_mat_csc_.nnz);
    map<GMatLoc, double>::iterator it;
    map<GMatLoc, double>::iterator it_lower
        = A_mat_dok_.values.lower_bound(make_pair(col, 0));
    map<GMatLoc, double>::iterator it_upper
        = A_mat_dok_.values.upper_bound(make_pair(col, A_mat_csc_.num_rows));
    for (it = it_lower; it != it_upper; ++it) {
      A_mat_csc_.values.push_back(it->second);           // push back value
      A_mat_csc_.row_idx.push_back((it->first).second);  // push back row idx
      A_mat_csc_.nnz++;
    }
  }
  A_mat_csc_.col_ptr.push_back(A_mat_csc_.nnz);
  return true;
}

//! Function which returns the value of the conductance row and column in G
// matrix
/*!
 * Directly updates the G matrix
     \param row Row index
     \param col Column index
     \return Conductance value
*/
double GMat::getConductance(NodeIdx row, NodeIdx col)
{
  if (G_mat_dok_.num_cols <= col || G_mat_dok_.num_rows <= row) {
    logger_->error(utl::PSM,
                   51,
                   "Index out of bound for getting G matrix conductance. ",
                   "Ensure object is initialized to the correct size first.");
  }
  GMatLoc key = make_pair(col, row);
  map<GMatLoc, double>::iterator it = G_mat_dok_.values.find(key);
  if (it != G_mat_dok_.values.end()) {
    return it->second;
  }
  return 0;
}

//! Function which modifies the values the values in the G matrix for the
// voltage sources in MNA
/*!
 * Directly updates the G matrix
     \param row Row index
     \param col  Column index
     \param cond  New conductance value
     \return nothing
*/

void GMat::updateConductance(NodeIdx row, NodeIdx col, double cond)
{
  if (G_mat_dok_.num_cols <= col || G_mat_dok_.num_rows <= row) {
    logger_->error(utl::PSM,
                   52,
                   "Index out of bound for getting G matrix conductance. ",
                   "Ensure object is initialized to the correct size first.");
  }
  GMatLoc key = make_pair(col, row);
  G_mat_dok_.values[key] = cond;
  A_mat_dok_.values[key] = 1;
}

//! Function to find the nearest node to a given location in Y direction
/*!
 * Directly updates the G matrix
     \param row Row index
     \param col  Column index
     \param cond  New conductance value
     \return Pointer to the node
*/
Node* GMat::nearestYNode(NodeMap::const_iterator x_itr, int y)
{
  const map<int, Node*>& y_map = x_itr->second;
  const auto y_itr = y_map.lower_bound(y);
  if (y_map.size() == 1) {
    return y_map.begin()->second;
  }
  if (y_itr == y_map.end()) {
    return prev(y_itr)->second;
  }
  if (y_itr == y_map.begin()) {
    return y_itr->second;
  }
  const auto y_prev = prev(y_itr);
  const int dist1 = abs(y_prev->first - y);
  const int dist2 = abs(y_itr->first - y);
  if (dist1 < dist2) {
    return y_prev->second;
  }
  return y_itr->second;
}

//! Function to get conductivity using formula R = rho*l/A
/*!
 * Directly updates the G matrix
     \param width Width of the wire
     \param length Length of the wire
     \param rho  Resistivity of the material of the wire
     \return Conductance
*/
double GMat::getConductivity(double width, double length, double rho)
{
  if (0 >= length || 0 >= width || 0 >= rho) {
    return 0.0;
  }
  return width / (rho * length);
}

//! Function to return a vector which contains pointers to all nodes
vector<Node*> GMat::getAllNodes()
{
  return G_mat_nodes_;
}
}  // namespace psm
