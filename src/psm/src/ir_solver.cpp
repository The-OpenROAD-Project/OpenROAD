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
#include <queue>
#include <math.h>
#include <cmath>
#include <iostream>
#include <stdlib.h>
#include <map>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <time.h>
#include <sstream>
#include <iterator>
#include <string>

#include <Eigen/Sparse>
#include <Eigen/SparseLU>
#include "odb/db.h"
#include "get_voltage.h"
#include "ir_solver.h"
#include "node.h"
#include "gmat.h"
#include "get_power.h"

namespace psm {
using odb::dbBlock;
using odb::dbBox;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbNet;
using odb::dbSBox;
using odb::dbSet;
using odb::dbSigType;
using odb::dbSWire;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbVia;
using odb::dbViaParams;

using std::endl;
using std::get;
using std::ifstream;
using std::make_pair;
using std::make_tuple;
using std::map;
using std::ofstream;
using std::pair;
using std::queue;
using std::setprecision;
using std::stod;
using std::string;
using std::stringstream;
using std::to_string;
using std::tuple;
using std::vector;

using Eigen::Map;
using Eigen::SparseLU;
using Eigen::SparseMatrix;
using Eigen::Success;
using Eigen::VectorXd;

//! Returns the created G matrix for the design
/*
 * \return G Matrix
 */
GMat* IRSolver::GetGMat()
{
  return m_Gmat;
}

//! Returns current map represented as a 1D vector
/*
 * \return J vector
 */
vector<double> IRSolver::GetJ()
{
  return m_J;
}

//! Function to solve for voltage using SparseLU
void IRSolver::SolveIR()
{
  if (!m_connection) {
    m_logger->warn(utl::PSM,
                   8,
                   "Powergrid is not connected to all instances, therefore the "
                   "IR Solver may not be accurate. LVS may also fail.");
  }
  int        unit_micron = (m_db->getTech())->getDbUnitsPerMicron();
  CscMatrix* Gmat = m_Gmat->GetGMat();
  // fill A
  double*                   values  = &(Gmat->values[0]);
  int*                      row_idx = &(Gmat->row_idx[0]);
  int*                      col_ptr = &(Gmat->col_ptr[0]);
  Map<SparseMatrix<double>> A(Gmat->num_rows,
                              Gmat->num_cols,
                              Gmat->nnz,
                              col_ptr,  // read-write
                              row_idx,
                              values);

  vector<double>                 J = GetJ();
  Map<VectorXd>                  b(J.data(), J.size());
  VectorXd                       x;
  SparseLU<SparseMatrix<double>> solver;
  debugPrint(m_logger, utl::PSM, "IR Solver", 1, "Factorizing the G matrix");
  solver.compute(A);
  if (solver.info() != Success) {
    // decomposition failed
    m_logger->error(utl::PSM, 10, "LU factorization of the G Matrix failed.");
  }
  debugPrint(m_logger, utl::PSM, "IR Solver", 1, "Solving system of equations GV=J");
  x = solver.solve(b);
  if (solver.info() != Success) {
    // solving failed
    m_logger->error(utl::PSM, 12, "Solving V = inv(G)*J failed.");
  } else {
    debugPrint(m_logger, utl::PSM, "IR Solver", 1, "Solving system of equations GV=J complete");
  }
  ofstream ir_report;
  ir_report.open(m_out_file);
  ir_report << "Instance name, "
            << " X location, "
            << " Y location, "
            << " Voltage "
            << "\n";
  int    num_nodes = m_Gmat->GetNumNodes();
  int    node_num  = 0;
  double sum_volt  = 0;
  wc_voltage       = supply_voltage_src;
  while (node_num < num_nodes) {
    Node*  node = m_Gmat->GetNode(node_num);
    double volt = x(node_num);
    sum_volt    = sum_volt + volt;
    if (m_power_net_type == dbSigType::POWER) {
      if (volt < wc_voltage) {
        wc_voltage = volt;
      }
    } else {
      if (volt > wc_voltage) {
        wc_voltage = volt;
      }
    }
    node->SetVoltage(volt);
    node_num++;
    if (node->HasInstances()) {
      NodeLoc         node_loc = node->GetLoc();
      float           loc_x = ((float) node_loc.first) / ((float) unit_micron);
      float           loc_y = ((float) node_loc.second) / ((float) unit_micron);
      vector<dbInst*> insts = node->GetInstances();
      vector<dbInst*>::iterator inst_it;
      if (m_out_file != "") {
        for (inst_it = insts.begin(); inst_it != insts.end(); inst_it++) {
          ir_report << (*inst_it)->getName() << ", " << loc_x << ", " << loc_y
                    << ", " << setprecision(6) << volt << "\n";
        }
      }
    }
  }
  ir_report << endl;
  ir_report.close();
  avg_voltage = sum_volt / num_nodes;
  if (m_em_flag == 1) {
    map<GMatLoc, double>::iterator it;
    DokMatrix*                     Gmat_dok          = m_Gmat->GetGMatDOK();
    int                            resistance_number = 0;
    max_cur                                          = 0;
    double   sum_cur                                 = 0;
    ofstream em_report;
    if (m_em_out_file != "") {
      em_report.open(m_em_out_file);
      em_report << "Segment name, "
                << " Current, "
                << " Node 1, "
                << " Node 2 "
                << "\n";
    }
    NodeLoc node_loc;
    for (it = Gmat_dok->values.begin(); it != Gmat_dok->values.end(); it++) {
      NodeIdx col = (it->first).first;
      NodeIdx row = (it->first).second;
      if (col <= row) {
        continue;  // ignore lower half and diagonal as matrix is symmetric
      }
      double cond = it->second;  // get cond value
      if (abs(cond) < 1e-15) {   // ignore if an empty cell
        continue;
      }
      string net_name = m_power_net;
      if (col < num_nodes) {  // resistances
        double resistance = -1 / cond;

        Node* node1       = m_Gmat->GetNode(col);
        Node* node2       = m_Gmat->GetNode(row);
        node_loc          = node1->GetLoc();
        int    x1         = node_loc.first;
        int    y1         = node_loc.second;
        int    l1         = node1->GetLayerNum();
        string node1_name = net_name + "_" + to_string(x1) + "_" + to_string(y1)
                            + "_" + to_string(l1);

        node_loc          = node2->GetLoc();
        int    x2         = node_loc.first;
        int    y2         = node_loc.second;
        int    l2         = node2->GetLayerNum();
        string node2_name = net_name + "_" + to_string(x2) + "_" + to_string(y2)
                            + "_" + to_string(l2);

        string segment_name = "seg_" + to_string(resistance_number);

        double v1 = node1->GetVoltage();
        double v2 = node2->GetVoltage();
        double seg_cur;
        seg_cur = (v1 - v2) / resistance;
        sum_cur += abs(seg_cur);
        if (m_em_out_file != "") {
          em_report << segment_name << ", " << setprecision(3) << seg_cur
                    << ", " << node1_name << ", " << node2_name << endl;
        }
        seg_cur = abs(seg_cur);
        if (seg_cur > max_cur) {
          max_cur = seg_cur;
        }
        resistance_number++;
      }
    }  // for gmat values
    avg_cur = sum_cur / resistance_number;
    num_res = resistance_number;

  }  // enable em
}

//! Function to add C4 bumps to the G matrix
bool IRSolver::AddC4Bump()
{
  if (m_C4Bumps.size() == 0) {
    m_logger->error(utl::PSM, 14, "Number of voltage sources cannot be 0.");
  }
  m_logger->info(utl::PSM, 64, "Number of voltage sources = {}.", m_C4Bumps.size());
  for (size_t it = 0; it < m_C4Nodes.size(); ++it) {
    NodeIdx node_loc      = m_C4Nodes[it].first;
    double  voltage_value = m_C4Nodes[it].second;
    m_Gmat->AddC4Bump(node_loc, it);  // add the 0th bump
    m_J.push_back(voltage_value);     // push back first vdd
  }
  return true;
}

//! Function that parses the Vsrc file
void IRSolver::ReadC4Data()
{
  int unit_micron = (m_db->getTech())->getDbUnitsPerMicron();
  if (m_vsrc_file != "") {
    m_logger->info(utl::PSM,
                   15,
                   "Reading location of VDD and VSS sources from {}.",
                   m_vsrc_file);
    ifstream file(m_vsrc_file);
    string   line = "";
    // Iterate through each line and split the content using delimiter
    while (getline(file, line)) {
      tuple<int, int, int, double> c4_bump;
      int                          first, second, size;
      stringstream                 X(line);
      string                       val;
      for (int i = 0; i < 4; ++i) {
        getline(X, val, ',');
        if (i == 0) {
          first = (int) (unit_micron * stod(val));
        } else if (i == 1) {
          second = (int) (unit_micron * stod(val));
        } else if (i == 2) {
          size = (int) (unit_micron * stod(val));
        } else {
          supply_voltage_src = stod(val);
        }
      }
      m_C4Bumps.push_back(make_tuple(first, second, size, supply_voltage_src));
    }
    file.close();
  } else {
    m_logger->warn(utl::PSM,
                   16,
                   "Voltage pad location (VSRC) file not specified, defaulting "
                   "pad location to checkerboard pattern on core area.");
    dbChip*   chip  = m_db->getChip();
    dbBlock*  block = chip->getBlock();
    odb::Rect coreRect;
    block->getCoreArea(coreRect);
    int       coreW = coreRect.xMax() - coreRect.xMin();
    int       coreL = coreRect.yMax() - coreRect.yMin();
    odb::Rect dieRect;
    block->getDieArea(dieRect);
    int offset_x = coreRect.xMin() - dieRect.xMin();
    int offset_y = coreRect.yMin() - dieRect.yMin();
    if (m_bump_pitch_x == 0) {
      m_bump_pitch_x = m_bump_pitch_default * unit_micron;
      m_logger->warn(
          utl::PSM,
          17,
          "X direction bump pitch is not specified, defaulting to {}um.",
          m_bump_pitch_default);
    }
    if (m_bump_pitch_y == 0) {
      m_bump_pitch_y = m_bump_pitch_default * unit_micron;
      m_logger->warn(
          utl::PSM,
          18,
          "Y direction bump pitch is not specified, defaulting to {}um.",
          m_bump_pitch_default);
    }
    if (!m_net_voltage_map.empty()
        && m_net_voltage_map.count(m_power_net) > 0) {
      supply_voltage_src = m_net_voltage_map.at(m_power_net);
    } else {
      m_logger->warn(utl::PSM,
                     19,
                     "Voltage on net {} is not explicitly set.",
                     m_power_net);
      pair<double, double> supply_voltages = GetSupplyVoltage();
      dbNet*               power_net       = block->findNet(m_power_net.data());
      if (power_net == NULL) {
        m_logger->error(utl::PSM,
                        20,
                        "Cannot find net {} in the design. Please provide a "
                        "valid VDD/VSS net.",
                        m_power_net);
      }
      m_power_net_type = power_net->getSigType();
      if (m_power_net_type == dbSigType::GROUND) {
        supply_voltage_src = supply_voltages.second;
        m_logger->warn(utl::PSM,
                       21,
                       "Using voltage {:4.3f}V for ground network.",
                       supply_voltage_src);
      } else {
        supply_voltage_src = supply_voltages.first;
        m_logger->warn(utl::PSM,
                       22,
                       "Using voltage {:4.3f}V for VDD network.",
                       supply_voltage_src);
      }
    }
    int x_cor, y_cor;
    if (coreW < m_bump_pitch_x || coreL < m_bump_pitch_y) {
      float to_micron = 1.0f/unit_micron;
      m_logger->warn(utl::PSM,
                     63,
                     "Specified bump pitches of {:4.3f} and {:4.3f} are less "
                     "than core width of {:4.3f} or core height of {:4.3f}. "
                     "Changing bump location to the center of the die at "
                     "({:4.3f}, {:4.3f}).",
                     m_bump_pitch_x * to_micron, m_bump_pitch_y * to_micron,
                     coreW * to_micron, coreL * to_micron,
                     (coreW * to_micron)/2,
                     (coreL * to_micron)/2);
      x_cor = coreW/2;
      y_cor = coreL/2;
      m_C4Bumps.push_back(make_tuple(x_cor, y_cor, m_bump_size * unit_micron, supply_voltage_src));
    }
    int num_b_x = coreW / m_bump_pitch_x;
    int num_b_y = coreL / m_bump_pitch_y;
    m_logger->warn(utl::PSM,
                   65,
                   "VSRC location not specified, using default checkerboard "
                   "pattern with one VDD every size bumps in x-direction and "
                   "one in two bumps in the y-direction");
    for (int i = 0; i < num_b_y; i++) {
      for (int j = 0; j < num_b_x; j=j+6) {
        x_cor = (m_bump_pitch_x * j) + (((2 * i) % 6) * m_bump_pitch_x)
                + offset_x;
        y_cor = (m_bump_pitch_y * i) + offset_y;
        if (x_cor <= coreW && y_cor <= coreL) {
          m_C4Bumps.push_back(make_tuple(
              x_cor, y_cor, m_bump_size * unit_micron, supply_voltage_src));
        }
      }
    }
  }
}

//! Function to create a J vector from the current map
bool IRSolver::CreateJ()
{  // take current_map as an input?
  int num_nodes = m_Gmat->GetNumNodes();
  m_J.resize(num_nodes, 0);

  vector<pair<string, double>> power_report = GetPower();
  dbChip*                      chip         = m_db->getChip();
  dbBlock*                     block        = chip->getBlock();
  for (vector<pair<string, double>>::iterator it = power_report.begin();
       it != power_report.end();
       ++it) {
    dbInst* inst = block->findInst(it->first.c_str());
    if (inst == NULL) {
      m_logger->warn(
          utl::PSM, 23, "Instance {} not found in the database.", it->first);
      continue;
    }
    int x, y;
    inst->getLocation(x, y);
    // cout << "Got location" <<endl;
    int     l      = m_bottom_layer;  // atach to the bottom most routing layer
    Node*   node_J = m_Gmat->GetNode(x, y, l, true);
    NodeLoc node_loc = node_J->GetLoc();
    if (abs(node_loc.first - x) > m_node_density
        || abs(node_loc.second - y) > m_node_density) {
      m_logger->warn(utl::PSM,
                     24,
                     "Instance {}, current node at ({}, {}) at layer {} have "
                     "been moved from ({}, {}).",
                     it->first,
                     node_loc.first,
                     node_loc.second,
                     l,
                     x,
                     y);
    }
    // Both these lines will change in the future for multiple power domains
    node_J->AddCurrentSrc(it->second);
    node_J->AddInstance(inst);
  }
  for (int i = 0; i < num_nodes; ++i) {
    Node* node_J = m_Gmat->GetNode(i);
    if (m_power_net_type == dbSigType::GROUND) {
      m_J[i] = (node_J->GetCurrent());
    } else {
      m_J[i] = -1 * (node_J->GetCurrent());
    }
  }
  debugPrint(m_logger, utl::PSM, "IR Solver", 1, "Created J vector");
  //m_logger->info(utl::PSM, 25, "Created J vector.");
  return true;
}

//! Function to create a G matrix using the nodes
bool IRSolver::CreateGmat(bool connection_only)
{
  debugPrint(m_logger, utl::PSM, "G Matrix", 1, "Creating G matrix");
  vector<Node*> node_vector;
  dbTech*       tech = m_db->getTech();
  // dbSet<dbTechLayer>           layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator litr;
  int                          unit_micron = tech->getDbUnitsPerMicron();
  int num_routing_layers                   = tech->getRoutingLayerCount();

  m_Gmat                 = new GMat(num_routing_layers, m_logger);
  dbChip*      chip      = m_db->getChip();
  dbBlock*     block     = chip->getBlock();
  dbNet*       power_net = block->findNet(m_power_net.data());
  if (power_net == NULL) {
    m_logger->error(utl::PSM,
                    27,
                    "Cannot find net {} in the design. Please provide a valid "
                    "VDD/VSS net.",
                    m_power_net);
  }
  m_power_net_type = power_net->getSigType();
  vector<dbNet*> power_nets;
  int            num_wires = 0;
  debugPrint(m_logger, utl::PSM, "G Matrix", 1, "Extracting power stripes on net {}", power_net->getName());
  power_nets.push_back(power_net);

  if (power_nets.size() == 0) {
    m_logger->error(
        utl::PSM,
        29,
        "No power stripes found in design. Power grid checker will not run.");
  }
  vector<dbNet*>::iterator vIter;
  for (vIter = power_nets.begin(); vIter != power_nets.end(); ++vIter) {
    dbNet*                   curDnet = *vIter;
    dbSet<dbSWire>           swires  = curDnet->getSWires();
    dbSet<dbSWire>::iterator sIter;
    for (sIter = swires.begin(); sIter != swires.end(); ++sIter) {
      dbSWire*                curSWire = *sIter;
      dbSet<dbSBox>           wires    = curSWire->getWires();
      dbSet<dbSBox>::iterator wIter;
      for (wIter = wires.begin(); wIter != wires.end(); ++wIter) {
        num_wires++;
        dbSBox*               curWire = *wIter;
        int                   l;
        dbTechLayerDir::Value layer_dir;
        if (curWire->isVia()) {
          dbVia*       via       = curWire->getBlockVia();
          dbTechLayer* via_layer = via->getTopLayer();
          l                      = via_layer->getRoutingLevel();
          layer_dir              = via_layer->getDirection();
        } else {
          dbTechLayer* wire_layer = curWire->getTechLayer();
          l                       = wire_layer->getRoutingLevel();
          layer_dir               = wire_layer->getDirection();
          if (l < m_bottom_layer) {
            m_bottom_layer     = l;
            m_bottom_layer_dir = layer_dir;
          }
        }
        if (l > m_top_layer) {
          m_top_layer     = l;
          m_top_layer_dir = layer_dir;
        }
      }
    }
  }
  for (vIter = power_nets.begin(); vIter != power_nets.end(); ++vIter) {
    dbNet*                   curDnet = *vIter;
    dbSet<dbSWire>           swires  = curDnet->getSWires();
    dbSet<dbSWire>::iterator sIter;
    for (sIter = swires.begin(); sIter != swires.end(); ++sIter) {
      dbSWire*                curSWire = *sIter;
      dbSet<dbSBox>           wires    = curSWire->getWires();
      dbSet<dbSBox>::iterator wIter;
      for (wIter = wires.begin(); wIter != wires.end(); ++wIter) {
        dbSBox* curWire = *wIter;
        if (curWire->isVia()) {
          dbVia* via                = curWire->getBlockVia();
          dbBox* via_bBox           = via->getBBox();
          int    check_params       = via->hasParams();
          int    x_cut_size         = 0;
          int    y_cut_size         = 0;
          int    x_bottom_enclosure = 0;
          int    y_bottom_enclosure = 0;
          int    x_top_enclosure    = 0;
          int    y_top_enclosure    = 0;
          if (check_params == 1) {
            dbViaParams params;
            via->getViaParams(params);
            x_cut_size         = params.getXCutSize();
            y_cut_size         = params.getYCutSize();
            x_bottom_enclosure = params.getXBottomEnclosure();
            y_bottom_enclosure = params.getYBottomEnclosure();
            x_top_enclosure    = params.getXTopEnclosure();
            y_top_enclosure    = params.getYTopEnclosure();
          }
          BBox bBox
              = make_pair((via_bBox->getDX()) / 2, (via_bBox->getDY()) / 2);
          int x, y;
          curWire->getViaXY(x, y);
          dbTechLayer*          via_layer = via->getBottomLayer();
          dbTechLayerDir::Value layer_dir = via_layer->getDirection();
          int                   l         = via_layer->getRoutingLevel();
          int                   x_loc1, x_loc2, y_loc1, y_loc2;
          if (m_bottom_layer != l
              && l != m_top_layer) {  // do not set for top and bottom layers
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              y_loc1 = y;
              y_loc2 = y;
              x_loc1 = x - (x_bottom_enclosure + x_cut_size / 2);
              x_loc2 = x + (x_bottom_enclosure + x_cut_size / 2);
            } else {
              y_loc1 = y - (y_bottom_enclosure + y_cut_size / 2);
              y_loc2 = y + (y_bottom_enclosure + y_cut_size / 2);
              x_loc1 = x;
              x_loc2 = x;
            }
            m_Gmat->SetNode(x_loc1, y_loc1, l, make_pair(0, 0));
            m_Gmat->SetNode(x_loc2, y_loc2, l, make_pair(0, 0));
            m_Gmat->SetNode(x, y, l, bBox);
          }
          via_layer = via->getTopLayer();
          l         = via_layer->getRoutingLevel();

          // TODO this may count the stripe conductance twice but is needed to
          // fix a staggered stacked via
          layer_dir = via_layer->getDirection();
          if (m_bottom_layer != l
              && l != m_top_layer) {  // do not set for top and bottom layers
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              y_loc1 = y;
              y_loc2 = y;
              x_loc1 = x - (x_top_enclosure + x_cut_size / 2);
              x_loc2 = x + (x_top_enclosure + x_cut_size / 2);
            } else {
              y_loc1 = y - (y_top_enclosure + y_cut_size / 2);
              y_loc2 = y + (y_top_enclosure + y_cut_size / 2);
              x_loc1 = x;
              x_loc2 = x;
            }
            m_Gmat->SetNode(x_loc1, y_loc1, l, make_pair(0, 0));
            m_Gmat->SetNode(x_loc2, y_loc2, l, make_pair(0, 0));
            m_Gmat->SetNode(x, y, l, bBox);
          }
        } else {
          int                   x_loc1, x_loc2, y_loc1, y_loc2;
          dbTechLayer*          wire_layer = curWire->getTechLayer();
          int                   l          = wire_layer->getRoutingLevel();
          dbTechLayerDir::Value layer_dir  = wire_layer->getDirection();
          if (l == m_bottom_layer) {
            layer_dir = dbTechLayerDir::Value::HORIZONTAL;
          }
          if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
            y_loc1 = (curWire->yMin() + curWire->yMax()) / 2;
            y_loc2 = (curWire->yMin() + curWire->yMax()) / 2;
            x_loc1 = curWire->xMin();
            x_loc2 = curWire->xMax();
          } else {
            x_loc1 = (curWire->xMin() + curWire->xMax()) / 2;
            x_loc2 = (curWire->xMin() + curWire->xMax()) / 2;
            y_loc1 = curWire->yMin();
            y_loc2 = curWire->yMax();
          }
          if (l == m_bottom_layer
              || l == m_top_layer) {  // special case for bottom and top layers
                                      // we design a dense grid
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              int x_i;
              x_loc1 = (x_loc1 / m_node_density)
                       * m_node_density;  // quantize the horizontal direction
              x_loc2 = (x_loc2 / m_node_density)
                       * m_node_density;  // quantize the horizontal direction
              for (x_i = x_loc1; x_i <= x_loc2; x_i = x_i + m_node_density) {
                m_Gmat->SetNode(x_i, y_loc1, l, make_pair(0, 0));
              }
            } else {
              y_loc1 = (y_loc1 / m_node_density)
                       * m_node_density;  // quantize the vertical direction
              y_loc2 = (y_loc2 / m_node_density)
                       * m_node_density;  // quantize the vertical direction
              int y_i;
              for (y_i = y_loc1; y_i <= y_loc2; y_i = y_i + m_node_density) {
                m_Gmat->SetNode(x_loc1, y_i, l, make_pair(0, 0));
              }
            }
          } else {  // add end nodes
            m_Gmat->SetNode(x_loc1, y_loc1, l, make_pair(0, 0));
            m_Gmat->SetNode(x_loc2, y_loc2, l, make_pair(0, 0));
          }
        }
      }
    }
  }
  // insert c4 bumps as nodes
  int num_C4 = 0;
  for (size_t it = 0; it < m_C4Bumps.size(); ++it) {
    int           x    = get<0>(m_C4Bumps[it]);
    int           y    = get<1>(m_C4Bumps[it]);
    int           size = get<2>(m_C4Bumps[it]);
    double        v    = get<3>(m_C4Bumps[it]);
    vector<Node*> RDL_nodes;
    RDL_nodes = m_Gmat->GetRDLNodes(m_top_layer,
                                    m_top_layer_dir,
                                    x - size / 2,
                                    x + size / 2,
                                    y - size / 2,
                                    y + size / 2);
    if (RDL_nodes.empty() == true) {
      Node*   node     = m_Gmat->GetNode(x, y, m_top_layer, true);
      NodeLoc node_loc = node->GetLoc();
      double  new_loc1 = ((double) node_loc.first) / ((double) unit_micron);
      double  new_loc2 = ((double) node_loc.second) / ((double) unit_micron);
      double  old_loc1 = ((double) x) / ((double) unit_micron);
      double  old_loc2 = ((double) y) / ((double) unit_micron);
      double  old_size = ((double) size) / ((double) unit_micron);
      m_logger->warn(utl::PSM,
                     30,
                     "VSRC location at ({:4.3f}um, {:4.3f}um) "
                     "and size {:4.3f}um, is not located on a power stripe. "
                     "Moving to closest stripe at ({:4.3f}um, {:4.3f}um).",
                     old_loc1,
                     old_loc2,
                     old_size,
                     new_loc1,
                     new_loc2);
      RDL_nodes = m_Gmat->GetRDLNodes(m_top_layer,
                                      m_top_layer_dir,
                                      node_loc.first - size / 2,
                                      node_loc.first + size / 2,
                                      node_loc.second - size / 2,
                                      node_loc.second + size / 2);
    }
    vector<Node*>::iterator node_it;
    for (node_it = RDL_nodes.begin(); node_it != RDL_nodes.end(); ++node_it) {
      Node* node = *node_it;
      m_C4Nodes.push_back(make_pair(node->GetGLoc(), v));
      num_C4++;
    }
  }
  // All new nodes must be inserted by this point
  // initialize G Matrix
  m_logger->info(utl::PSM,
                 31,
                 "Number of PDN nodes on net {} = {}.",
                 m_power_net,
                 m_Gmat->GetNumNodes());
  m_Gmat->InitializeGmatDok(num_C4);
  for (vIter = power_nets.begin(); vIter != power_nets.end();
       ++vIter) {  // only 1 is expected?
    dbNet*                   curDnet = *vIter;
    dbSet<dbSWire>           swires  = curDnet->getSWires();
    dbSet<dbSWire>::iterator sIter;
    for (sIter = swires.begin(); sIter != swires.end();
         ++sIter) {  // only 1 is expected?
      dbSWire*                curSWire = *sIter;
      dbSet<dbSBox>           wires    = curSWire->getWires();
      dbSet<dbSBox>::iterator wIter;
      for (wIter = wires.begin(); wIter != wires.end(); ++wIter) {
        dbSBox* curWire = *wIter;
        if (curWire->isVia()) {
          dbVia* via                = curWire->getBlockVia();
          int    num_via_rows       = 1;
          int    num_via_cols       = 1;
          int    check_params       = via->hasParams();
          int    x_cut_size         = 0;
          int    y_cut_size         = 0;
          int    x_bottom_enclosure = 0;
          int    y_bottom_enclosure = 0;
          int    x_top_enclosure    = 0;
          int    y_top_enclosure    = 0;
          if (check_params == 1) {
            dbViaParams params;
            via->getViaParams(params);
            num_via_rows       = params.getNumCutRows();
            num_via_cols       = params.getNumCutCols();
            x_cut_size         = params.getXCutSize();
            y_cut_size         = params.getYCutSize();
            x_bottom_enclosure = params.getXBottomEnclosure();
            y_bottom_enclosure = params.getYBottomEnclosure();
            x_top_enclosure    = params.getXTopEnclosure();
            y_top_enclosure    = params.getYTopEnclosure();
          }
          int x, y;
          curWire->getViaXY(x, y);
          dbTechLayer* via_layer = via->getBottomLayer();
          int          l         = via_layer->getRoutingLevel();

          double R = via_layer->getUpperLayer()->getResistance();
          R        = R / (num_via_rows * num_via_cols);
          if (!CheckValidR(R) && !connection_only) {
            m_logger->error(utl::PSM,
                            35,
                            "{} resistance not found in DB. Check the LEF or "
                            "set it using the 'set_layer_rc' command.",
                            via_layer->getName());
          }
          bool    top_or_bottom = ((l == m_bottom_layer) || (l == m_top_layer));
          Node*   node_bot      = m_Gmat->GetNode(x, y, l, top_or_bottom);
          NodeLoc node_loc      = node_bot->GetLoc();
          if (abs(node_loc.first - x) > m_node_density
              || abs(node_loc.second - y) > m_node_density) {
            m_logger->warn(utl::PSM,
                           32,
                           "Node at ({}, {}) and layer {} moved from ({}, {}).",
                           node_loc.first,
                           node_loc.second,
                           l,
                           x,
                           y);
          }

          via_layer      = via->getTopLayer();
          l              = via_layer->getRoutingLevel();
          top_or_bottom  = ((l == m_bottom_layer) || (l == m_top_layer));
          Node* node_top = m_Gmat->GetNode(x, y, l, top_or_bottom);
          node_loc       = node_top->GetLoc();
          if (abs(node_loc.first - x) > m_node_density
              || abs(node_loc.second - y) > m_node_density) {
            m_logger->warn(utl::PSM,
                           33,
                           "Node at ({}, {}) and layer {} moved from ({}, {}).",
                           node_loc.first,
                           node_loc.second,
                           l,
                           x,
                           y);
          }

          if (node_bot == nullptr || node_top == nullptr) {
            m_logger->error(
                utl::PSM,
                34,
                "Unexpected condition. Null pointer received for node.");
          } else {
            if (R <= 1e-12) {  // if the resistance was not set.
              m_Gmat->SetConductance(node_bot, node_top, 0);
            } else {
              m_Gmat->SetConductance(node_bot, node_top, 1 / R);
            }
          }

          via_layer                       = via->getBottomLayer();
          dbTechLayerDir::Value layer_dir = via_layer->getDirection();
          l                               = via_layer->getRoutingLevel();
          if (l != m_bottom_layer) {
            double rho = via_layer->getResistance();
            if (!CheckValidR(rho) && !connection_only) {
              m_logger->error(utl::PSM,
                              36,
                              "Layer {} per-unit resistance not found in DB. "
                              "Check the LEF or set it using the command "
                              "'set_layer_rc -layer'.",
                              via_layer->getName());
            }
            int x_loc1, x_loc2, y_loc1, y_loc2;
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              y_loc1 = y - y_cut_size / 2;
              y_loc2 = y + y_cut_size / 2;
              x_loc1 = x - (x_bottom_enclosure + x_cut_size / 2);
              x_loc2 = x + (x_bottom_enclosure + x_cut_size / 2);
            } else {
              y_loc1 = y - (y_bottom_enclosure + y_cut_size / 2);
              y_loc2 = y + (y_bottom_enclosure + y_cut_size / 2);
              x_loc1 = x - x_cut_size / 2;
              x_loc2 = x + x_cut_size / 2;
            }
            m_Gmat->GenerateStripeConductance(via_layer->getRoutingLevel(),
                                              layer_dir,
                                              x_loc1,
                                              x_loc2,
                                              y_loc1,
                                              y_loc2,
                                              rho);
          }
          via_layer = via->getTopLayer();
          layer_dir = via_layer->getDirection();
          l         = via_layer->getRoutingLevel();
          if (l != m_top_layer) {
            double rho = via_layer->getResistance();
            if (!CheckValidR(rho) && !connection_only) {
              m_logger->error(utl::PSM,
                              37,
                              "Layer {} per-unit resistance not found in DB. "
                              "Check the LEF or set it using the command "
                              "'set_layer_rc -layer'.",
                              via_layer->getName());
            }
            int x_loc1, x_loc2, y_loc1, y_loc2;
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              y_loc1 = y - y_cut_size / 2;
              y_loc2 = y + y_cut_size / 2;
              x_loc1 = x - (x_top_enclosure + x_cut_size / 2);
              x_loc2 = x + (x_top_enclosure + x_cut_size / 2);
            } else {
              y_loc1 = y - (y_top_enclosure + y_cut_size / 2);
              y_loc2 = y + (y_top_enclosure + y_cut_size / 2);
              x_loc1 = x - x_cut_size / 2;
              x_loc2 = x + x_cut_size / 2;
            }
            m_Gmat->GenerateStripeConductance(via_layer->getRoutingLevel(),
                                              layer_dir,
                                              x_loc1,
                                              x_loc2,
                                              y_loc1,
                                              y_loc2,
                                              rho);
          }

        } else {
          dbTechLayer* wire_layer = curWire->getTechLayer();
          int          l          = wire_layer->getRoutingLevel();
          double       rho        = wire_layer->getResistance();
          if (!CheckValidR(rho) && !connection_only) {
            m_logger->error(utl::PSM,
                            66,
                            "Layer {} per-unit resistance not found in DB. "
                            "Check the LEF or set it using the command "
                            "'set_layer_rc -layer'.",
                            wire_layer->getName());
          }
          dbTechLayerDir::Value layer_dir = wire_layer->getDirection();
          if (l == m_bottom_layer) {  // ensure that the bottom layer(rail) is
                                      // horizontal
            layer_dir = dbTechLayerDir::Value::HORIZONTAL;
          }
          int x_loc1 = curWire->xMin();
          int x_loc2 = curWire->xMax();
          int y_loc1 = curWire->yMin();
          int y_loc2 = curWire->yMax();
          if (l == m_bottom_layer
              || l == m_top_layer) {  // special case for bottom and top layers
                                      // we design a dense grid
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              x_loc1 = (x_loc1 / m_node_density)
                       * m_node_density;  // quantize the horizontal direction
              x_loc2 = (x_loc2 / m_node_density)
                       * m_node_density;  // quantize the horizontal direction
            } else {
              y_loc1 = (y_loc1 / m_node_density)
                       * m_node_density;  // quantize the vertical direction
              y_loc2 = (y_loc2 / m_node_density)
                       * m_node_density;  // quantize the vertical direction
            }
          }
          m_Gmat->GenerateStripeConductance(wire_layer->getRoutingLevel(),
                                            layer_dir,
                                            x_loc1,
                                            x_loc2,
                                            y_loc1,
                                            y_loc2,
                                            rho);
        }
      }
    }
  }
  debugPrint(m_logger, utl::PSM, "G Matrix", 1, "G matrix created successfully.");
  return true;
}


