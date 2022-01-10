///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

#include "dbDescriptors.h"

#include "db.h"
#include "dbShape.h"

#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "sta/Liberty.hh"

#include <QInputDialog>
#include <QStringList>

#include <iomanip>
#include <limits>
#include <queue>
#include <regex>
#include <sstream>

namespace gui {

static std::string convertUnits(double value)
{
  std::stringstream ss;
  const int precision = 3;
  ss << std::fixed << std::setprecision(precision);
  const char* micron = "\u03BC";
  int log_units = std::floor(std::log10(value) / 3.0) * 3;
  if (log_units <= -18) {
    ss << value * 1e18 << "a";
  } else if (log_units <= -15) {
    ss << value * 1e15 << "f";
  } else if (log_units <= -12) {
    ss << value * 1e12 << "p";
  } else if (log_units <= -9) {
    ss << value * 1e9 << "n";
  } else if (log_units <= -6) {
    ss << value * 1e6 << micron;
  } else if (log_units <= -3) {
    ss << value * 1e3 << "m";
  } else if (log_units <= 0) {
    ss << value;
  } else if (log_units <= 3) {
    ss << value * 1e-3 << "k";
  } else if (log_units <= 6) {
    ss << value * 1e-6 << "M";
  } else {
    ss << value;
  }
  return ss.str();
}

// renames an object
template<typename T>
static void addRenameEditor(T obj, Descriptor::Editors& editor)
{
  editor.insert({"Name", Descriptor::makeEditor([obj](std::any value) {
    const std::string new_name = std::any_cast<std::string>(value);
    // check if empty
    if (new_name.empty()) {
      return false;
    }
    // check for illegal characters
    for (const char ch : {obj->getBlock()->getHierarchyDelimeter()}) {
      if (new_name.find(ch) != std::string::npos) {
        return false;
      }
    }
    obj->rename(new_name.c_str());
    return true;
  })});
}

// timing cone actions
template<typename T>
static void addTimingConeActions(T obj, const Descriptor* desc, Descriptor::Actions& actions)
{
  auto* gui = Gui::get();

  actions.push_back({std::string(Descriptor::deselect_action_), [obj, desc, gui]() {
    gui->timingCone(static_cast<T>(nullptr), false, false);
    return desc->makeSelected(obj, nullptr);
  }});
  actions.push_back({"Fanin Cone", [obj, desc, gui]() {
    gui->timingCone(obj, true, false);
    return desc->makeSelected(obj, nullptr);
  }});
  actions.push_back({"Fanout Cone", [obj, desc, gui]() {
    gui->timingCone(obj, false, true);
    return desc->makeSelected(obj, nullptr);
  }});
}

// get list of tech layers as EditorOption list
static void addLayersToOptions(odb::dbTech* tech, std::vector<Descriptor::EditorOption>& options)
{
  for (auto layer : tech->getLayers()) {
    options.push_back({layer->getName(), layer});
  }
}

// request user input to select tech layer, returns nullptr or current if none was selected
static odb::dbTechLayer* getLayerSelection(odb::dbTech* tech, odb::dbTechLayer* current = nullptr)
{
  std::vector<Descriptor::EditorOption> options;
  addLayersToOptions(tech, options);
  QStringList layers;
  for (auto& [name, layer] : options) {
    layers.append(QString::fromStdString(name));
  }
  bool okay;
  int default_selection = current == nullptr ? 0 : layers.indexOf(QString::fromStdString(current->getName()));
  QString selection = QInputDialog::getItem(
      nullptr,
      "Select technology layer",
      "Layer",
      layers,
      default_selection, // current layer
      false,
      &okay);
  if (okay) {
    int selection_idx = layers.indexOf(selection);
    if (selection_idx != -1) {
      return std::any_cast<odb::dbTechLayer*>(options[selection_idx].value);
    } else {
      // selection not found, return current
      return current;
    }
  } else {
    return current;
  }
}

////////

DbInstDescriptor::DbInstDescriptor(odb::dbDatabase* db, sta::dbSta* sta) :
    db_(db),
    sta_(sta)
{
}

std::string DbInstDescriptor::getName(std::any object) const
{
  return std::any_cast<odb::dbInst*>(object)->getName();
}

std::string DbInstDescriptor::getTypeName() const
{
  return "Inst";
}

bool DbInstDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  inst->getBBox()->getBox(bbox);
  return true;
}

void DbInstDescriptor::highlight(std::any object,
                                 Painter& painter,
                                 void* additional_data) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  if (!inst->getPlacementStatus().isPlaced()) {
    return;
  }

  odb::dbBox* bbox = inst->getBBox();
  odb::Rect rect;
  bbox->getBox(rect);
  painter.drawRect(rect);
}

bool DbInstDescriptor::isInst(std::any object) const
{
  return true;
}

Descriptor::Properties DbInstDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto inst = std::any_cast<odb::dbInst*>(object);
  auto placed = inst->getPlacementStatus();
  Properties props({{"Master", gui->makeSelected(inst->getMaster())},
                    {"Placement status", placed.getString()},
                    {"Source type", inst->getSourceType().getString()}});
  if (placed.isPlaced()) {
    int x, y;
    inst->getLocation(x, y);
    props.insert(props.end(),
                 {{"Orientation", inst->getOrient().getString()},
                  {"X", Property::convert_dbu(x, true)},
                  {"Y", Property::convert_dbu(y, true)}});
  }
  Descriptor::PropertyList iterms;
  for (auto iterm : inst->getITerms()) {
    auto* net = iterm->getNet();
    std::any net_value;
    if (net == nullptr) {
      net_value = "<none>";
    } else {
      net_value = gui->makeSelected(net);
    }
    iterms.push_back({gui->makeSelected(iterm), net_value});
  }
  props.push_back({"ITerms", iterms});
  return props;
}

Descriptor::Actions DbInstDescriptor::getActions(std::any object) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  return Actions({{"Delete", [inst]() {
    odb::dbInst::destroy(inst);
    return Selected(); // unselect since this object is now gone
  }}});
}

