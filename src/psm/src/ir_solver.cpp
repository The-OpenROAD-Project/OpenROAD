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
#include "ir_solver.h"

#include <Eigen/Sparse>
#include <Eigen/SparseLU>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "gmat.h"
#include "node.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "pad/ICeWall.h"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"

namespace psm {
using odb::dbBlock;
using odb::dbBox;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbNet;
using odb::dbRow;
using odb::dbSBox;
using odb::dbSet;
using odb::dbSigType;
using odb::dbSWire;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechVia;
using odb::dbVia;
using odb::dbViaParams;

using std::endl;
using std::ifstream;
using std::map;
using std::ofstream;
using std::pair;
using std::queue;
using std::setprecision;
using std::stod;
using std::string;
using std::stringstream;
using std::to_string;
using std::vector;

using Eigen::Map;
using Eigen::SparseLU;
using Eigen::SparseMatrix;
using Eigen::Success;
using Eigen::VectorXd;

IRSolver::IRSolver(odb::dbDatabase* db,
                   sta::dbSta* sta,
                   rsz::Resizer* resizer,
                   utl::Logger* logger,
                   odb::dbNet* net,
                   const std::optional<float>& voltage,
                   const std::string& vsrc_loc,
                   bool em_analyze,
                   int bump_pitch_x,
                   int bump_pitch_y,
                   float node_density_um,
                   int node_density_factor_user,
                   sta::Corner* corner)
{
  db_ = db;
  sta_ = sta;
  resizer_ = resizer;
  logger_ = logger;
  net_ = net;
  supply_voltage_src_ = voltage;
  vsrc_file_ = vsrc_loc;
  em_flag_ = em_analyze;
  bump_pitch_x_ = bump_pitch_x;
  bump_pitch_y_ = bump_pitch_y;
  node_density_um_ = node_density_um;
  node_density_factor_user_ = node_density_factor_user;
  corner_ = corner;

  if (net_ == nullptr) {
    logger_->error(utl::PSM, 88, "Invalid net specified");
  }
  if (!net_->getSigType().isSupply()) {
    logger_->error(utl::PSM, 87, "{} is not a supply net.", net_->getName());
  }

  if (corner_ == nullptr) {
    corner_ = sta_->cmdCorner();
  }
  if (corner_ == nullptr) {
    corner_ = sta_->corners()->findCorner(0);
  }
  if (corner_ == nullptr) {
    logger_->error(utl::PSM, 86, "Unable to proceed without a valid corner");
  }
}

IRSolver::~IRSolver() = default;

//! Returns the created G matrix for the design
/*
 * \return G Matrix
 */
GMat* IRSolver::getGMat()
{
  return Gmat_.get();
}

//! Returns current map represented as a 1D vector
/*
 * \return J vector
 */
const vector<double>& IRSolver::getJ() const
{
  return J_;
}

//! Function to solve for voltage using SparseLU
void IRSolver::solveIR()
{
  if (!connection_) {
    logger_->warn(utl::PSM,
                  8,
                  "Powergrid is not connected to all instances, therefore the "
                  "IR Solver may not be accurate. LVS may also fail.");
  }
  CscMatrix* Gmat = Gmat_->getGMat();
  // fill A
  double* values = &(Gmat->values[0]);
  int* row_idx = &(Gmat->row_idx[0]);
  int* col_ptr = &(Gmat->col_ptr[0]);
  Map<SparseMatrix<double>> A(Gmat->num_rows,
                              Gmat->num_cols,
                              Gmat->nnz,
                              col_ptr,  // read-write
                              row_idx,
                              values);
  vector<double> J = getJ();
  Map<VectorXd> b(J.data(), J.size());
  SparseLU<SparseMatrix<double>> solver;
  debugPrint(logger_, utl::PSM, "IR Solver", 1, "Factorizing the G matrix");
  debugPrint(logger_,
             utl::PSM,
             "IR Solver",
             2,
             "Gmat/rows {}, Gmat/cols {}, J/size {}",
             Gmat->num_rows,
             Gmat->num_cols,
             J.size());
  solver.compute(A);
  if (solver.info() != Success) {
    // decomposition failed
    logger_->error(
        utl::PSM,
        10,
        "LU factorization of the G Matrix failed. SparseLU solver message: {}.",
        solver.lastErrorMessage());
  }
  debugPrint(
      logger_, utl::PSM, "IR Solver", 1, "Solving system of equations GV=J");
  VectorXd x = solver.solve(b);
  if (solver.info() != Success) {
    // solving failed
    logger_->error(utl::PSM, 12, "Solving V = inv(G)*J failed.");
  } else {
    debugPrint(logger_,
               utl::PSM,
               "IR Solver",
               1,
               "Solving system of equations GV=J complete");
  }

  const int num_nodes = Gmat_->getNumNodes();
  int node_num = 0;
  double sum_volt = 0;
  wc_voltage_ = getSupplyVoltageSrc();
  while (node_num < num_nodes) {
    Node* node = Gmat_->getNode(node_num);
    const double volt = x(node_num);
    sum_volt = sum_volt + volt;
    if (net_->getSigType() == dbSigType::POWER) {
      if (volt < wc_voltage_) {
        wc_voltage_ = volt;
      }
    } else {
      if (volt > wc_voltage_) {
        wc_voltage_ = volt;
      }
    }
    node->setVoltage(volt);
    node_num++;
  }
  avg_voltage_ = sum_volt / num_nodes;

  if (em_flag_) {
    DokMatrix* Gmat_dok = Gmat_->getGMatDOK();
    int resistance_number = 0;
    max_cur_ = 0;
    double sum_cur = 0;
    Point node_loc;
    for (auto [loc, value] : Gmat_dok->values) {
      const NodeIdx col = loc.first;
      const NodeIdx row = loc.second;
      if (col <= row) {
        continue;  // ignore lower half and diagonal as matrix is symmetric
      }
      const double cond = value;  // get cond value
      if (abs(cond) < 1e-15) {    // ignore if an empty cell
        continue;
      }
      const string net_name = net_->getName();
      if (col < num_nodes) {  // resistances
        const double resistance = -1 / cond;

        const Node* node1 = Gmat_->getNode(col);
        const Node* node2 = Gmat_->getNode(row);
        node_loc = node1->getLoc();
        const int x1 = node_loc.getX();
        const int y1 = node_loc.getY();
        const int l1 = node1->getLayerNum();
        const string node1_name = net_name + "_" + to_string(x1) + "_"
                                  + to_string(y1) + "_" + to_string(l1);

        node_loc = node2->getLoc();
        int x2 = node_loc.getX();
        int y2 = node_loc.getY();
        int l2 = node2->getLayerNum();
        string node2_name = net_name + "_" + to_string(x2) + "_" + to_string(y2)
                            + "_" + to_string(l2);

        const string segment_name = "seg_" + to_string(resistance_number);

        const double v1 = node1->getVoltage();
        const double v2 = node2->getVoltage();
        double seg_cur = (v1 - v2) / resistance;
        sum_cur += abs(seg_cur);
        seg_cur = abs(seg_cur);
        if (seg_cur > max_cur_) {
          max_cur_ = seg_cur;
        }
        resistance_number++;
      }
    }  // for gmat values
    avg_cur_ = sum_cur / resistance_number;
    num_res_ = resistance_number;

  }  // enable em
}

void IRSolver::writeVoltageFile(const std::string& file) const
{
  ofstream ir_report;
  ir_report.open(file);
  if (!ir_report) {
    logger_->error(utl::PSM, 90, "Unable to open {}", file);
  }

  ir_report << "Instance name, "
            << "X location, "
            << "Y location, "
            << "Voltage" << endl;

  const int unit_micron = db_->getTech()->getDbUnitsPerMicron();
  const int num_nodes = Gmat_->getNumNodes();
  int node_num = 0;
  while (node_num < num_nodes) {
    Node* node = Gmat_->getNode(node_num);
    node_num++;
    if (node->hasInstances()) {
      const Point node_loc = node->getLoc();
      const float loc_x = node_loc.getX() / ((float) unit_micron);
      const float loc_y = node_loc.getY() / ((float) unit_micron);
      for (dbInst* inst : node->getInstances()) {
        ir_report << inst->getName() << ", " << loc_x << ", " << loc_y << ", "
                  << setprecision(6) << node->getVoltage() << endl;
      }
    }
  }
  ir_report << endl;
  ir_report.close();
}

void IRSolver::writeEMFile(const std::string& file) const
{
  DokMatrix* Gmat_dok = Gmat_->getGMatDOK();
  int resistance_number = 0;
  ofstream em_report;
  em_report.open(file);
  if (!em_report) {
    logger_->error(utl::PSM, 91, "Unable to open {}", file);
  }
  em_report << "Segment name, "
            << "Current, "
            << "Node 1, "
            << "Node 2" << endl;

  const int num_nodes = Gmat_->getNumNodes();
  Point node_loc;
  for (auto [loc, value] : Gmat_dok->values) {
    const NodeIdx col = loc.first;
    const NodeIdx row = loc.second;
    if (col <= row) {
      continue;  // ignore lower half and diagonal as matrix is symmetric
    }
    const double cond = value;  // get cond value
    if (abs(cond) < 1e-15) {    // ignore if an empty cell
      continue;
    }
    const string net_name = net_->getName();
    if (col < num_nodes) {  // resistances
      const double resistance = -1 / cond;

      const Node* node1 = Gmat_->getNode(col);
      const Node* node2 = Gmat_->getNode(row);
      node_loc = node1->getLoc();
      const int x1 = node_loc.getX();
      const int y1 = node_loc.getY();
      const int l1 = node1->getLayerNum();
      const string node1_name = net_name + "_" + to_string(x1) + "_"
                                + to_string(y1) + "_" + to_string(l1);

      node_loc = node2->getLoc();
      int x2 = node_loc.getX();
      int y2 = node_loc.getY();
      int l2 = node2->getLayerNum();
      string node2_name = net_name + "_" + to_string(x2) + "_" + to_string(y2)
                          + "_" + to_string(l2);

      const string segment_name = "seg_" + to_string(resistance_number);

      const double v1 = node1->getVoltage();
      const double v2 = node2->getVoltage();
      double seg_cur = (v1 - v2) / resistance;
      em_report << segment_name << ", " << setprecision(3) << seg_cur << ", "
                << node1_name << ", " << node2_name << endl;
      resistance_number++;
    }
  }
}

//! Function to add sources to the G matrix
bool IRSolver::addSources()
{
  if (sources_.empty()) {
    logger_->error(utl::PSM, 14, "Number of voltage sources cannot be 0.");
  }
  logger_->info(
      utl::PSM, 64, "Number of voltage sources = {}.", sources_.size());
  size_t it = 0;
  for (auto [node_loc, voltage_value] : source_nodes_) {
    Gmat_->addSource(node_loc, it++);  // add the  bump
  }
  return true;
}

//! Function that parses the Vsrc file
void IRSolver::readSourceData(bool require_voltage)
{
  findPdnWires();

  if (!vsrc_file_.empty()) {
    createSourcesFromVsrc(vsrc_file_);
    return;
  }

  if (require_voltage && !supply_voltage_src_.has_value()) {
    logger_->error(
        utl::PSM, 93, "Voltage on net {} is not set.", net_->getName());
  }
  if (!supply_voltage_src_.has_value()) {
    // default to 0 if voltage is not required
    supply_voltage_src_ = 0;
  }

  if (require_voltage) {
    logger_->info(utl::PSM,
                  22,
                  "Using {:.3f}V for {}",
                  getSupplyVoltageSrc(),
                  net_->getName());
  }

  const bool added_from_pads = createSourcesFromPads();
  const bool added_from_bterms = createSourcesFromBTerms();
  if (added_from_pads || added_from_bterms) {
    return;
  }

  createDefaultSources();
}

void IRSolver::createSourcesFromVsrc(const std::string& vsrc_file)
{
  ifstream file(vsrc_file);
  if (!file) {
    logger_->error(utl::PSM, 89, "Unable to open {}.", vsrc_file);
  }

  logger_->info(utl::PSM,
                15,
                "Reading location of VDD and VSS sources from {}.",
                vsrc_file);

  const int unit_micron = db_->getTech()->getDbUnitsPerMicron();
  string line;
  // Iterate through each line and split the content using delimiter
  while (getline(file, line)) {
    int x = -1, y = -1, size = -1;
    stringstream X(line);
    string val;
    for (int i = 0; i < 4; ++i) {
      getline(X, val, ',');
      if (i == 0) {
        x = (int) (unit_micron * stod(val));
      } else if (i == 1) {
        y = (int) (unit_micron * stod(val));
      } else if (i == 2) {
        size = (int) (unit_micron * stod(val));
      } else {
        supply_voltage_src_ = stod(val);
      }
    }
    if (x == -1 || y == -1 || size == -1) {
      logger_->error(utl::PSM, 75, "Expected four values on line: {}", line);
    } else {
      sources_.push_back({x, y, size, getSupplyVoltageSrc(), top_layer_, true});
    }
  }
  file.close();
}

void IRSolver::createDefaultSources()
{
  logger_->warn(utl::PSM,
                16,
                "Voltage pad location (VSRC) file not specified, defaulting "
                "pad location to checkerboard pattern on core area.");
  dbChip* chip = db_->getChip();
  dbBlock* block = chip->getBlock();
  odb::Rect coreRect = block->getCoreArea();
  const int coreW = coreRect.xMax() - coreRect.xMin();
  const int coreL = coreRect.yMax() - coreRect.yMin();
  const odb::Rect dieRect = block->getDieArea();
  const int offset_x = coreRect.xMin() - dieRect.xMin();
  const int offset_y = coreRect.yMin() - dieRect.yMin();
  const int unit_micron = db_->getTech()->getDbUnitsPerMicron();
  if (bump_pitch_x_ == 0) {
    bump_pitch_x_ = bump_pitch_default_ * unit_micron;
    logger_->warn(
        utl::PSM,
        17,
        "X direction bump pitch is not specified, defaulting to {}um.",
        bump_pitch_default_);
  }
  if (bump_pitch_y_ == 0) {
    bump_pitch_y_ = bump_pitch_default_ * unit_micron;
    logger_->warn(
        utl::PSM,
        18,
        "Y direction bump pitch is not specified, defaulting to {}um.",
        bump_pitch_default_);
  }
  if (coreW < bump_pitch_x_ || coreL < bump_pitch_y_) {
    float to_micron = 1.0f / unit_micron;
    const int x_cor = coreW / 2 + offset_x;
    const int y_cor = coreL / 2 + offset_y;
    logger_->warn(utl::PSM,
                  63,
                  "Specified bump pitches of {:4.3f} and {:4.3f} are less "
                  "than core width of {:4.3f} or core height of {:4.3f}. "
                  "Changing bump location to the center of the die at "
                  "({:4.3f}, {:4.3f}).",
                  bump_pitch_x_ * to_micron,
                  bump_pitch_y_ * to_micron,
                  coreW * to_micron,
                  coreL * to_micron,
                  x_cor * to_micron,
                  y_cor * to_micron);
    sources_.push_back({x_cor,
                        y_cor,
                        bump_size_ * unit_micron,
                        getSupplyVoltageSrc(),
                        top_layer_,
                        true});
  }
  const int num_b_x = coreW / bump_pitch_x_;
  const int centering_offset_x = (coreW - (num_b_x - 1) * bump_pitch_x_) / 2;
  const int num_b_y = coreL / bump_pitch_y_;
  const int centering_offset_y = (coreL - (num_b_y - 1) * bump_pitch_y_) / 2;
  logger_->warn(utl::PSM,
                65,
                "VSRC location not specified, using default checkerboard "
                "pattern with one VDD every size bumps in x-direction and "
                "one in two bumps in the y-direction");
  for (int i = 0; i < num_b_y; i++) {
    for (int j = 0; j < num_b_x; j = j + 6) {
      const int x_cor = (bump_pitch_x_ * j) + (((2 * i) % 6) * bump_pitch_x_)
                        + offset_x + centering_offset_x;
      const int y_cor = (bump_pitch_y_ * i) + offset_y + centering_offset_y;
      if (x_cor <= coreW && y_cor <= coreL) {
        sources_.push_back({x_cor,
                            y_cor,
                            bump_size_ * unit_micron,
                            getSupplyVoltageSrc(),
                            top_layer_,
                            true});
      }
    }
  }
}

bool IRSolver::createSourcesFromBTerms()
{
  const int pitch_multiplier = 10;

  bool added = false;
  for (auto* bterm : net_->getBTerms()) {
    for (auto* bpin : bterm->getBPins()) {
      if (!bpin->getPlacementStatus().isPlaced()) {
        continue;
      }

      for (auto* box : bpin->getBoxes()) {
        auto* layer = box->getTechLayer();
        if (layer == nullptr) {
          continue;
        }
        const auto rect = box->getBox();

        const int src_size = rect.minDXDY();

        int pitch = 0;
        auto* next_layer
            = db_->getTech()->findRoutingLayer(layer->getRoutingLevel() + 1);
        if (next_layer != nullptr) {
          pitch = pitch_multiplier * next_layer->getPitch();
        }
        if (pitch == 0) {
          pitch = pitch_multiplier * src_size;
        }

        odb::Point src;
        int dx = 0;
        int dy = 0;
        if (rect.dx() < rect.dy()) {
          dy = pitch;
          src = odb::Point(rect.xCenter(), rect.yMin() + dy / 2);
        } else if (rect.dy() < rect.dx()) {
          dx = pitch;
          src = odb::Point(rect.xMin() + dx / 2, rect.yCenter());
        } else {
          sources_.push_back({rect.xCenter(),
                              rect.yCenter(),
                              src_size,
                              getSupplyVoltageSrc(),
                              layer->getRoutingLevel(),
                              false});
          continue;
        }

        for (; rect.intersects(src);) {
          sources_.push_back({src.x(),
                              src.y(),
                              src_size,
                              getSupplyVoltageSrc(),
                              layer->getRoutingLevel(),
                              false});
          src.addX(dx);
          src.addY(dy);
        }
        added = true;
      }
    }
  }
  return added;
}

bool IRSolver::createSourcesFromPads()
{
  bool added = false;
  for (auto* iterm : net_->getITerms()) {
    auto* inst = iterm->getInst();
    if (!inst->isPlaced()) {
      continue;
    }
    if (!inst->isPad()) {
      continue;
    }

    const odb::dbTransform xform = inst->getTransform();

    auto* mterm = iterm->getMTerm();
    for (auto* mpin : mterm->getMPins()) {
      for (auto* box : mpin->getGeometry()) {
        auto* layer = box->getTechLayer();
        if (layer == nullptr) {
          continue;
        }
        auto rect = box->getBox();
        xform.apply(rect);
        const int src_size = rect.minDXDY();

        sources_.push_back({rect.xCenter(),
                            rect.yCenter(),
                            src_size,
                            getSupplyVoltageSrc(),
                            layer->getRoutingLevel(),
                            false});

        added = true;
      }
    }
  }
  return added;
}

std::pair<bool, std::set<psm::Node*>> IRSolver::getInstNodes(
    odb::dbInst* inst) const
{
  // Find the pin layers for the macro
  std::set<Node*> nodes_J;
  bool is_connected = false;
  for (auto* iterm : inst->getITerms()) {
    if (iterm->getNet() != net_) {
      continue;
    }

    auto find_abutment_iterm = iterms_connected_by_abutment_.find(iterm);
    if (find_abutment_iterm != iterms_connected_by_abutment_.end()) {
      debugPrint(logger_,
                 utl::PSM,
                 "IR Solver",
                 2,
                 "Using {} instead of {} due to abutment.",
                 find_abutment_iterm->second->getName(),
                 iterm->getName());
      iterm = find_abutment_iterm->second;
    }

    debugPrint(logger_,
               utl::PSM,
               "IR Solver",
               3,
               "Collecting nodes for {}",
               iterm->getName());
    is_connected = true;
    const odb::dbTransform xform = iterm->getInst()->getTransform();
    for (auto mpin : iterm->getMTerm()->getMPins()) {
      for (auto box : mpin->getGeometry()) {
        dbTechLayer* pin_layer = box->getTechLayer();
        if (pin_layer) {
          odb::Rect pin_shape = box->getBox();
          xform.apply(pin_shape);
          Gmat_->foreachNode(pin_layer->getRoutingLevel(),
                             pin_shape.xMin(),
                             pin_shape.xMax(),
                             pin_shape.yMin(),
                             pin_shape.yMax(),
                             [&](Node* node) { nodes_J.insert(node); });
        }
      }
    }
  }

  return {is_connected, nodes_J};
}

//! Function to create a J vector from the current map
bool IRSolver::createJ(size_t gmat_nodes)
{  // take current_map as an input?
  J_.resize(gmat_nodes, 0);

  for (auto [inst, power] : getPower()) {
    if (!inst->isPlaced()) {
      logger_->warn(utl::PSM,
                    71,
                    "Instance {} is not placed. Therefore, the"
                    " power drawn by this instance is not considered for IR "
                    " drop estimation. Please run analyze_power_grid after "
                    "instances are placed.",
                    inst->getName());
      continue;
    }
    // Special condition to distribute power across multiple nodes for macro
    // blocks
    // TODO: The condition for PADs needs to be handled sperately once an
    // appropriate testcase is found. Conditionally treated the same as a macro.
    if (inst->isBlock() || inst->isPad()) {
      const auto& [is_connected, nodes_J] = getInstNodes(inst);
      // If nodes are not found on the pin layers we search for the lowest
      // metal layer that overlaps the macro
      if (is_connected && nodes_J.empty()) {
        logger_->error(utl::PSM,
                       42,
                       "Unable to connect macro/pad Instance {} "
                       "to the power grid.",
                       inst->getName());
      }
      // Distribute the power across all nodes within the bounding box
      for (auto node_J : nodes_J) {
        node_J->addCurrentSrc(power / nodes_J.size());
        node_J->addInstance(inst);
      }
    } else {
      // For normal instances we only attach the current source to one node
      int x, y;
      inst->getLocation(x, y);
      Node* node_J = Gmat_->getNode(x, y, bottom_layer_, true);
      const Point node_loc = node_J->getLoc();
      if (abs(node_loc.getX() - x) > node_density_
          || abs(node_loc.getY() - y) > node_density_) {
        logger_->warn(utl::PSM,
                      24,
                      "Instance {}, current node at ({}, {}) at layer {} have "
                      "been moved from ({}, {}).",
                      inst->getName(),
                      node_loc.getX(),
                      node_loc.getY(),
                      bottom_layer_,
                      x,
                      y);
      }
      // Both these lines will change in the future for multiple power domains
      node_J->addCurrentSrc(power);
      node_J->addInstance(inst);
    }
  }
  // Creating the J matrix
  for (int i = 0; i < gmat_nodes; ++i) {
    const Node* node_J = Gmat_->getNode(i);
    if (net_->getSigType() == dbSigType::GROUND) {
      J_[i] = (node_J->getCurrent());
    } else {
      J_[i] = -1 * (node_J->getCurrent());
    }
  }
  for (const auto& [node_loc, voltage_value] : source_nodes_) {
    J_.push_back(voltage_value);
  }

  debugPrint(logger_, utl::PSM, "IR Solver", 1, "Created J vector");
  return true;
}

//! Function to find and store the upper and lower PDN layers and return a list
// of wires for all PDN tasks
void IRSolver::findPdnWires()
{
  power_wires_.clear();
  // Iterate through all wires till we reach the lowest abstraction level
  for (dbSWire* curSWire : net_->getSWires()) {
    for (dbSBox* curWire : curSWire->getWires()) {
      // Store wires in an easy to access format as we reuse it multiple times
      power_wires_.push_back(curWire);
      int l;
      // If the wire is a via get extract the top layer
      // We assume the bottom most layer must have power stripes.
      if (curWire->isVia()) {
        dbTechLayer* via_layer;
        if (curWire->getBlockVia()) {
          via_layer = curWire->getBlockVia()->getTopLayer();
        } else {
          via_layer = curWire->getTechVia()->getTopLayer();
        }
        l = via_layer->getRoutingLevel();
        // If the wire is a power stripe extract the bottom and bottom layer
      } else {
        dbTechLayer* wire_layer = curWire->getTechLayer();
        l = wire_layer->getRoutingLevel();
        if (l < bottom_layer_) {
          bottom_layer_ = l;
        }
      }
      if (l > top_layer_) {
        top_layer_ = l;
      }
    }
  }
}

map<Point, ViaCut> IRSolver::getViaCuts(Point loc,
                                        dbSet<dbBox> via_boxes,
                                        int lb,
                                        int lt,
                                        bool has_params,
                                        const dbViaParams& params)
{
  int num_rows = 1;
  int num_cols = 1;
  if (has_params) {
    num_rows = params.getNumCutRows();
    num_cols = params.getNumCutCols();
  }
  NodeEnclosure bot_encl = getViaEnclosure(lb, via_boxes);
  NodeEnclosure top_encl = getViaEnclosure(lt, via_boxes);
  map<Point, ViaCut> via_cuts;

  // create multiple cuts only if the overall enclosure is larger than
  // node_density
  if ((bot_encl.dx() > node_density_ || bot_encl.dy() > node_density_
       || top_encl.dy() > node_density_ || top_encl.dx() > node_density_)
      && num_rows * num_cols > 1) {
    for (auto* via_box : via_boxes) {
      auto* layer = via_box->getTechLayer();
      // Only capture shapes from the cut layer
      if (layer == nullptr) {
        continue;
      }
      int layer_num = layer->getRoutingLevel();
      if (layer_num == lb || layer_num == lt) {
        continue;
      }
      // Find the relative location and absolute of the via cut
      odb::Rect cut = via_box->getBox();
      int cut_x = (cut.xMin() + cut.xMax()) / 2;
      int cut_y = (cut.yMin() + cut.yMax()) / 2;
      Point cut_loc = Point(loc.getX() + cut_x, loc.getY() + cut_y);
      NodeEnclosure cut_bot_encl{0, 0, 0, 0};
      NodeEnclosure cut_top_encl{0, 0, 0, 0};

      // create a default enclosure of node_density/2 while ensuring
      // it does not exceed the overall enclosure
      cut_bot_encl.neg_x = std::min(cut_x + bot_encl.neg_x, node_density_);
      cut_bot_encl.pos_x = std::min(bot_encl.pos_x - cut_x, node_density_);
      cut_bot_encl.neg_y = std::min(cut_y + bot_encl.neg_y, node_density_);
      cut_bot_encl.pos_y = std::min(bot_encl.pos_y - cut_y, node_density_);

      cut_top_encl.neg_x = std::min(cut_x + top_encl.neg_x, node_density_);
      cut_top_encl.pos_x = std::min(top_encl.pos_x - cut_x, node_density_);
      cut_top_encl.neg_y = std::min(cut_y + top_encl.neg_y, node_density_);
      cut_top_encl.pos_y = std::min(top_encl.pos_y - cut_y, node_density_);

      ViaCut via_cut{cut_loc, cut_bot_encl, cut_top_encl};
      via_cuts.insert({cut_loc, via_cut});
    }
    // Reduce the number of nodes to speedup the computation
    int cut_x = 0;
    int cut_y = 0;
    auto prev_cut = via_cuts.begin();
    int x_st = prev_cut->first.getX();
    int y_st = prev_cut->first.getY();
    for (auto it = via_cuts.begin(); it != via_cuts.end();) {
      if (it->first.getX() >= x_st + cut_x * node_density_) {
        cut_x++;  // Keep Via if x distance is large
        cut_y = 1;
        prev_cut = it++;
      } else if ((it->first.getX() == prev_cut->first.getX())
                 && (it->first.getY() >= y_st + cut_y * node_density_)) {
        cut_y++;  // Keep via if Y distance is large
        prev_cut = it++;
      } else {
        it = via_cuts.erase(it);  // else delete via
      }
    }
  } else {
    ViaCut via_cut{loc, bot_encl, top_encl};
    via_cuts.insert({loc, via_cut});
  }

  if (via_cuts.empty()) {
    logger_->error(utl::PSM,
                   81,
                   "Via connection failed at {}, {}",
                   loc.getX(),
                   loc.getY());
  }
  return via_cuts;
}

//! Function to create the nodes of the G matrix
void IRSolver::createGmatViaNodes()
{
  for (auto curWire : power_wires_) {
    // For a Via we create the nodes at the top and bottom ends of the via
    if (!(curWire->isVia())) {
      continue;
    }
    dbTechLayer* via_bottom_layer;
    dbTechLayer* via_top_layer;
    dbSet<dbBox> via_boxes;
    dbViaParams params;
    bool has_params;
    if (curWire->getBlockVia()) {
      dbVia* via = curWire->getBlockVia();
      via_top_layer = via->getTopLayer();
      via_bottom_layer = via->getBottomLayer();
      via_boxes = via->getBoxes();
      params = via->getViaParams();
      has_params = via->hasParams();
    } else {
      dbTechVia* via = curWire->getTechVia();
      via_top_layer = via->getTopLayer();
      via_bottom_layer = via->getBottomLayer();
      via_boxes = via->getBoxes();
      params = via->getViaParams();
      has_params = via->hasParams();
    }
    const Point loc = curWire->getViaXY();
    const int lb = via_bottom_layer->getRoutingLevel();
    const int lt = via_top_layer->getRoutingLevel();
    // For large block vias that have multiple cuts
    auto via_cuts = getViaCuts(loc, via_boxes, lb, lt, has_params, params);
    for (auto& [cut_loc, via_cut] : via_cuts) {
      auto bot_node = Gmat_->setNode(cut_loc, lb);
      bot_node->setEnclosure(via_cut.bot_encl);
      auto top_node = Gmat_->setNode(cut_loc, lt);
      top_node->setEnclosure(via_cut.top_encl);
    }
  }
}

void IRSolver::createGmatWireNodes()
{
  std::set<odb::dbITerm*> macros_terms;
  dbBlock* block = db_->getChip()->getBlock();
  for (auto* inst : block->getInsts()) {
    if (inst->isBlock() || inst->isPad()) {
      for (auto* iterm : inst->getITerms()) {
        if (iterm->getNet() == net_) {
          macros_terms.insert(iterm);
        }
      }
    }
  }

  for (auto curWire : power_wires_) {
    // For a stripe we create nodes at the ends of the stripes and at a fixed
    // frequency in the lowermost layer.
    if (curWire->isVia()) {
      continue;
    }
    dbTechLayer* wire_layer = curWire->getTechLayer();
    const int l = wire_layer->getRoutingLevel();
    dbTechLayerDir::Value layer_dir = wire_layer->getDirection();
    if (l == bottom_layer_) {
      layer_dir = dbTechLayerDir::Value::HORIZONTAL;
    }
    int x_loc1, x_loc2, y_loc1, y_loc2;
    if (layer_dir == dbTechLayerDir::Value::HORIZONTAL) {
      y_loc1 = (curWire->yMin() + curWire->yMax()) / 2;
      y_loc2 = y_loc1;
      x_loc1 = curWire->xMin();
      x_loc2 = curWire->xMax();
    } else {
      x_loc1 = (curWire->xMin() + curWire->xMax()) / 2;
      x_loc2 = x_loc1;
      y_loc1 = curWire->yMin();
      y_loc2 = curWire->yMax();
    }
    // For all layers we create the end nodes
    Gmat_->setNode({x_loc1, y_loc1}, l);
    Gmat_->setNode({x_loc2, y_loc2}, l);

    // Check if shape overlaps a macro pin
    // and add node if there is an overlap with the current shape
    for (auto* iterm : macros_terms) {
      if (iterm->getBBox().intersects(curWire->getBox())) {
        const odb::dbTransform xform = iterm->getInst()->getTransform();
        for (auto* mpin : iterm->getMTerm()->getMPins()) {
          for (auto* geom : mpin->getGeometry()) {
            if (geom->getTechLayer() != wire_layer) {
              continue;
            }
            odb::Rect pin_rect = geom->getBox();
            xform.apply(pin_rect);
            if (pin_rect.intersects(curWire->getBox())) {
              // add node at center of overlap
              const odb::Rect overlap = pin_rect.intersect(curWire->getBox());

              Gmat_->setNode({overlap.xCenter(), overlap.yCenter()}, l);
            }
          }
        }
      }
    }

    if (l != bottom_layer_) {
      continue;
    }

    // special case for bottom layers we design a dense grid at a fixed
    // frequency
    auto node_map
        = Gmat_->getNodes(l, layer_dir, x_loc1, x_loc2, y_loc1, y_loc2);
    pair<pair<int, int>, Node*> node_prev;
    int v_itr, v_prev, length;
    int i = 0;
    for (auto& node_itr : node_map) {
      v_itr = (node_itr.first).first;
      if (i == 0) {
        // Before the first existing node
        i = 1;
        v_prev = x_loc1;  // assumes bottom layer is always horizontal
      } else {
        v_prev = (node_prev.first).first;
      }
      length = v_itr - v_prev;
      if (length > node_density_) {
        int num_nodes
            = length / node_density_;  // truncated integer will always be >= 1
        if (length % node_density_ == 0) {
          num_nodes -= 1;
        }
        float dist
            = float(length)
              / (num_nodes + 1);  // evenly distribute the length among nodes
        for (int v_i = 1; v_i <= num_nodes; v_i++) {
          int loc = v_prev + v_i * dist;  // ensures that the rounding is
                                          // distributed though the nodes.
          Gmat_->setNode({loc, y_loc1},
                         l);  // assumes bottom layer is always horizontal
        }
      }
      node_prev = node_itr;
    }
    // from the last node to the end
    if (i == 1) {
      int v_loc;
      v_loc = x_loc2;  // assumes bottom layer is always horizontal
      length = v_loc - v_itr;
      if (length > node_density_) {
        int num_nodes
            = length / node_density_;  // truncated integer will always be>1
        if (length % node_density_ == 0) {
          num_nodes -= 1;
        }
        float dist
            = float(length)
              / (num_nodes + 1);  // evenly distribute the length among nodes
        for (int v_i = 1; v_i <= num_nodes; v_i++) {
          int loc = v_itr + v_i * dist;  // ensures that the rounding is
                                         // distributed though the nodes.
          Gmat_->setNode({loc, y_loc1},
                         l);  // assumes bottom layer is always horizontal
        }
      }
    }
  }  // for power_wires
}

NodeEnclosure IRSolver::getViaEnclosure(int layer, dbSet<dbBox> via_boxes)
{
  NodeEnclosure encl{0, 0, 0, 0};
  for (auto via_box : via_boxes) {
    const auto via_box_layer = via_box->getTechLayer();
    if (via_box_layer->getRoutingLevel() == layer) {
      encl.neg_x = -via_box->xMin();
      encl.pos_x = via_box->xMax();
      encl.neg_y = -via_box->yMin();
      encl.pos_y = via_box->yMax();
      break;  // Only one box should be present in the layer
    }
  }
  return encl;
}

//! Function to create the connections of the G matrix
void IRSolver::createGmatConnections(bool connection_only)
{
  odb::dbTech* tech = db_->getTech();

  for (auto curWire : power_wires_) {
    // For vias we make 3 connections
    // 1) From the top node to the bottom node
    // 2) Nodes within the top enclosure
    // 3) Nodes within the bottom enclosure
    if (curWire->isVia()) {
      bool has_params;
      dbViaParams params;
      dbTechLayer* via_top_layer;
      dbTechLayer* via_bottom_layer;
      dbSet<dbBox> via_boxes;
      if (curWire->getBlockVia()) {
        dbVia* via = curWire->getBlockVia();
        has_params = via->hasParams();
        via_boxes = via->getBoxes();
        if (has_params) {
          params = via->getViaParams();
        }
        via_top_layer = via->getTopLayer();
        via_bottom_layer = via->getBottomLayer();
      } else {
        dbTechVia* via = curWire->getTechVia();
        has_params = via->hasParams();
        via_boxes = via->getBoxes();
        if (has_params) {
          params = via->getViaParams();
        }
        via_top_layer = via->getTopLayer();
        via_bottom_layer = via->getBottomLayer();
      }
      int num_via_rows = 1;
      int num_via_cols = 1;
      if (has_params) {
        num_via_rows = params.getNumCutRows();
        num_via_cols = params.getNumCutCols();
      }
      int x, y;
      curWire->getViaXY(x, y);
      const Point loc = curWire->getViaXY();

      const int bot_l = via_bottom_layer->getRoutingLevel();
      const int top_l = via_top_layer->getRoutingLevel();

      auto via_cuts
          = getViaCuts(loc, via_boxes, bot_l, top_l, has_params, params);

      // Find the resistance of each via cut
      const double R = getResistance(via_bottom_layer->getUpperLayer())
                       * via_cuts.size() / (num_via_rows * num_via_cols);
      if (!checkValidR(R) && !connection_only) {
        logger_->error(utl::PSM,
                       35,
                       "{} resistance not found in DB. Check the LEF or "
                       "set it using the 'set_layer_rc' command.",
                       via_bottom_layer->getName());
      }
      // Create a connection at each via cut
      for (auto& [cut_loc, via_cut] : via_cuts) {
        // Find the nodes of the via
        const Node* node_bot = Gmat_->getNode(
            cut_loc.getX(), cut_loc.getY(), bot_l, bot_l == bottom_layer_);
        const Node* node_top
            = Gmat_->getNode(cut_loc.getX(), cut_loc.getY(), top_l, false);

        // Get the exact location from the node
        const Point bot_node_loc = node_bot->getLoc();
        const Point top_node_loc = node_top->getLoc();

        if (abs(bot_node_loc.getX() - cut_loc.getX()) > node_density_
            || abs(bot_node_loc.getY() - cut_loc.getY()) > node_density_) {
          logger_->warn(utl::PSM,
                        32,
                        "Node at ({}, {}) and layer {} moved from ({}, {}).",
                        bot_node_loc.getX(),
                        bot_node_loc.getY(),
                        tech->findRoutingLayer(bot_l)->getName(),
                        cut_loc.getX(),
                        cut_loc.getY());
        }
        if (abs(top_node_loc.getX() - cut_loc.getX()) > node_density_
            || abs(top_node_loc.getY() - y) > node_density_) {
          logger_->warn(utl::PSM,
                        33,
                        "Node at ({}, {}) and layer {} moved from ({}, {}).",
                        top_node_loc.getX(),
                        top_node_loc.getY(),
                        tech->findRoutingLayer(top_l)->getName(),
                        cut_loc.getX(),
                        cut_loc.getY());
        }
        // Make a connection between the top and bottom nodes of the via
        if (R <= 1e-12) {  // if the resistance was not set.
          Gmat_->setConductance(node_bot, node_top, 0);
        } else {
          Gmat_->setConductance(node_bot, node_top, 1 / R);
        }
      }
      // Create the connections in the bottom enclosure
      const auto bot_layer_dir = via_bottom_layer->getDirection();
      // The bottom layer must be connected by a rail and not by the enclosure.
      if (bot_l != bottom_layer_) {
        const double rho = getResistance(via_bottom_layer);
        if (!checkValidR(rho) && !connection_only) {
          logger_->error(utl::PSM,
                         36,
                         "Layer {} per-unit resistance not found in DB. "
                         "Check the LEF or set it using the command "
                         "'set_layer_rc -layer'.",
                         via_bottom_layer->getName());
        }
        int x_loc1, x_loc2, y_loc1, y_loc2;
        // Create a conductance over the entire original enclosure
        NodeEnclosure bot_encl = getViaEnclosure(bot_l, via_boxes);
        y_loc1 = y - bot_encl.neg_y;
        y_loc2 = y + bot_encl.pos_y;
        x_loc1 = x - bot_encl.neg_x;
        x_loc2 = x + bot_encl.pos_x;
        Gmat_->generateStripeConductance(
            bot_l, bot_layer_dir, x_loc1, x_loc2, y_loc1, y_loc2, rho);
      }
      // Create the connections in the top enclosure
      const auto top_layer_dir = via_top_layer->getDirection();
      const double rho = getResistance(via_top_layer);
      if (!checkValidR(rho) && !connection_only) {
        logger_->error(utl::PSM,
                       37,
                       "Layer {} per-unit resistance not found in DB. "
                       "Check the LEF or set it using the command "
                       "'set_layer_rc -layer'.",
                       via_top_layer->getName());
      }
      int x_loc1, x_loc2, y_loc1, y_loc2;
      NodeEnclosure top_encl = getViaEnclosure(top_l, via_boxes);
      y_loc1 = y - top_encl.neg_y;
      y_loc2 = y + top_encl.pos_y;
      x_loc1 = x - top_encl.neg_x;
      x_loc2 = x + top_encl.pos_x;

      Gmat_->generateStripeConductance(
          top_l, top_layer_dir, x_loc1, x_loc2, y_loc1, y_loc2, rho);
    } else {
      // If it is a strip we create a connection between all the nodes in the
      // stripe
      dbTechLayer* wire_layer = curWire->getTechLayer();
      int l = wire_layer->getRoutingLevel();
      double rho = getResistance(wire_layer);
      if (!checkValidR(rho) && !connection_only) {
        logger_->error(utl::PSM,
                       66,
                       "Layer {} per-unit resistance not found in DB. "
                       "Check the LEF or set it using the command "
                       "'set_layer_rc -layer'.",
                       wire_layer->getName());
      }
      dbTechLayerDir::Value layer_dir = wire_layer->getDirection();
      if (l == bottom_layer_) {  // ensure that the bottom layer(rail) is
                                 // horizontal
        layer_dir = dbTechLayerDir::Value::HORIZONTAL;
      }
      int x_loc1 = curWire->xMin();
      int x_loc2 = curWire->xMax();
      int y_loc1 = curWire->yMin();
      int y_loc2 = curWire->yMax();
      Gmat_->generateStripeConductance(wire_layer->getRoutingLevel(),
                                       layer_dir,
                                       x_loc1,
                                       x_loc2,
                                       y_loc1,
                                       y_loc2,
                                       rho);
    }
  }
}

//! Function to create the nodes for the sources
int IRSolver::createSourceNodes(bool connection_only, int unit_micron)
{
  int num = 0;
  for (const auto& source : sources_) {
    const int x = source.x;
    const int y = source.y;
    const int size = source.size;
    const double v = source.voltage;
    const Node* node = Gmat_->getNode(x, y, source.layer, true);
    const Point node_loc = node->getLoc();
    const double new_loc1 = node_loc.getX() / ((double) unit_micron);
    const double new_loc2 = node_loc.getY() / ((double) unit_micron);
    if (2 * abs(node_loc.getX() - x) > size
        || 2 * abs(node_loc.getY() - y) > size) {
      const double old_loc1 = x / ((double) unit_micron);
      const double old_loc2 = y / ((double) unit_micron);
      const double old_size = size / ((double) unit_micron);
      if (source.user_specified) {
        logger_->warn(utl::PSM,
                      30,
                      "VSRC location at ({:4.3f}um, {:4.3f}um) and "
                      "size {:4.3f}um, is not located on an existing "
                      "power stripe node. Moving to closest node at "
                      "({:4.3f}um, {:4.3f}um).",
                      old_loc1,
                      old_loc2,
                      old_size,
                      new_loc1,
                      new_loc2);
      }
    }
    const NodeIdx k = node->getGLoc();
    const auto ret = source_nodes_.insert({k, v});
    if (ret.second == false) {
      if (ret.first->second != v) {
        // key already exists and voltage value is different occurs when a user
        // specifies two different voltage supply values by mistake in two
        // nearby nodes
        logger_->warn(
            utl::PSM,
            67,
            "Multiple voltage supply values mapped "
            "at the same node ({:4.3f}um, {:4.3f}um). "
            "If you provided a vsrc file. Check for duplicate entries. "
            "Choosing voltage value {:4.3f}.",
            new_loc1,
            new_loc2,
            ret.first->second);
      }
    } else {
      num++;
    }
  }
  return num;
}

//! Function to create a G matrix using the nodes
bool IRSolver::createGmat(bool connection_only)
{
  debugPrint(logger_, utl::PSM, "G Matrix", 1, "Creating G matrix");
  dbTech* tech = db_->getTech();
  const int unit_micron = tech->getDbUnitsPerMicron();
  const int num_routing_layers = tech->getRoutingLayerCount();
  dbChip* chip = db_->getChip();
  dbBlock* block = chip->getBlock();

  if (node_density_um_ > 0) {  // User-specified node density
    node_density_ = node_density_um_ * unit_micron;
    logger_->info(
        utl::PSM,
        73,
        "Setting lower metal node density to {}um as specfied by user.",
        node_density_um_);
  } else {  // Node density as a factor of row height either set by user or by
            // default
    dbRow* row = nullptr;
    for (auto* db_row : block->getRows()) {
      if (db_row->getSite()->getClass() != odb::dbSiteClass::PAD) {
        row = db_row;
        break;
      }
    }
    if (row == nullptr) {
      logger_->error(utl::PSM, 82, "Unable to find a row");
    }
    const int siteHeight = row->getSite()->getHeight();
    if (node_density_factor_user_ > 0) {
      node_density_factor_ = node_density_factor_user_;
    }
    logger_->info(utl::PSM,
                  76,
                  "Setting metal node density to "
                  "be standard cell height times {}.",
                  node_density_factor_);
    node_density_ = siteHeight * node_density_factor_;
  }

  Gmat_ = std::make_unique<GMat>(num_routing_layers, logger_, db_->getTech());
  debugPrint(logger_,
             utl::PSM,
             "G Matrix",
             1,
             "Extracting power stripes on net {}",
             net_->getName());

  // Create all the nodes for the G matrix
  createGmatViaNodes();
  createGmatWireNodes();

  if (Gmat_->getNumNodes() == 0) {
    logger_->warn(utl::PSM,
                  70,
                  "Net {} has no nodes and will be skipped",
                  net_->getName());
    return true;
  }

  // insert source as nodes
  const int num_sources = createSourceNodes(connection_only, unit_micron);

  // All new nodes must be inserted by this point
  // initialize G Matrix
  logger_->info(utl::PSM,
                31,
                "Number of PDN nodes on net {} = {}.",
                net_->getName(),
                Gmat_->getNumNodes());
  Gmat_->initializeGmatDok(num_sources);

  // Iterate through all the wires to populate conductance matrix
  createGmatConnections(connection_only);

  debugPrint(
      logger_, utl::PSM, "G Matrix", 1, "G matrix created successfully.");
  return true;
}

bool IRSolver::checkValidR(double R) const
{
  return R >= 1e-12;
}

bool IRSolver::checkConnectivity(const std::string& error_file,
                                 bool connection_only)
{
  const CscMatrix* Amat = Gmat_->getAMat();
  const int num_nodes = Gmat_->getNumNodes();
  const int unit_micron = db_->getTech()->getDbUnitsPerMicron();

  queue<Node*> node_q;
  // If we want to test the connectivity of the grid we just start from a single
  // point
  if (connection_only) {
    Node* node = Gmat_->getNode(source_nodes_.begin()->first);
    node_q.push(node);
  } else {
    // If we do IR analysis, we assume the grid can be connected by different
    // bumps
    for (auto [node_loc, voltage] : source_nodes_) {
      Node* node = Gmat_->getNode(node_loc);
      node_q.push(node);
    }
  }
  while (!node_q.empty()) {
    Node* node = node_q.front();
    node_q.pop();
    node->setConnected();
    const NodeIdx col_num = node->getGLoc();
    const NodeIdx col_loc = Amat->col_ptr[col_num];
    NodeIdx n_col_loc;
    if (col_num < Amat->col_ptr.size() - 1) {
      n_col_loc = Amat->col_ptr[col_num + 1];
    } else {
      n_col_loc = Amat->row_idx.size();
    }
    const vector<NodeIdx> col_vec(Amat->row_idx.begin() + col_loc,
                                  Amat->row_idx.begin() + n_col_loc);

    for (NodeIdx idx : col_vec) {
      if (idx < num_nodes) {
        Node* node_next = Gmat_->getNode(idx);
        if (!(node_next->getConnected())) {
          node_q.push(node_next);
        }
      }
    }
  }

  bool unconnected_node = false;
  auto tech = db_->getTech();
  for (Node* node : Gmat_->getAllNodes()) {
    if (!node->getConnected()) {
      const Point node_loc = node->getLoc();
      const float loc_x = node_loc.getX() / ((float) unit_micron);
      const float loc_y = node_loc.getY() / ((float) unit_micron);
      unconnected_node = true;
      logger_->warn(utl::PSM,
                    38,
                    "Unconnected PDN node on net {} at location ({:4.3f}um, "
                    "{:4.3f}um), layer: {}.",
                    net_->getName(),
                    loc_x,
                    loc_y,
                    tech->findRoutingLayer(node->getLayerNum())->getName());
      if (node->hasInstances()) {
        for (dbInst* inst : node->getInstances()) {
          logger_->warn(utl::PSM,
                        39,
                        "Unconnected instance {} at location ({:4.3f}um, "
                        "{:4.3f}um) layer: {}.",
                        inst->getName(),
                        loc_x,
                        loc_y,
                        tech->findRoutingLayer(node->getLayerNum())->getName());
        }
      }
    }
  }
  findUnconnectedInstances();
  for (auto* inst : unconnected_insts_) {
    unconnected_node = true;
    logger_->warn(utl::PSM,
                  94,
                  "{} is not connected to {}.",
                  inst->getName(),
                  net_->getName());
  }
  if (!unconnected_node) {
    logger_->info(utl::PSM,
                  40,
                  "All PDN stripes on net {} are connected.",
                  net_->getName());
  }

  if (!error_file.empty()) {
    writeErrorFile(error_file);
  }

  return !unconnected_node;
}

bool IRSolver::isStdCell(odb::dbInst* inst) const
{
  if (inst->isCore()) {
    return true;
  }
  if (inst->isBlock() || inst->isPad() || inst->getMaster()->isCover()) {
    return false;
  }
  if (inst->isEndCap()) {
    if (inst->getMaster()->getType() == odb::dbMasterType::ENDCAP_BOTTOMLEFT
        || inst->getMaster()->getType() == odb::dbMasterType::ENDCAP_BOTTOMRIGHT
        || inst->getMaster()->getType() == odb::dbMasterType::ENDCAP_TOPLEFT
        || inst->getMaster()->getType() == odb::dbMasterType::ENDCAP_TOPRIGHT) {
      // endcap is pad
      return false;
    }
  }
  return true;
}

bool IRSolver::isConnected(odb::dbITerm* iterm) const
{
  const odb::dbTransform xform = iterm->getInst()->getTransform();

  for (auto* mpin : iterm->getMTerm()->getMPins()) {
    for (auto* geom : mpin->getGeometry()) {
      auto* layer = geom->getTechLayer();
      if (layer == nullptr) {
        continue;
      }
      odb::Rect pin_shape = geom->getBox();
      xform.apply(pin_shape);
      bool has_connection = false;
      Gmat_->foreachNode(layer->getRoutingLevel(),
                         pin_shape.xMin(),
                         pin_shape.xMax(),
                         pin_shape.yMin(),
                         pin_shape.yMax(),
                         [&](Node* node) { has_connection = true; });

      if (has_connection) {
        return true;
      }
    }
  }
  return false;
}

void IRSolver::findUnconnectedInstancesByStdCells(
    ITermMap& iterms,
    std::set<odb::dbInst*>& connected_insts)
{
  for (auto itr = iterms.begin(); itr != iterms.end();) {
    odb::dbInst* inst = itr->first;
    if (!isStdCell(inst)) {
      itr++;
      continue;
    }
    int x, y;
    inst->getLocation(x, y);
    Node* node = Gmat_->getNode(x, y, bottom_layer_, true);
    if (!node) {
      unconnected_insts_.push_back(inst);
    } else {
      connected_insts.insert(inst);
    }
    itr = iterms.erase(itr);
  }
}

void IRSolver::findUnconnectedInstancesByITerms(
    ITermMap& iterms,
    std::set<odb::dbInst*>& connected_insts)
{
  for (auto itr = iterms.begin(); itr != iterms.end();) {
    odb::dbInst* inst = itr->first;

    bool all_connected = true;
    for (auto* iterm : itr->second) {
      if (!isConnected(iterm)) {
        all_connected = false;
      }
    }
    if (all_connected) {
      connected_insts.insert(inst);
      itr = iterms.erase(itr);
    } else {
      itr++;
    }
  }
}

void IRSolver::findUnconnectedInstancesByAbutment(
    ITermMap& iterms,
    std::set<odb::dbInst*>& connected_insts)
{
  iterms_connected_by_abutment_.clear();

  std::set<odb::dbInst*> inst_set;
  std::set<odb::dbITerm*> terminals_connected_to_grid;
  for (auto* iterm : net_->getITerms()) {
    odb::dbInst* inst = iterm->getInst();
    if (isStdCell(inst)) {
      continue;
    }
    inst_set.insert(inst);
    const bool inst_connected
        = connected_insts.find(inst) != connected_insts.end();
    if (inst_connected || isConnected(iterm)) {
      terminals_connected_to_grid.insert(iterm);
    }
  }
  std::vector<odb::dbInst*> insts;
  insts.insert(insts.begin(), inst_set.begin(), inst_set.end());

  std::map<odb::dbITerm*, std::set<odb::dbITerm*>> connections;
  for (size_t i = 0; i < insts.size(); i++) {
    auto* inst0 = insts[i];
    for (size_t j = i + 1; j < insts.size(); j++) {
      auto* inst1 = insts[j];
      const auto inst_connections
          = pad::ICeWall::getTouchingIterms(inst0, inst1);
      for (const auto& [iterm0, iterm1] : inst_connections) {
        connections[iterm0].insert(iterm1);
        connections[iterm1].insert(iterm0);
      }
    }
  }

  for (auto itr = iterms.begin(); itr != iterms.end();) {
    bool connected = false;
    for (auto* iterm : itr->second) {
      if (terminals_connected_to_grid.find(iterm)
          != terminals_connected_to_grid.end()) {
        connected = true;
        break;
      }

      std::queue<odb::dbITerm*> terms_to_check;
      terms_to_check.push(iterm);
      std::set<odb::dbITerm*> checked;
      while (!terms_to_check.empty()) {
        odb::dbITerm* check_term = terms_to_check.front();
        terms_to_check.pop();
        checked.insert(check_term);

        if (terminals_connected_to_grid.find(check_term)
            != terminals_connected_to_grid.end()) {
          connected = true;
          terminals_connected_to_grid.insert(iterm);

          odb::dbITerm* grid_iterm = check_term;
          // check if iterm is also an abutment and use its connection
          auto find_abutment = iterms_connected_by_abutment_.find(check_term);
          if (find_abutment != iterms_connected_by_abutment_.end()) {
            grid_iterm = find_abutment->second;
          }
          iterms_connected_by_abutment_[iterm] = grid_iterm;
          break;
        } else {
          for (auto* next_iterm : connections[check_term]) {
            if (checked.find(next_iterm) == checked.end()) {
              terms_to_check.push(next_iterm);
            }
          }
        }
      }
    }

    if (connected) {
      itr = iterms.erase(itr);
    } else {
      itr++;
    }
  }
}

void IRSolver::findUnconnectedInstances()
{
  unconnected_insts_.clear();

  std::set<odb::dbInst*> connected_insts;
  for (Node* node : Gmat_->getAllNodes()) {
    if (node->hasInstances()) {
      for (odb::dbInst* inst : node->getInstances()) {
        connected_insts.insert(inst);
      }
    }
  }

  ITermMap iterms;
  for (odb::dbInst* inst : db_->getChip()->getBlock()->getInsts()) {
    if (!inst->isPlaced()) {
      continue;
    }
    const bool has_inst = connected_insts.find(inst) != connected_insts.end();
    if (has_inst) {
      continue;
    }
    for (auto* iterm : inst->getITerms()) {
      if (iterm->getNet() == net_) {
        iterms[inst].push_back(iterm);
      }
    }
  }

  findUnconnectedInstancesByStdCells(iterms, connected_insts);
  findUnconnectedInstancesByITerms(iterms, connected_insts);
  findUnconnectedInstancesByAbutment(iterms, connected_insts);

  for (const auto& [inst, inst_iterms] : iterms) {
    unconnected_insts_.push_back(inst);
  }
}

void IRSolver::writeErrorFile(const std::string& file) const
{
  ofstream error_report;
  error_report.open(file);
  if (!error_report) {
    logger_->error(utl::PSM, 92, "Unable to open {}", file);
  }

  auto* tech = db_->getTech();
  const float unit_micron = tech->getDbUnitsPerMicron();
  for (Node* node : Gmat_->getAllNodes()) {
    if (!node->getConnected()) {
      const Point node_loc = node->getLoc();
      const float loc_x = node_loc.getX() / unit_micron;
      const float loc_y = node_loc.getY() / unit_micron;
      error_report << "violation type: Unconnected PDN node" << endl;
      error_report << "  srcs: " << endl;
      error_report << fmt::format(
          "    bbox = ({}, {}) - ({}, {}) on Layer {}",
          loc_x - 0.05,
          loc_y - 0.05,
          loc_x + 0.05,
          loc_y + 0.05,
          tech->findRoutingLayer(node->getLayerNum())->getName())
                   << endl;
    }
  }
  for (auto* inst : unconnected_insts_) {
    const odb::Rect inst_rect = inst->getBBox()->getBox();
    error_report << "violation type: Unconnected instance" << endl;
    error_report << "  srcs: inst:" << inst->getName() << endl;
    error_report << fmt::format(
        "    bbox = ({}, {}) - ({}, {}) on Layer {}",
        inst_rect.xMin() / unit_micron,
        inst_rect.yMin() / unit_micron,
        inst_rect.xMax() / unit_micron,
        inst_rect.yMax() / unit_micron,
        tech->findRoutingLayer(bottom_layer_)->getName())
                 << endl;
  }
}

bool IRSolver::getConnectionTest() const
{
  return connection_;
}

//! Function to get the power value from OpenSTA
/*
 *\return vector of pairs of instance name
 and its corresponding power value
*/
vector<pair<odb::dbInst*, float>> IRSolver::getPower()
{
  debugPrint(
      logger_, utl::PSM, "IR Solver", 1, "Executing STA for power calculation");

  vector<pair<odb::dbInst*, float>> power_report;
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network->leafInstanceIterator());
  sta::PowerResult total_calc;
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    sta::LibertyCell* cell = network->libertyCell(inst);
    if (cell != nullptr) {
      sta::PowerResult inst_power = sta_->power(inst, corner_);
      total_calc.incr(inst_power);
      power_report.emplace_back(network->staToDb(inst), inst_power.total());
      debugPrint(logger_,
                 utl::PSM,
                 "get power",
                 2,
                 "Power of instance {} is {}",
                 network->name(inst),
                 inst_power.total());
    }
  }

  debugPrint(
      logger_, utl::PSM, "get power", 1, "Total power: {}", total_calc.total());
  return power_report;
}

