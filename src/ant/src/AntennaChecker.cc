// BSD 3-Clause License
//
// Copyright (c) 2020, MICL, DD-Lab, University of Michigan
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>

#include <cstring>
#include <iostream>
#include <unordered_set>

#include <tcl.h>

#include "ant/AntennaChecker.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/dbWireGraph.h"
#include "odb/wOrder.h"
#include "sta/StaMain.hh"
#include "utl/Logger.h"
#include "grt/GlobalRouter.h"

namespace ant {

using odb::dbBox;
using odb::dbLib;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbITerm;
using odb::dbITermObj;
using odb::dbIoType;
using odb::dbMaster;
using odb::dbMasterType;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbSet;
using odb::dbTechAntennaPinModel;
using odb::dbTechLayer;
using odb::dbTechLayerAntennaRule;
using odb::dbTechLayerType;
using odb::dbTechVia;
using odb::dbVia;
using odb::dbViaParams;
using odb::dbWire;
using odb::dbWireGraph;
using odb::dbWireType;
using odb::dbIoType;

using utl::ANT;

using std::unordered_set;

// Abbreviations Index:
//   `PAR`: Partial Area Ratio
//   `CAR`: Cumulative Area Ratio
//   `Area`: Gate Area
//   `S. Area`: Side Diffusion Area
//   `C. Area`: Cumulative Gate Area
//   `C. S. Area`: Cumulative Side (Diffusion) Area

struct PARinfo
{
  odb::dbWireGraph::Node* wire_root;
  std::set<odb::dbITerm*> iterms;
  double wire_area;
  double side_wire_area;
  double iterm_gate_area;
  double iterm_diff_area;
  double PAR;
  double PSR;
  double diff_PAR;
  double diff_PSR;
};

struct ARinfo
{
  odb::dbWireGraph::Node* wire_root;
  odb::dbWireGraph::Node* GateNode;
  double PAR;
  double PSR;
  double diff_PAR;
  double diff_PSR;
  double CAR;
  double CSR;
  double diff_CAR;
  double diff_CSR;
  double diff_area;
};

struct AntennaModel
{
  odb::dbTechLayer* layer;

  double metal_factor;
  double diff_metal_factor;

  double cut_factor;
  double diff_cut_factor;

  double side_metal_factor;
  double diff_side_metal_factor;

