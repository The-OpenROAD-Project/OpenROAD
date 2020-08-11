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
#include "opendb/db.h"
#include "ir_solver.h"
#include "node.h"
#include "gmat.h"
#include "get_power.h"
#include "openroad/Error.hh"

using ord::error;
using ord::warn;
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

using namespace std;
using std::vector;
using Eigen::Map;
using Eigen::VectorXd;
using Eigen::SparseMatrix; 
using Eigen::SparseLU;
using Eigen::Success;


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
  if(!m_connection) {
    cout<<"WARNING: Powergrid is not connected to all instances,"<<
          "IR Solver may not be accurate, LVS may also fail."<<endl;
  }
  int unit_micron = (m_db->getTech())->getDbUnitsPerMicron();
  clock_t t1, t2;
  CscMatrix*        Gmat = m_Gmat->GetGMat();
// fill A
  int     nnz     = Gmat->nnz;
  int               m    = Gmat->num_rows;
  int               n    = Gmat->num_cols;
  double* values  = &(Gmat->values[0]);
  int*    row_idx = &(Gmat->row_idx[0]);
  int*    col_ptr = &(Gmat->col_ptr[0]);
  Map<SparseMatrix<double> > A( Gmat->num_rows,
                                Gmat->num_cols,
                                Gmat->nnz,
                                col_ptr, // read-write
                                row_idx,
                                values);
    

  vector<double>    J = GetJ();
  Map<VectorXd> b(J.data(),J.size());
  VectorXd x;
  SparseLU<SparseMatrix<double> > solver;
  cout << "INFO: Factorizing G" << endl;
  solver.compute(A);
  if(solver.info()!=Success) {
    // decomposition failed
    cout<<"Error: LU factorization of GMatrix failed"<<endl;
    return;
  }
  cout << "INFO: Solving GV=J" << endl;
  cout << "INFO: SparseLU begin solving" << endl;
  x = solver.solve(b);
  cout << "INFO: SparseLU finished solving" << endl;
  if(solver.info()!=Success) {
    // solving failed
    cout<<"Error: Solving V = inv(G)*J failed"<<endl;
    return;
  }
  ofstream ir_report;
  ir_report.open (m_out_file);
  ir_report<<"Instance name, "<<" X location, "<<" Y location, "<<" Voltage "<<"\n";
  int num_nodes         = m_Gmat->GetNumNodes();
  int node_num =0;
  double       sum_volt = 0;
  wc_voltage            = vdd;
  while(node_num < num_nodes) {
    Node* node = m_Gmat->GetNode(node_num);
    double volt = x(node_num);
    sum_volt   = sum_volt + volt;
    if (volt < wc_voltage) {
      wc_voltage = volt;
    }
    node->SetVoltage(volt);
    node_num++;
    if(node->HasInstances()) {
      NodeLoc node_loc = node->GetLoc();
      float loc_x = ((float)node_loc.first)/((float)unit_micron);
      float loc_y = ((float)node_loc.second)/((float)unit_micron);
      std::vector<dbInst*> insts = node->GetInstances();
      std::vector<dbInst*>::iterator inst_it;
      if (m_out_file != "") {
        for(inst_it = insts.begin();inst_it!=insts.end();inst_it++) {
          ir_report<<(*inst_it)->getName()<<", "<<loc_x<<", " <<loc_y<<", "<<setprecision(10)<<volt<<"\n";
        }
      }
    }
  }
  ir_report<<endl;
  ir_report.close();
  avg_voltage = sum_volt / num_nodes;
}

//! Function to add C4 bumps to the G matrix
bool IRSolver::AddC4Bump()
{
  if (m_C4Bumps.size() == 0) {
    cout << "ERROR: Invalid number of voltage sources" << endl;
    return false;
  }
  for (unsigned int it = 0; it < m_C4Nodes.size(); ++it) {
    NodeIdx node_loc = m_C4Nodes[it].first;
    double  voltage  = m_C4Nodes[it].second;
    m_Gmat->AddC4Bump(node_loc, it);  // add the 0th bump
    m_J.push_back(voltage);           // push back first vdd
    vdd = voltage;
  }
  return true;
}