Descriptor::Editors DbInstDescriptor::getEditors(std::any object) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);

  std::vector<Descriptor::EditorOption> master_options;
  makeMasterOptions(inst->getMaster(), master_options);

  std::vector<Descriptor::EditorOption> orient_options;
  makeOrientationOptions(orient_options);

  std::vector<Descriptor::EditorOption> placement_options;
  makePlacementStatusOptions(placement_options);

  Editors editors;
  addRenameEditor(inst, editors);
  if (!master_options.empty()) {
    editors.insert({"Master", makeEditor([inst](std::any value) {
      inst->swapMaster(std::any_cast<odb::dbMaster*>(value));
      return true;
    }, master_options)});
  }
  editors.insert({"Orientation", makeEditor([inst](std::any value) {
      inst->setLocationOrient(std::any_cast<odb::dbOrientType>(value));
      return true;
    }, orient_options)});
  editors.insert({"Placement status", makeEditor([inst](std::any value) {
      inst->setPlacementStatus(std::any_cast<odb::dbPlacementStatus>(value));
      return true;
    }, placement_options)});

  editors.insert({"X", makeEditor([this, inst](std::any value) {
    return setNewLocation(inst, value, true);
    })});
  editors.insert({"Y", makeEditor([this, inst](std::any value) {
    return setNewLocation(inst, value, false);
    })});
  return editors;
}

// get list of equivalent masters as EditorOptions
void DbInstDescriptor::makeMasterOptions(odb::dbMaster* master, std::vector<EditorOption>& options) const
{
  std::set<odb::dbMaster*> masters;
  DbMasterDescriptor::getMasterEquivalent(sta_, master, masters);
  for (auto master : masters) {
    options.push_back({master->getConstName(), master});
  }
}

// get list if instance orientations for the editor
void DbInstDescriptor::makeOrientationOptions(std::vector<EditorOption>& options) const
{
  for (odb::dbOrientType type :
      {odb::dbOrientType::R0,
       odb::dbOrientType::R90,
       odb::dbOrientType::R180,
       odb::dbOrientType::R270,
       odb::dbOrientType::MY,
       odb::dbOrientType::MYR90,
       odb::dbOrientType::MX,
       odb::dbOrientType::MXR90}) {
    options.push_back({type.getString(), type});
  }
}

// get list of placement statuses for the editor
void DbInstDescriptor::makePlacementStatusOptions(std::vector<EditorOption>& options) const
{
  for (odb::dbPlacementStatus type :
      {odb::dbPlacementStatus::NONE,
       odb::dbPlacementStatus::UNPLACED,
       odb::dbPlacementStatus::SUGGESTED,
       odb::dbPlacementStatus::PLACED,
       odb::dbPlacementStatus::LOCKED,
       odb::dbPlacementStatus::FIRM,
       odb::dbPlacementStatus::COVER}) {
    options.push_back({type.getString(), type});
  }
}

// change location of instance
bool DbInstDescriptor::setNewLocation(odb::dbInst* inst, std::any value, bool is_x) const
{
  bool accept = false;
  int new_value = Descriptor::Property::convert_string(std::any_cast<std::string>(value), &accept);
  if (!accept) {
    return false;
  }
  int x_dbu, y_dbu;
  inst->getLocation(x_dbu, y_dbu);
  if (is_x) {
    x_dbu = new_value;
  } else {
    y_dbu = new_value;
  }
  inst->setLocation(x_dbu, y_dbu);
  return true;
}

Selected DbInstDescriptor::makeSelected(std::any object,
                                        void* additional_data) const
{
  if (auto inst = std::any_cast<odb::dbInst*>(&object)) {
    return Selected(*inst, this, additional_data);
  }
  return Selected();
}

bool DbInstDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_inst = std::any_cast<odb::dbInst*>(l);
  auto r_inst = std::any_cast<odb::dbInst*>(r);
  return l_inst->getId() < r_inst->getId();
}

bool DbInstDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* inst : block->getInsts()) {
    objects.insert(makeSelected(inst, nullptr));
  }
  return true;
}

//////////////////////////////////////////////////

DbMasterDescriptor::DbMasterDescriptor(odb::dbDatabase* db, sta::dbSta* sta) :
    db_(db),
    sta_(sta)
{
}

std::string DbMasterDescriptor::getName(std::any object) const
{
  return std::any_cast<odb::dbMaster*>(object)->getName();
}

std::string DbMasterDescriptor::getTypeName() const
{
  return "Master";
}

bool DbMasterDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto master = std::any_cast<odb::dbMaster*>(object);
  master->getPlacementBoundary(bbox);
  return true;
}

void DbMasterDescriptor::highlight(std::any object,
                                   Painter& painter,
                                   void* additional_data) const
{
  auto master = std::any_cast<odb::dbMaster*>(object);
  std::set<odb::dbInst*> insts;
  getInstances(master, insts);
  for (auto inst : insts) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }

    odb::dbBox* bbox = inst->getBBox();
    odb::Rect rect;
    bbox->getBox(rect);
    painter.drawRect(rect);
  }
}

Descriptor::Properties DbMasterDescriptor::getProperties(std::any object) const
{
  auto master = std::any_cast<odb::dbMaster*>(object);
  Properties props({{"Master type", master->getType().getString()}});
  auto site = master->getSite();
  if (site != nullptr) {
    props.push_back({"Site", site->getConstName()});
  }
  auto gui = Gui::get();
  std::vector<std::any> mterms;
  for (auto mterm : master->getMTerms()) {
    mterms.push_back(mterm->getConstName());
  }
  props.push_back({"MTerms", mterms});
  SelectionSet equivalent;
  std::set<odb::dbMaster*> equivalent_masters;
  getMasterEquivalent(sta_, master, equivalent_masters);
  for (auto other_master : equivalent_masters) {
    if (other_master != master) {
      equivalent.insert(gui->makeSelected(other_master));
    }
  }
  props.push_back({"Equivalent", equivalent});
  SelectionSet instances;
  std::set<odb::dbInst*> insts;
  getInstances(master, insts);
  for (auto inst : insts) {
    instances.insert(gui->makeSelected(inst));
  }
  props.push_back({"Instances", instances});

  return props;
}