bool IRSolver::CheckValidR(double R) {
  return R>=1e-12;
}

bool IRSolver::CheckConnectivity()
{
  vector<pair<NodeIdx, double>>::iterator c4_node_it;
  CscMatrix*                              Amat      = m_Gmat->GetAMat();
  int                                     num_nodes = m_Gmat->GetNumNodes();

  dbTech* tech        = m_db->getTech();
  int     unit_micron = tech->getDbUnitsPerMicron();

  for (c4_node_it = m_C4Nodes.begin(); c4_node_it != m_C4Nodes.end();
       c4_node_it++) {
    Node*        c4_node = m_Gmat->GetNode((*c4_node_it).first);
    queue<Node*> node_q;
    node_q.push(c4_node);
    while (!node_q.empty()) {
      NodeIdx col_loc, n_col_loc;
      Node*   node = node_q.front();
      node_q.pop();
      node->SetConnected();
      NodeIdx col_num = node->GetGLoc();
      col_loc         = Amat->col_ptr[col_num];
      if (col_num < Amat->col_ptr.size() - 1) {
        n_col_loc = Amat->col_ptr[col_num + 1];
      } else {
        n_col_loc = Amat->row_idx.size();
      }
      vector<NodeIdx> col_vec(Amat->row_idx.begin() + col_loc,
                              Amat->row_idx.begin() + n_col_loc);

      vector<NodeIdx>::iterator col_vec_it;
      for (col_vec_it = col_vec.begin(); col_vec_it != col_vec.end();
           col_vec_it++) {
        if (*col_vec_it < num_nodes) {
          Node* node_next = m_Gmat->GetNode(*col_vec_it);
          if (!(node_next->GetConnected())) {
            node_q.push(node_next);
          }
        }
      }
    }
  }
  int                     uncon_err_cnt   = 0;
  int                     uncon_inst_cnt  = 0;
  vector<Node*>           node_list       = m_Gmat->GetAllNodes();
  vector<Node*>::iterator node_list_it;
  bool                    unconnected_node = false;
  for (node_list_it = node_list.begin(); node_list_it != node_list.end();
       node_list_it++) {
    if (!(*node_list_it)->GetConnected()) {
      uncon_err_cnt++;
      NodeLoc node_loc = (*node_list_it)->GetLoc();
      float   loc_x    = ((float) node_loc.first) / ((float) unit_micron);
      float   loc_y    = ((float) node_loc.second) / ((float) unit_micron);
      unconnected_node = true;
      m_logger->warn(utl::PSM, 38, "Unconnected PDN node on net {} at location ({:4.3f}um, {:4.3f}um), layer: {}.",
                     m_power_net, loc_x, loc_y,(*node_list_it)->GetLayerNum());
      if ((*node_list_it)->HasInstances()) {
        vector<dbInst*>           insts = (*node_list_it)->GetInstances();
        vector<dbInst*>::iterator inst_it;
        for (inst_it = insts.begin(); inst_it != insts.end(); inst_it++) {
          uncon_inst_cnt++;
          m_logger->warn(utl::PSM, 39,"Unconnected instance {} at location ({:4.3f}um, {:4.3f}um) layer: {}.",
            (*inst_it)->getName(), loc_x, loc_y, (*node_list_it)->GetLayerNum());
        }
      }
    }
  }
  if (unconnected_node == false) {
    m_logger->info(utl::PSM, 40, "All PDN stripes on net {} are connected.", m_power_net);
  }
  return !unconnected_node;
}