void IRSolver::writeSpiceFile(const std::string& file) const
{
  DokMatrix* Gmat = Gmat_->getGMatDOK();

  ofstream pdnsim_spice_file;
  pdnsim_spice_file.open(file);
  if (!pdnsim_spice_file.is_open()) {
    logger_->error(
        utl::PSM,
        41,
        "Could not open SPICE file {}. Please check if it is a valid path.",
        file);
  }
  const vector<double>& J = getJ();
  const int num_nodes = Gmat_->getNumNodes();
  int resistance_number = 0;
  int voltage_number = 0;
  int current_number = 0;

  for (const auto& [loc, cond] : Gmat->values) {
    const NodeIdx col = loc.first;
    const NodeIdx row = loc.second;
    if (col <= row) {
      continue;  // ignore lower half and diagonal as matrix is symmetric
    }
    if (abs(cond) < 1e-15) {  // ignore if an empty cell
      continue;
    }

    const string net_name = net_->getName();
    if (col < num_nodes) {  // resistances
      const double resistance = -1 / cond;

      const Node* node1 = Gmat_->getNode(col);
      const Node* node2 = Gmat_->getNode(row);
      const Point node_loc1 = node1->getLoc();
      const int x1 = node_loc1.getX();
      const int y1 = node_loc1.getY();
      const int l1 = node1->getLayerNum();
      const string node1_name = net_name + "_" + to_string(x1) + "_"
                                + to_string(y1) + "_" + to_string(l1);

      const Point node_loc2 = node2->getLoc();
      const int x2 = node_loc2.getX();
      const int y2 = node_loc2.getY();
      const int l2 = node2->getLayerNum();
      const string node2_name = net_name + "_" + to_string(x2) + "_"
                                + to_string(y2) + "_" + to_string(l2);

      const string resistance_name = "R" + to_string(resistance_number);
      resistance_number++;

      pdnsim_spice_file << resistance_name << " " << node1_name << " "
                        << node2_name << " " << to_string(resistance) << endl;

      const double current = node1->getCurrent();
      const string current_name = "I" + to_string(current_number);
      if (abs(current) > 1e-18) {
        pdnsim_spice_file << current_name << " " << node1_name << " " << 0
                          << " " << current << endl;
        current_number++;
      }
    } else {                                    // voltage
      const Node* node1 = Gmat_->getNode(row);  // VDD location
      const Point node_loc = node1->getLoc();
      const double voltage_value = J[col];
      const int x1 = node_loc.getX();
      const int y1 = node_loc.getY();
      const int l1 = node1->getLayerNum();
      const string node1_name = net_name + "_" + to_string(x1) + "_"
                                + to_string(y1) + "_" + to_string(l1);
      const string voltage_name = "V" + to_string(voltage_number);
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
}

int IRSolver::getMinimumResolution() const
{
  return node_density_;
}

bool IRSolver::build(const std::string& error_file, bool connectivity_only)
{
  if (em_flag_) {
    logger_->info(utl::PSM, 4, "EM calculation is enabled.");
  }

  connection_ = false;

  readSourceData(!connectivity_only);

  bool res = createGmat(connectivity_only);
  const size_t gmat_nodes = Gmat_->getNumNodes();
  if (gmat_nodes == 0) {
    connection_ = true;
    return false;
  }

  if (res) {
    res = addSources();
  }
  if (res && !connectivity_only) {
    res = Gmat_->generateCSCMatrix();
  }
  if (res) {
    res = Gmat_->generateACSCMatrix();
  }
  if (res) {
    connection_ = checkConnectivity(error_file, connectivity_only);
    if (!connectivity_only) {
      res = connection_;
    }
  }
  if (res && !connectivity_only) {
    res = createJ(gmat_nodes);
  }
  return res;
}

double IRSolver::getResistance(odb::dbTechLayer* layer) const
{
  double res;

  if (layer->getRoutingLevel() == 0) {
    double cap;
    resizer_->layerRC(layer, corner_, res, cap);
  } else {
    double r_per_meter, cap_per_meter;
    resizer_->layerRC(layer, corner_, r_per_meter, cap_per_meter);

    const double width_meter = static_cast<double>(layer->getWidth())
                               / layer->getTech()->getLefUnits() * 1e-6;

    res = r_per_meter * width_meter;
  }

  if (res == 0.0) {
    // Get database resistance
    res = layer->getResistance();
  }

  return res;
}

}  // namespace psm