Selected DbMasterDescriptor::makeSelected(std::any object,
                                          void* additional_data) const
{
  if (auto master = std::any_cast<odb::dbMaster*>(&object)) {
    return Selected(*master, this, additional_data);
  }
  return Selected();
}

bool DbMasterDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_master = std::any_cast<odb::dbMaster*>(l);
  auto r_master = std::any_cast<odb::dbMaster*>(r);
  return l_master->getId() < r_master->getId();
}

// get list of equivalent masters as EditorOptions
void DbMasterDescriptor::getMasterEquivalent(sta::dbSta* sta,
                                             odb::dbMaster* master,
                                             std::set<odb::dbMaster*>& masters)
{
  // mirrors method used in Resizer.cpp
  auto network = sta->getDbNetwork();

  sta::LibertyLibrarySeq libs;
  sta::LibertyLibraryIterator *lib_iter = network->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary *lib = lib_iter->next();
    libs.push_back(lib);
  }
  delete lib_iter;
  sta->makeEquivCells(&libs, nullptr);

  sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));
  auto equiv_cells = sta->equivCells(cell);
  if (equiv_cells != nullptr) {
    for (auto equiv : *equiv_cells) {
      auto eq_master = network->staToDb(equiv);
      if (eq_master != nullptr) {
        masters.insert(eq_master);
      }
    }
  }
}

// get list of instances of that type
void DbMasterDescriptor::getInstances(odb::dbMaster* master, std::set<odb::dbInst*>& insts) const
{
  for (auto inst : master->getDb()->getChip()->getBlock()->getInsts()) {
    if (inst->getMaster() == master) {
      insts.insert(inst);
    }
  }
}

bool DbMasterDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  std::vector<odb::dbMaster*> masters;
  block->getMasters(masters);

  for (auto* master : masters) {
    objects.insert(makeSelected(master, nullptr));
  }
  return true;
}

//////////////////////////////////////////////////

DbNetDescriptor::DbNetDescriptor(odb::dbDatabase* db) :
    db_(db)
{
}

std::string DbNetDescriptor::getName(std::any object) const
{
  return std::any_cast<odb::dbNet*>(object)->getName();
}

std::string DbNetDescriptor::getTypeName() const
{
  return "Net";
}

bool DbNetDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto net = std::any_cast<odb::dbNet*>(object);
  auto wire = net->getWire();
  bool has_box = false;
  bbox.mergeInit();
  if (wire) {
    odb::Rect wire_box;
    if (wire->getBBox(wire_box)) {
      bbox.merge(wire_box);
      has_box = true;
    }
  }

  for (auto inst_term : net->getITerms()) {
    if (!inst_term->getInst()->getPlacementStatus().isPlaced()) {
      continue;
    }
    bbox.merge(inst_term->getBBox());
    has_box = true;
  }

  for (auto blk_term : net->getBTerms()) {
    for (auto pin : blk_term->getBPins()) {
      bbox.merge(pin->getBBox());
      has_box = true;
    }
  }

  return has_box;
}

void DbNetDescriptor::findSourcesAndSinks(odb::dbNet* net,
                                          const odb::dbObject* sink,
                                          std::vector<GraphTarget>& sources,
                                          std::vector<GraphTarget>& sinks) const
{
  // gets all the shapes that make up the iterm
  auto get_graph_iterm_targets = [](odb::dbMTerm* mterm, const odb::dbTransform& transform, std::vector<GraphTarget>& targets) {
    for (auto* mpin : mterm->getMPins()) {
      for (auto* box : mpin->getGeometry()) {
        odb::Rect rect;
        box->getBox(rect);
        transform.apply(rect);
        targets.push_back({rect, box->getTechLayer()});
      }
    }
  };

  // gets all the shapes that make up the bterm
  auto get_graph_bterm_targets = [](odb::dbBTerm* bterm, std::vector<GraphTarget>& targets) {
    for (auto* bpin : bterm->getBPins()) {
      for (auto* box : bpin->getBoxes()) {
        odb::Rect rect;
        box->getBox(rect);
        targets.push_back({rect, box->getTechLayer()});
      }
    }
  };

  // find sources and sinks on this net
  for (auto* iterm : net->getITerms()) {
    if (iterm == sink) {
      odb::dbTransform transform;
      iterm->getInst()->getTransform(transform);
      get_graph_iterm_targets(iterm->getMTerm(), transform, sinks);
      continue;
    }

    auto iotype = iterm->getIoType();
    if (iotype == odb::dbIoType::OUTPUT ||
        iotype == odb::dbIoType::INOUT) {
      odb::dbTransform transform;
      iterm->getInst()->getTransform(transform);
      get_graph_iterm_targets(iterm->getMTerm(), transform, sources);
    }
  }
  for (auto* bterm : net->getBTerms()) {
    if (bterm == sink) {
      get_graph_bterm_targets(bterm, sinks);
      continue;
    }

    auto iotype = bterm->getIoType();
    if (iotype == odb::dbIoType::INPUT ||
        iotype == odb::dbIoType::INOUT ||
        iotype == odb::dbIoType::FEEDTHRU) {
      get_graph_bterm_targets(bterm, sources);
    }
  }
}

void DbNetDescriptor::findSourcesAndSinksInGraph(odb::dbNet* net,
                                                 const odb::dbObject* sink,
                                                 odb::dbWireGraph* graph,
                                                 NodeList& source_nodes,
                                                 NodeList& sink_nodes) const
{
  // find sources and sinks on this net
  std::vector<GraphTarget> sources;
  std::vector<GraphTarget> sinks;
  findSourcesAndSinks(net, sink, sources, sinks);

  // find the nodes on the wire graph that intersect the sinks identified
  for (auto itr = graph->begin_nodes(); itr != graph->end_nodes(); itr++) {
    const auto* node = *itr;
    int x, y;
    node->xy(x, y);
    const odb::Point node_pt(x, y);
    const odb::dbTechLayer* node_layer = node->layer();

    for (const auto& [source_rect, source_layer] : sources) {
      if (source_rect.intersects(node_pt) && source_layer == node_layer) {
        source_nodes.insert(node);
      }
    }

    for (const auto& [sink_rect, sink_layer] : sinks) {
      if (sink_rect.intersects(node_pt) && sink_layer == node_layer) {
        sink_nodes.insert(node);
      }
    }
  }
}