int IRSolver::GetConnectionTest()
{
  if (m_connection) {
    return 1;
  } else {
    return 0;
  }
}

//! Function to get the power value from OpenSTA
/*
 *\return vector of pairs of instance name
 and its corresponding power value
*/
vector<pair<string, double>> IRSolver::GetPower()
{
  PowerInst power_inst;

  debugPrint(m_logger, utl::PSM, "IR Solver", 1, "Executing STA for power calculation");
  return power_inst.executePowerPerInst(m_sta, m_logger);
}

pair<double, double> IRSolver::GetSupplyVoltage()
{
  SupplyVoltage supply_volt;
  return supply_volt.getSupplyVoltage(m_sta);
}

bool IRSolver::GetResult()
{
  return m_result;
}

int IRSolver::PrintSpice()
{
  DokMatrix*                     Gmat = m_Gmat->GetGMatDOK();
  map<GMatLoc, double>::iterator it;

  ofstream pdnsim_spice_file;
  pdnsim_spice_file.open(m_spice_out_file);
  if (!pdnsim_spice_file.is_open()) {
    m_logger->error(
        utl::PSM,
        41,
        "Could not open SPICE file {}. Please check if it is a valid path.",
        m_spice_out_file);
  }
  vector<double> J                 = GetJ();
  int            num_nodes         = m_Gmat->GetNumNodes();
  int            resistance_number = 0;
  int            voltage_number    = 0;
  int            current_number    = 0;

  NodeLoc node_loc;
  for (it = Gmat->values.begin(); it != Gmat->values.end(); it++) {
    NodeIdx col = (it->first).first;
    NodeIdx row = (it->first).second;
    if (col <= row) {
      continue;  // ignore lower half and diagonal as matrix is symmetric
    }
    double cond = it->second;  // get cond value
    if (abs(cond) < 1e-15) {   // ignore if an empty cell
      continue;
    }

    string net_name = m_power_net;
    if (col < num_nodes) {  // resistances
      double resistance = -1 / cond;

      Node* node1       = m_Gmat->GetNode(col);
      Node* node2       = m_Gmat->GetNode(row);
      node_loc          = node1->GetLoc();
      int    x1         = node_loc.first;
      int    y1         = node_loc.second;
      int    l1         = node1->GetLayerNum();
      string node1_name = net_name + "_" + to_string(x1) + "_" + to_string(y1)
                          + "_" + to_string(l1);

      node_loc          = node2->GetLoc();
      int    x2         = node_loc.first;
      int    y2         = node_loc.second;
      int    l2         = node2->GetLayerNum();
      string node2_name = net_name + "_" + to_string(x2) + "_" + to_string(y2)
                          + "_" + to_string(l2);

      string resistance_name = "R" + to_string(resistance_number);
      resistance_number++;

      pdnsim_spice_file << resistance_name << " " << node1_name << " "
                        << node2_name << " " << to_string(resistance) << endl;

      double current      = node1->GetCurrent();
      string current_name = "I" + to_string(current_number);
      if (abs(current) > 1e-18) {
        pdnsim_spice_file << current_name << " " << node1_name << " " << 0
                          << " " << current << endl;
        current_number++;
      }

    } else {                                        // voltage
      Node* node1          = m_Gmat->GetNode(row);  // VDD location
      node_loc             = node1->GetLoc();
      double voltage_value = J[col];
      int    x1            = node_loc.first;
      int    y1            = node_loc.second;
      int    l1            = node1->GetLayerNum();
      string node1_name = net_name + "_" + to_string(x1) + "_" + to_string(y1)
                          + "_" + to_string(l1);
      string voltage_name = "V" + to_string(voltage_number);
      voltage_number++;
      pdnsim_spice_file << voltage_name << " " << node1_name << " 0 "
                        << to_string(voltage_value) << endl;
    }
  }

  pdnsim_spice_file << ".OPTION NUMDGT=6" << endl;
  pdnsim_spice_file << ".OP" << endl;
  pdnsim_spice_file << ".END" << endl;
  pdnsim_spice_file << endl;
  pdnsim_spice_file.close();
  return 1;
}

bool IRSolver::Build()
{
  bool res = true;
  ReadC4Data();
  if (res) {
    res = CreateGmat();
  }
  if (res) {
    res = CreateJ();
  }
  if (res) {
    res = AddC4Bump();
  }
  if (res) {
    res = m_Gmat->GenerateCSCMatrix();
    res = m_Gmat->GenerateACSCMatrix();
  }
  if (res) {
    m_connection = CheckConnectivity();
    res          = m_connection;
  }
  m_result = res;
  return m_result;
}

bool IRSolver::BuildConnection()
{
  bool res = true;
  ReadC4Data();
  if (res) {
    res = CreateGmat(true);
  }
  if (res) {
    res = AddC4Bump();
  }
  if (res) {
    res = m_Gmat->GenerateACSCMatrix();
  }
  if (res) {
    m_connection = CheckConnectivity();
    res          = m_connection;
  }
  m_result = res;
  return m_result;
}
}  // namespace psm
