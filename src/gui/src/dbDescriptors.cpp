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
    double dbuPerUU = inst->getBlock()->getDbUnitsPerMicron();
    props.insert(props.end(),
                 {{"Orientation", inst->getOrient().getString()},
                  {"X", x / dbuPerUU},
                  {"Y", y / dbuPerUU}});
  }
  SelectionSet iterms;
  for (auto iterm : inst->getITerms()) {
    iterms.insert(gui->makeSelected(iterm));
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
  int new_value = std::any_cast<double>(value) * inst->getBlock()->getDbUnitsPerMicron();
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
  if (wire && wire->getBBox(bbox)) {
    return true;
  }
  return false;
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

  // Draw regular routing
  odb::Rect rect;
  odb::dbWire* wire = net->getWire();
  if (wire) {
    odb::dbWireShapeItr it;
    it.begin(wire);
    odb::dbShape shape;
    while (it.next(shape)) {
      shape.getBox(rect);
      painter.drawRect(rect);
    }
  } else if (!net->getSigType().isSupply()) {
    std::set<odb::Point> driver_locs;
    std::set<odb::Point> sink_locs;
    for (auto inst_term : net->getITerms()) {
      odb::Point rect_center;
      int x, y;
      if (!inst_term->getAvgXY(&x, &y)) {
        auto inst_term_inst = inst_term->getInst();
        odb::dbBox* bbox = inst_term_inst->getBBox();
        odb::Rect rect;
        bbox->getBox(rect);
        rect_center = odb::Point((rect.xMax() + rect.xMin()) / 2.0,
                                 (rect.yMax() + rect.yMin()) / 2.0);
      } else
        rect_center = odb::Point(x, y);
      auto iotype = inst_term->getIoType();
      if (iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT) {
        if (sink_object != nullptr && sink_object != inst_term)
          continue;
        sink_locs.insert(rect_center);
      }
      if (iotype == odb::dbIoType::INOUT || iotype == odb::dbIoType::OUTPUT)
        driver_locs.insert(rect_center);
    }
    for (auto blk_term : net->getBTerms()) {
      auto blk_term_pins = blk_term->getBPins();
      auto iotype = blk_term->getIoType();
      bool driver_term = iotype == odb::dbIoType::INPUT
                         || iotype == odb::dbIoType::INOUT
                         || iotype == odb::dbIoType::FEEDTHRU;
      bool sink_term = iotype == odb::dbIoType::INOUT
                       || iotype == odb::dbIoType::OUTPUT
                       || iotype == odb::dbIoType::FEEDTHRU;
      for (auto pin : blk_term_pins) {
        auto pin_rect = pin->getBBox();
        odb::Point rect_center((pin_rect.xMax() + pin_rect.xMin()) / 2.0,
                               (pin_rect.yMax() + pin_rect.yMin()) / 2.0);
        if (driver_term == true)
          driver_locs.insert(rect_center);
        if (sink_term) {
          if (sink_object != nullptr && sink_object != blk_term) {
            continue;
          }

          sink_locs.insert(rect_center);
        }
      }
    }

    if (driver_locs.empty() || sink_locs.empty())
      return;
    auto color = painter.getPenColor();
    color.a = 255;
    painter.setPen(color, true);
    for (auto& driver : driver_locs) {
      for (auto& sink : sink_locs) {
        painter.drawLine(driver, sink);
      }
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
  return Properties({{"Instance", gui->makeSelected(iterm->getInst())},
                     {"IO type", iterm->getIoType().getString()},
                     {"Net", net_value},
                     {"Special", iterm->isSpecial()},
                     {"MTerm", iterm->getMTerm()->getConstName()}});
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
  double dbuPerUU = blockage->getBlock()->getDbUnitsPerMicron();
  return Properties({{"Instance", inst_value},
                     {"X", rect.xMin() / dbuPerUU},
                     {"Y", rect.yMin() / dbuPerUU},
                     {"Width", rect.dx() / dbuPerUU},
                     {"Height", rect.dy() / dbuPerUU},
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
  double dbuPerUU = obs->getBlock()->getDbUnitsPerMicron();
  Properties props({{"Instance", inst_value},
                    {"Layer", gui->makeSelected(obs->getBBox()->getTechLayer())},
                    {"X", rect.xMin() / dbuPerUU},
                    {"Y", rect.yMin() / dbuPerUU},
                    {"Width", rect.dx() / dbuPerUU},
                    {"Height", rect.dy() / dbuPerUU},
                    {"Slot", obs->isSlotObstruction()},
                    {"Fill", obs->isFillObstruction()}});
  if (obs->hasEffectiveWidth()) {
    props.push_back({"Effective width", obs->getEffectiveWidth() / dbuPerUU});
  }

  if (obs->hasMinSpacing()) {
    props.push_back({"Min spacing", obs->getMinSpacing() / dbuPerUU});
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
  double dbuPerUU = layer->getTech()->getDbUnitsPerMicron();
  Properties props({{"Direction", layer->getDirection().getString()},
                    {"Minimum width", layer->getWidth() / dbuPerUU},
                    {"Minimum spacing", layer->getSpacing() / dbuPerUU}});
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

}  // namespace gui