void DbNetDescriptor::drawPathSegment(odb::dbNet* net, const odb::dbObject* sink, Painter& painter) const
{
  odb::dbWireGraph graph;
  graph.decode(net->getWire());

  // find the nodes on the wire graph that intersect the sinks identified
  NodeList source_nodes;
  NodeList sink_nodes;
  findSourcesAndSinksInGraph(net, sink, &graph, source_nodes, sink_nodes);

  if (source_nodes.empty() || sink_nodes.empty()) {
    return;
  }

  // build connectivity map from the wire graph
  NodeMap node_map;
  buildNodeMap(&graph, node_map);

  Painter::Color highlight_color = painter.getPenColor();
  highlight_color.a = 255;

  painter.saveState();
  painter.setPen(highlight_color, true, 4);
  for (const auto* source_node : source_nodes) {
    for (const auto* sink_node : sink_nodes) {
      // find the shortest path from source to sink
      std::vector<odb::Point> path;
      findPath(node_map, source_node, sink_node, path);

      if (!path.empty()) {
        odb::Point prev_pt = path[0];
        for (const auto& pt : path) {
          if (pt == prev_pt) {
            continue;
          }

          painter.drawLine(prev_pt, pt);
          prev_pt = pt;
        }
      } else {
        // unable to find path so just draw a fly-wire
        int x, y;
        source_node->xy(x, y);
        odb::Point source_pt(x, y);
        sink_node->xy(x, y);
        odb::Point sink_pt(x, y);
        painter.drawLine(source_pt, sink_pt);
      }
    }
  }
  painter.restoreState();
}

void DbNetDescriptor::buildNodeMap(odb::dbWireGraph* graph, NodeMap& node_map) const
{
  for (auto itr = graph->begin_nodes(); itr != graph->end_nodes(); itr++) {
    const auto* node = *itr;
    NodeList connections;

    int x, y;
    node->xy(x, y);
    const odb::Point node_pt(x, y);
    const odb::dbTechLayer* layer = node->layer();

    for (auto itr = graph->begin_edges(); itr != graph->end_edges(); itr++) {
      const auto* edge = *itr;
      const auto* source = edge->source();
      const auto* target = edge->target();

      if (source->layer() == layer || target->layer() == layer) {
        int sx, sy;
        source->xy(sx, sy);

        int tx, ty;
        target->xy(tx, ty);
        if (sx > tx) {
          std::swap(sx, tx);
        }
        if (sy > ty) {
          std::swap(sy, ty);
        }
        const odb::Rect path_line(sx, sy, tx, ty);
        if (path_line.intersects(node_pt)) {
          connections.insert(source);
          connections.insert(target);
        }
      }
    }

    node_map[node].insert(connections.begin(), connections.end());
    for (const auto* cnode : connections) {
      if (cnode == node) {
        // don't insert the source node
        continue;
      }
      node_map[cnode].insert(node);
    }
  }
}

void DbNetDescriptor::findPath(NodeMap& graph,
                               const Node* source,
                               const Node* sink,
                               std::vector<odb::Point>& path) const
{
  // find path from source to sink using A*
  // https://en.wikipedia.org/w/index.php?title=A*_search_algorithm&oldid=1050302256

  auto distance = [](const Node* node0, const Node* node1) -> int {
    int x0, y0;
    node0->xy(x0, y0);
    int x1, y1;
    node1->xy(x1, y1);
    return std::abs(x0 - x1) + std::abs(y0 - y1);
  };

  std::map<const Node*, const Node*> came_from;
  std::map<const Node*, int> g_score;
  std::map<const Node*, int> f_score;

  struct DistNode {
    const Node* node;
    int dist;

    public:
      // used for priority queue
      bool operator<(const DistNode& other) const { return dist > other.dist; }
  };
  std::priority_queue<DistNode> open_set;
  std::set<const Node*> open_set_nodes;
  const int source_sink_dist = distance(source, sink);
  open_set.push({source, source_sink_dist});
  open_set_nodes.insert(source);

  for (const auto& [node, nodes] : graph) {
    g_score[node] = std::numeric_limits<int>::max();
    f_score[node] = std::numeric_limits<int>::max();
  }
  g_score[source] = 0;
  f_score[source] = source_sink_dist;

  while (!open_set.empty()) {
    auto current = open_set.top().node;

    open_set.pop();
    open_set_nodes.erase(current);

    if (current == sink) {
      // build path
      int x, y;
      while (current != source) {
        current->xy(x, y);
        path.push_back(odb::Point(x, y));
        current = came_from[current];
      }
      current->xy(x, y);
      path.push_back(odb::Point(x, y));
      return;
    }

    const int current_g_score = g_score[current];
    for (const auto& neighbor : graph[current]) {
      const int possible_g_score = current_g_score + distance(current, neighbor);
      if (possible_g_score < g_score[neighbor]) {
        const int new_f_score = possible_g_score + distance(neighbor, sink);
        came_from[neighbor] = current;
        g_score[neighbor] = possible_g_score;
        f_score[neighbor] = new_f_score;

        if (open_set_nodes.find(neighbor) == open_set_nodes.end()) {
          open_set.push({neighbor, new_f_score});
          open_set_nodes.insert(neighbor);
        }
      }
    }
  }
}