//! Function that parses the Vsrc file
void IRSolver::ReadC4Data()
{
  int unit_micron = (m_db->getTech())->getDbUnitsPerMicron();
  cout << "INFO: Reading location of VDD and VSS sources " << endl;
  if(m_vsrc_file != "") {
  std::ifstream file(m_vsrc_file);
    std::string line = "";
  // Iterate through each line and split the content using delimiter
  while (getline(file, line)) {
    tuple<int, int, int, double> c4_bump;
    int                          first, second, size;
    double                       voltage;
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
        voltage = stod(val);
      }
    }
    m_C4Bumps.push_back(make_tuple(first, second, size, voltage));
  }
  file.close();
  }
  else {
    cout << "Warning: Voltage pad location file not spcified, defaulting pad location to origin" << endl;
    m_C4Bumps.push_back(make_tuple(0,0,0,0));
  }
}


//! Function that parses the Vsrc file
/*void IRSolver::ReadResData()
{
  cout << "Default resistance file" << m_def_res << endl;
  cout << "INFO: Reading resistance of layers and vias " << endl;
  std::ifstream file(m_def_res);    
  std::string line = "";
  int line_num = 0;
  // Iterate through each line and split the content using delimiter
  while (getline(file, line)) {
    line_num ++;
    if (line_num == 1) {
      continue;
    }
    //tuple<int, double, double> layer_res;
    int                          routing_level;
    double                       res_per_unit;
    double                       res_via;
    stringstream                 X(line);
    string                       val;
    for (int i = 0; i < 3; ++i) {
      getline(X, val, ',');
      if (i == 0) {
        routing_level = stoi(val);
      } else if (i == 1) {
        res_per_unit = stod(val);
      } else {
        res_via = stod(val);
      }
    }
    m_layer_res.push_back(make_tuple(routing_level, res_per_unit, res_via));
  }
  file.close();
}
*/

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
      cout << "Warning instance " << it->first << " not found within database"
           << endl;
      continue;
    }
    int x, y;
    inst->getLocation(x, y);
    //cout << "Got location" <<endl;
    int   l      = m_bottom_layer;  // atach to the bottom most routing layer
    Node* node_J = m_Gmat->GetNode(x, y, l,true);
    NodeLoc node_loc = node_J->GetLoc();
    if( abs(node_loc.first - x) > m_node_density || abs(node_loc.second - y) > m_node_density ){
      cout<<"WARNING: Instance current node at "<<node_loc.first<<" "<<node_loc.second<<" layer "<<l<<" moved from "<<x<<" "<<y<<endl;
      cout<<"Instance: " << it->first <<endl;
    }
    //TODO modify for ground network
    node_J->AddCurrentSrc(it->second);
    node_J->AddInstance(inst);
  }
  for (int i = 0; i < num_nodes; ++i) {
    Node* node_J = m_Gmat->GetNode(i);
    m_J[i] = -1 * (node_J->GetCurrent());  // as MNA needs negative
    // cout << m_J[i] <<endl;
  }
  cout << "INFO: Created J vector" << endl;
  return true;
}