  double minus_diff_factor;
  double plus_diff_factor;
  double diff_metal_reduce_factor;
};

extern "C" {
extern int Ant_Init(Tcl_Interp* interp);
}

AntennaChecker::AntennaChecker() :
  net_violation_count_(0)
{
}

AntennaChecker::~AntennaChecker()
{
}

void AntennaChecker::init(odb::dbDatabase* db,
                          grt::GlobalRouter* global_router,
                          Logger* logger)
{
  db_ = db;
  global_router_ = global_router;
  logger_ = logger;
}

double AntennaChecker::dbuToMicrons(int dbu)
{
  return static_cast<double>(dbu) / dbu_per_micron_;
}

void AntennaChecker::initAntennaRules()
{
  odb::dbTech* tech = db_->getTech();
  for (odb::dbTechLayer* tech_layer : tech->getLayers()) {
    double metal_factor = 1.0;
    double diff_metal_factor = 1.0;

    double cut_factor = 1.0;
    double diff_cut_factor = 1.0;

    double side_metal_factor = 1.0;
    double diff_side_metal_factor = 1.0;

    double minus_diff_factor = 0.0;
    double plus_diff_factor = 0.0;
    double diff_metal_reduce_factor = 1.0;

    if (tech_layer->hasDefaultAntennaRule()) {
      dbTechLayerAntennaRule* antenna_rule
          = tech_layer->getDefaultAntennaRule();

      odb::dbStringProperty* layer_prop = odb::dbStringProperty::find(
          tech_layer, "LEF57_ANTENNAGATEPLUSDIFF");
      if (layer_prop != nullptr) {
        std::string gate_plus_diff_info = layer_prop->getValue();
        int start = 0;
        std::string gate_plus_diff = "";
        for (int i = 0; i < gate_plus_diff_info.size(); i++) {
          if (gate_plus_diff_info.at(i) == ' ') {
            gate_plus_diff
                = gate_plus_diff_info.substr(start, i - start - 1);
            start = i;
          }
        }
        plus_diff_factor = std::stod(gate_plus_diff);
      }
      if (antenna_rule->isAreaFactorDiffUseOnly()) {
        diff_metal_factor = antenna_rule->getAreaFactor();

        diff_cut_factor = antenna_rule->getAreaFactor();
      } else {
        metal_factor = antenna_rule->getAreaFactor();
        diff_metal_factor = antenna_rule->getAreaFactor();

        cut_factor = antenna_rule->getAreaFactor();
        diff_cut_factor = antenna_rule->getAreaFactor();
      }
      if (antenna_rule->isSideAreaFactorDiffUseOnly()) {
        diff_side_metal_factor = antenna_rule->getSideAreaFactor();
      } else {
        side_metal_factor = antenna_rule->getSideAreaFactor();
        diff_side_metal_factor = antenna_rule->getSideAreaFactor();
      }

      minus_diff_factor = antenna_rule->getAreaMinusDiffFactor();
    }

    AntennaModel layer_antenna = {tech_layer,
                                  metal_factor,
                                  diff_metal_factor,
                                  cut_factor,
                                  diff_cut_factor,
                                  side_metal_factor,
                                  diff_side_metal_factor,
                                  minus_diff_factor,
                                  plus_diff_factor,
                                  diff_metal_reduce_factor};
    layer_info_[tech_layer] = layer_antenna;
  }

  block_ = db_->getChip()->getBlock();
  dbu_per_micron_ = block_->getDbUnitsPerMicron();
}

dbWireGraph::Node* AntennaChecker::findSegmentRoot(dbWireGraph::Node* node,
                                                   int wire_level)
{
  if (!node->in_edge())
    return node;

  if (node->in_edge()->type() == dbWireGraph::Edge::Type::VIA
      || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA) {
    if (node->in_edge()->source()->layer()->getRoutingLevel() > wire_level)
      return node;

    dbWireGraph::Node* new_root
        = findSegmentRoot(node->in_edge()->source(), wire_level);

    if (new_root->layer()->getRoutingLevel() == wire_level)
      return new_root;
    else
      return node;
  }

  if (node->in_edge()->type() == dbWireGraph::Edge::Type::SEGMENT
      || node->in_edge()->type() == dbWireGraph::Edge::Type::SHORT)
    return findSegmentRoot(node->in_edge()->source(), wire_level);

  return node;
}

dbWireGraph::Node* AntennaChecker::findSegmentStart(dbWireGraph::Node* node)
{
  if ((node->object() && node->object()->getObjectType() == dbITermObj)
      || !node->in_edge())
    return node;
  else if (node->in_edge()->type() == dbWireGraph::Edge::Type::VIA
           || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA)
    return node;
  else if (node->in_edge()->type() == dbWireGraph::Edge::Type::SEGMENT
           || node->in_edge()->type() == dbWireGraph::Edge::Type::SHORT)
    return findSegmentStart(node->in_edge()->source());
  else
    return NULL;
}

bool AntennaChecker::ifSegmentRoot(dbWireGraph::Node* node, int wire_level)
{
  if ((node->object() && node->object()->getObjectType() == dbITermObj)
      || !node->in_edge())
    return true;
  else if (node->in_edge()->type() == dbWireGraph::Edge::Type::VIA
           || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA) {
    if (node->in_edge()->source()->layer()->getRoutingLevel() <= wire_level) {
      dbWireGraph::Node* new_root
          = findSegmentRoot(node->in_edge()->source(), wire_level);
      if (new_root->layer()->getRoutingLevel() == wire_level)
        return false;
      else
        return true;
    } else
      return true;
  } else
    return false;
}

void AntennaChecker::findWireBelowIterms(dbWireGraph::Node* node,
                                         double &iterm_gate_area,
                                         double &iterm_diff_area,
                                         int wire_level,
                                         std::set<dbITerm*>& iv,
                                         std::set<dbWireGraph::Node*>& nv)
{
  if (node->object() && node->object()->getObjectType() == dbITermObj) {
    dbITerm* iterm = dbITerm::getITerm(block_, node->object()->getId());
    if (iterm) {
      dbMTerm* mterm = iterm->getMTerm();
      iterm_gate_area += gateArea(mterm);
      iterm_diff_area += diffArea(mterm);
      iv.insert(iterm);
    }
  }

  nv.insert(node);

  if (node->in_edge()
      && node->in_edge()->source()->layer()->getRoutingLevel() <= wire_level) {
    if ((node->in_edge()->type() == dbWireGraph::Edge::Type::VIA
         || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA)
        && nv.find(node->in_edge()->source()) == nv.end()) {
      findWireBelowIterms(findSegmentStart(node->in_edge()->source()),
                          iterm_gate_area, iterm_diff_area,
                          wire_level, iv, nv);
    } else if ((node->in_edge()->type() == dbWireGraph::Edge::Type::SEGMENT
                || node->in_edge()->type() == dbWireGraph::Edge::Type::SHORT)
               && nv.find(node->in_edge()->source()) == nv.end()) {
      findWireBelowIterms(node->in_edge()->source(), iterm_gate_area, iterm_diff_area,
                          wire_level, iv, nv);
    }
  }

  dbWireGraph::Node::edge_iterator edge_itr;
  for (edge_itr = node->begin(); edge_itr != node->end(); ++edge_itr) {
    if ((*edge_itr)->type() == dbWireGraph::Edge::Type::VIA
        || (*edge_itr)->type() == dbWireGraph::Edge::Type::TECH_VIA) {
      if ((*edge_itr)->target()->layer()->getRoutingLevel() <= wire_level
          && nv.find((*edge_itr)->target()) == nv.end()) {
        findWireBelowIterms(findSegmentStart((*edge_itr)->target()),
                            iterm_gate_area, iterm_diff_area,
                            wire_level,
                            iv,
                            nv);
      }
    }

    else if (((*edge_itr)->type() == dbWireGraph::Edge::Type::SEGMENT
              || (*edge_itr)->type() == dbWireGraph::Edge::Type::SHORT)
             && nv.find((*edge_itr)->target()) == nv.end()) {
      findWireBelowIterms((*edge_itr)->target(), iterm_gate_area, iterm_diff_area,
                          wire_level, iv, nv);
    }
  }
}

std::pair<double, double> AntennaChecker::calculateWireArea(
    dbWireGraph::Node* node,
    int wire_level,
    std::set<dbWireGraph::Node*>& nv,
    std::set<dbWireGraph::Node*>& level_nodes)
{
  double wire_area = 0;
  double side_wire_area = 0;

  double wire_width = dbuToMicrons(node->layer()->getWidth());

  uint wire_thickness_dbu = 0;
  node->layer()->getThickness(wire_thickness_dbu);

  int start_x, start_y;
  int end_x, end_y;
  node->xy(start_x, start_y);

  vector<std::pair<dbWireGraph::Edge*, dbIoType>> edge_vec;
  if (node->in_edge() != nullptr
      && nv.find(node->in_edge()->source()) == nv.end())
    edge_vec.push_back({node->in_edge(), dbIoType::INPUT});

  dbWireGraph::Node::edge_iterator edge_it;
  int out_edges_count = 0;

  for (edge_it = node->begin(); edge_it != node->end(); edge_it++) {
    if (nv.find((*edge_it)->source()) == nv.end()) {
      out_edges_count++;
      edge_vec.push_back({*edge_it, dbIoType::OUTPUT});
    }
  }

  nv.insert(node);

  for (auto edge_info : edge_vec) {
    dbWireGraph::Edge* edge = edge_info.first;
    dbIoType edge_io_type = edge_info.second;
    if (edge->type() == dbWireGraph::Edge::Type::VIA
        || edge->type() == dbWireGraph::Edge::Type::TECH_VIA) {
      if (edge_io_type == dbIoType::INPUT) {
        wire_area += 0.5 * wire_width * wire_width;
        side_wire_area += dbuToMicrons(wire_thickness_dbu) * wire_width;

        if (edge->source()->layer()->getRoutingLevel() <= wire_level) {
          std::pair<double, double> areas
              = calculateWireArea(edge->source(), wire_level, nv, level_nodes);
          wire_area += areas.first;
          side_wire_area += areas.second;
        }
      }

      if (edge_io_type == dbIoType::OUTPUT) {
        if (out_edges_count == 1) {
          wire_area += 0.5 * wire_width * wire_width;
          side_wire_area += dbuToMicrons(wire_thickness_dbu) * wire_width;
        }

        if (edge->target()->layer()->getRoutingLevel() <= wire_level) {
          std::pair<double, double> areas
              = calculateWireArea(edge->target(), wire_level, nv, level_nodes);
          wire_area += areas.first;
          side_wire_area += areas.second;
        }
      }
    }

    if (edge->type() == dbWireGraph::Edge::Type::SEGMENT
        || edge->type() == dbWireGraph::Edge::Type::SHORT) {
      if (edge_io_type == dbIoType::INPUT) {
        if (node->layer()->getRoutingLevel() == wire_level) {
          level_nodes.insert(node);
          edge->source()->xy(end_x, end_y);
          wire_area += dbuToMicrons(abs(end_x - start_x) + abs(end_y - start_y))
                       * wire_width;
          side_wire_area
              += (dbuToMicrons(abs(end_x - start_x) + abs(end_y - start_y))
                  + wire_width)
                 * dbuToMicrons(wire_thickness_dbu) * 2;
        }

        std::pair<double, double> areas
            = calculateWireArea(edge->source(), wire_level, nv, level_nodes);
        wire_area += areas.first;
        side_wire_area += areas.second;
      }

      if (edge_io_type == dbIoType::OUTPUT) {
        if (node->layer()->getRoutingLevel() == wire_level) {
          level_nodes.insert(node);
          edge->target()->xy(end_x, end_y);
          wire_area += dbuToMicrons(abs(end_x - start_x) + abs(end_y - start_y))
                       * wire_width;
          side_wire_area
              += (dbuToMicrons(abs(end_x - start_x) + abs(end_y - start_y))
                  + wire_width)
                 * dbuToMicrons(wire_thickness_dbu) * 2;
        }

        std::pair<double, double> areas
            = calculateWireArea(edge->target(), wire_level, nv, level_nodes);
        wire_area += areas.first;
        side_wire_area += areas.second;
      }
    }
  }
  return {wire_area, side_wire_area};
}

double AntennaChecker::getViaArea(dbWireGraph::Edge* edge)
{
  double via_area = 0.0;
  if (edge->type() == dbWireGraph::Edge::Type::TECH_VIA) {
    dbWireGraph::TechVia* tech_via_edge = (dbWireGraph::TechVia*) edge;
    dbTechVia* tech_via = tech_via_edge->via();
    for (dbBox* box : tech_via->getBoxes()) {
      if (box->getTechLayer()->getType() == dbTechLayerType::CUT) {
        uint dx = box->getDX();
        uint dy = box->getDY();
        via_area = dbuToMicrons(dx) * dbuToMicrons(dy);
      }
    }
  } else if (edge->type() == dbWireGraph::Edge::Type::VIA) {
    dbWireGraph::Via* via_edge = (dbWireGraph::Via*) edge;
    dbVia* via = via_edge->via();
    for (dbBox* box : via->getBoxes()) {
      if (box->getTechLayer()->getType() == dbTechLayerType::CUT) {
        uint dx = box->getDX();
        uint dy = box->getDY();
        via_area = dbuToMicrons(dx) * dbuToMicrons(dy);
      }
    }
  }
  return via_area;
}

dbTechLayer* AntennaChecker::getViaLayer(dbWireGraph::Edge* edge)
{
  if (edge->type() == dbWireGraph::Edge::Type::TECH_VIA) {
    dbWireGraph::TechVia* tech_via_edge = (dbWireGraph::TechVia*) edge;
    dbTechVia* tech_via = tech_via_edge->via();
    for (dbBox* box : tech_via->getBoxes()) {
      if (box->getTechLayer()->getType() == dbTechLayerType::CUT)
        return box->getTechLayer();
    }
  } else if (edge->type() == dbWireGraph::Edge::Type::VIA) {
    dbWireGraph::Via* via_edge = (dbWireGraph::Via*) edge;
    dbVia* via = via_edge->via();
    for (dbBox* box : via->getBoxes()) {
      if (box->getTechLayer()->getType() == dbTechLayerType::CUT)
        return box->getTechLayer();
    }
  }
  return nullptr;
}

std::string AntennaChecker::getViaName(dbWireGraph::Edge* edge)
{
  if (edge->type() == dbWireGraph::Edge::Type::TECH_VIA) {
    dbWireGraph::TechVia* tech_via_edge = (dbWireGraph::TechVia*) edge;
    dbTechVia* tech_via = tech_via_edge->via();
    return tech_via->getName();
  } else if (edge->type() == dbWireGraph::Edge::Type::VIA) {
    dbWireGraph::Via* via_edge = (dbWireGraph::Via*) edge;
    dbVia* via = via_edge->via();
    return via->getName();
  }
  return nullptr;
}

double AntennaChecker::calculateViaArea(dbWireGraph::Node* node, int wire_level)
{
  double via_area = 0.0;
  if (node->in_edge()
      && (node->in_edge()->type() == dbWireGraph::Edge::Type::VIA
          || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA))
    if (node->in_edge()->source()->layer()->getRoutingLevel() > wire_level)
      via_area = via_area + getViaArea(node->in_edge());

  dbWireGraph::Node::edge_iterator edge_itr;
  for (edge_itr = node->begin(); edge_itr != node->end(); ++edge_itr) {
    if ((*edge_itr)->type() == dbWireGraph::Edge::Type::SEGMENT
        || (*edge_itr)->type() == dbWireGraph::Edge::Type::SHORT) {
      via_area = via_area + calculateViaArea((*edge_itr)->target(), wire_level);
    } else if ((*edge_itr)->type() == dbWireGraph::Edge::Type::VIA
               || (*edge_itr)->type() == dbWireGraph::Edge::Type::TECH_VIA) {
      if ((*edge_itr)->target()->layer()->getRoutingLevel() > wire_level) {
        via_area = via_area + getViaArea((*edge_itr));
      } else {
        via_area
            = via_area + calculateViaArea((*edge_itr)->target(), wire_level);
      }
    }
  }
  return via_area;
}

dbWireGraph::Edge* AntennaChecker::findVia(dbWireGraph::Node* node,
                                           int wire_level)
{
  if (node->in_edge()
      && (node->in_edge()->type() == dbWireGraph::Edge::Type::VIA
          || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA))
    if (node->in_edge()->source()->layer()->getRoutingLevel() > wire_level) {
      return node->in_edge();
    }
  dbWireGraph::Node::edge_iterator edge_itr;
  for (edge_itr = node->begin(); edge_itr != node->end(); ++edge_itr) {
    if ((*edge_itr)->type() == dbWireGraph::Edge::Type::SEGMENT
        || (*edge_itr)->type() == dbWireGraph::Edge::Type::SHORT) {
      dbWireGraph::Edge* via = findVia((*edge_itr)->target(), wire_level);
      if (via)
        return via;
    } else if ((*edge_itr)->type() == dbWireGraph::Edge::Type::VIA
               || (*edge_itr)->type() == dbWireGraph::Edge::Type::TECH_VIA) {
      if ((*edge_itr)->target()->layer()->getRoutingLevel() > wire_level) {
        return (*edge_itr);
      } else {
        dbWireGraph::Edge* via = findVia((*edge_itr)->target(), wire_level);
        if (via)
          return via;
      }
    }
  }
  return nullptr;
}

void AntennaChecker::findCarPath(dbWireGraph::Node* node,
                                 int wire_level,
                                 dbWireGraph::Node* goal,
                                 vector<dbWireGraph::Node*>& current_path,
                                 vector<dbWireGraph::Node*>& path_found)
{
  current_path.push_back(node);

  if (node == goal) {
    for (dbWireGraph::Node* node : current_path) {
      bool node_exists = false;
      for (dbWireGraph::Node* found_node : path_found) {
        if (node == found_node) {
          node_exists = true;
          break;
        }
      }
      if (!node_exists)
        path_found.push_back(node);
    }
  } else {
    if (node->in_edge()
        && (node->in_edge()->type() == dbWireGraph::Edge::Type::VIA
            || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA))
      if (node->in_edge()->source()->layer()->getRoutingLevel()
          < node->in_edge()->target()->layer()->getRoutingLevel()) {
        auto root_info = findSegmentRoot(
            node->in_edge()->source(),
            node->in_edge()->source()->layer()->getRoutingLevel());
        findCarPath(root_info,
                    node->in_edge()->source()->layer()->getRoutingLevel(),
                    goal,
                    current_path,
                    path_found);
      }
    dbWireGraph::Node::edge_iterator edge_itr;
    for (edge_itr = node->begin(); edge_itr != node->end(); ++edge_itr) {
      if ((*edge_itr)->type() == dbWireGraph::Edge::Type::VIA
          || (*edge_itr)->type() == dbWireGraph::Edge::Type::TECH_VIA) {
        if ((*edge_itr)->target()->layer()->getRoutingLevel() <= wire_level)
          findCarPath(findSegmentStart((*edge_itr)->target()),
                      wire_level,
                      goal,
                      current_path,
                      path_found);
      } else if ((*edge_itr)->type() == dbWireGraph::Edge::Type::SEGMENT
                 || (*edge_itr)->type() == dbWireGraph::Edge::Type::SHORT)
        findCarPath(
            (*edge_itr)->target(), wire_level, goal, current_path, path_found);
    }
  }
  current_path.pop_back();
}

vector<PARinfo>
AntennaChecker::buildWireParTable(const vector<dbWireGraph::Node*> &wire_roots)
{
  vector<PARinfo> PARtable;
  std::set<dbWireGraph::Node*> level_nodes;
  for (dbWireGraph::Node* wire_root : wire_roots) {
    if (level_nodes.find(wire_root) != level_nodes.end())
      continue;

    std::set<dbWireGraph::Node*> nv;
    std::pair<double, double> areas = calculateWireArea(
        wire_root, wire_root->layer()->getRoutingLevel(), nv, level_nodes);

    double wire_area = areas.first;
    double side_wire_area = areas.second;
    double iterm_gate_area = 0.0;
    double iterm_diff_area = 0.0;
    std::set<dbITerm*> iv;
    nv.clear();

    findWireBelowIterms(wire_root, iterm_gate_area, iterm_diff_area,
                        wire_root->layer()->getRoutingLevel(), iv, nv);

    PARinfo par_info = {wire_root,
      iv,
      wire_area,
      side_wire_area,
      iterm_gate_area,
      iterm_diff_area,
      0.0,
      0.0,
      0.0,
      0.0};
    PARtable.push_back(par_info);
  }

  for (PARinfo& par_info : PARtable)
    calculateParInfo(par_info);

  return PARtable;
}

double AntennaChecker::gateArea(dbMTerm *mterm)
{
  double max_gate_area = 0;
  if (mterm->hasDefaultAntennaModel()) {
    dbTechAntennaPinModel* pin_model = mterm->getDefaultAntennaModel();
    vector<std::pair<double, dbTechLayer*>> gate_areas;
    pin_model->getGateArea(gate_areas);

    for (auto gate_area_layer : gate_areas) {
      double gate_area = gate_area_layer.first;
      max_gate_area = std::max(max_gate_area, gate_area);
    }
  }
  return max_gate_area;
}

double AntennaChecker::getPwlFactor(dbTechLayerAntennaRule::pwl_pair pwl_info,
                                    double ref_value,
                                    double default_value)
{
  if (pwl_info.indices.size() != 0) {
    if (pwl_info.indices.size() == 1) {
      return pwl_info.ratios[0];
    } else {
      double pwl_info_index1 = pwl_info.indices[0];
      double pwl_info_ratio1 = pwl_info.ratios[0];
      double slope = 1.0;
      for (int i = 0; i < pwl_info.indices.size(); i++) {
        double pwl_info_index2 = pwl_info.indices[i];
        double pwl_info_ratio2 = pwl_info.ratios[i];
        slope = (pwl_info_ratio2 - pwl_info_ratio1)
          / (pwl_info_index2 - pwl_info_index1);

        if (ref_value >= pwl_info_index1 && ref_value < pwl_info_index2) {
          return pwl_info_ratio1 + (ref_value - pwl_info_index1) * slope;
        } else {
          pwl_info_index1 = pwl_info_index2;
          pwl_info_ratio1 = pwl_info_ratio2;
        }
      }
      return pwl_info_ratio1 + (ref_value - pwl_info_index1) * slope;
    }
  }
  return default_value;
}

void AntennaChecker::calculateParInfo(PARinfo& par_info)
{
  dbWireGraph::Node* wire_root = par_info.wire_root;
  odb::dbTechLayer* tech_layer = wire_root->layer();
  AntennaModel &am = layer_info_[tech_layer];

  double metal_factor = am.metal_factor;
  double diff_metal_factor = am.diff_metal_factor;
  double side_metal_factor = am.side_metal_factor;
  double diff_side_metal_factor = am.diff_side_metal_factor;

  double minus_diff_factor = am.minus_diff_factor;
  double plus_diff_factor = am.plus_diff_factor;

  double diff_metal_reduce_factor = am.diff_metal_reduce_factor;

  if (tech_layer->hasDefaultAntennaRule()) {
    dbTechLayerAntennaRule* antenna_rule = tech_layer->getDefaultAntennaRule();
    diff_metal_reduce_factor = getPwlFactor(
        antenna_rule->getAreaDiffReduce(), par_info.iterm_diff_area, 1.0);
  }

  if (par_info.iterm_gate_area == 0)
    return;

  if (par_info.iterm_diff_area != 0) {
    par_info.PAR = (diff_metal_factor * par_info.wire_area) / par_info.iterm_gate_area;
    par_info.PSR
        = (diff_side_metal_factor * par_info.side_wire_area) / par_info.iterm_gate_area;
    par_info.diff_PAR
        = (diff_metal_factor * par_info.wire_area * diff_metal_reduce_factor
           - minus_diff_factor * par_info.iterm_diff_area)
          / (par_info.iterm_gate_area + plus_diff_factor * par_info.iterm_diff_area);
    par_info.diff_PSR
        = (diff_side_metal_factor * par_info.side_wire_area * diff_metal_reduce_factor
           - minus_diff_factor * par_info.iterm_diff_area)
          / (par_info.iterm_gate_area + plus_diff_factor * par_info.iterm_diff_area);
  } else {
    par_info.PAR = (metal_factor * par_info.wire_area) / par_info.iterm_gate_area;
    par_info.PSR = (side_metal_factor * par_info.side_wire_area) / par_info.iterm_gate_area;
    par_info.diff_PAR = (metal_factor * par_info.wire_area * diff_metal_reduce_factor)
                        / par_info.iterm_gate_area;
    par_info.diff_PSR
        = (side_metal_factor * par_info.side_wire_area * diff_metal_reduce_factor)
          / (par_info.iterm_gate_area);
  }
}

vector<ARinfo>
AntennaChecker::buildWireCarTable(const vector<PARinfo> &PARtable,
                                  const vector<PARinfo> &VIA_PARtable,
                                  const vector<dbWireGraph::Node*> &gate_iterms)
{
  vector<ARinfo> CARtable;
  for (dbWireGraph::Node* gate : gate_iterms) {
    for (const PARinfo &ar : PARtable) {
      dbWireGraph::Node* wire_root = ar.wire_root;
      double par = ar.PAR;
      double psr = ar.PSR;
      double diff_par = ar.diff_PAR;
      double diff_psr = ar.diff_PSR;
      double diff_area = ar.iterm_diff_area;
      double car = 0.0;
      double csr = 0.0;
      double diff_car = 0.0;
      double diff_csr = 0.0;
      vector<dbWireGraph::Node*> current_path;
      vector<dbWireGraph::Node*> path_found;
      vector<dbWireGraph::Node*> car_wire_roots;

      findCarPath(wire_root,
                  wire_root->layer()->getRoutingLevel(),
                  gate,
                  current_path,
                  path_found);
      if (!path_found.empty()) {
        for (dbWireGraph::Node* node : path_found) {
          if (ifSegmentRoot(node, node->layer()->getRoutingLevel()))
            car_wire_roots.push_back(node);
        }

        vector<dbWireGraph::Node*>::iterator car_root_itr;
        for (car_root_itr = car_wire_roots.begin();
             car_root_itr != car_wire_roots.end();
             ++car_root_itr) {
          dbWireGraph::Node* car_root = *car_root_itr;
          for (const PARinfo &par_info : PARtable) {
            if (par_info.wire_root == car_root) {
              car = car + par_info.PAR;
              csr = csr + par_info.PSR;
              diff_car += par_info.diff_PAR;
              diff_csr += par_info.diff_PSR;
              break;
            }
          }
          dbTechLayer* wire_layer = wire_root->layer();
          if (wire_layer->hasDefaultAntennaRule()) {
            dbTechLayerAntennaRule* antenna_rule
                = wire_layer->getDefaultAntennaRule();
            if (antenna_rule->hasAntennaCumRoutingPlusCut()) {
              if (car_root->layer()->getRoutingLevel()
                  < wire_root->layer()->getRoutingLevel()) {
                for (const PARinfo &via_par_info : VIA_PARtable) {
                  if (via_par_info.wire_root == car_root) {
                    car += via_par_info.PAR;
                    diff_car += via_par_info.diff_PAR;
                    break;
                  }
                }
              }
            }
          }
        }

        ARinfo car_info = {wire_root,
          gate,
          par,
          psr,
          diff_par,
          diff_psr,
          car,
          csr,
          diff_car,
          diff_csr,
          diff_area};
        CARtable.push_back(car_info);
      }
    }
  }
  return CARtable;
}

vector<PARinfo>
AntennaChecker::buildViaParTable(const vector<dbWireGraph::Node*> &wire_roots)
{
  vector<PARinfo> VIA_PARtable;
  for (dbWireGraph::Node* wire_root : wire_roots) {
    double via_area
        = calculateViaArea(wire_root, wire_root->layer()->getRoutingLevel());
    double iterm_gate_area = 0.0;
    double iterm_diff_area = 0.0;
    std::set<dbITerm*> iv;
    std::set<dbWireGraph::Node*> nv;
    findWireBelowIterms(wire_root, iterm_gate_area, iterm_diff_area,
                        wire_root->layer()->getRoutingLevel(), iv, nv);
    double par = 0.0;
    double diff_par = 0.0;

    double cut_factor = 1.0;
    double diff_cut_factor = 1.0;

    double minus_diff_factor = 0.0;
    double plus_diff_factor = 0.0;
    double diff_metal_reduce_factor = 1.0;

    if (via_area != 0 && iterm_gate_area != 0) {
      dbTechLayer* layer = getViaLayer(
          findVia(wire_root, wire_root->layer()->getRoutingLevel()));

      AntennaModel &am = layer_info_[layer];
      minus_diff_factor = am.minus_diff_factor;
      plus_diff_factor = am.plus_diff_factor;
      diff_metal_reduce_factor = am.diff_metal_reduce_factor;
      if (layer->hasDefaultAntennaRule()) {
        dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
        diff_metal_reduce_factor = getPwlFactor(
            antenna_rule->getAreaDiffReduce(), iterm_diff_area, 1.0);
      }

      cut_factor = am.cut_factor;
      diff_cut_factor = am.diff_cut_factor;

      minus_diff_factor = am.minus_diff_factor;
      plus_diff_factor = am.plus_diff_factor;

      if (iterm_diff_area != 0) {
        par = (diff_cut_factor * via_area) / iterm_gate_area;
        diff_par = (diff_cut_factor * via_area * diff_metal_reduce_factor
                    - minus_diff_factor * iterm_diff_area)
                   / (iterm_gate_area + plus_diff_factor * iterm_diff_area);
      } else {
        par = (cut_factor * via_area) / iterm_gate_area;
        diff_par = (cut_factor * via_area * diff_metal_reduce_factor
                    - minus_diff_factor * iterm_diff_area)
                   / (iterm_gate_area + plus_diff_factor * iterm_diff_area);
      }
      PARinfo par_info = {wire_root, iv, 0.0, 0.0, 0.0, 0.0, par, 0.0, diff_par, 0.0};
      VIA_PARtable.push_back(par_info);
    }
  }
  return VIA_PARtable;
}

vector<ARinfo>
AntennaChecker::buildViaCarTable(const vector<PARinfo> &PARtable,
                                 const vector<PARinfo> &VIA_PARtable,
                                 const vector<dbWireGraph::Node*> &gate_iterms)
{
  vector<ARinfo> VIA_CARtable;
  for (dbWireGraph::Node* gate : gate_iterms) {
    int x, y;
    gate->xy(x, y);

    for (const PARinfo &ar : VIA_PARtable) {
      dbWireGraph::Node* wire_root = ar.wire_root;
      double par = ar.PAR;
      double diff_par = ar.diff_PAR;
      double diff_area = ar.iterm_diff_area;
      double car = 0.0;
      double diff_car = 0.0;
      vector<dbWireGraph::Node*> current_path;
      vector<dbWireGraph::Node*> path_found;
      vector<dbWireGraph::Node*> car_wire_roots;

      findCarPath(wire_root,
                  wire_root->layer()->getRoutingLevel(),
                  gate,
                  current_path,
                  path_found);
      if (!path_found.empty()) {
        for (dbWireGraph::Node* node : path_found) {
          int x, y;
          node->xy(x, y);
          if (ifSegmentRoot(node, node->layer()->getRoutingLevel()))
            car_wire_roots.push_back(node);
        }
        for (dbWireGraph::Node* car_root : car_wire_roots) {
          int x, y;
          car_root->xy(x, y);
          for (const PARinfo &via_par : VIA_PARtable) {
            if (via_par.wire_root == car_root) {
              car = car + via_par.PAR;
              diff_car = diff_car + via_par.diff_PAR;
              break;
            }
          }
          dbTechLayer* via_layer = getViaLayer(
              findVia(wire_root, wire_root->layer()->getRoutingLevel()));
          if (via_layer->hasDefaultAntennaRule()) {
            dbTechLayerAntennaRule* antenna_rule
                = via_layer->getDefaultAntennaRule();
            if (antenna_rule->hasAntennaCumRoutingPlusCut()) {
              for (const PARinfo &par : PARtable) {
                if (par.wire_root == car_root) {
                  car += par.PAR;
                  diff_car += par.diff_PAR;
                  break;
                }
              }
            }
          }
        }

        ARinfo car_info = {wire_root,
          gate,
          par,
          0.0,
          diff_par,
          0.0,
          car,
          0.0,
          diff_car,
          0.0,
          diff_area};
        VIA_CARtable.push_back(car_info);
      }
    }
  }
  return VIA_CARtable;
}

std::pair<bool, bool> AntennaChecker::checkWirePar(ARinfo AntennaRatio,
                                                   bool report,
                                                   bool verbose)
{
  dbTechLayer* layer = AntennaRatio.wire_root->layer();
  double par = AntennaRatio.PAR;
  double psr = AntennaRatio.PSR;
  double diff_par = AntennaRatio.diff_PAR;
  double diff_psr = AntennaRatio.diff_PSR;
  double diff_area = AntennaRatio.diff_area;

  bool checked = false;
  bool violated = false;

  bool par_violation = false;
  bool diff_par_violation = false;
  bool psr_violation = false;
  bool diff_psr_violation = false;

  if (layer->hasDefaultAntennaRule()) {
    dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();

    double PAR_ratio = antenna_rule->getPAR();
    dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
    double diffPAR_PWL_ratio = getPwlFactor(diffPAR, diff_area, 0);

    if (PAR_ratio != 0) {
      if (par > PAR_ratio) {
        par_violation = true;
        violated = true;
      }
    } else {
      if (diffPAR_PWL_ratio != 0) {
        checked = true;
        if (diff_par > diffPAR_PWL_ratio) {
          diff_par_violation = true;
          violated = true;
        }
      }
    }

    double PSR_ratio = antenna_rule->getPSR();
    dbTechLayerAntennaRule::pwl_pair diffPSR = antenna_rule->getDiffPSR();
    double diffPSR_PWL_ratio = getPwlFactor(diffPSR, diff_area, 0.0);
    if (PSR_ratio != 0) {
      if (psr > PSR_ratio) {
        psr_violation = true;
        violated = true;
      }
    } else {
      if (diffPSR_PWL_ratio != 0) {
        checked = true;
        if (diff_psr > diffPSR_PWL_ratio) {
          diff_psr_violation = true;
          violated = true;
        }
      }
    }

    if (report) {
      if (PAR_ratio != 0) {
        if (par_violation || verbose)
          logger_->report("    PAR: {:7.2f}{} Ratio: {:7.2f} (Area)",
                          par,
                          par_violation ? "*" : " ",
                          PAR_ratio);
      } else {
        if (diff_par_violation || verbose)
          logger_->report("    PAR: {:7.2f}{} Ratio: {:7.2f} (Area)",
                          diff_par,
                          diff_par_violation ? "*" : " ",
                          diffPAR_PWL_ratio);
      }

      if (PSR_ratio != 0) {
        if (psr_violation || verbose)
          logger_->report("    PAR: {:7.2f}{} Ratio: {:7.2f} (S.Area)",
                          psr,
                          psr_violation ? "*" : " ",
                          PSR_ratio);
      } else {
        if (diff_psr_violation || verbose)
          logger_->report("    PAR: {:7.2f}{} Ratio: {:7.2f} (S.Area)",
                          diff_psr,
                          diff_psr_violation ? "*" : " ",
                          diffPSR_PWL_ratio);
      }
    }
  }
  return {violated, checked};
}

std::pair<bool, bool> AntennaChecker::checkWireCar(ARinfo AntennaRatio,
                                                   bool par_checked,
                                                   bool report,
                                                   bool verbose)
{
  dbTechLayer* layer = AntennaRatio.wire_root->layer();
  double car = AntennaRatio.CAR;
  double csr = AntennaRatio.CSR;
  double diff_csr = AntennaRatio.diff_CSR;
  double diff_area = AntennaRatio.diff_area;

  bool checked = false;
  bool violated = false;

  bool car_violation = false;
  bool diff_car_violation = false;
  bool csr_violation = false;
  bool diff_csr_violation = false;

  if (layer->hasDefaultAntennaRule()) {
    dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();

    double CAR_ratio = par_checked ? 0.0 : antenna_rule->getCAR();
    dbTechLayerAntennaRule::pwl_pair diffCAR = antenna_rule->getDiffCAR();
    double diffCAR_PWL_ratio
        = par_checked ? 0.0 : getPwlFactor(diffCAR, diff_area, 0);
    if (CAR_ratio != 0) {
      if (car > CAR_ratio) {
        car_violation = true;
        violated = true;
      }
    } else {
      if (diffCAR_PWL_ratio != 0) {
        checked = true;
        if (car > diffCAR_PWL_ratio) {
          diff_car_violation = true;
          violated = true;
        }
      }
    }

    double CSR_ratio = par_checked ? 0.0 : antenna_rule->getCSR();
    dbTechLayerAntennaRule::pwl_pair diffCSR = antenna_rule->getDiffCSR();
    double diffCSR_PWL_ratio
        = par_checked ? 0.0 : getPwlFactor(diffCSR, diff_area, 0.0);
    if (CSR_ratio != 0) {
      if (csr > CSR_ratio) {
        csr_violation = true;
        violated = true;
      }
    } else {
      if (diffCSR_PWL_ratio != 0) {
        checked = true;
        if (diff_csr > diffCSR_PWL_ratio) {
          diff_csr_violation = true;
          violated = true;
        }
      }
    }

    if (report) {
      if (CAR_ratio != 0) {
        if (car_violation || verbose)
          logger_->report("    CAR: {:7.2f}{} Ratio: {:7.2f} (C.Area)",
                          car,
                          car_violation ? "*" : " ",
                          CAR_ratio);
      } else {
        if (diff_car_violation || verbose)
          logger_->report("    CAR: {:7.2f}{} Ratio: {:7.2f} (C.Area)",
                          car,
                          diff_car_violation ? "*" : " ",
                          diffCAR_PWL_ratio);
      }

      if (CSR_ratio != 0) {
        if (car_violation || verbose)
          logger_->report("    CAR: {:7.2f}{} Ratio: {:7.2f} (C.S.Area)",
                          csr,
                          csr_violation ? "*" : " ",
                          CSR_ratio);
      } else {
        if (diff_car_violation || verbose)
          logger_->report("    CAR: {:7.2f}{} Ratio: {:7.2f} (C.S.Area)",
                          diff_csr,
                          diff_csr_violation ? "*" : " ",
                          diffCSR_PWL_ratio);
      }
    }
  }
  return {violated, checked};
}

bool AntennaChecker::checkViaPar(ARinfo AntennaRatio,
                                 bool report,
                                 bool verbose)
{
  dbTechLayer* layer = getViaLayer(
                                   findVia(AntennaRatio.wire_root,
                                           AntennaRatio.wire_root->layer()->getRoutingLevel()));
  double par = AntennaRatio.PAR;
  double diff_par = AntennaRatio.diff_PAR;
  double diff_area = AntennaRatio.diff_area;

  bool violated = false;
  bool par_violation = false;
  bool diff_par_violation = false;

  if (layer->hasDefaultAntennaRule()) {
    dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
    double PAR_ratio = antenna_rule->getPAR();

    dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
    double diffPAR_PWL_ratio = getPwlFactor(diffPAR, diff_area, 0);
    if (PAR_ratio != 0) {
      if (par > PAR_ratio) {
        par_violation = true;
        violated = true;
      }
    } else {
      if (diffPAR_PWL_ratio != 0) {
        if (diff_par > diffPAR_PWL_ratio) {
          diff_par_violation = true;
          violated = true;
        }
      }
    }

    if (report) {
      if (PAR_ratio != 0) {
        if (par_violation || verbose)
          logger_->report("    PAR: {:7.2f}{} Ratio: {:7.2f} (Area)",
                          par,
                          par_violation ? "*" : " ",
                          PAR_ratio);
      } else {
        if (diff_par_violation || verbose)
          logger_->report("    PAR: {:7.2f}{} Ratio: {:7.2f} (Area)",
                          par,
                          diff_par_violation ? "*" : " ",
                          diffPAR_PWL_ratio);
      }
    }
  }
  return violated;
}

bool AntennaChecker::checkViaCar(ARinfo AntennaRatio,
                                 bool report,
                                 bool verbose)
{
  dbTechLayer* layer = getViaLayer(findVia(AntennaRatio.wire_root,
                                           AntennaRatio.wire_root->layer()->getRoutingLevel()));
  double car = AntennaRatio.CAR;
  double diff_area = AntennaRatio.diff_area;

  bool violated = false;

  bool car_violation = false;
  bool diff_car_violation = false;

  if (layer->hasDefaultAntennaRule()) {
    dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
    double CAR_ratio = antenna_rule->getCAR();

    dbTechLayerAntennaRule::pwl_pair diffCAR = antenna_rule->getDiffCAR();
    double diffCAR_PWL_ratio = getPwlFactor(diffCAR, diff_area, 0);

    if (CAR_ratio != 0) {
      if (car > CAR_ratio) {
        car_violation = true;
        violated = true;
      }
    } else {
      if (diffCAR_PWL_ratio != 0) {
        if (car > diffCAR_PWL_ratio) {
          diff_car_violation = true;
          violated = true;
        }
      }
    }

    if (report) {
      if (CAR_ratio != 0) {
        if (car_violation || verbose)
          logger_->report("    CAR: {:7.2f}{} Ratio: {:7.2f} (C.Area)",
                          car,
                          car_violation ? "*" : " ",
                          CAR_ratio);
      } else {
        if (diff_car_violation || verbose)
          logger_->report("    CAR: {:7.2f}{} Ratio: {:7.2f} (C.Area)",
                          car,
                          diff_car_violation ? "*" : " ",
                          diffCAR_PWL_ratio);
      }
    }
  }
  return violated;
}

vector<dbWireGraph::Node*>
AntennaChecker::findWireRoots(dbWire* wire)
{
  vector<dbWireGraph::Node*> wire_roots;
  vector<dbWireGraph::Node*> gate_iterms;
  findWireRoots(wire, wire_roots, gate_iterms);
  return wire_roots;
}

void AntennaChecker::findWireRoots(dbWire* wire,
                                   // Return values.
                                   vector<dbWireGraph::Node*> &wire_roots,
                                   vector<dbWireGraph::Node*> &gate_iterms)
{
  dbWireGraph graph;
  graph.decode(wire);
  dbWireGraph::node_iterator node_itr;
  for (node_itr = graph.begin_nodes(); node_itr != graph.end_nodes();
       ++node_itr) {
    dbWireGraph::Node* node = *node_itr;

    auto wire_root_info
      = findSegmentRoot(node, node->layer()->getRoutingLevel());
    dbWireGraph::Node* wire_root = wire_root_info;

    if (wire_root) {
      bool found_root = false;
      for (dbWireGraph::Node* root : wire_roots) {
        if (found_root)
          break;
        else {
          if (root == wire_root)
            found_root = true;
        }
      }
      if (!found_root) {
        wire_roots.push_back(wire_root_info);
      }
    }
    if (node->object()
        && node->object()->getObjectType() == dbITermObj) {
      dbITerm* iterm = dbITerm::getITerm(block_, node->object()->getId());
      dbMTerm* mterm = iterm->getMTerm();
      if (mterm->getIoType() == dbIoType::INPUT
          && gateArea(mterm) > 0.0)
        gate_iterms.push_back(node);
    }
  }
}

void AntennaChecker::checkNet(dbNet* net,
                              bool report_if_no_violation,
                              bool verbose,
                              // Return values.
                              int &net_violation_count,
                              int &pin_violation_count)
{
  dbWire* wire = net->getWire();
  if (wire) {
    vector<dbWireGraph::Node*> wire_roots;
    vector<dbWireGraph::Node*> gate_nodes;
    findWireRoots(wire, wire_roots, gate_nodes);

    vector<PARinfo> PARtable = buildWireParTable(wire_roots);
    vector<PARinfo> VIA_PARtable = buildViaParTable(wire_roots);
    vector<ARinfo> CARtable = buildWireCarTable(PARtable, VIA_PARtable, gate_nodes);
    vector<ARinfo> VIA_CARtable = buildViaCarTable(PARtable, VIA_PARtable, gate_nodes);

    bool violation = false;
    unordered_set<dbWireGraph::Node*> violated_gates;
    for (dbWireGraph::Node* gate : gate_nodes)
      checkGate(gate, CARtable, VIA_CARtable,
                false, verbose, violation, violated_gates);

    if (violation) {
      net_violation_count++;
      pin_violation_count += violated_gates.size();
    }

    // Repeat with reporting.
    if (violation || report_if_no_violation) {
      logger_->report("Net {}", net->getConstName());

      for (dbWireGraph::Node* gate : gate_nodes) {
        checkGate(gate, CARtable, VIA_CARtable,
                  true, verbose, violation, violated_gates);
      }
      logger_->report("");
    }
  }
}

void AntennaChecker::checkGate(dbWireGraph::Node* gate,
                               vector<ARinfo> &CARtable,
                               vector<ARinfo> &VIA_CARtable,
                               bool report,
                               bool verbose,
                               // Return values.
                               bool &violation,
                               unordered_set<dbWireGraph::Node*> &violated_gates)
{
  bool first_pin_violation = true;
  for (auto ar : CARtable) {
    if (ar.GateNode == gate) {
      auto wire_PAR_violation = checkWirePar(ar, false, verbose);
      auto wire_CAR_violation = checkWireCar(ar, wire_PAR_violation.second,
                                             false, verbose);
      bool wire_violation = wire_PAR_violation.first || wire_CAR_violation.first;
      violation |= wire_violation;
      if (wire_violation)
        violated_gates.insert(gate);

      if (report) {
        if (wire_violation || verbose) {
          if (first_pin_violation) {
            dbITerm* iterm = dbITerm::getITerm(block_, gate->object()->getId());
            dbMTerm* mterm = iterm->getMTerm();
            logger_->report("  {}/{} ({})",
                            iterm->getInst()->getConstName(),
                            mterm->getConstName(),
                            mterm->getMaster()->getConstName());
          }
          logger_->report("    {}",
                          ar.wire_root->layer()->getConstName());
          first_pin_violation = false;
        }
        checkWirePar(ar, true, verbose);
        checkWireCar(ar, wire_PAR_violation.second, true, verbose);
        if (wire_violation || verbose)
          logger_->report("");
      }
    }
  }
  for (auto via_ar : VIA_CARtable) {
    if (via_ar.GateNode == gate) {
      bool VIA_PAR_violation = checkViaPar(via_ar, false, verbose);
      bool VIA_CAR_violation = checkViaCar(via_ar, false, verbose);
      bool via_violation = VIA_PAR_violation || VIA_CAR_violation;
      violation |= via_violation;
      if (via_violation)
        violated_gates.insert(gate);

      if (report) {
        if (via_violation || verbose) {
          dbWireGraph::Edge* via = findVia(via_ar.wire_root,
                                           via_ar.wire_root->layer()->getRoutingLevel());
          logger_->report("    {}", getViaName(via).c_str());
        }
        checkViaPar(via_ar, true, verbose);
        checkViaCar(via_ar, true, verbose);
        if (via_violation || verbose)
          logger_->report("");
      }
    }
  }
}

int AntennaChecker::checkAntennas(const char *net_name,
                                  bool verbose)
{
  initAntennaRules();

  bool grt_routes = global_router_->haveRoutes();
  bool drt_routes = haveRoutedNets();
  bool use_grt_routes = (grt_routes && !drt_routes);
  if (!grt_routes && !drt_routes)
    logger_->error(ANT, 8, "No detailed or global routing found. Run global_route or detailed_route first.");

  if (use_grt_routes)
    global_router_->makeNetWires();
  else
    // detailed routes
    odb::orderWires(block_, false);

  int net_violation_count = 0;
  int pin_violation_count = 0;

  if (strlen(net_name) > 0) {
    dbNet *net = block_->findNet(net_name);
    if (net
        && !net->isSpecial())
      checkNet(net, true, verbose,
               net_violation_count,
               pin_violation_count);
    else
      logger_->error(ANT, 12, "-net {} not Found.", net_name);
  }
  else {
    for (dbNet* net : block_->getNets()) {
      if (!net->isSpecial()) {
        checkNet(net, false, verbose,
                 net_violation_count,
                 pin_violation_count);
      }
    }
  }

  logger_->info(ANT, 2, "Found {} net violations.", net_violation_count);
  logger_->info(ANT, 1, "Found {} pin violations.", pin_violation_count);

  if (use_grt_routes)
    global_router_->destroyNetWires();

  net_violation_count_ = net_violation_count;
  return net_violation_count;
}

int AntennaChecker::antennaViolationCount() const
{
  return net_violation_count_;
}

bool AntennaChecker::haveRoutedNets()
{
  for (dbNet* net : block_->getNets()) {
    if (!net->isSpecial()
        && net->getWireType() == dbWireType::ROUTED
        && net->getWire())
      return true;
  }
  return false;
}
          
void AntennaChecker::findWireRootIterms(dbWireGraph::Node* node,
                                        int wire_level,
                                        vector<dbITerm*>& gates)
{
  double iterm_gate_area = 0.0;
  double iterm_diff_area = 0.0;
  std::set<dbITerm*> iv;
  std::set<dbWireGraph::Node*> nv;

  findWireBelowIterms(node, iterm_gate_area, iterm_diff_area,
                      wire_level, iv, nv);
  gates.assign(iv.begin(), iv.end());
}

vector<std::pair<double, vector<dbITerm*>>>
AntennaChecker::parMaxWireLength(dbNet* net, int layer)
{
  vector<std::pair<double, vector<dbITerm*>>> par_wires;
  if (net->isSpecial())
    return par_wires;
  dbWire* wire = net->getWire();
  if (wire != nullptr) {
    dbWireGraph graph;
    graph.decode(wire);

    std::set<dbWireGraph::Node*> level_nodes;
    vector<dbWireGraph::Node*> wire_roots = findWireRoots(wire);
    for (dbWireGraph::Node* wire_root : wire_roots) {
      odb::dbTechLayer* tech_layer = wire_root->layer();
      if (level_nodes.find(wire_root) == level_nodes.end()
          && tech_layer->getRoutingLevel() == layer) {
        double max_length = 0;
        std::set<dbWireGraph::Node*> nv;
        std::pair<double, double> areas =
          calculateWireArea(wire_root, tech_layer->getRoutingLevel(),
                            nv, level_nodes);
        double wire_area = areas.first;
        double iterm_gate_area = 0.0;
        double iterm_diff_area = 0.0;
        std::set<dbITerm*> iv;
        nv.clear();
        findWireBelowIterms(wire_root, iterm_gate_area, iterm_diff_area,
                            tech_layer->getRoutingLevel(), iv, nv);
        double wire_width = dbuToMicrons(tech_layer->getWidth());

        AntennaModel &am = layer_info_[tech_layer];
        double metal_factor = am.metal_factor;
        double diff_metal_factor = am.diff_metal_factor;

        double minus_diff_factor = am.minus_diff_factor;
        double plus_diff_factor = am.plus_diff_factor;
        double diff_metal_reduce_factor = am.diff_metal_reduce_factor;

        if (iterm_gate_area != 0 && tech_layer->hasDefaultAntennaRule()) {
          dbTechLayerAntennaRule* antenna_rule
            = tech_layer->getDefaultAntennaRule();
          dbTechLayerAntennaRule::pwl_pair diff_metal_reduce_factor_pwl
            = antenna_rule->getAreaDiffReduce();
          diff_metal_reduce_factor
            = getPwlFactor(diff_metal_reduce_factor_pwl, iterm_diff_area, 1.0);

          double PAR_ratio = antenna_rule->getPAR();
          if (PAR_ratio != 0) {
            if (iterm_diff_area != 0)
              max_length
                = (PAR_ratio * iterm_gate_area - diff_metal_factor * wire_area)
                / wire_width;
            else
              max_length
                = (PAR_ratio * iterm_gate_area - metal_factor * wire_area)
                / wire_width;
          } else {
            dbTechLayerAntennaRule::pwl_pair diffPAR
              = antenna_rule->getDiffPAR();
            double diffPAR_ratio = getPwlFactor(diffPAR, iterm_diff_area, 0.0);
            if (iterm_diff_area != 0)
              max_length
                = (diffPAR_ratio
                   * (iterm_gate_area + plus_diff_factor * iterm_diff_area)
                   - (diff_metal_factor * wire_area * diff_metal_reduce_factor
                      - minus_diff_factor * iterm_diff_area))
                / wire_width;
            else
              max_length
                = (diffPAR_ratio
                   * (iterm_gate_area + plus_diff_factor * iterm_diff_area)
                   - (metal_factor * wire_area * diff_metal_reduce_factor
                      - minus_diff_factor * iterm_diff_area))
                / wire_width;
          }
          if (max_length != 0) {
            vector<dbITerm*> gates;
            findWireRootIterms(
                               wire_root, wire_root->layer()->getRoutingLevel(), gates);
            std::pair<double, vector<dbITerm*>> par_wire
              = std::make_pair(max_length, gates);
            par_wires.push_back(par_wire);
          }
        }
      }
    }
  }
  return par_wires;
}

void AntennaChecker::checkMaxLength(const char* net_name, int layer)
{
  dbNet* net = block_->findNet(net_name);
  if (!net->isSpecial()) {
    vector<std::pair<double, vector<dbITerm*>>> par_max_length_wires
        = parMaxWireLength(net, layer);
    for (auto par_wire : par_max_length_wires) {
      logger_->report("Net {}: Routing Level: {}, Max Length for PAR: {:3.2f}",
                      net_name,
                      layer,
                      par_wire.first);
    }
  }
}

bool AntennaChecker::checkViolation(PARinfo &par_info, dbTechLayer* layer)
{
  double par = par_info.PAR;
  double psr = par_info.PSR;
  double diff_par = par_info.diff_PAR;
  double diff_psr = par_info.diff_PSR;
  double diff_area = par_info.iterm_diff_area;

  if (layer->hasDefaultAntennaRule()) {
    dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
    double PAR_ratio = antenna_rule->getPAR();
    if (PAR_ratio != 0) {
      if (par > PAR_ratio)
        return true;
    } else {
      dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
      double diffPAR_ratio = getPwlFactor(diffPAR, diff_area, 0.0);
      if (diffPAR_ratio != 0 && diff_par > diffPAR_ratio)
        return true;
    }

    double PSR_ratio = antenna_rule->getPSR();
    if (PSR_ratio != 0) {
      if (psr > PSR_ratio)
        return true;
    } else {
      dbTechLayerAntennaRule::pwl_pair diffPSR = antenna_rule->getDiffPSR();
      double diffPSR_ratio = getPwlFactor(diffPSR, diff_area, 0.0);
      if (diffPSR_ratio != 0 && diff_psr > diffPSR_ratio)
        return true;
    }
  }

  return false;
}

vector<Violation> AntennaChecker::getAntennaViolations(dbNet* net,
                                                       dbMTerm* diode_mterm)
{
  double diode_diff_area = 0.0;
  if (diode_mterm) 
    diode_diff_area = diffArea(diode_mterm);

  vector<Violation> antenna_violations;
  if (net->isSpecial())
    return antenna_violations;
  dbWire* wire = net->getWire();
  dbWireGraph graph;
  if (wire) {
    auto wire_roots = findWireRoots(wire);

    vector<PARinfo> PARtable = buildWireParTable(wire_roots);
    for (PARinfo &par_info : PARtable) {
      dbTechLayer* layer = par_info.wire_root->layer();
      bool wire_PAR_violation = checkViolation(par_info, layer);

      if (wire_PAR_violation) {
        vector<dbITerm*> gates;
        findWireRootIterms(par_info.wire_root,
                           layer->getRoutingLevel(), gates);
        int diode_count_per_gate = 0;
        if (diode_mterm && antennaRatioDiffDependent(layer)) {
          while (wire_PAR_violation) {
            par_info.iterm_diff_area += diode_diff_area * gates.size();
            diode_count_per_gate++;
            calculateParInfo(par_info);
            wire_PAR_violation = checkViolation(par_info, layer);
            if (diode_count_per_gate > max_diode_count_per_gate) {
              logger_->warn(ANT, 9, "Net {} requires more than {} diodes per gate to repair violations.",
                            net->getConstName(),
                            max_diode_count_per_gate);
              break;
            }
          }
        }
        Violation antenna_violation
          = {layer->getRoutingLevel(), gates, diode_count_per_gate};
        antenna_violations.push_back(antenna_violation);
      }
    }
  }
  return antenna_violations;
}

bool AntennaChecker::antennaRatioDiffDependent(dbTechLayer* layer)
{
  if (layer->hasDefaultAntennaRule()) {
    dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
    dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
    dbTechLayerAntennaRule::pwl_pair diffPSR = antenna_rule->getDiffPSR();
    return diffPAR.indices.size() > 1
      || diffPSR.indices.size() > 1;
  }
  return false;
}

double
AntennaChecker::diffArea(dbMTerm *mterm)
{
  double max_diff_area = 0.0;
  vector<std::pair<double, dbTechLayer*>> diff_areas;
  mterm->getDiffArea(diff_areas);
  for (auto area_layer : diff_areas) {
    double diff_area = area_layer.first;
    max_diff_area = std::max(max_diff_area, diff_area);
  }
  return max_diff_area;
}

vector<std::pair<double, vector<dbITerm*>>>
AntennaChecker::getViolatedWireLength(dbNet* net, int routing_level)
{
  vector<std::pair<double, vector<dbITerm*>>> violated_wires;
  if (net->isSpecial() || net->getWire() == nullptr)
    return violated_wires;
  dbWire* wire = net->getWire();

  dbWireGraph graph;
  std::set<dbWireGraph::Node*> level_nodes;
  for (dbWireGraph::Node* wire_root : findWireRoots(wire)) {
    odb::dbTechLayer* tech_layer = wire_root->layer();
    if (level_nodes.find(wire_root) == level_nodes.end()
        && tech_layer->getRoutingLevel() == routing_level) {
      std::set<dbWireGraph::Node*> nv;
      auto areas =
        calculateWireArea(wire_root, tech_layer->getRoutingLevel(), nv, level_nodes);
      double wire_area = areas.first;
      double iterm_gate_area = 0.0;
      double iterm_diff_area = 0.0;

      std::set<dbITerm*> iv;
      nv.clear();
      findWireBelowIterms(wire_root, iterm_gate_area, iterm_diff_area,
                          tech_layer->getRoutingLevel(), iv, nv);
      if (iterm_gate_area == 0)
        continue;

      double wire_width = dbuToMicrons(tech_layer->getWidth());

      AntennaModel &am = layer_info_[tech_layer];
      double metal_factor = am.metal_factor;
      double diff_metal_factor = am.diff_metal_factor;

      double minus_diff_factor = am.minus_diff_factor;
      double plus_diff_factor = am.plus_diff_factor;
      double diff_metal_reduce_factor = am.diff_metal_reduce_factor;

      if (wire_root->layer()->hasDefaultAntennaRule()) {
        dbTechLayerAntennaRule* antenna_rule
            = tech_layer->getDefaultAntennaRule();
        diff_metal_reduce_factor = getPwlFactor(
            antenna_rule->getAreaDiffReduce(), iterm_diff_area, 1.0);

        double par = 0;
        double diff_par = 0;

        if (iterm_diff_area != 0) {
          par = (diff_metal_factor * wire_area) / iterm_gate_area;
          diff_par = (diff_metal_factor * wire_area * diff_metal_reduce_factor
                      - minus_diff_factor * iterm_diff_area)
                     / (iterm_gate_area + plus_diff_factor * iterm_diff_area);
        } else {
          par = (metal_factor * wire_area) / iterm_gate_area;
          diff_par = (metal_factor * wire_area * diff_metal_reduce_factor)
                     / iterm_gate_area;
        }

        double cut_length = 0;
        double PAR_ratio = antenna_rule->getPAR();
        if (PAR_ratio != 0) {
          if (par > PAR_ratio) {
            if (iterm_diff_area != 0)
              cut_length = ((par - PAR_ratio) * iterm_gate_area
                            - diff_metal_factor * wire_area)
                           / wire_width;
            else
              cut_length = ((par - PAR_ratio) * iterm_gate_area
                            - metal_factor * wire_area)
                           / wire_width;
          }

        } else {
          dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
          double diffPAR_ratio = getPwlFactor(diffPAR, iterm_diff_area, 0.0);
          if (iterm_diff_area != 0)
            cut_length
                = ((diff_par - diffPAR_ratio)
                       * (iterm_gate_area + plus_diff_factor * iterm_diff_area)
                   - (diff_metal_factor * wire_area * diff_metal_reduce_factor
                      - minus_diff_factor * iterm_diff_area))
                  / wire_width;
          else
            cut_length
                = ((diff_par - diffPAR_ratio)
                       * (iterm_gate_area + plus_diff_factor * iterm_diff_area)
                   - (metal_factor * wire_area * diff_metal_reduce_factor
                      - minus_diff_factor * iterm_diff_area))
                  / wire_width;
        }

        if (cut_length != 0) {
          vector<dbITerm*> gates;
          findWireRootIterms(wire_root, routing_level, gates);
          std::pair<double, vector<dbITerm*>> violated_wire
              = std::make_pair(cut_length, gates);
          violated_wires.push_back(violated_wire);
        }
      }
    }
  }
  return violated_wires;
}

void AntennaChecker::findMaxWireLength()
{
  dbNet* max_wire_net = nullptr;
  double max_wire_length = 0.0;

  for (dbNet* net : block_->getNets()) {
    dbWire* wire = net->getWire();
    if (wire && !net->isSpecial()) {
      dbWireGraph graph;
      graph.decode(wire);

      double wire_length = 0;
      dbWireGraph::edge_iterator edge_itr;
      for (edge_itr = graph.begin_edges(); edge_itr != graph.end_edges();
           ++edge_itr) {
        dbWireGraph::Edge* edge = *edge_itr;
        int x1, y1, x2, y2;
        edge->source()->xy(x1, y1);
        edge->target()->xy(x2, y2);
        if (edge->type() == dbWireGraph::Edge::Type::SEGMENT
            || edge->type() == dbWireGraph::Edge::Type::SHORT)
          wire_length += dbuToMicrons((abs(x2 - x1) + abs(y2 - y1)));
      }

      if (wire_length > max_wire_length) {
        max_wire_length = wire_length;
        max_wire_net = net;
      }
    }
  }
  if (max_wire_net)
    logger_->report("net {} length {}",
                    max_wire_net->getConstName(),
                    max_wire_length);
}

}  // namespace ant