// additional_data is used define the related sink for this net
// this will limit the fly-wires to just those related to that sink
// if nullptr, all flywires will be drawn
void DbNetDescriptor::highlight(std::any object,
                                Painter& painter,
                                void* additional_data) const
{
  odb::dbObject* sink_object = nullptr;
  if (additional_data != nullptr)
    sink_object = static_cast<odb::dbObject*>(additional_data);
  auto net = std::any_cast<odb::dbNet*>(object);

  auto* iterm_descriptor = Gui::get()->getDescriptor<odb::dbITerm*>();
  auto* bterm_descriptor = Gui::get()->getDescriptor<odb::dbBTerm*>();

  const bool is_supply = net->getSigType().isSupply();
  auto should_draw_term = [sink_object](const odb::dbObject* term) -> bool {
    if (sink_object == nullptr) {
      return true;
    }
    return sink_object == term;
  };

  auto is_source_iterm = [](odb::dbITerm* iterm) -> bool {
    const auto iotype = iterm->getIoType();
    return iotype == odb::dbIoType::OUTPUT ||
           iotype == odb::dbIoType::INOUT;
  };
  auto is_sink_iterm = [](odb::dbITerm* iterm) -> bool {
    const auto iotype = iterm->getIoType();
    return iotype == odb::dbIoType::INPUT ||
           iotype == odb::dbIoType::INOUT;
  };

  auto is_source_bterm = [](odb::dbBTerm* bterm) -> bool {
    const auto iotype = bterm->getIoType();
    return iotype == odb::dbIoType::INPUT ||
           iotype == odb::dbIoType::INOUT ||
           iotype == odb::dbIoType::FEEDTHRU;
  };
  auto is_sink_bterm = [](odb::dbBTerm* bterm) -> bool {
    const auto iotype = bterm->getIoType();
    return iotype == odb::dbIoType::OUTPUT ||
           iotype == odb::dbIoType::INOUT ||
           iotype == odb::dbIoType::FEEDTHRU;
  };

  // Draw regular routing
  if (!is_supply) { // don't draw iterms on supply nets
    // draw iterms
    for (auto* iterm : net->getITerms()) {
      if (is_sink_iterm(iterm)) {
        if (should_draw_term(iterm)) {
          iterm_descriptor->highlight(iterm, painter);
        }
      } else {
        iterm_descriptor->highlight(iterm, painter);
      }
    }
  }
  // draw bterms
  for (auto* bterm : net->getBTerms()) {
    if (is_sink_bterm(bterm)) {
      if (should_draw_term(bterm)) {
        bterm_descriptor->highlight(bterm, painter);
      }
    } else {
      bterm_descriptor->highlight(bterm, painter);
    }
  }

  odb::Rect rect;
  odb::dbWire* wire = net->getWire();
  if (wire) {
    if (sink_object != nullptr) {
      drawPathSegment(net, sink_object, painter);
    }

    odb::dbWireShapeItr it;
    it.begin(wire);
    odb::dbShape shape;
    while (it.next(shape)) {
      shape.getBox(rect);
      painter.drawRect(rect);
    }
  } else if (!is_supply) {
    std::set<odb::Point> driver_locs;
    std::set<odb::Point> sink_locs;
    for (auto inst_term : net->getITerms()) {
      if (!inst_term->getInst()->getPlacementStatus().isPlaced()) {
        continue;
      }

      odb::Point rect_center;
      int x, y;
      if (!inst_term->getAvgXY(&x, &y)) {
        odb::dbBox* bbox = inst_term->getInst()->getBBox();
        odb::Rect rect;
        bbox->getBox(rect);
        rect_center = odb::Point((rect.xMax() + rect.xMin()) / 2.0,
                                 (rect.yMax() + rect.yMin()) / 2.0);
      } else {
        rect_center = odb::Point(x, y);
      }

      if (is_sink_iterm(inst_term)) {
        if (should_draw_term(inst_term)) {
          sink_locs.insert(rect_center);
        }
      }
      if (is_source_iterm(inst_term)) {
        driver_locs.insert(rect_center);
      }
    }
    for (auto blk_term : net->getBTerms()) {
      const bool driver_term = is_source_bterm(blk_term);
      const bool sink_term = is_sink_bterm(blk_term);

      for (auto pin : blk_term->getBPins()) {
        auto pin_rect = pin->getBBox();
        odb::Point rect_center((pin_rect.xMax() + pin_rect.xMin()) / 2.0,
                               (pin_rect.yMax() + pin_rect.yMin()) / 2.0);
        if (sink_term) {
          if (should_draw_term(blk_term)) {
            sink_locs.insert(rect_center);
          }
        }
        if (driver_term) {
          driver_locs.insert(rect_center);
        }
      }
    }

    if (!driver_locs.empty() && !sink_locs.empty()) {
      painter.saveState();
      auto color = painter.getPenColor();
      color.a = 255;
      painter.setPen(color, true);
      for (auto& driver : driver_locs) {
        for (auto& sink : sink_locs) {
          painter.drawLine(driver, sink);
        }
      }
      painter.restoreState();
    }
  }

  // Draw special (i.e. geometric) routing
  for (auto swire : net->getSWires()) {
    for (auto sbox : swire->getWires()) {
      sbox->getBox(rect);
      painter.drawGeomShape(sbox->getGeomShape());
    }
  }
}

bool DbNetDescriptor::isSlowHighlight(std::any object) const
{
  auto net = std::any_cast<odb::dbNet*>(object);
  return net->getSigType().isSupply();
}

bool DbNetDescriptor::isNet(std::any object) const
{
  return true;
}

Descriptor::Properties DbNetDescriptor::getProperties(std::any object) const
{
  auto net = std::any_cast<odb::dbNet*>(object);
  Properties props({{"Signal type", net->getSigType().getString()},
                    {"Source type", net->getSourceType().getString()},
                    {"Wire type", net->getWireType().getString()},
                    {"Special", net->isSpecial()}});
  auto gui = Gui::get();
  int iterm_size = net->getITerms().size();
  std::any iterm_item;
  if (iterm_size > max_iterms_) {
    iterm_item = std::to_string(iterm_size) + " items";
  } else {
    SelectionSet iterms;
    for (auto iterm : net->getITerms()) {
      iterms.insert(gui->makeSelected(iterm));
    }
    iterm_item = iterms;
  }
  props.push_back({"ITerms", iterm_item});
  SelectionSet bterms;
  for (auto bterm : net->getBTerms()) {
    bterms.insert(gui->makeSelected(bterm));
  }
  props.push_back({"BTerms", bterms});
  return props;
}

Descriptor::Editors DbNetDescriptor::getEditors(std::any object) const
{
  auto net = std::any_cast<odb::dbNet*>(object);
  Editors editors;
  addRenameEditor(net, editors);
  editors.insert({"Special", makeEditor([net](std::any value) {
    const bool new_special = std::any_cast<bool>(value);
    if (new_special) {
      net->setSpecial();
    } else {
      net->clearSpecial();
    }
    for (auto* iterm : net->getITerms()) {
      if (new_special) {
        iterm->setSpecial();
      } else {
        iterm->clearSpecial();
      }
    }
    return true;
  })});
  return editors;
}

