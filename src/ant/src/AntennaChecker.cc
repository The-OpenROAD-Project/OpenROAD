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
  bool violated_net;
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

AntennaChecker::AntennaChecker()
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

void AntennaChecker::loadAntennaRules()
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
  dbu_per_micron_ = db_->getChip()->getBlock()->getDbUnitsPerMicron();
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
  if (checkIterm(node, iterm_gate_area, iterm_diff_area))
    iv.insert(
        dbITerm::getITerm(db_->getChip()->getBlock(), node->object()->getId()));

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
      findWireBelowIterms(
          (*edge_itr)->target(), iterm_gate_area, iterm_diff_area, wire_level, iv, nv);
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

  uint wire_thickness_uint = 0;
  node->layer()->getThickness(wire_thickness_uint);

  int start_x, start_y;
  int end_x, end_y;
  node->xy(start_x, start_y);

  std::vector<std::pair<dbWireGraph::Edge*, std::string>> edge_vec;
  if (node->in_edge() != nullptr
      && nv.find(node->in_edge()->source()) == nv.end())
    edge_vec.push_back({node->in_edge(), "IN"});

  dbWireGraph::Node::edge_iterator edge_it;
  int out_edges_count = 0;

  for (edge_it = node->begin(); edge_it != node->end(); edge_it++) {
    if (nv.find((*edge_it)->source()) == nv.end()) {
      out_edges_count++;
      edge_vec.push_back({*edge_it, "OUT"});
    }
  }

  nv.insert(node);

  for (auto edge_info : edge_vec) {
    dbWireGraph::Edge* edge = edge_info.first;
    std::string edge_io_type = edge_info.second;
    if (edge->type() == dbWireGraph::Edge::Type::VIA
        || edge->type() == dbWireGraph::Edge::Type::TECH_VIA) {
      if (edge_io_type.compare("IN") == 0) {
        wire_area += 0.5 * wire_width * wire_width;
        side_wire_area += dbuToMicrons(wire_thickness_uint) * wire_width;

        if (edge->source()->layer()->getRoutingLevel() <= wire_level) {
          std::pair<double, double> areas
              = calculateWireArea(edge->source(), wire_level, nv, level_nodes);
          wire_area += areas.first;
          side_wire_area += areas.second;
        }
      }

      if (edge_io_type.compare("OUT") == 0) {
        if (out_edges_count == 1) {
          wire_area += 0.5 * wire_width * wire_width;
          side_wire_area += dbuToMicrons(wire_thickness_uint) * wire_width;
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
      if (edge_io_type.compare("IN") == 0) {
        if (node->layer()->getRoutingLevel() == wire_level) {
          level_nodes.insert(node);
          edge->source()->xy(end_x, end_y);
          wire_area += dbuToMicrons(abs(end_x - start_x) + abs(end_y - start_y))
                       * wire_width;
          side_wire_area
              += (dbuToMicrons(abs(end_x - start_x) + abs(end_y - start_y))
                  + wire_width)
                 * dbuToMicrons(wire_thickness_uint) * 2;
        }

        std::pair<double, double> areas
            = calculateWireArea(edge->source(), wire_level, nv, level_nodes);
        wire_area += areas.first;
        side_wire_area += areas.second;
      }

      if (edge_io_type.compare("OUT") == 0) {
        if (node->layer()->getRoutingLevel() == wire_level) {
          level_nodes.insert(node);
          edge->target()->xy(end_x, end_y);
          wire_area += dbuToMicrons(abs(end_x - start_x) + abs(end_y - start_y))
                       * wire_width;
          side_wire_area
              += (dbuToMicrons(abs(end_x - start_x) + abs(end_y - start_y))
                  + wire_width)
                 * dbuToMicrons(wire_thickness_uint) * 2;
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
                                 std::vector<dbWireGraph::Node*>& current_path,
                                 std::vector<dbWireGraph::Node*>& path_found)
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

void AntennaChecker::buildWireParTable(
    std::vector<PARinfo>& PARtable,
    std::vector<dbWireGraph::Node*> wireroots_info)
{
  std::set<dbWireGraph::Node*> level_nodes;
  for (dbWireGraph::Node* wireroot : wireroots_info) {
    if (level_nodes.find(wireroot) != level_nodes.end())
      continue;

    std::set<dbWireGraph::Node*> nv;
    std::pair<double, double> areas = calculateWireArea(
        wireroot, wireroot->layer()->getRoutingLevel(), nv, level_nodes);

    double wire_area = areas.first;
    double side_wire_area = areas.second;
    double iterm_gate_area = 0.0;
    double iterm_diff_area = 0.0;
    std::set<dbITerm*> iv;
    nv.clear();

    findWireBelowIterms(wireroot, iterm_gate_area, iterm_diff_area,
                        wireroot->layer()->getRoutingLevel(), iv, nv);

    PARinfo par_info = {wireroot,
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
}

bool AntennaChecker::checkIterm(dbWireGraph::Node* node,
                                double &iterm_gate_area,
                                double &iterm_diff_area)
{
  if (node->object() && node->object()->getObjectType() == dbITermObj) {
    dbITerm* iterm = dbITerm::getITerm(db_->getChip()->getBlock(),
                                       node->object()->getId());
    dbMTerm* mterm = iterm->getMTerm();
    if (mterm->hasDefaultAntennaModel()) {
      dbTechAntennaPinModel* pinmodel = mterm->getDefaultAntennaModel();
      std::vector<std::pair<double, dbTechLayer*>> gate_area;
      pinmodel->getGateArea(gate_area);

      std::vector<std::pair<double, dbTechLayer*>>::iterator gate_area_iter;
      double max_gate_area = 0;
      for (gate_area_iter = gate_area.begin();
           gate_area_iter != gate_area.end();
           gate_area_iter++) {
        max_gate_area = std::max(max_gate_area, (*gate_area_iter).first);
      }
      iterm_gate_area += + max_gate_area;
    }

    iterm_diff_area += + maxDiffArea(mterm);
    return true;
  } else
    return false;
}

double AntennaChecker::getPwlFactor(dbTechLayerAntennaRule::pwl_pair pwl_info,
                                    double ref_val,
                                    double def)
{
  if (pwl_info.indices.size() != 0) {
    if (pwl_info.indices.size() == 1) {
      return pwl_info.ratios[0];
    } else {
      double pwl_info_indice1 = pwl_info.indices[0];
      double pwl_info_ratio1 = pwl_info.ratios[0];
      double slope_factor = 1.0;
      for (int i = 0; i < pwl_info.indices.size(); i++) {
        double pwl_info_indice2 = pwl_info.indices[i];
        double pwl_info_ratio2 = pwl_info.ratios[i];
        slope_factor = (pwl_info_ratio2 - pwl_info_ratio1)
          / (pwl_info_ratio2 - pwl_info_indice1);

        if (ref_val >= pwl_info_indice1 && ref_val < pwl_info_indice2) {
          return slope_factor * (ref_val - pwl_info_indice1) + pwl_info_ratio1;
        } else {
          pwl_info_indice1 = pwl_info_indice2;
          pwl_info_ratio1 = pwl_info_ratio2;
        }
      }
      return slope_factor * (ref_val - pwl_info_indice1) + pwl_info_ratio1;
    }
  }
  return def;
}

void AntennaChecker::calculateParInfo(PARinfo& par_info)
{
  dbWireGraph::Node* wireroot = par_info.wire_root;
  odb::dbTechLayer* tech_layer = wireroot->layer();
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

void AntennaChecker::buildWireCarTable(
    std::vector<ARinfo>& CARtable,
    std::vector<PARinfo> PARtable,
    std::vector<PARinfo> VIA_PARtable,
    std::vector<dbWireGraph::Node*> gate_iterms)
{
  for (dbWireGraph::Node* gate : gate_iterms) {
    for (PARinfo &ar : PARtable) {
      dbWireGraph::Node* wireroot = ar.wire_root;
      double par = ar.PAR;
      double psr = ar.PSR;
      double diff_par = ar.diff_PAR;
      double diff_psr = ar.diff_PSR;
      double diff_area = ar.iterm_diff_area;
      double car = 0.0;
      double csr = 0.0;
      double diff_car = 0.0;
      double diff_csr = 0.0;
      std::vector<dbWireGraph::Node*> current_path;
      std::vector<dbWireGraph::Node*> path_found;
      std::vector<dbWireGraph::Node*> car_wireroots;

      findCarPath(wireroot,
                  wireroot->layer()->getRoutingLevel(),
                  gate,
                  current_path,
                  path_found);
      if (!path_found.empty()) {
        for (dbWireGraph::Node* node : path_found) {
          if (ifSegmentRoot(node, node->layer()->getRoutingLevel()))
            car_wireroots.push_back(node);
        }

        std::vector<dbWireGraph::Node*>::iterator car_root_itr;
        for (car_root_itr = car_wireroots.begin();
             car_root_itr != car_wireroots.end();
             ++car_root_itr) {
          dbWireGraph::Node* car_root = *car_root_itr;
          for (PARinfo &par_info : PARtable) {
            if (par_info.wire_root == car_root) {
              car = car + par_info.PAR;
              csr = csr + par_info.PSR;
              diff_car += par_info.diff_PAR;
              diff_csr += par_info.diff_PSR;
              break;
            }
          }
          dbTechLayer* wire_layer = wireroot->layer();
          if (wire_layer->hasDefaultAntennaRule()) {
            dbTechLayerAntennaRule* antenna_rule
                = wire_layer->getDefaultAntennaRule();
            if (antenna_rule->hasAntennaCumRoutingPlusCut()) {
              if (car_root->layer()->getRoutingLevel()
                  < wireroot->layer()->getRoutingLevel()) {
                for (PARinfo &via_par_info : VIA_PARtable) {
                  if (via_par_info.wire_root == car_root) {
                    car = car + via_par_info.PAR;
                    diff_car = diff_car + via_par_info.diff_PAR;
                    break;
                  }
                }
              }
            }
          }
        }

        ARinfo car_info = {wireroot,
          gate,
          false,
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
}

void AntennaChecker::buildViaParTable(
    std::vector<PARinfo>& VIA_PARtable,
    std::vector<dbWireGraph::Node*> wireroots_info)
{
  for (dbWireGraph::Node* wireroot : wireroots_info) {
    double via_area
        = calculateViaArea(wireroot, wireroot->layer()->getRoutingLevel());
    double iterm_gate_area = 0.0;
    double iterm_diff_area = 0.0;
    std::set<dbITerm*> iv;
    std::set<dbWireGraph::Node*> nv;
    findWireBelowIterms(wireroot, iterm_gate_area, iterm_diff_area,
                        wireroot->layer()->getRoutingLevel(), iv, nv);
    double par = 0.0;
    double diff_par = 0.0;

    double cut_factor = 1.0;
    double diff_cut_factor = 1.0;

    double minus_diff_factor = 0.0;
    double plus_diff_factor = 0.0;
    double diff_metal_reduce_factor = 1.0;

    if (via_area != 0 && iterm_gate_area != 0) {
      dbTechLayer* layer = getViaLayer(
          findVia(wireroot, wireroot->layer()->getRoutingLevel()));

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
      PARinfo par_info = {wireroot, iv, 0.0, 0.0, 0.0, 0.0, par, 0.0, diff_par, 0.0};
      VIA_PARtable.push_back(par_info);
    }
  }
}

void AntennaChecker::buildViaCarTable(
    std::vector<ARinfo>& VIA_CARtable,
    std::vector<PARinfo> PARtable,
    std::vector<PARinfo> VIA_PARtable,
    std::vector<dbWireGraph::Node*> gate_iterms)
{
  for (dbWireGraph::Node* gate : gate_iterms) {
    int x, y;
    gate->xy(x, y);

    for (PARinfo &ar : VIA_PARtable) {
      dbWireGraph::Node* wireroot = ar.wire_root;
      double par = ar.PAR;
      double diff_par = ar.diff_PAR;
      double diff_area = ar.iterm_diff_area;
      double car = 0.0;
      double diff_car = 0.0;
      std::vector<dbWireGraph::Node*> current_path;
      std::vector<dbWireGraph::Node*> path_found;
      std::vector<dbWireGraph::Node*> car_wireroots;

      findCarPath(wireroot,
                  wireroot->layer()->getRoutingLevel(),
                  gate,
                  current_path,
                  path_found);
      if (!path_found.empty()) {
        for (dbWireGraph::Node* node : path_found) {
          int x, y;
          node->xy(x, y);
          if (ifSegmentRoot(node, node->layer()->getRoutingLevel()))
            car_wireroots.push_back(node);
        }
        for (dbWireGraph::Node* car_root : car_wireroots) {
          int x, y;
          car_root->xy(x, y);
          for (PARinfo &via_par : VIA_PARtable) {
            if (via_par.wire_root == car_root) {
              car = car + via_par.PAR;
              diff_car = diff_car + via_par.diff_PAR;
              break;
            }
          }
          dbTechLayer* via_layer = getViaLayer(
              findVia(wireroot, wireroot->layer()->getRoutingLevel()));
          if (via_layer->hasDefaultAntennaRule()) {
            dbTechLayerAntennaRule* antenna_rule
                = via_layer->getDefaultAntennaRule();
            if (antenna_rule->hasAntennaCumRoutingPlusCut()) {
              for (PARinfo &par : PARtable) {
                if (par.wire_root == car_root) {
                  car = car + par.PAR;
                  diff_car = diff_car + par.diff_PAR;
                  break;
                }
              }
            }
          }
        }

        ARinfo car_info = {wireroot,
          gate,
          false,
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
}

std::pair<bool, bool> AntennaChecker::checkWirePar(ARinfo AntennaRatio,
                                                   bool report_violating_nets,
                                                   bool print)
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
    // pre-check if there are violations, in case a simple report is required
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

    if (!print) {
      return {violated, checked};
    }

    // generate final report, depnding on if report_violating_nets is needed
    if (!violated && report_violating_nets)
      return {violated, checked};
    else {
      if (report_violating_nets) {
        if (par_violation) {
          fprintf(stream_,
                  "    PAR: %7.2f* Ratio: %7.2f (Area)\n",
                  par,
                  PAR_ratio);
        } else if (diff_par_violation) {
          fprintf(stream_,
                  "    PAR: %7.2f* Ratio: %7.2f (Area)\n",
                  diff_par,
                  diffPAR_PWL_ratio);
        } else if (psr_violation) {
          fprintf(stream_,
                  "    PAR: %7.2f* Ratio: %7.2f (S.Area)\n",
                  psr,
                  PSR_ratio);
        } else {
          fprintf(stream_,
                  "    PAR: %7.2f* Ratio: %7.2f (S.Area)\n",
                  diff_psr,
                  diffPSR_PWL_ratio);
        }
      } else {
        if (PAR_ratio != 0) {
          fprintf(stream_, "    PAR: %7.2f%s Ratio: %7.2f (Area)\n",
                  par,
                  par_violation ? "*" : " ",
                  PAR_ratio);
        } else {
          fprintf(stream_, "    PAR: %7.2f%s Ratio: %7.2f (Area)\n",
                  diff_par,
                  diff_par_violation ? "*" : " ",
                  diffPAR_PWL_ratio);
        }

        if (PSR_ratio != 0) {
          fprintf(stream_, "    PAR: %7.2f%s Ratio: %7.2f (S.Area)\n",
                  psr,
                  psr_violation ? "*" : " ",
                  PSR_ratio);
        } else {
          fprintf(stream_, "    PAR: %7.2f%s Ratio: %7.2f (S.Area)\n",
                  diff_psr,
                  diff_psr_violation ? "*" : " ",
                  diffPSR_PWL_ratio);
        }
      }
    }
  }
  return {violated, checked};
}

std::pair<bool, bool> AntennaChecker::checkWireCar(ARinfo AntennaRatio,
                                                   bool par_checked,
                                                   bool report_violating_nets,
                                                   bool print)
{
  dbTechLayer* layer = AntennaRatio.wire_root->layer();
  double car = AntennaRatio.CAR;
  double csr = AntennaRatio.CSR;
  double diff_car = AntennaRatio.diff_CAR;
  double diff_csr = AntennaRatio.diff_CSR;
  double diff_area = AntennaRatio.diff_area;

  bool checked = 0;
  bool violated = 0;

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

    if (!print) {
      return {violated, checked};
    }

    if (!violated && report_violating_nets) {
      return {violated, checked};
    } else {
      if (report_violating_nets) {
        if (car_violation) {
          fprintf(stream_,
                  "    CAR: %7.2f* Ratio: %7.2f (Area)\n",
                  car,
                  CAR_ratio);
        } else if (diff_car_violation) {
          fprintf(stream_,
                  "    CAR: %7.2f* Ratio: %7.2f (Area)\n",
                  diff_car,
                  diffCAR_PWL_ratio);
        } else if (csr_violation) {
          fprintf(stream_,
                  "    CAR: %7.2f* Ratio: %7.2f (C.S.Area)\n",
                  csr,
                  CSR_ratio);
        } else {
          fprintf(stream_,
                  "    CAR: %7.2f* Ratio: %7.2f (C.S.Area)\n",
                  diff_csr,
                  diffCSR_PWL_ratio);
        }
      } else {
        if (CAR_ratio != 0) {
          fprintf(stream_, "    CAR: %7.2f%s Ratio: %7.2f (C.Area)\n",
                  car,
                  car_violation ? "*" : " ",
                  CAR_ratio);
        } else {
          fprintf(stream_, "    CAR: %7.2f%s Ratio: %7.2f (C.Area)\n",
                  car,
                  diff_car_violation ? "*" : " ",
                  diffCAR_PWL_ratio);
        }

        if (CSR_ratio != 0) {
          fprintf(stream_, "    CAR: %7.2f%s Ratio: %7.2f (C.S.Area)\n",
                  csr,
                  csr_violation ? "*" : " ",
                  CSR_ratio);
        } else {
          fprintf(stream_, "    CAR: %7.2f%s Ratio: %7.2f (C.S.Area)\n",
                  diff_csr,
                  diff_csr_violation ? "*" : " ",
                  diffCSR_PWL_ratio);
        }
      }
    }
  }
  return {violated, checked};
}

bool AntennaChecker::checkViaPar(ARinfo AntennaRatio,
                                 bool report_violating_nets,
                                 bool print)
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

    if (!print) {
      return violated;
    }

    if (!violated && report_violating_nets) {
      return false;
    } else {
      if (report_violating_nets) {
        if (par_violation) {
          fprintf(stream_,
                  "    PAR: %7.2f* Ratio: %7.2f (Area)\n",
                  par,
                  PAR_ratio);
        } else {
          fprintf(stream_,
                  "    PAR: %7.2f* Ratio: %7.2f (Area)\n",
                  par,
                  diffPAR_PWL_ratio);
        }
      } else {
        if (PAR_ratio != 0) {
          fprintf(stream_, "    PAR: %7.2f%s Ratio: %7.2f (Area)\n",
                  par,
                  par_violation ? "*" : " ",
                  PAR_ratio);
        } else {
          fprintf(stream_, "    PAR: %7.2f%s Ratio: %7.2f (Area)\n",
                  par,
                  diff_par_violation ? "*" : " ",
                  diffPAR_PWL_ratio);
        }
      }
    }
  }
  return violated;
}

bool AntennaChecker::checkViaCar(ARinfo AntennaRatio,
                                 bool report_violating_nets,
                                 bool print)
{
  dbTechLayer* layer = getViaLayer(
      findVia(AntennaRatio.wire_root,
              AntennaRatio.wire_root->layer()->getRoutingLevel()));
  double car = AntennaRatio.CAR;
  double diff_area = AntennaRatio.diff_area;

  bool violated = 0;

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

    if (!print) {
      return violated;
    }

    if (!violated && report_violating_nets) {
      return false;
    } else {
      if (report_violating_nets) {
        if (car_violation) {
          fprintf(stream_,
                  "    CAR: %7.2f* Ratio: %7.2f (C.Area)\n",
                  car,
                  CAR_ratio);
        } else {
          fprintf(stream_,
                  "    CAR: %7.2f* Ratio: %7.2f (C.Area)\n",
                  car,
                  diffCAR_PWL_ratio);
        }
      } else {
        if (CAR_ratio != 0) {
          fprintf(stream_, "    CAR: %7.2f%s Ratio: %7.2f (C.Area)\n",
                  car,
                  car_violation ? "*" : " ",
                  CAR_ratio);
        } else {
          fprintf(stream_, "    CAR: %7.2f%s Ratio: %7.2f (C.Area)\n",
                  car,
                  diff_car_violation ? "*" : " ",
                  diffCAR_PWL_ratio);
        }
      }
    }
  }
  return violated;
}

void AntennaChecker::getAntennaRatio(std::string report_filename,
                                     bool report_violating_nets,
                                     // Return values.
                                     int &pin_violation_count,
                                     int &net_violation_count)
{
  net_violation_count = 0;
  pin_violation_count = 0;

  stream_ = fopen(report_filename.c_str(), "w");
  if (stream_) {
    checkDiodeCell();

    dbSet<dbNet> nets = db_->getChip()->getBlock()->getNets();
    if (nets.empty())
      return;

    for (dbNet* net : nets) {
      if (net->isSpecial())
        continue;
      dbWire* wire = net->getWire();
      if (wire) {
        dbWireGraph graph;
        graph.decode(wire);
        dbWireGraph::node_iterator node_itr;
        dbWireGraph::edge_iterator edge_itr;

        std::vector<dbWireGraph::Node*> wireroots_info;
        std::vector<dbWireGraph::Node*> gate_iterms;

        for (node_itr = graph.begin_nodes(); node_itr != graph.end_nodes();
             ++node_itr) {
          dbWireGraph::Node* node = *node_itr;

          auto wireroot_info
            = findSegmentRoot(node, node->layer()->getRoutingLevel());
          dbWireGraph::Node* wireroot = wireroot_info;

          if (wireroot) {
            bool found_root = 0;
            for (dbWireGraph::Node* root : wireroots_info) {
              if (found_root)
                break;
              else {
                if (root == wireroot)
                  found_root = 1;
              }
            }
            if (!found_root) {
              wireroots_info.push_back(wireroot_info);
            }
          }
          if (node->object()
              && node->object()->getObjectType() == dbITermObj) {
            dbITerm* iterm = dbITerm::getITerm(db_->getChip()->getBlock(),
                                               node->object()->getId());
            dbMTerm* mterm = iterm->getMTerm();
            if (mterm->getIoType() == dbIoType::INPUT)
              if (mterm->hasDefaultAntennaModel())
                gate_iterms.push_back(node);
          }
        }

        std::vector<PARinfo> PARtable;
        buildWireParTable(PARtable, wireroots_info);

        std::vector<PARinfo> VIA_PARtable;
        buildViaParTable(VIA_PARtable, wireroots_info);

        std::vector<ARinfo> CARtable;
        buildWireCarTable(CARtable, PARtable, VIA_PARtable, gate_iterms);

        std::vector<ARinfo> VIA_CARtable;
        buildViaCarTable(VIA_CARtable, PARtable, VIA_PARtable, gate_iterms);

        bool violated_wire = 0;
        bool violated_VIA = 0;

        std::set<dbWireGraph::Node*> violated_iterms;

        std::vector<dbWireGraph::Node*>::iterator gate_itr;
        bool print_net = true;
        for (gate_itr = gate_iterms.begin(); gate_itr != gate_iterms.end();
             ++gate_itr) {
          dbWireGraph::Node* gate = *gate_itr;

          dbITerm* iterm = dbITerm::getITerm(db_->getChip()->getBlock(),
                                             gate->object()->getId());
          dbMTerm* mterm = iterm->getMTerm();

          bool violation = false;
          unordered_set<dbWireGraph::Node*> violated_gates;

          for (auto ar : CARtable) {
            if (ar.GateNode == gate) {
              auto wire_PAR_violation
                = checkWirePar(ar, report_violating_nets, false);
              auto wire_CAR_violation = checkWireCar(ar, wire_PAR_violation.second,
                                                     report_violating_nets, false);
              bool wire_violation
                = wire_PAR_violation.first || wire_CAR_violation.first;
              violation |= wire_violation;
              if (wire_violation)
                violated_gates.insert(gate);
            }
          }
          for (auto via_ar : VIA_CARtable) {
            if (via_ar.GateNode == gate) {
              bool VIA_PAR_violation
                = checkViaPar(via_ar, report_violating_nets, false);
              bool VIA_CAR_violation
                = checkViaCar(via_ar, report_violating_nets, false);
              bool via_violation = VIA_PAR_violation || VIA_CAR_violation;
              violation |= via_violation;
              if (via_violation
                  && (violated_gates.find(gate) == violated_gates.end()))
                violated_gates.insert(gate);
            }
          }

          if ((!report_violating_nets || violation) && print_net) {
            fprintf(stream_, "\nNet %s\n", net->getConstName());
            print_net = false;
          }

          if (!report_violating_nets
              || (violated_gates.find(gate) != violated_gates.end())) {
            fprintf(stream_,
                    "  %s/%s (%s)\n",
                    iterm->getInst()->getConstName(),
                    mterm->getConstName(),
                    mterm->getMaster()->getConstName());
          }

          for (auto ar : CARtable) {
            if (ar.GateNode == gate) {
              auto wire_PAR_violation
                = checkWirePar(ar, report_violating_nets, false);
              auto wire_CAR_violation = checkWireCar(
                                                     ar, wire_PAR_violation.second, report_violating_nets, false);
              if (wire_PAR_violation.first || wire_CAR_violation.first
                  || !report_violating_nets) {
                fprintf(stream_, "    %s\n",
                        ar.wire_root->layer()->getConstName());
              }
              wire_PAR_violation
                = checkWirePar(ar, report_violating_nets, true);
              wire_CAR_violation = checkWireCar(
                                                ar, wire_PAR_violation.second, report_violating_nets, true);
              if (wire_PAR_violation.first || wire_CAR_violation.first) {
                violated_wire = 1;
                if (violated_iterms.find(gate) == violated_iterms.end())
                  violated_iterms.insert(gate);
              }
              if (wire_PAR_violation.first || wire_CAR_violation.first
                  || !report_violating_nets) {
                fprintf(stream_, "\n");
              }
            }
          }

          for (auto via_ar : VIA_CARtable) {
            if (via_ar.GateNode == gate) {
              dbWireGraph::Edge* via
                = findVia(via_ar.wire_root,
                          via_ar.wire_root->layer()->getRoutingLevel());

              bool VIA_PAR_violation
                = checkViaPar(via_ar, report_violating_nets, false);
              bool VIA_CAR_violation
                = checkViaCar(via_ar, report_violating_nets, false);
              if (VIA_PAR_violation || VIA_CAR_violation
                  || !report_violating_nets) {
                fprintf(stream_, "  %s:\n", getViaName(via).c_str());
              }
              VIA_PAR_violation
                = checkViaPar(via_ar, report_violating_nets, true);
              VIA_CAR_violation
                = checkViaCar(via_ar, report_violating_nets, true);
              if (VIA_PAR_violation || VIA_CAR_violation) {
                violated_VIA = 1;
                if (violated_iterms.find(gate) == violated_iterms.end())
                  violated_iterms.insert(gate);
              }
              if (VIA_PAR_violation || VIA_CAR_violation
                  || !report_violating_nets) {
                fprintf(stream_, "\n");
              }
            }
          }
        }

        if (violated_wire || violated_VIA) {
          net_violation_count++;
          pin_violation_count += violated_iterms.size();
        }
      }
    }
    fprintf(stream_, "Number of pins violated: %d\n", pin_violation_count);
    fprintf(stream_, "Number of nets violated: %d\n", net_violation_count);
    fclose(stream_);
  } else {
    logger_->error(
                   ANT, 7, "Cannot open report file ({}) for writing", report_filename);
  }
}

void AntennaChecker::checkDiodeCell()
{
  for (dbLib *lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      dbMasterType type = master->getType();
      if (type == dbMasterType::CORE_ANTENNACELL) {
        double max_diff_area = 0.0;
        for (dbMTerm* mterm : master->getMTerms())
          max_diff_area = std::max(max_diff_area, maxDiffArea(mterm));

        if (max_diff_area != 0.0)
          fprintf(stream_,
                  "Found antenna cell with diffusion area %f\n",
                  max_diff_area);
        else
          fprintf(stream_,
                  "Warning: found antenna cell but no diffusion area is specified\n");

        return;
      }
    }
  }

  fprintf(stream_,
          "Warning: no LEF master with for CORE ANTENNACELL found. This message can be "
          "ignored if not repairing antennas.\n");
}

int AntennaChecker::checkAntennas(std::string report_file,
                                  bool report_violating_nets)
{
  bool grt_routes = global_router_->haveRoutes();
  bool drt_routes = haveRoutedNets();
  if (!grt_routes && !drt_routes)
    logger_->error(ANT, 8, "No detailed or global routing found. Run global_route or detailed_route first.");

  bool use_grt_routes = (grt_routes && !drt_routes);

  if (use_grt_routes)
    global_router_->makeNetWires();

  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::orderWires(block, false);

  loadAntennaRules();
  int pin_violation_count;
  int net_violation_count;
  int net_count;
  getAntennaRatio(report_file, report_violating_nets,
                  pin_violation_count,
                  net_violation_count);
  logger_->info(ANT, 1, "Found {} pin violations.", pin_violation_count);
  logger_->info(ANT, 2, "Found {} net violations.",
                net_violation_count,
                net_count);

  if (use_grt_routes)
    global_router_->destroyNetWires();

  return net_violation_count;
}

bool AntennaChecker::haveRoutedNets()
{
  for (dbNet* net : db_->getChip()->getBlock()->getNets()) {
    if (!net->isSpecial()
        && net->getWireType() == dbWireType::ROUTED
        && net->getWire())
      return true;
  }
  return false;
}
          
void AntennaChecker::findWirerootIterms(dbWireGraph::Node* node,
                                        int wire_level,
                                        std::vector<dbITerm*>& gates)
{
  double iterm_gate_area = 0.0;
  double iterm_diff_area = 0.0;
  std::set<dbITerm*> iv;
  std::set<dbWireGraph::Node*> nv;

  findWireBelowIterms(node, iterm_gate_area, iterm_diff_area,
                      wire_level, iv, nv);
  gates.assign(iv.begin(), iv.end());
}

std::vector<std::pair<double, std::vector<dbITerm*>>>
AntennaChecker::parMaxWireLength(dbNet* net, int layer)
{
  std::vector<std::pair<double, std::vector<dbITerm*>>> par_wires;
  if (net->isSpecial())
    return par_wires;
  dbWire* wire = net->getWire();
  if (wire != nullptr) {
    dbWireGraph graph;
    graph.decode(wire);

    dbWireGraph::node_iterator node_itr;
    dbWireGraph::edge_iterator edge_itr;

    std::vector<dbWireGraph::Node*> wireroots;
    auto wireroots_info = getWireroots(graph);

    std::set<dbWireGraph::Node*> level_nodes;
    for (auto root_itr : wireroots_info) {
      dbWireGraph::Node* wireroot = root_itr;
      odb::dbTechLayer* tech_layer = wireroot->layer();
      if (level_nodes.find(wireroot) == level_nodes.end()
          && tech_layer->getRoutingLevel() == layer) {
        double max_length = 0;
        std::set<dbWireGraph::Node*> nv;
        std::pair<double, double> areas = calculateWireArea(
            wireroot, tech_layer->getRoutingLevel(), nv, level_nodes);
        double wire_area = areas.first;
        double iterm_gate_area = 0.0;
        double iterm_diff_area = 0.0;
        std::set<dbITerm*> iv;
        nv.clear();
        findWireBelowIterms(
            wireroot, iterm_gate_area, iterm_diff_area,
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
            std::vector<dbITerm*> gates;
            findWirerootIterms(
                wireroot, wireroot->layer()->getRoutingLevel(), gates);
            std::pair<double, std::vector<dbITerm*>> par_wire
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
  dbNet* net = db_->getChip()->getBlock()->findNet(net_name);
  if (!net->isSpecial()) {
    std::vector<std::pair<double, std::vector<dbITerm*>>> par_max_length_wires
        = parMaxWireLength(net, layer);
    for (auto par_wire : par_max_length_wires) {
      logger_->report("Net {}: Routing Level: {}, Max Length for PAR: {:3.2f}",
                      net_name,
                      layer,
                      par_wire.first);
    }
  }
}

std::vector<dbWireGraph::Node*> AntennaChecker::getWireroots(dbWireGraph graph)
{
  std::vector<dbWireGraph::Node*> wireroots_info;
  dbWireGraph::node_iterator node_itr;

  for (node_itr = graph.begin_nodes(); node_itr != graph.end_nodes();
       ++node_itr) {
    dbWireGraph::Node* node = *node_itr;
    auto wireroot_info
        = findSegmentRoot(node, node->layer()->getRoutingLevel());
    dbWireGraph::Node* wireroot = wireroot_info;
    if (wireroot) {
      bool found_root = 0;
      for (auto root_itr = wireroots_info.begin();
           root_itr != wireroots_info.end();
           ++root_itr) {
        if (found_root)
          break;
        else {
          if (*root_itr == wireroot)
            found_root = 1;
        }
      }
      if (!found_root)
        wireroots_info.push_back(wireroot_info);
    }
  }
  return wireroots_info;
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

std::vector<ViolationInfo> AntennaChecker::getNetAntennaViolations(dbNet* net,
                                                                   dbMTerm* diode_mterm)
{
  double diode_diff_area = 0.0;
  if (diode_mterm) 
    diode_diff_area = maxDiffArea(diode_mterm);

  std::vector<ViolationInfo> antenna_violations;
  if (net->isSpecial())
    return antenna_violations;
  dbWire* wire = net->getWire();
  dbWireGraph graph;
  if (wire) {
    graph.decode(wire);

    auto wireroots_info = getWireroots(graph);

    std::vector<PARinfo> PARtable;
    buildWireParTable(PARtable, wireroots_info);
    for (PARinfo &par_info : PARtable) {
      dbTechLayer* layer = par_info.wire_root->layer();
      bool wire_PAR_violation = checkViolation(par_info, layer);

      if (wire_PAR_violation) {
        std::vector<dbITerm*> gates;
        findWirerootIterms(par_info.wire_root,
                           layer->getRoutingLevel(), gates);
        int required_diode_count = 0;
        if (diode_mterm && antennaRatioDiffDependent(layer)) {
          while (wire_PAR_violation) {
            par_info.iterm_diff_area += diode_diff_area * gates.size();
            required_diode_count += gates.size();
            calculateParInfo(par_info);
            wire_PAR_violation = checkViolation(par_info, layer);
            if (required_diode_count > repair_max_diode_count) {
              logger_->warn(ANT, 8, "Net {} requires more than {} diodes to repair violations.",
                            net->getConstName(),
                            repair_max_diode_count);
              break;
            }
          }
        }
        ViolationInfo antenna_violation
            = {layer->getRoutingLevel(), gates, required_diode_count};
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
AntennaChecker::maxDiffArea(dbMTerm *mterm)
{
  double max_diff_area = 0.0;
  std::vector<std::pair<double, dbTechLayer*>> diff_areas;
  mterm->getDiffArea(diff_areas);
  for (auto area_layer : diff_areas) {
    double diff_area = area_layer.first;
    max_diff_area = std::max(max_diff_area, diff_area);
  }
  return max_diff_area;
}

std::vector<std::pair<double, std::vector<dbITerm*>>>
AntennaChecker::getViolatedWireLength(dbNet* net, int routing_level)
{
  std::vector<std::pair<double, std::vector<dbITerm*>>> violated_wires;
  if (net->isSpecial() || net->getWire() == nullptr)
    return violated_wires;
  dbWire* wire = net->getWire();

  dbWireGraph graph;
  graph.decode(wire);

  std::set<dbWireGraph::Node*> level_nodes;
  for (dbWireGraph::Node* wireroot : getWireroots(graph)) {
    odb::dbTechLayer* tech_layer = wireroot->layer();
    if (level_nodes.find(wireroot) == level_nodes.end()
        && tech_layer->getRoutingLevel() == routing_level) {
      std::set<dbWireGraph::Node*> nv;
      auto areas = calculateWireArea(
          wireroot, tech_layer->getRoutingLevel(), nv, level_nodes);
      double wire_area = areas.first;
      double iterm_gate_area = 0.0;
      double iterm_diff_area = 0.0;

      std::set<dbITerm*> iv;
      nv.clear();
      findWireBelowIterms(
          wireroot, iterm_gate_area, iterm_diff_area,
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

      if (wireroot->layer()->hasDefaultAntennaRule()) {
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
          std::vector<dbITerm*> gates;
          findWirerootIterms(wireroot, routing_level, gates);
          std::pair<double, std::vector<dbITerm*>> violated_wire
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

  for (dbNet* net : db_->getChip()->getBlock()->getNets()) {
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
