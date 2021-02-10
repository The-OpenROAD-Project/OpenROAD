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

#include <vector>
#include <iostream>
#include "gmat.h"
#include "node.h"

namespace psm {
using std::make_pair;
using std::map;
using std::pair;
using std::vector;

//! Function to return a pointer to the node with a index
/*!
     \param t_node Node index number
     \return Pointer to the node in the matrix
*/
Node* GMat::GetNode(NodeIdx t_node)
{
  if (0 <= t_node && m_n_nodes > t_node) {
    return m_G_mat_nodes[t_node];
  } else {
    return nullptr;
  }
}

//! Function to return a pointer to the node with a index
/*!
     \param t_x x location coordinate
     \param t_y y location coordinate
     \param t_l layer number
     \return Pointer to the node in the matrix
*/
Node* GMat::GetNode(int t_x, int t_y, int t_l, bool t_nearest /*=false*/)
{
  NodeMap& layer_map = m_layer_maps[t_l];
  if (t_l != 1 && t_nearest == false) {
    NodeMap::iterator x_itr = layer_map.find(t_x);
    if (x_itr != layer_map.end()) {
      map<int, Node*>::iterator y_itr = x_itr->second.find(t_y);
      if (y_itr != x_itr->second.end()) {
        return y_itr->second;
      } else {
        m_logger->error(utl::PSM, 46, "Node location lookup error for y.");
      }
    } else {
      m_logger->error(utl::PSM, 47, "Node location lookup error for x.");
    }
  } else {
    NodeMap::iterator        x_itr = layer_map.lower_bound(t_x);
    vector<pair<int, Node*>> node_dist_vector;
    if (layer_map.size() == 1 || x_itr == layer_map.end()
        || x_itr == layer_map.begin()) {
      if (layer_map.size() == 1) {
        x_itr = layer_map.begin();
      } else if (x_itr == layer_map.end()) {
        x_itr = prev(x_itr);
      } else {  // do nothing as x_itr has the correct value
      }
      // cout <<"Added current source" <<endl;
      return NearestYNode(x_itr, t_y);
      // cout <<"Added current source" <<endl;

    } else {
      NodeMap::iterator x_prev;
      x_prev            = prev(x_itr);
      Node*   node1     = NearestYNode(x_itr, t_y);
      Node*   node2     = NearestYNode(x_prev, t_y);
      NodeLoc node1_loc = node1->GetLoc();
      NodeLoc node2_loc = node2->GetLoc();
      int     dist1 = abs(node1_loc.first - t_x) + abs(node1_loc.second - t_y);
      int     dist2 = abs(node2_loc.first - t_x) + abs(node2_loc.second - t_y);
      if (dist1 < dist2) {
        return node1;
      } else {
        return node2;
      }
    }
  }
}

//! Function to add a node to the matrix
/*!
 * Directly updates the G matrix
     \param t_node Node index
     \param t_node Node pointer
     \param t_l layer number
     \return nothing
*/
void GMat::SetNode(NodeIdx t_node_loc, Node* t_node)
{
  if (0 <= t_node_loc && m_n_nodes > t_node_loc) {
    t_node->SetGLoc(t_node_loc);
    m_G_mat_nodes[t_node_loc] = t_node;
  }
}

//! Function to add a node to the matrix
/*!
 * Directly updates the G node vector
     \param t_node Location to insert the node
     \return nothing
*/
void GMat::InsertNode(Node* t_node)
{
  t_node->SetGLoc(m_n_nodes);
  int      layer                           = t_node->GetLayerNum();
  NodeLoc  nodeLoc                         = t_node->GetLoc();
  NodeMap& layer_map                       = m_layer_maps[layer];
  layer_map[nodeLoc.first][nodeLoc.second] = t_node;
  m_G_mat_nodes.push_back(t_node);
  m_n_nodes++;
}

//! Function to create a node
/*!
     \param t_x Node index
     \param t_y Node pointer
     \param t_l layer number
     \param t_bBox layer number
     \return Pointer to the created node
*/
Node* GMat::SetNode(int t_x, int t_y, int t_layer, BBox t_bBox)
{
  NodeMap& layer_map = m_layer_maps[t_layer];
  if (layer_map.empty()) {
    Node* node = new Node();
    node->SetLoc(t_x, t_y, t_layer);
    node->UpdateMaxBbox(t_bBox.first, t_bBox.second);
    InsertNode(node);
    return (node);
  }
  NodeMap::iterator x_itr = layer_map.find(t_x);
  if (x_itr != layer_map.end()) {
    map<int, Node*>::iterator y_itr = x_itr->second.find(t_y);
    if (y_itr != x_itr->second.end()) {
      Node* node = y_itr->second;
      node->UpdateMaxBbox(t_bBox.first, t_bBox.second);
      return (node);
    } else {
      Node* node = new Node();
      node->SetLoc(t_x, t_y, t_layer);
      node->UpdateMaxBbox(t_bBox.first, t_bBox.second);
      InsertNode(node);
      return (node);
    }

  } else {
    Node* node = new Node();
    node->SetLoc(t_x, t_y, t_layer);
    node->UpdateMaxBbox(t_bBox.first, t_bBox.second);
    InsertNode(node);
    return (node);
  }
}

//! Function to print the G matrix
void GMat::Print()
{
  m_logger->info(utl::PSM, 48, "Printing GMat obj, with {} nodes.", m_n_nodes);
  for (NodeIdx i = 0; i < m_n_nodes; i++) {
    Node* node_ptr = m_G_mat_nodes[i];
    if (node_ptr != nullptr) {
      node_ptr->Print(m_logger);
    }
  }
}

//! Function to set conductance values in the G matrix
/*!
 * Directly updates the G matrix
     \param t_node1 Node pointer 1
     \param t_node2 Node pointer 2
     \param t_cond conductance value to be added between node 1 and 2
     \return nothing
*/
void GMat::SetConductance(Node* t_node1, Node* t_node2, double t_cond)
{
  NodeIdx node1_r     = t_node1->GetGLoc();
  NodeIdx node2_r     = t_node2->GetGLoc();
  double  node11_cond = GetConductance(node1_r, node1_r);
  double  node22_cond = GetConductance(node2_r, node2_r);
  double  node12_cond = GetConductance(node1_r, node2_r);
  double  node21_cond = GetConductance(node2_r, node1_r);
  UpdateConductance(node1_r, node1_r, node11_cond + t_cond);
  UpdateConductance(node2_r, node2_r, node22_cond + t_cond);
  UpdateConductance(node1_r, node2_r, node12_cond - t_cond);
  UpdateConductance(node2_r, node1_r, node21_cond - t_cond);
}

//! Function to initialize the Dictionary of keys (DOK) matrix
/*! Based on the size of the G matrix
 * initialize the number of rows and columns
 */
void GMat::InitializeGmatDok(int t_numC4)
{
  if (m_n_nodes <= 0) {
    m_logger->error(utl::PSM, 49, "No nodes in object initialization stopped.");
  } else {
    m_G_mat_dok.num_cols = m_n_nodes + t_numC4;
    m_G_mat_dok.num_rows = m_n_nodes + t_numC4;
    m_A_mat_dok.num_cols = m_n_nodes + t_numC4;
    m_A_mat_dok.num_rows = m_n_nodes + t_numC4;
  }
}

//! Function that returns the number of nodes in the G matrix
NodeIdx GMat::GetNumNodes()
{  // debug
  return m_n_nodes;
}

//! Function to return a pointer to the G matrix in CSC format
CscMatrix* GMat::GetGMat()
{  // Nodes debug
  return &m_G_mat_csc;
}

//! Function to return a pointer to the A matrix in CSC format
CscMatrix* GMat::GetAMat()
{  // Nodes debug
  return &m_A_mat_csc;
}

//! Function to return a pointer to the G matrix in DOK format
DokMatrix* GMat::GetGMatDOK()
{  // Nodes debug
  return &m_G_mat_dok;
}

//! Function that gets the value of the conductance of the stripe and
// updates the G matrix
/*!
 * Directly updates the G matrix
     \param t_l Layer number
     \param layer_dir  Direction of the layer
     \param t_x_min Lower left x location
     \param t_x_max Upper right x location
     \param t_y_min Lower left y location
     \param t_y_max Upper right y location
     \return nothing
*/
void GMat::GenerateStripeConductance(int                        t_l,
                                     odb::dbTechLayerDir::Value layer_dir,
                                     int                        t_x_min,
                                     int                        t_x_max,
                                     int                        t_y_min,
                                     int                        t_y_max,
                                     double                     t_rho)
{
  NodeMap& layer_map = m_layer_maps[t_l];
  if (t_x_min > t_x_max || t_y_min > t_y_max)
    m_logger->warn(utl::PSM,
                   50,
                   "Creating stripe condunctance with invalid inputs. Min and "
                   "max values for X or Y are interchanged.");
  if (layer_dir == odb::dbTechLayerDir::Value::HORIZONTAL) {
    NodeMap::iterator x_itr;
    NodeMap::iterator x_prev;
    // int               y_loc = (t_y_min + t_y_max) / 2;
    int i = 0;
    for (x_itr = layer_map.lower_bound(t_x_min);
         x_itr != layer_map.upper_bound(t_x_max);
         ++x_itr) {
      map<int, Node*>::iterator y_itr = (x_itr->second).lower_bound(t_y_min);
      if (y_itr == (x_itr->second).end())
        continue;
      else if (y_itr->first < t_y_min || y_itr->first > t_y_max)
        continue;
      // if ((x_itr->second).find(y_loc) == (x_itr->second).end()) {
      //  continue;
      //}
      if (i == 0) {
        i = 1;
      } else {
        y_itr       = (x_itr->second).lower_bound(t_y_min);
        Node* node1 = y_itr->second;
        y_itr       = (x_prev->second).lower_bound(t_y_min);
        Node* node2 = y_itr->second;
        // Node* node1 = (x_itr->second).at(y_loc);
        // Node*  node2  = (x_prev->second).at(y_loc);
        int    width  = t_y_max - t_y_min;
        int    length = x_itr->first - x_prev->first;
        double cond   = GetConductivity(width, length, t_rho);
        SetConductance(node1, node2, cond);
      }
      x_prev = x_itr;
    }
  } else {
    // int                       x_loc = (t_x_min + t_x_max) / 2;

    map<pair<int,int>, Node*> y_map;
    for (auto x_itr = layer_map.lower_bound(t_x_min);
         x_itr != layer_map.upper_bound(t_x_max);
         ++x_itr) {
      map<int, Node*> y_itr_map = x_itr->second;
      map<int, Node*>::iterator y_map_itr;
      for (y_map_itr = y_itr_map.lower_bound(t_y_min);
         y_map_itr->first <= t_y_max && y_map_itr != y_itr_map.end();
         ++y_map_itr)
        y_map.insert(make_pair(make_pair(y_map_itr->first,x_itr->first),y_map_itr->second));
    }

    // map<int, Node*>           y_map = layer_map.at(x_loc);
    map<pair<int,int>, Node*>::iterator y_itr;
    map<pair<int,int>, Node*>::iterator y_prev;
    int                       i = 0;
    for (y_itr = y_map.begin();
         y_itr != y_map.end();
         ++y_itr) {
      if (i == 0) {
        i = 1;
      } else {
        Node*  node1  = y_itr->second;
        Node*  node2  = y_prev->second;
        NodeLoc node_loc1      = node1->GetLoc();
        NodeLoc node_loc2      = node2->GetLoc();
        int    width  = t_x_max - t_x_min;
        int    length = (y_itr->first).first - (y_prev->first).first;
        if(length == 0)
            length = (y_itr->first).second - (y_prev->first).second;
        double cond   = GetConductivity(width, length, t_rho);
        SetConductance(node1, node2, cond);
      }
      y_prev = y_itr;
    }
  }
}
//! Function that gets the locations of nodes to connect RDL vias
/*!
 * Directly updates the G matrix
     \param t_l Layer number
     \param layer_dir  Direction of the layer
     \param t_x_min Lower left x location
     \param t_x_max Upper right x location
     \param t_y_min Lower left y location
     \param t_y_max Upper right y location
     \return std::vector<Node*>
*/
vector<Node*> GMat::GetRDLNodes(int                        t_l,
                                odb::dbTechLayerDir::Value layer_dir,
                                int                        t_x_min,
                                int                        t_x_max,
                                int                        t_y_min,
                                int                        t_y_max)
{
  vector<Node*> RDLNodes;
  NodeMap&      layer_map = m_layer_maps[t_l];
  NodeLoc       node_loc;
  Node*         node1;
  Node*         node2;
  if (layer_dir == odb::dbTechLayerDir::Value::HORIZONTAL) {
    int y_loc                       = (t_y_min + t_y_max) / 2;
    node1                           = GetNode(t_x_min, y_loc, t_l, true);
    node_loc                        = node1->GetLoc();
    int x1                          = node_loc.first;
    node2                           = GetNode(t_x_max, y_loc, t_l, true);
    node_loc                        = node2->GetLoc();
    int                       x2    = node_loc.first;
    map<int, Node*>           y_map = layer_map.at(x1);
    map<int, Node*>::iterator y_itr;
    for (y_itr = y_map.lower_bound(t_y_min);
         y_itr->first <= t_y_max && y_itr != y_map.end();
         ++y_itr) {
      node1 = y_itr->second;
      RDLNodes.push_back(node1);
    }
    y_map = layer_map.at(x2);
    for (y_itr = y_map.lower_bound(t_y_min);
         y_itr->first <= t_y_max && y_itr != y_map.end();
         ++y_itr) {
      node2 = y_itr->second;
      RDLNodes.push_back(node2);
    }
  } else {
    //std::cout << "Vertical " << std::endl;
    int x_loc  = (t_x_min + t_x_max) / 2;
    node1      = GetNode(x_loc, t_y_min, t_l, true);
    node_loc   = node1->GetLoc();
    int y1     = node_loc.second;
    //std::cout <<"N1 " << node_loc.first << " "<<node_loc.second << std::endl;
    node2      = GetNode(x_loc, t_y_max, t_l, true);
    node_loc   = node2->GetLoc();
    int y2     = node_loc.second;
    NodeMap::iterator x_itr;
    for (x_itr = layer_map.lower_bound(t_x_min);
         x_itr->first <= t_x_max && x_itr != layer_map.end();
         ++x_itr) {
      map<int, Node*>::iterator y_iter;
      y_iter = (x_itr->second).find(y1);
      if(y_iter != (x_itr->second).end()) {
        RDLNodes.push_back(y_iter->second);
      }
      y_iter = (x_itr->second).find(y2);
      if(y_iter != (x_itr->second).end()) {
        RDLNodes.push_back(y_iter->second);
      }
      //node1 = (x_itr->second).at(y1);
      //node2 = (x_itr->second).at(y2);
      //RDLNodes.push_back(node1);
      //RDLNodes.push_back(node2);
    }
  }
  return RDLNodes;
}

//! Function which add the values in the G matrix for the
// voltage sources in MNA
/*!
 * Directly updates the G matrix
     \param t_loc Location of the C4 bump
     \param t_C4Num  C4 bump number
     \return nothing
*/
void GMat::AddC4Bump(int t_loc, int t_C4Num)
{
  UpdateConductance(t_loc, t_C4Num + m_n_nodes, 1);
  UpdateConductance(t_C4Num + m_n_nodes, t_loc, 1);
}

//! Function which converts the DOK matrix into CSC format in a sparse method
/*!
 */
bool GMat::GenerateCSCMatrix()
{
  m_G_mat_csc.num_cols = m_G_mat_dok.num_cols;
  m_G_mat_csc.num_rows = m_G_mat_dok.num_rows;
  m_G_mat_csc.nnz      = 0;

  for (NodeIdx col = 0; col < m_G_mat_csc.num_cols; ++col) {
    m_G_mat_csc.col_ptr.push_back(m_G_mat_csc.nnz);
    map<GMatLoc, double>::iterator it;
    map<GMatLoc, double>::iterator it_lower
        = m_G_mat_dok.values.lower_bound(make_pair(col, 0));
    map<GMatLoc, double>::iterator it_upper
        = m_G_mat_dok.values.upper_bound(make_pair(col, m_G_mat_csc.num_rows));
    for (it = it_lower; it != it_upper; ++it) {
      m_G_mat_csc.values.push_back(it->second);           // push back value
      m_G_mat_csc.row_idx.push_back((it->first).second);  // push back row idx
      m_G_mat_csc.nnz++;
    }
  }
  m_G_mat_csc.col_ptr.push_back(m_G_mat_csc.nnz);
  return true;
}
bool GMat::GenerateACSCMatrix()
{
  m_A_mat_csc.num_cols = m_A_mat_dok.num_cols;
  m_A_mat_csc.num_rows = m_A_mat_dok.num_rows;
  m_A_mat_csc.nnz      = 0;

  for (NodeIdx col = 0; col < m_A_mat_csc.num_cols; ++col) {
    m_A_mat_csc.col_ptr.push_back(m_A_mat_csc.nnz);
    map<GMatLoc, double>::iterator it;
    map<GMatLoc, double>::iterator it_lower
        = m_A_mat_dok.values.lower_bound(make_pair(col, 0));
    map<GMatLoc, double>::iterator it_upper
        = m_A_mat_dok.values.upper_bound(make_pair(col, m_A_mat_csc.num_rows));
    for (it = it_lower; it != it_upper; ++it) {
      m_A_mat_csc.values.push_back(it->second);           // push back value
      m_A_mat_csc.row_idx.push_back((it->first).second);  // push back row idx
      m_A_mat_csc.nnz++;
    }
  }
  m_A_mat_csc.col_ptr.push_back(m_A_mat_csc.nnz);
  return true;
}

//! Function which returns the value of the conductance row and column in G
// matrix
/*!
 * Directly updates the G matrix
     \param t_row Row index
     \param t_col Column index
     \return Conductance value
*/
double GMat::GetConductance(NodeIdx t_row, NodeIdx t_col)
{
  if (m_G_mat_dok.num_cols <= t_col || m_G_mat_dok.num_rows <= t_row) {
    m_logger->error(utl::PSM,
                    51,
                    "Index out of bound for getting G matrix conductance. \n",
                    "Ensure object is initialized to the correct size first.");
  }
  GMatLoc                        key = make_pair(t_col, t_row);
  map<GMatLoc, double>::iterator it  = m_G_mat_dok.values.find(key);
  if (it != m_G_mat_dok.values.end()) {
    return it->second;
  } else {
    return 0;
  }
}

//! Function which modifies the values the values in the G matrix for the
// voltage sources in MNA
/*!
 * Directly updates the G matrix
     \param t_row Row index
     \param t_col  Column index
     \param t_cond  New conductance value
     \return nothing
*/

void GMat::UpdateConductance(NodeIdx t_row, NodeIdx t_col, double t_cond)
{
  if (m_G_mat_dok.num_cols <= t_col || m_G_mat_dok.num_rows <= t_row) {
    m_logger->error(utl::PSM,
                    52,
                    "Index out of bound for getting G matrix conductance. \n",
                    "Ensure object is initialized to the correct size first.");
  }
  GMatLoc key             = make_pair(t_col, t_row);
  m_G_mat_dok.values[key] = t_cond;
  m_A_mat_dok.values[key] = 1;
}

//! Function to find the nearest node to a given location in Y direction
/*!
 * Directly updates the G matrix
     \param t_row Row index
     \param t_col  Column index
     \param t_cond  New conductance value
     \return Pointer to the node
*/
Node* GMat::NearestYNode(NodeMap::iterator x_itr, int t_y)
{
  map<int, Node*>&          y_map = x_itr->second;
  map<int, Node*>::iterator y_itr = y_map.lower_bound(t_y);
  // if(t_y ==67200){
  // cout<<"Ymap"<<endl;
  // for(auto it = y_map.begin(); it != y_map.end(); ++it)
  //{
  //  std::cout << it->first<<"  ";
  //}
  // cout<<endl;
  //}
  if (y_map.size() == 1 || y_itr == y_map.end() || y_itr == y_map.begin()) {
    if (y_map.size() == 1) {
      y_itr = y_map.begin();
    } else if (y_itr == y_map.end()) {
      y_itr = prev(y_itr);
    } else {
    }
    return y_itr->second;
  } else {
    map<int, Node*>::iterator y_prev;
    y_prev    = prev(y_itr);
    int dist1 = abs(y_prev->first - t_y);
    int dist2 = abs(y_itr->first - t_y);
    if (dist1 < dist2) {
      return y_prev->second;
    } else {
      return y_itr->second;
    }
  }
}

//! Function to get conductivity using formula R = rho*l/A
/*!
 * Directly updates the G matrix
     \param width Width of the wire
     \param length Length of the wire
     \param rho  Resistivity of the material of the wire
     \return Conductance
*/
double GMat::GetConductivity(double width, double length, double rho)
{
  if (0 >= length || 0 >= width || 0 >= rho) {
    return 0.0;
  } else {
    return width / (rho * length);
  }
}

//! Function to return a vector which contains pointers to all nodes
vector<Node*> GMat::GetAllNodes()
{
  return m_G_mat_nodes;
}
}  // namespace psm