Selected DbNetDescriptor::makeSelected(std::any object,
                                       void* additional_data) const
{
  if (auto net = std::any_cast<odb::dbNet*>(&object)) {
    return Selected(*net, this, additional_data);
  }
  return Selected();
}

bool DbNetDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_net = std::any_cast<odb::dbNet*>(l);
  auto r_net = std::any_cast<odb::dbNet*>(r);
  return l_net->getId() < r_net->getId();
}

bool DbNetDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* net : block->getNets()) {
    objects.insert(makeSelected(net, nullptr));
  }
  return true;
}

//////////////////////////////////////////////////

DbITermDescriptor::DbITermDescriptor(odb::dbDatabase* db) :
    db_(db)
{
}

std::string DbITermDescriptor::getName(std::any object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  return iterm->getInst()->getName() + '/' + iterm->getMTerm()->getName();
}

std::string DbITermDescriptor::getShortName(std::any object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  return iterm->getMTerm()->getName();
}

std::string DbITermDescriptor::getTypeName() const
{
  return "ITerm";
}

bool DbITermDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  if (iterm->getInst()->getPlacementStatus().isPlaced()) {
    bbox = iterm->getBBox();
    return true;
  }
  return false;
}

void DbITermDescriptor::highlight(std::any object,
                                  Painter& painter,
                                  void* additional_data) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);

  if (!iterm->getInst()->getPlacementStatus().isPlaced()) {
    return;
  }

  odb::dbTransform inst_xfm;
  iterm->getInst()->getTransform(inst_xfm);

  auto mterm = iterm->getMTerm();
  for (auto mpin : mterm->getMPins()) {
    for (auto box : mpin->getGeometry()) {
      odb::Rect rect;
      box->getBox(rect);
      inst_xfm.apply(rect);
      painter.drawRect(rect);
    }
  }
}

Descriptor::Properties DbITermDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  auto net = iterm->getNet();
  std::any net_value;
  if (net != nullptr) {
    net_value = gui->makeSelected(net);
  } else {
    net_value = "<none>";
  }
  SelectionSet aps;
  for (auto& [mpin, ap_vec] : iterm->getAccessPoints()) {
    for (auto ap :ap_vec) {
      DbItermAccessPoint iap{ap, iterm};
      aps.insert(gui->makeSelected(iap));
    }
  }

  return Properties({{"Instance", gui->makeSelected(iterm->getInst())},
                     {"IO type", iterm->getIoType().getString()},
                     {"Net", net_value},
                     {"Special", iterm->isSpecial()},
                     {"MTerm", iterm->getMTerm()->getConstName()},
                     {"Access Points", aps}});
}

Descriptor::Actions DbITermDescriptor::getActions(std::any object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);

  Descriptor::Actions actions;
  addTimingConeActions<odb::dbITerm*>(iterm, this, actions);

  return actions;
}

Selected DbITermDescriptor::makeSelected(std::any object,
                                         void* additional_data) const
{
  if (auto iterm = std::any_cast<odb::dbITerm*>(&object)) {
    return Selected(*iterm, this, additional_data);
  }
  return Selected();
}

bool DbITermDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_iterm = std::any_cast<odb::dbITerm*>(l);
  auto r_iterm = std::any_cast<odb::dbITerm*>(r);
  return l_iterm->getId() < r_iterm->getId();
}

bool DbITermDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* term : block->getITerms()) {
    objects.insert(makeSelected(term, nullptr));
  }
  return true;
}

//////////////////////////////////////////////////

DbBTermDescriptor::DbBTermDescriptor(odb::dbDatabase* db) :
    db_(db)
{
}

std::string DbBTermDescriptor::getName(std::any object) const
{
  return std::any_cast<odb::dbBTerm*>(object)->getName();
}

std::string DbBTermDescriptor::getTypeName() const
{
  return "BTerm";
}

bool DbBTermDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto* bterm = std::any_cast<odb::dbBTerm*>(object);
  bbox = bterm->getBBox();
  return true;
}

void DbBTermDescriptor::highlight(std::any object,
                                  Painter& painter,
                                  void* additional_data) const
{
  auto* bterm = std::any_cast<odb::dbBTerm*>(object);
  for (auto bpin : bterm->getBPins()) {
    for (auto box : bpin->getBoxes()) {
      odb::Rect rect;
      box->getBox(rect);
      painter.drawRect(rect);
    }
  }
}

Descriptor::Properties DbBTermDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto bterm = std::any_cast<odb::dbBTerm*>(object);
  return Properties({{"Net", gui->makeSelected(bterm->getNet())},
                     {"Signal type", bterm->getSigType().getString()},
                     {"IO type", bterm->getIoType().getString()}});
}

Descriptor::Editors DbBTermDescriptor::getEditors(std::any object) const
{
  auto bterm = std::any_cast<odb::dbBTerm*>(object);
  Editors editors;
  addRenameEditor(bterm, editors);
  return editors;
}

Descriptor::Actions DbBTermDescriptor::getActions(std::any object) const
{
  auto bterm = std::any_cast<odb::dbBTerm*>(object);

  Descriptor::Actions actions;
  addTimingConeActions<odb::dbBTerm*>(bterm, this, actions);

  return actions;
}

Selected DbBTermDescriptor::makeSelected(std::any object,
                                         void* additional_data) const
{
  if (auto bterm = std::any_cast<odb::dbBTerm*>(&object)) {
    return Selected(*bterm, this, additional_data);
  }
  return Selected();
}

bool DbBTermDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_bterm = std::any_cast<odb::dbBTerm*>(l);
  auto r_bterm = std::any_cast<odb::dbBTerm*>(r);
  return l_bterm->getId() < r_bterm->getId();
}

bool DbBTermDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* term : block->getBTerms()) {
    objects.insert(makeSelected(term, nullptr));
  }
  return true;
}

//////////////////////////////////////////////////