//! Function to create a G matrix using the nodes
bool IRSolver::CreateGmat(bool connection_only)
{
  cout<<"Creating G Matrix"<<endl;
  std::vector<Node*> node_vector;
  dbTech*                      tech   = m_db->getTech();
  //dbSet<dbTechLayer>           layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator litr;
  int unit_micron = tech->getDbUnitsPerMicron();
  int num_routing_layers = tech->getRoutingLayerCount();

  m_Gmat                    = new GMat(num_routing_layers);
  dbChip*             chip  = m_db->getChip();
  dbBlock*            block = chip->getBlock();
  dbSet<dbNet>        nets  = block->getNets();
  std::vector<dbNet*> vdd_nets;
  std::vector<dbNet*> gnd_nets;
  std::vector<dbNet*> power_nets;
  int num_wires =0;
  cout << "Extracting power stripes on net " << m_power_net <<endl;
  dbSet<dbNet>::iterator nIter;
  for (nIter = nets.begin(); nIter != nets.end(); ++nIter) {
    dbNet* curDnet = *nIter;
    dbSigType nType = curDnet->getSigType();
    if(m_power_net == "VSS") {
      if (nType == dbSigType::GROUND) {
        power_nets.push_back(curDnet);
      } else {
        continue;
      }
    } else if(m_power_net == "VDD") {
      if (nType == dbSigType::POWER) {
        power_nets.push_back(curDnet);
      } else {
        continue;
      }
    } else {
      cout << "Warning: Net not specifed as VDD or VSS. Power grid checker is not run." <<endl;
      return false;
    }
  }
  if(power_nets.size() == 0) {
    cout<<"Warning:No power stipes found in design. Power grid checker is not run."<<endl;
    return false;
  }
  std::vector<dbNet*>::iterator vIter;
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
        dbSBox* curWire = *wIter;
        int l;
        dbTechLayerDir::Value layer_dir; 
        if (curWire->isVia()) {
          dbVia* via      = curWire->getBlockVia();
          dbTechLayer* via_layer = via->getTopLayer();
          l = via_layer->getRoutingLevel();
          layer_dir = via_layer->getDirection();
        } else {
          dbTechLayer* wire_layer = curWire->getTechLayer();
          l = wire_layer->getRoutingLevel();
          layer_dir = wire_layer->getDirection();
          if (l < m_bottom_layer) {
            m_bottom_layer = l ; 
            m_bottom_layer_dir = layer_dir;
          }
        }
        if (l > m_top_layer) {
          m_top_layer = l ; 
          m_top_layer_dir = layer_dir;
        }
      }
    }
  }
  cout<<"Creating Nodes:     ";
  int progress_wires=0;
  int progress_percent=1;
  for (vIter = power_nets.begin(); vIter != power_nets.end(); ++vIter) {
    dbNet*                   curDnet = *vIter;
    dbSet<dbSWire>           swires  = curDnet->getSWires();
    dbSet<dbSWire>::iterator sIter;
    for (sIter = swires.begin(); sIter != swires.end(); ++sIter) {
      dbSWire*                curSWire = *sIter;
      dbSet<dbSBox>           wires    = curSWire->getWires();
      dbSet<dbSBox>::iterator wIter;
      for (wIter = wires.begin(); wIter != wires.end(); ++wIter) {
        if(progress_wires >= ((progress_percent/100.0)*num_wires)-1.0 ){
          cout<<"\b\b\b\b"<<setw(3)<<progress_percent++<<"%"<< std::flush;
        }
        progress_wires++;
        dbSBox* curWire = *wIter;
        if (curWire->isVia()) {
          dbVia* via      = curWire->getBlockVia();
          dbBox* via_bBox = via->getBBox();
          int check_params = via->hasParams();
          int x_cut_size = 0;
          int y_cut_size = 0;
          int x_bottom_enclosure = 0;
          int y_bottom_enclosure = 0;
          int x_top_enclosure = 0;
          int y_top_enclosure = 0;
		  if(check_params == 1) {
		    dbViaParams params;
			via->getViaParams(params);
		    x_cut_size = params.getXCutSize();
		    y_cut_size = params.getYCutSize();
            x_bottom_enclosure = params.getXBottomEnclosure();
            y_bottom_enclosure = params.getYBottomEnclosure();
            x_top_enclosure = params.getXTopEnclosure();
            y_top_enclosure = params.getYTopEnclosure();
		  }
          BBox   bBox = make_pair((via_bBox->getDX()) / 2, (via_bBox->getDY()) / 2);
          int x, y;
          curWire->getViaXY(x, y);
          dbTechLayer* via_layer = via->getBottomLayer();
          dbTechLayerDir::Value layer_dir  = via_layer->getDirection();
          int          l         = via_layer->getRoutingLevel();
          int x_loc1,x_loc2,y_loc1,y_loc2;
          if (m_bottom_layer != l && l != m_top_layer) {//do not set for top and bottom layers
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              y_loc1 = y;
              y_loc2 = y;
              x_loc1 = x - (x_bottom_enclosure+x_cut_size/2);
              x_loc2 = x + (x_bottom_enclosure+x_cut_size/2);
            } else {
              y_loc1 = y - (y_bottom_enclosure+y_cut_size/2);
              y_loc2 = y + (y_bottom_enclosure+y_cut_size/2);
              x_loc1 = x;
              x_loc2 = x;
            }
            m_Gmat->SetNode(x_loc1, y_loc1, l, make_pair(0, 0));
            m_Gmat->SetNode(x_loc2, y_loc2, l, make_pair(0, 0));
            m_Gmat->SetNode(x, y, l, bBox);
          }
          via_layer = via->getTopLayer();
          l         = via_layer->getRoutingLevel();

          //TODO this may count the stripe conductance twice but is needed to
          //fix a staggered stacked via
          layer_dir  = via_layer->getDirection();
          if (m_bottom_layer != l && l != m_top_layer) {//do not set for top and bottom layers
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              y_loc1 = y;
              y_loc2 = y;
              x_loc1 = x - (x_top_enclosure+x_cut_size/2);
              x_loc2 = x + (x_top_enclosure+x_cut_size/2);
            } else {
              y_loc1 = y - (y_top_enclosure+y_cut_size/2);
              y_loc2 = y + (y_top_enclosure+y_cut_size/2);
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
          if(l == m_bottom_layer){
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
          if (l == m_bottom_layer || l == m_top_layer) {  // special case for bottom and top layers we design a dense grid
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL ) {
              int x_i;
              x_loc1 = (x_loc1/m_node_density)*m_node_density; //quantize the horizontal direction
              x_loc2 = (x_loc2/m_node_density)*m_node_density; //quantize the horizontal direction
              for (x_i = x_loc1; x_i <= x_loc2; x_i = x_i + m_node_density) {
                m_Gmat->SetNode(x_i, y_loc1, l, make_pair(0, 0));
              }
            } else {
              y_loc1 = (y_loc1/m_node_density)*m_node_density; //quantize the vertical direction
              y_loc2 = (y_loc2/m_node_density)*m_node_density; //quantize the vertical direction
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
  cout<<endl;
  progress_wires =0;
  progress_percent =1;
  // insert c4 bumps as nodes
  int num_C4 =0;
  for (unsigned int it = 0; it < m_C4Bumps.size(); ++it) {
    int x = get<0>(m_C4Bumps[it]);
    int y = get<1>(m_C4Bumps[it]);
    int size = get<2>(m_C4Bumps[it]);
    double v  = get<3>(m_C4Bumps[it]);
    std::vector<Node*> RDL_nodes;
    RDL_nodes = m_Gmat->GetRDLNodes(m_top_layer, 
                                    m_top_layer_dir,
                                    x-size/2, 
                                    x+size/2,
                                    y-size/2,
                                    y+size/2);
    if (RDL_nodes.empty() == true) {
      Node* node = m_Gmat->GetNode(x,y,m_top_layer,true);
      NodeLoc node_loc = node->GetLoc();
      double new_loc1 = ((double)node_loc.first) /((double) unit_micron);
      double new_loc2 = ((double)node_loc.second) /((double) unit_micron);
      double old_loc1 = ((double)x) /((double) unit_micron);
      double old_loc2 = ((double)y) /((double) unit_micron);
      double old_size = ((double)size) /((double) unit_micron);
      cout<<"WARNING: Vsrc location at x="<<std::setprecision(3)<<old_loc1<<"um , y="<<old_loc2
          <<"um and size ="<<old_size<<"um,  is not located on a power stripe."<<endl;
      cout<<"         Moving to closest stripe at x="<<std::setprecision(3)<<new_loc1<<"um , y="<<new_loc2<<"um"<<endl;
      RDL_nodes = m_Gmat->GetRDLNodes(m_top_layer, 
                                      m_top_layer_dir,
                                      node_loc.first-size/2, 
                                      node_loc.first+size/2,
                                      node_loc.second-size/2,
                                      node_loc.second+size/2);

    }
    vector<Node*>::iterator node_it;
    for(node_it = RDL_nodes.begin(); node_it != RDL_nodes.end(); ++node_it) {
      Node* node = *node_it;
      m_C4Nodes.push_back(make_pair(node->GetGLoc(),v));
      num_C4++;
    }
  }
  // All new nodes must be inserted by this point
  // initialize G Matrix

  cout << "INFO: Number of nodes on net " << m_power_net <<" =" << m_Gmat->GetNumNodes() << endl;
  cout << "Creating Connections:     ";
  m_Gmat->InitializeGmatDok(num_C4);
  int err_flag_via = 1;
  int err_flag_layer = 1;
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
        if(progress_wires >= ((progress_percent/100.0)*num_wires)-1.0 ){
          cout<<"\b\b\b\b"<<setw(3)<<progress_percent++<<"%"<< std::flush;
        }
        progress_wires++;
        dbSBox* curWire = *wIter;
        if (curWire->isVia()) {
          dbVia* via      = curWire->getBlockVia();
          int num_via_rows = 1;
          int num_via_cols = 1;
          int check_params = via->hasParams();
          int x_cut_size = 0;
          int y_cut_size = 0;
          int x_bottom_enclosure = 0;
          int y_bottom_enclosure = 0;
          int x_top_enclosure = 0;
          int y_top_enclosure = 0;
		  if(check_params == 1) {
		    dbViaParams params;
			via->getViaParams(params);
		    num_via_rows = params.getNumCutRows();
		    num_via_cols = params.getNumCutCols();
		    x_cut_size = params.getXCutSize();
		    y_cut_size = params.getYCutSize();
            x_bottom_enclosure = params.getXBottomEnclosure();
            y_bottom_enclosure = params.getYBottomEnclosure();
            x_top_enclosure = params.getXTopEnclosure();
            y_top_enclosure = params.getYTopEnclosure();
		  }
          dbBox* via_bBox = via->getBBox();
          BBox   bBox
              = make_pair((via_bBox->getDX()) / 2, (via_bBox->getDY()) / 2);
          int x, y;
          curWire->getViaXY(x, y);
          dbTechLayer* via_layer = via->getBottomLayer();
          int          l         = via_layer->getRoutingLevel();

          double R = via_layer->getUpperLayer()->getResistance();
          R = R/(num_via_rows * num_via_cols);
          if (R == 0.0) {
            err_flag_via = 0;
            //R = get<2>(m_layer_res[l]); /// Must figure out via resistance value
            //cout << "Via Resistance" << R << endl;
          }
          bool top_or_bottom = ((l == m_bottom_layer) || (l == m_top_layer));
          Node* node_bot = m_Gmat->GetNode(x, y, l,top_or_bottom);
          NodeLoc node_loc = node_bot->GetLoc();
          if( abs(node_loc.first - x) > m_node_density || abs(node_loc.second - y) > m_node_density ){
            cout<<"WARNING: Node at "<<node_loc.first<<" "<<node_loc.second<<" layer "<<l<<" moved from "<<x<<" "<<y<<endl;
          }

          via_layer      = via->getTopLayer();
          l              = via_layer->getRoutingLevel();
          top_or_bottom = ((l == m_bottom_layer) || (l == m_top_layer));
          Node* node_top = m_Gmat->GetNode(x, y, l,top_or_bottom);
          node_loc = node_top->GetLoc();
          if( abs(node_loc.first - x) > m_node_density || abs(node_loc.second - y) > m_node_density ){
            cout<<"WARNING: Node at "<<node_loc.first<<" "<<node_loc.second<<" layer "<<l<<" moved from "<<x<<" "<<y<<endl;
          }

          if (node_bot == nullptr || node_top == nullptr) {
            cout << "ERROR: null pointer received for expected node. Code may "
                    "fail ahead."<<endl;
            return false;
          } else {
            if(R <= 1e-12){ //if the resitance was not set.
                m_Gmat->SetConductance(node_bot, node_top, 0);
            } else {
                m_Gmat->SetConductance(node_bot, node_top, 1 / R);
            }
          }
          
          via_layer = via->getBottomLayer();
          dbTechLayerDir::Value layer_dir  = via_layer->getDirection();
          l         = via_layer->getRoutingLevel();
          if(l != m_bottom_layer) {
            double       rho        = via_layer->getResistance()
                         * double(via_layer->getWidth())
                         / double(unit_micron);
            if (rho <= 1e-12) {
              rho = 0;
              err_flag_layer = 0;
            }
            int x_loc1,x_loc2,y_loc1,y_loc2;
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              y_loc1 = y - y_cut_size/2;
              y_loc2 = y + y_cut_size/2;
              x_loc1 = x - (x_bottom_enclosure+x_cut_size/2);
              x_loc2 = x + (x_bottom_enclosure+x_cut_size/2);
            } else {
              y_loc1 = y - (y_bottom_enclosure+y_cut_size/2);
              y_loc2 = y + (y_bottom_enclosure+y_cut_size/2);
              x_loc1 = x - x_cut_size/2;
              x_loc2 = x + x_cut_size/2;
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
          layer_dir  = via_layer->getDirection();
          l         = via_layer->getRoutingLevel();
          if(l != m_top_layer) {
            double       rho        = via_layer->getResistance()
                         * double(via_layer->getWidth())
                         / double(unit_micron);
            if (rho <= 1e-12) {
              rho = 0;
              err_flag_layer = 0;
            }
            int x_loc1,x_loc2,y_loc1,y_loc2;
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
              y_loc1 = y - y_cut_size/2;
              y_loc2 = y + y_cut_size/2;
              x_loc1 = x - (x_top_enclosure+x_cut_size/2);
              x_loc2 = x + (x_top_enclosure+x_cut_size/2);
            } else {
              y_loc1 = y - (y_top_enclosure+y_cut_size/2);
              y_loc2 = y + (y_top_enclosure+y_cut_size/2);
              x_loc1 = x - x_cut_size/2;
              x_loc2 = x + x_cut_size/2;
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
          int l  = wire_layer->getRoutingLevel();
          double       rho        = wire_layer->getResistance()
                       * double(wire_layer->getWidth())
                       / double(unit_micron);
          if (rho <= 1e-12) {
            rho = 0;
            err_flag_layer = 0;
          }
          dbTechLayerDir::Value layer_dir = wire_layer->getDirection();
          if (l == m_bottom_layer){//ensure that the bootom layer(rail) is horizontal
            layer_dir = dbTechLayerDir::Value::HORIZONTAL;
          }
          int x_loc1 = curWire->xMin();
          int x_loc2 = curWire->xMax();
          int y_loc1 = curWire->yMin();
          int y_loc2 = curWire->yMax();
          if (l == m_bottom_layer || l == m_top_layer) {  // special case for bottom and top layers we design a dense grid
            if (layer_dir == dbTechLayerDir::Value::HORIZONTAL ) {
              x_loc1 = (x_loc1/m_node_density)*m_node_density; //quantize the horizontal direction
              x_loc2 = (x_loc2/m_node_density)*m_node_density; //quantize the horizontal direction
            } else {
              y_loc1 = (y_loc1/m_node_density)*m_node_density; //quantize the vertical direction
              y_loc2 = (y_loc2/m_node_density)*m_node_density; //quantize the vertical direction
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
  cout<<endl;
  cout << "INFO: G matrix created " << endl;
  if (err_flag_via == 0 && !connection_only) {
    cout << "Error: Atleast one via resistance not found in DB. Check the LEF or set it with a odb::setResistance command"<<endl;
    return false;
  }
  if (err_flag_layer == 0 && !connection_only) {
    cout << "Error: Atleast one layer per unit resistance not found in DB. Check the LEF or set it with a odb::setResistance command"<<endl;
    return false;
  }
  return true;
}

bool IRSolver::CheckConnectivity()
{
  std::vector<std::pair<NodeIdx,double>>::iterator c4_node_it;
  int x,y;
  CscMatrix*        Amat = m_Gmat->GetAMat();
  int num_nodes = m_Gmat->GetNumNodes();

  dbTech* tech   = m_db->getTech();
  int unit_micron = tech->getDbUnitsPerMicron();

  for(c4_node_it = m_C4Nodes.begin(); c4_node_it != m_C4Nodes.end() ; c4_node_it++){
    Node* c4_node = m_Gmat->GetNode((*c4_node_it).first);
    std::queue<Node*> node_q;
    node_q.push(c4_node);
    while(!node_q.empty()) {
      NodeIdx col_loc, n_col_loc;
      Node* node = node_q.front();
      node_q.pop();
      node->SetConnected();
      NodeIdx col_num = node->GetGLoc();
      col_loc  = Amat->col_ptr[col_num];
      if(col_num < Amat->col_ptr.size()-1) {
        n_col_loc  = Amat->col_ptr[col_num+1];
      } else {
        n_col_loc  = Amat->row_idx.size() ;
      }
      std::vector<NodeIdx> col_vec(Amat->row_idx.begin()+col_loc,
                                   Amat->row_idx.begin()+n_col_loc);


      std::vector<NodeIdx>::iterator col_vec_it;
      for(col_vec_it = col_vec.begin(); col_vec_it != col_vec.end(); col_vec_it++){
        if(*col_vec_it<num_nodes) {
          Node* node_next = m_Gmat->GetNode(*col_vec_it);
          if(!(node_next->GetConnected())) {
            node_q.push(node_next);
          }
        }
      }
    }
  }
  int uncon_err_cnt = 0;
  int uncon_err_flag = 0;
  int uncon_inst_cnt = 0;
  int uncon_inst_flag = 0;
  std::vector<Node*> node_list = m_Gmat->GetAllNodes();
  std::vector<Node*>::iterator node_list_it;
  bool unconnected_node =false;
  for(node_list_it = node_list.begin(); node_list_it != node_list.end(); node_list_it++){
    if(!(*node_list_it)->GetConnected()){
      uncon_err_cnt++;
      NodeLoc node_loc = (*node_list_it)->GetLoc();
      float loc_x = ((float)node_loc.first)/((float)unit_micron);
      float loc_y = ((float)node_loc.second)/((float)unit_micron);

      //if(uncon_err_cnt>25 && uncon_err_flag ==0 ) {
      //  uncon_err_flag =1;
      //  cout<<"Error display limit reached, suppressing further unconnected node error messages"<<endl;
      //} else if( uncon_err_flag ==0) {
        //cout<<"node_not_connected ================================="<<endl;
        unconnected_node =true;
        cout<<"Warning: Unconnected PDN node on net " << m_power_net<<" at location x:"<<loc_x<<"um, y:"
            <<loc_y<<"um ,layer: "<<(*node_list_it)->GetLayerNum()<<endl;
      //}
      //if(uncon_inst_cnt>25 && uncon_inst_flag ==0 ) {
      //  uncon_inst_flag =1;
      //  cout<<"Error display limit reached, suppressing further unconnected instance error messages"<<endl;
      //} else if( uncon_inst_flag ==0) {
        if((*node_list_it)->HasInstances()){
          std::vector<dbInst*> insts = (*node_list_it)->GetInstances();
          std::vector<dbInst*>::iterator inst_it;
          for(inst_it = insts.begin();inst_it!=insts.end();inst_it++) {
            uncon_inst_cnt++;
            cout<<"Warning: Instance: "<< (*inst_it)->getName() <<"at location x:"<<loc_x<<"um, y:"
              <<loc_y<<"um ,layer: "<<(*node_list_it)->GetLayerNum()<<endl;
          }
        }
      //}
    }
  }
  if(unconnected_node == false){
    cout<<"INFO: No dangling stripe found on net "<<m_power_net<< endl;
    cout <<"INFO: Connection between all PDN nodes established on net "<<m_power_net <<endl;
  }
  return !unconnected_node;
}

int IRSolver::GetConnectionTest(){
  if(m_connection){
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
  PowerInst                    power_inst;
  vector<pair<string, double>> power_report = power_inst.executePowerPerInst(
      m_sta);

  return power_report;
}

bool IRSolver::GetResult(){
  return m_result; 
}

int IRSolver::PrintSpice() {
  DokMatrix*        Gmat = m_Gmat->GetGMatDOK();
  map<GMatLoc, double>::iterator it;
  
  ofstream pdnsim_spice_file;
  pdnsim_spice_file.open (m_spice_out_file);
  if (!pdnsim_spice_file.is_open()) {
    cout << "File did not open" << endl;
    return 0;
  }
  vector<double>    J = GetJ();
  int num_nodes = m_Gmat->GetNumNodes();
  int resistance_number = 0;
  int voltage_number = 0;
  int current_number = 0; 

  NodeLoc node_loc;
  for(it = Gmat->values.begin(); it!= Gmat->values.end(); it++){
    NodeIdx col = (it->first).first;
    NodeIdx row = (it->first).second;
    if(col <= row) {
      continue; //ignore lower half and diagonal as matrix is symmetric
    }
    double cond = it->second;           // get cond value
    if(abs(cond) < 1e-15){            //ignore if an empty cell
      continue;
    }

    string net_name = "vdd";
    if(col < num_nodes) { //resistances
      double resistance = -1/cond;

      Node* node1 = m_Gmat->GetNode(col); 
      Node* node2 = m_Gmat->GetNode(row); 
      node_loc = node1->GetLoc();
      int x1 = node_loc.first;
      int y1 = node_loc.second;
      int l1 = node1->GetLayerNum();
      string node1_name = net_name + "_" + to_string(x1) + "_" + to_string(y1) + "_" + to_string(l1);

      node_loc = node2->GetLoc();
      int x2 = node_loc.first;
      int y2 = node_loc.second;
      int l2 = node2->GetLayerNum();
      string node2_name = net_name + "_" + to_string(x2) + "_" + to_string(y2) + "_" + to_string(l2);
      
      string resistance_name = "R" + to_string(resistance_number); 
      resistance_number++;

      pdnsim_spice_file<< resistance_name <<" "<< node1_name << " " << node2_name <<" "<< to_string(resistance) <<endl;

      double current = node1->GetCurrent();
      string current_name = "I" + to_string(current_number); 
      if(abs(current)> 1e-18) {
        pdnsim_spice_file<< current_name <<" "<< node1_name << " " << 0 <<" "<< current <<endl;
        current_number++;
      }


    } else { //voltage
      Node* node1 = m_Gmat->GetNode(row); //VDD location 
      node_loc = node1->GetLoc();
      double voltage = J[col];
      int x1 = node_loc.first;
      int y1 = node_loc.second;
      int l1 = node1->GetLayerNum();
      string node1_name = net_name + "_" + to_string(x1) + "_" + to_string(y1) + "_" + to_string(l1);
      string voltage_name = "V" + to_string(voltage_number); 
      voltage_number++;
      pdnsim_spice_file<< voltage_name <<" "<< node1_name << " 0 " << to_string(voltage) <<endl;
    }
  } 
  
  pdnsim_spice_file<<".OPTION NUMDGT=6"<<endl;
  pdnsim_spice_file<<".OP"<<endl;
  pdnsim_spice_file<<".END"<<endl;
  pdnsim_spice_file<<endl;
  pdnsim_spice_file.close();
  return 1;
}

bool IRSolver::Build() {
  bool res = true;
  ReadC4Data();
  if(res) {
    res = CreateGmat(); 
  }
  if(res) {
    res = CreateJ();
  }
  if(res) {
    res = AddC4Bump();
  }
  if(res) {
    res = m_Gmat->GenerateCSCMatrix();
    res = m_Gmat->GenerateACSCMatrix();
  }
  if(res) {
    m_connection = CheckConnectivity();
    res = m_connection;
  }
  m_result = res;
  return m_result;
}

bool IRSolver::BuildConnection() {
  bool res = true;
  ReadC4Data();
  if(res) {
    res = CreateGmat(true); 
  }
  if(res) {
    res = AddC4Bump();
  }
  if(res) {
    res = m_Gmat->GenerateACSCMatrix();
  }
  if(res) {
    m_connection = CheckConnectivity();
    res = m_connection;
  }
  m_result = res;
  return m_result;
}
 