DbBlockageDescriptor::DbBlockageDescriptor(odb::dbDatabase* db) :
    db_(db)
{
}

std::string DbBlockageDescriptor::getName(std::any object) const
{
  return "Blockage";
}

std::string DbBlockageDescriptor::getTypeName() const
{
  return "Blockage";
}

bool DbBlockageDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto* blockage = std::any_cast<odb::dbBlockage*>(object);
  odb::dbBox* box = blockage->getBBox();
  box->getBox(bbox);
  return true;
}

void DbBlockageDescriptor::highlight(std::any object,
                                     Painter& painter,
                                     void* additional_data) const
{
  odb::Rect rect;
  getBBox(object, rect);
  painter.drawRect(rect);
}

Descriptor::Properties DbBlockageDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto blockage = std::any_cast<odb::dbBlockage*>(object);
  odb::dbInst* inst = blockage->getInstance();
  std::any inst_value;
  if (inst != nullptr) {
    inst_value = gui->makeSelected(inst);
  } else {
    inst_value = "<none>";
  }
  odb::Rect rect;
  blockage->getBBox()->getBox(rect);
  return Properties({{"Instance", inst_value},
                     {"X", Property::convert_dbu(rect.xMin(), true)},
                     {"Y", Property::convert_dbu(rect.yMin(), true)},
                     {"Width", Property::convert_dbu(rect.dx(), true)},
                     {"Height", Property::convert_dbu(rect.dy(), true)},
                     {"Soft", blockage->isSoft()},
                     {"Max density", std::to_string(blockage->getMaxDensity()) + "%"}});
}

Descriptor::Editors DbBlockageDescriptor::getEditors(std::any object) const
{
  auto blockage = std::any_cast<odb::dbBlockage*>(object);
  Editors editors;
  editors.insert({"Max density", makeEditor([blockage](std::any any_value) {
    std::string value = std::any_cast<std::string>(any_value);
    std::regex density_regex("(1?[0-9]?[0-9]?(\\.[0-9]*)?)\\s*%?");
    std::smatch base_match;
    if (std::regex_match(value, base_match, density_regex)) {
      try {
        // try to convert to float
        float density = std::stof(base_match[0]);
        if (0 <= density && density <= 100) {
          blockage->setMaxDensity(density);
          return true;
        }
      } catch (std::out_of_range&) {
        // catch poorly formatted string
      } catch (std::logic_error&) {
        // catch poorly formatted string
      }
    }
    return false;
  })});
  return editors;
}

Selected DbBlockageDescriptor::makeSelected(std::any object,
                                            void* additional_data) const
{
  if (auto blockage = std::any_cast<odb::dbBlockage*>(&object)) {
    return Selected(*blockage, this, additional_data);
  }
  return Selected();
}

bool DbBlockageDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_blockage = std::any_cast<odb::dbBlockage*>(l);
  auto r_blockage = std::any_cast<odb::dbBlockage*>(r);
  return l_blockage->getId() < r_blockage->getId();
}

bool DbBlockageDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* blockage : block->getBlockages()) {
    objects.insert(makeSelected(blockage, nullptr));
  }
  return true;
}

//////////////////////////////////////////////////

DbObstructionDescriptor::DbObstructionDescriptor(odb::dbDatabase* db) :
    db_(db)
{
}

std::string DbObstructionDescriptor::getName(std::any object) const
{
  auto obs = std::any_cast<odb::dbObstruction*>(object);
  return "Obstruction: " + obs->getBBox()->getTechLayer()->getName();
}

std::string DbObstructionDescriptor::getTypeName() const
{
  return "Obstruction";
}

bool DbObstructionDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto obs = std::any_cast<odb::dbObstruction*>(object);
  odb::dbBox* box = obs->getBBox();
  box->getBox(bbox);
  return true;
}

void DbObstructionDescriptor::highlight(std::any object,
                                        Painter& painter,
                                        void* additional_data) const
{
  odb::Rect rect;
  getBBox(object, rect);
  painter.drawRect(rect);
}

Descriptor::Properties DbObstructionDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto obs = std::any_cast<odb::dbObstruction*>(object);
  odb::dbInst* inst = obs->getInstance();
  std::any inst_value;
  if (inst != nullptr) {
    inst_value = gui->makeSelected(inst);
  } else {
    inst_value = "<none>";
  }
  odb::Rect rect;
  obs->getBBox()->getBox(rect);
  Properties props({{"Instance", inst_value},
                    {"Layer", gui->makeSelected(obs->getBBox()->getTechLayer())},
                    {"X", Property::convert_dbu(rect.xMin(), true)},
                    {"Y", Property::convert_dbu(rect.yMin(), true)},
                    {"Width", Property::convert_dbu(rect.dx(), true)},
                    {"Height", Property::convert_dbu(rect.dy(), true)},
                    {"Slot", obs->isSlotObstruction()},
                    {"Fill", obs->isFillObstruction()}});
  if (obs->hasEffectiveWidth()) {
    props.push_back({"Effective width", Property::convert_dbu(obs->getEffectiveWidth(), true)});
  }

  if (obs->hasMinSpacing()) {
    props.push_back({"Min spacing", Property::convert_dbu(obs->getMinSpacing(), true)});
  }
  return props;
}

Descriptor::Actions DbObstructionDescriptor::getActions(std::any object) const
{
  auto obs = std::any_cast<odb::dbObstruction*>(object);
  return Actions({{"Copy to layer", [obs, object]() {
    odb::dbBox* box = obs->getBBox();
    odb::dbTechLayer* layer = getLayerSelection(obs->getBlock()->getDataBase()->getTech(), box->getTechLayer());
    auto gui = gui::Gui::get();
    if (layer == nullptr) {
      // select old layer again
      return gui->makeSelected(obs);
    }
    else {
      auto new_obs = odb::dbObstruction::create(
          obs->getBlock(),
          layer,
          box->xMin(),
          box->yMin(),
          box->xMax(),
          box->yMax());
      // does not copy other parameters
      return gui->makeSelected(new_obs);
    }
  }},
  {"Delete", [obs]() {
    odb::dbObstruction::destroy(obs);
    return Selected(); // unselect since this object is now gone
  }}});
}

Selected DbObstructionDescriptor::makeSelected(std::any object,
                                         void* additional_data) const
{
  if (auto obs = std::any_cast<odb::dbObstruction*>(&object)) {
    return Selected(*obs, this, additional_data);
  }
  return Selected();
}

bool DbObstructionDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_obs = std::any_cast<odb::dbObstruction*>(l);
  auto r_obs = std::any_cast<odb::dbObstruction*>(r);
  return l_obs->getId() < r_obs->getId();
}

bool DbObstructionDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* obs : block->getObstructions()) {
    objects.insert(makeSelected(obs, nullptr));
  }
  return true;
}

//////////////////////////////////////////////////

DbTechLayerDescriptor::DbTechLayerDescriptor(odb::dbDatabase* db) :
    db_(db)
{
}

std::string DbTechLayerDescriptor::getName(std::any object) const
{
  auto layer = std::any_cast<odb::dbTechLayer*>(object);
  return layer->getConstName();
}

std::string DbTechLayerDescriptor::getTypeName() const
{
  return "Tech layer";
}

bool DbTechLayerDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void DbTechLayerDescriptor::highlight(std::any object,
                                        Painter& painter,
                                        void* additional_data) const
{
}

Descriptor::Properties DbTechLayerDescriptor::getProperties(std::any object) const
{
  auto layer = std::any_cast<odb::dbTechLayer*>(object);
  Properties props({{"Direction", layer->getDirection().getString()},
                    {"Minimum width", Property::convert_dbu(layer->getWidth(), true)},
                    {"Minimum spacing", Property::convert_dbu(layer->getSpacing(), true)}});
  const char* micron = "\u03BC";
  if (layer->getResistance() != 0.0) {
    props.push_back({"Resistance", convertUnits(layer->getResistance()) + "\u03A9/sq"}); // ohm/sq
  }
  if (layer->getCapacitance() != 0.0) {
    props.push_back({"Capacitance", convertUnits(layer->getCapacitance() * 1e-12) + "F/" + micron + "m\u00B2"}); // F/um^2
  }
  return props;
}

Selected DbTechLayerDescriptor::makeSelected(std::any object,
                                         void* additional_data) const
{
  if (auto layer = std::any_cast<odb::dbTechLayer*>(&object)) {
    return Selected(*layer, this, additional_data);
  }
  return Selected();
}

bool DbTechLayerDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_layer = std::any_cast<odb::dbTechLayer*>(l);
  auto r_layer = std::any_cast<odb::dbTechLayer*>(r);
  return l_layer->getId() < r_layer->getId();
}

bool DbTechLayerDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* tech = db_->getTech();
  if (tech == nullptr) {
    return false;
  }

  for (auto* layer : tech->getLayers()) {
    objects.insert(makeSelected(layer, nullptr));
  }
  return true;
}

//////////////////////////////////////////////////

DbItermAccessPointDescriptor::DbItermAccessPointDescriptor(odb::dbDatabase* db) :
    db_(db)
{
}

std::string DbItermAccessPointDescriptor::getName(std::any object) const
{
  auto iterm_ap = std::any_cast<DbItermAccessPoint>(object);
  auto ap = iterm_ap.ap;
  std::string name(ap->getLowType().getString());
  name += std::string("/") + ap->getHighType().getString();
  return name;
}

std::string DbItermAccessPointDescriptor::getTypeName() const
{
  return "Access Point";
}

bool DbItermAccessPointDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto iterm_ap = std::any_cast<DbItermAccessPoint>(object);
  odb::Point pt = iterm_ap.ap->getPoint();
  int x, y;
  iterm_ap.iterm->getInst()->getLocation(x, y);
  odb::dbTransform xform({x, y});
  xform.apply(pt);
  bbox = {pt, pt};
  return true;
}

void DbItermAccessPointDescriptor::highlight(std::any object,
                                        Painter& painter,
                                        void* additional_data) const
{
  auto iterm_ap = std::any_cast<DbItermAccessPoint>(object);
  odb::Point pt = iterm_ap.ap->getPoint();
  int x, y;
  iterm_ap.iterm->getInst()->getLocation(x, y);
  odb::dbTransform xform({x, y});
  xform.apply(pt);
  const int shape_size = 100;
  painter.drawX(pt.x(), pt.y(), shape_size);
}

Descriptor::Properties DbItermAccessPointDescriptor::getProperties(std::any object) const
{
  auto iterm_ap = std::any_cast<DbItermAccessPoint>(object);
  auto ap = iterm_ap.ap;

  std::vector<odb::dbDirection> accesses;
  ap->getAccesses(accesses);

  std::string directions;
  for (auto dir : accesses) {
    if (!directions.empty()) {
      directions += ", ";
    }
    directions += dir.getString();
  }

  auto gui = Gui::get();
  Properties props({{"Low Type", ap->getLowType().getString()},
                    {"High Type", ap->getHighType().getString()},
                    {"Directions", directions},
                    {"Layer", gui->makeSelected(ap->getLayer())}
                    });
  return props;
}

Selected DbItermAccessPointDescriptor::makeSelected(std::any object,
                                         void* additional_data) const
{
  if (object.type() == typeid(DbItermAccessPoint)) {
    auto iterm_ap = std::any_cast<DbItermAccessPoint>(object);
    return Selected(iterm_ap, this, additional_data);
  }
  return Selected();
}

bool DbItermAccessPointDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_iterm_ap = std::any_cast<DbItermAccessPoint>(l);
  auto r_iterm_ap = std::any_cast<DbItermAccessPoint>(r);
  if (l_iterm_ap.iterm != r_iterm_ap.iterm) {
    return l_iterm_ap.iterm->getId() < r_iterm_ap.iterm->getId();
  }
  return l_iterm_ap.ap->getId() < r_iterm_ap.ap->getId();
}

bool DbItermAccessPointDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* iterm : block->getITerms()) {
    for (auto [mpin, aps] : iterm->getAccessPoints()) {
      for (auto* ap : aps) {
        objects.insert(makeSelected(DbItermAccessPoint{ap, iterm}, nullptr));
      }
    }
  }
  return true;
}

}  // namespace gui
