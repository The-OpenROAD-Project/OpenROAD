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

#include <QInputDialog>
#include <QStringList>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <limits>
#include <queue>
#include <regex>
#include <sstream>

#include "bufferTreeDescriptor.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace gui {

static void populateODBProperties(Descriptor::Properties& props,
                                  odb::dbObject* object,
                                  const std::string& prefix = "")
{
  Descriptor::PropertyList prop_list;
  for (const auto prop : odb::dbProperty::getProperties(object)) {
    std::any value;
    switch (prop->getType()) {
      case odb::dbProperty::STRING_PROP: {
        auto str = static_cast<odb::dbStringProperty*>(prop)->getValue();

        std::vector<std::string> lines;
        boost::split(lines, str, boost::is_any_of("\n"));

        std::vector<std::string> trimmed_lines;
        for (auto& line : lines) {
          boost::algorithm::trim(line);
          if (!line.empty()) {
            trimmed_lines.push_back(line);
          }
        }

        value = boost::algorithm::join(trimmed_lines, "\n");
        break;
      }
      case odb::dbProperty::BOOL_PROP:
        value = static_cast<odb::dbBoolProperty*>(prop)->getValue();
        break;
      case odb::dbProperty::INT_PROP:
        value = static_cast<odb::dbIntProperty*>(prop)->getValue();
        break;
      case odb::dbProperty::DOUBLE_PROP:
        value = static_cast<odb::dbDoubleProperty*>(prop)->getValue();
        break;
    }
    prop_list.emplace_back(prop->getName(), value);
  }

  if (!prop_list.empty()) {
    std::string prop_name = "Properties";
    if (!prefix.empty()) {
      prop_name = prefix + " " + prop_name;
    }
    props.push_back({std::move(prop_name), prop_list});
  }
}

std::string Descriptor::convertUnits(const double value,
                                     const bool area,
                                     int digits)
{
  double log_value = value;
  if (area) {
    log_value = std::sqrt(log_value);
  }
  int log_units = std::floor(std::log10(log_value) / 3.0) * 3;
  double unit_scale = 1.0;
  std::string unit;
  if (log_units <= -18) {
    unit_scale = 1e18;
    unit = "a";
  } else if (log_units <= -15) {
    unit_scale = 1e15;
    unit = "f";
  } else if (log_units <= -12) {
    unit_scale = 1e12;
    unit = "p";
  } else if (log_units <= -9) {
    unit_scale = 1e9;
    unit = "n";
  } else if (log_units <= -6) {
    unit_scale = 1e6;
    const char* micron = "μ";
    unit = micron;
  } else if (log_units <= -3) {
    unit_scale = 1e3;
    unit = "m";
  } else if (log_units <= 0) {
  } else if (log_units <= 3) {
    unit_scale = 1e-3;
    unit = "k";
  } else if (log_units <= 6) {
    unit_scale = 1e-6;
    unit = "M";
  }
  if (area) {
    unit_scale *= unit_scale;
  }

  auto str = utl::to_numeric_string(value * unit_scale, digits);
  str += " " + unit;

  return str;
}

// renames an object
template <typename T>
static void addRenameEditor(T obj, Descriptor::Editors& editor)
{
  editor.insert(
      {"Name", Descriptor::makeEditor([obj](std::any value) {
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
template <typename T>
static void addTimingActions(T obj,
                             const Descriptor* desc,
                             Descriptor::Actions& actions)
{
  if (obj->getSigType().isSupply()) {
    // no timing actions needed
    return;
  }

  auto* gui = Gui::get();

  actions.push_back(
      {std::string(Descriptor::deselect_action_), [obj, desc, gui]() {
         gui->timingCone(static_cast<T>(nullptr), false, false);
         return desc->makeSelected(obj);
       }});
  actions.push_back({"Fanin Cone", [obj, desc, gui]() {
                       gui->timingCone(obj, true, false);
                       return desc->makeSelected(obj);
                     }});
  actions.push_back({"Fanout Cone", [obj, desc, gui]() {
                       gui->timingCone(obj, false, true);
                       return desc->makeSelected(obj);
                     }});
  actions.push_back({"Timing", [obj, desc, gui]() {
                       gui->timingPathsThrough({obj});
                       return desc->makeSelected(obj);
                     }});
}

// get list of tech layers as EditorOption list
static void addLayersToOptions(odb::dbTech* tech,
                               std::vector<Descriptor::EditorOption>& options)
{
  for (auto layer : tech->getLayers()) {
    options.push_back({layer->getName(), layer});
  }
}

// request user input to select tech layer, returns nullptr or current if none
// was selected
static odb::dbTechLayer* getLayerSelection(odb::dbTech* tech,
                                           odb::dbTechLayer* current = nullptr)
{
  std::vector<Descriptor::EditorOption> options;
  addLayersToOptions(tech, options);
  QStringList layers;
  for (const auto& [name, layer] : options) {
    layers.append(QString::fromStdString(name));
  }
  bool okay;
  int default_selection
      = current == nullptr
            ? 0
            : layers.indexOf(QString::fromStdString(current->getName()));
  QString selection = QInputDialog::getItem(nullptr,
                                            "Select technology layer",
                                            "Layer",
                                            layers,
                                            default_selection,  // current layer
                                            false,
                                            &okay);
  if (okay) {
    int selection_idx = layers.indexOf(selection);
    if (selection_idx != -1) {
      return std::any_cast<odb::dbTechLayer*>(options[selection_idx].value);
    }
    // selection not found, return current
    return current;
  }
  return current;
}

////////

DbTechDescriptor::DbTechDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbTechDescriptor::getName(std::any object) const
{
  return "Default";
}

std::string DbTechDescriptor::getTypeName() const
{
  return "Tech";
}

bool DbTechDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void DbTechDescriptor::highlight(std::any object, Painter& painter) const
{
}

Descriptor::Properties DbTechDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto tech = std::any_cast<odb::dbTech*>(object);

  Properties props({{"DbUnits per Micron", tech->getDbUnitsPerMicron()},
                    {"LEF Units", tech->getLefUnits()},
                    {"LEF Version", tech->getLefVersionStr()}});

  if (tech->hasManufacturingGrid()) {
    props.push_back(
        {"Manufacturing Grid",
         Property::convert_dbu(tech->getManufacturingGrid(), true)});
  }

  SelectionSet tech_layers;
  for (auto tech_layer : tech->getLayers()) {
    tech_layers.insert(gui->makeSelected(tech_layer));
  }
  props.push_back({"Tech Layers", tech_layers});

  SelectionSet tech_vias;
  for (auto tech_via : tech->getVias()) {
    tech_vias.insert(gui->makeSelected(tech_via));
  }
  props.push_back({"Tech Vias", tech_vias});

  SelectionSet via_rules;
  for (auto via_rule : tech->getViaRules()) {
    via_rules.insert(gui->makeSelected(via_rule));
  }
  props.push_back({"Tech Via Rules", via_rules});

  SelectionSet generate_vias;
  for (auto via : tech->getViaGenerateRules()) {
    generate_vias.insert(gui->makeSelected(via));
  }
  props.push_back({"Tech Via Generate Rules", generate_vias});

  SelectionSet via_maps;
  for (auto map : tech->getMetalWidthViaMap()) {
    via_maps.insert(gui->makeSelected(map));
  }
  props.push_back({"Metal Width Via Map Rules", via_maps});

  std::vector<odb::dbTechSameNetRule*> rule_samenets;
  tech->getSameNetRules(rule_samenets);
  SelectionSet samenet_rules;
  for (auto samenet : rule_samenets) {
    samenet_rules.insert(gui->makeSelected(samenet));
  }
  props.push_back({"Same Net Rules", samenet_rules});

  SelectionSet nondefault_rules;
  for (auto nondefault : tech->getNonDefaultRules()) {
    nondefault_rules.insert(gui->makeSelected(nondefault));
  }
  props.push_back({"Non-Default Rules", nondefault_rules});

  return props;
}

Selected DbTechDescriptor::makeSelected(std::any object) const
{
  if (auto tech = std::any_cast<odb::dbTech*>(&object)) {
    return Selected(*tech, this);
  }
  return Selected();
}

bool DbTechDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_tech = std::any_cast<odb::dbTech*>(l);
  auto r_tech = std::any_cast<odb::dbTech*>(r);
  return l_tech->getId() < r_tech->getId();
}

bool DbTechDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto tech = db_->getTech();
  if (tech == nullptr) {
    return false;
  }
  objects.insert(makeSelected(tech));
  return true;
}

//////////////////////////////////////////////////

DbBlockDescriptor::DbBlockDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbBlockDescriptor::getName(std::any object) const
{
  auto block = std::any_cast<odb::dbBlock*>(object);
  return block->getName();
}

std::string DbBlockDescriptor::getTypeName() const
{
  return "Block";
}

bool DbBlockDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto block = std::any_cast<odb::dbBlock*>(object);
  bbox = block->getBBox()->getBox();
  return !bbox.isInverted();
}

void DbBlockDescriptor::highlight(std::any object, Painter& painter) const
{
  auto block = std::any_cast<odb::dbBlock*>(object);

  odb::dbBox* bbox = block->getBBox();
  odb::Rect rect = bbox->getBox();
  if (!rect.isInverted()) {
    painter.drawRect(rect);
  }
}

Descriptor::Properties DbBlockDescriptor::getProperties(std::any object) const
{
  auto block = std::any_cast<odb::dbBlock*>(object);

  auto gui = Gui::get();

  Properties props;
  SelectionSet children;
  for (auto child : block->getChildren()) {
    children.insert(gui->makeSelected(child));
  }
  props.push_back({"Child Blocks", children});

  SelectionSet modules;
  for (auto module : block->getModules()) {
    modules.insert(gui->makeSelected(module));
  }
  props.push_back({"Modules", modules});

  props.push_back({"Top Module", gui->makeSelected(block->getTopModule())});

  SelectionSet bterms;
  for (auto bterm : block->getBTerms()) {
    bterms.insert(gui->makeSelected(bterm));
  }
  props.push_back({"BTerms", bterms});

  SelectionSet vias;
  for (auto via : block->getVias()) {
    vias.insert(gui->makeSelected(via));
  }
  props.push_back({"Block Vias", vias});

  SelectionSet nets;
  for (auto net : block->getNets()) {
    nets.insert(gui->makeSelected(net));
  }
  props.push_back({"Nets", nets});

  SelectionSet regions;
  for (auto region : block->getRegions()) {
    regions.insert(gui->makeSelected(region));
  }
  props.push_back({"Regions", regions});

  SelectionSet insts;
  for (auto inst : block->getInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.push_back({"Instances", insts});

  SelectionSet blockages;
  for (auto blockage : block->getBlockages()) {
    blockages.insert(gui->makeSelected(blockage));
  }
  props.push_back({"Blockages", blockages});

  SelectionSet obstructions;
  for (auto obstruction : block->getObstructions()) {
    obstructions.insert(gui->makeSelected(obstruction));
  }
  props.push_back({"Obstructions", obstructions});

  SelectionSet rows;
  for (auto row : block->getRows()) {
    rows.insert(gui->makeSelected(row));
  }
  props.push_back({"Rows", rows});

  populateODBProperties(props, block);

  props.push_back({"Core Area", block->getCoreArea()});
  props.push_back({"Die Area", block->getDieArea()});

  return props;
}

Selected DbBlockDescriptor::makeSelected(std::any object) const
{
  if (auto block = std::any_cast<odb::dbBlock*>(&object)) {
    return Selected(*block, this);
  }
  return Selected();
}

bool DbBlockDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_layer = std::any_cast<odb::dbBlock*>(l);
  auto r_layer = std::any_cast<odb::dbBlock*>(r);
  return l_layer->getId() < r_layer->getId();
}

bool DbBlockDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto block = chip->getBlock();
  objects.insert(makeSelected(block));
  return true;
}

//////////////////////////////////////////////////

DbInstDescriptor::DbInstDescriptor(odb::dbDatabase* db, sta::dbSta* sta)
    : db_(db), sta_(sta)
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
  bbox = inst->getBBox()->getBox();
  return !bbox.isInverted();
}

void DbInstDescriptor::highlight(std::any object, Painter& painter) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  if (!inst->getPlacementStatus().isPlaced()) {
    return;
  }

  odb::dbBox* bbox = inst->getBBox();
  odb::Rect rect = bbox->getBox();
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
  auto* module = inst->getModule();
  Properties props;
  props.push_back({"Block", gui->makeSelected(inst->getBlock())});
  if (module != nullptr) {
    props.push_back({"Module", gui->makeSelected(module)});
  }
  props.push_back({"Master", gui->makeSelected(inst->getMaster())});

  props.push_back(
      {"Description", sta_->getInstanceTypeText(sta_->getInstanceType(inst))});
  props.push_back({"Placement status", placed.getString()});
  props.push_back({"Source type", inst->getSourceType().getString()});
  props.push_back({"Dont Touch", inst->isDoNotTouch()});
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

  auto* group = inst->getGroup();
  if (group != nullptr) {
    props.push_back({"Group", gui->makeSelected(group)});
  }

  auto* region = inst->getRegion();
  if (region != nullptr) {
    props.push_back({"Region", gui->makeSelected(region)});
  }

  auto* sta_inst = sta_->getDbNetwork()->dbToSta(inst);
  if (sta_inst != nullptr) {
    props.push_back({"Timing/Power", gui->makeSelected(sta_inst)});
  }

  populateODBProperties(props, inst);
  return props;
}

Descriptor::Actions DbInstDescriptor::getActions(std::any object) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  return Actions(
      {{"Delete", [inst]() {
          odb::dbInst::destroy(inst);
          return Selected();  // unselect since this object is now gone
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
    editors.insert({"Master",
                    makeEditor(
                        [inst](std::any value) {
                          inst->swapMaster(
                              std::any_cast<odb::dbMaster*>(value));
                          return true;
                        },
                        master_options)});
  }
  editors.insert({"Orientation",
                  makeEditor(
                      [inst](std::any value) {
                        inst->setLocationOrient(
                            std::any_cast<odb::dbOrientType>(value));
                        return true;
                      },
                      orient_options)});
  editors.insert({"Placement status",
                  makeEditor(
                      [inst](std::any value) {
                        inst->setPlacementStatus(
                            std::any_cast<odb::dbPlacementStatus>(value));
                        return true;
                      },
                      placement_options)});

  editors.insert({"X", makeEditor([this, inst](const std::any& value) {
                    return setNewLocation(inst, value, true);
                  })});
  editors.insert({"Y", makeEditor([this, inst](const std::any& value) {
                    return setNewLocation(inst, value, false);
                  })});
  editors.insert({"Dont Touch", makeEditor([inst](const std::any& value) {
                    inst->setDoNotTouch(std::any_cast<bool>(value));
                    return true;
                  })});
  return editors;
}

// get list of equivalent masters as EditorOptions
void DbInstDescriptor::makeMasterOptions(
    odb::dbMaster* master,
    std::vector<EditorOption>& options) const
{
  std::set<odb::dbMaster*> masters;
  DbMasterDescriptor::getMasterEquivalent(sta_, master, masters);
  for (auto master : masters) {
    options.push_back({master->getConstName(), master});
  }
}

// get list if instance orientations for the editor
void DbInstDescriptor::makeOrientationOptions(
    std::vector<EditorOption>& options) const
{
  for (odb::dbOrientType type : {odb::dbOrientType::R0,
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
void DbInstDescriptor::makePlacementStatusOptions(
    std::vector<EditorOption>& options) const
{
  for (odb::dbPlacementStatus type : {odb::dbPlacementStatus::NONE,
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
bool DbInstDescriptor::setNewLocation(odb::dbInst* inst,
                                      const std::any& value,
                                      bool is_x) const
{
  bool accept = false;
  int new_value = Descriptor::Property::convert_string(
      std::any_cast<std::string>(value), &accept);
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

Selected DbInstDescriptor::makeSelected(std::any object) const
{
  if (auto inst = std::any_cast<odb::dbInst*>(&object)) {
    return Selected(*inst, this);
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
    objects.insert(makeSelected(inst));
  }
  return true;
}

//////////////////////////////////////////////////

DbMasterDescriptor::DbMasterDescriptor(odb::dbDatabase* db, sta::dbSta* sta)
    : db_(db), sta_(sta)
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

void DbMasterDescriptor::highlight(std::any object, Painter& painter) const
{
  auto master = std::any_cast<odb::dbMaster*>(object);
  std::set<odb::dbInst*> insts;
  getInstances(master, insts);
  for (auto inst : insts) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }

    odb::dbBox* bbox = inst->getBBox();
    odb::Rect rect = bbox->getBox();
    painter.drawRect(rect);
  }
}

Descriptor::Properties DbMasterDescriptor::getProperties(std::any object) const
{
  auto master = std::any_cast<odb::dbMaster*>(object);
  Properties props({{"Master type", master->getType().getString()}});
  auto gui = Gui::get();
  auto site = master->getSite();
  if (site != nullptr) {
    props.push_back({"Site", gui->makeSelected(site)});
  }
  SelectionSet mterms;
  for (auto mterm : master->getMTerms()) {
    mterms.insert(gui->makeSelected(mterm));
  }
  props.push_back({"MTerms", mterms});

  std::vector<std::any> symmetry;
  if (master->getSymmetryX()) {
    symmetry.emplace_back("X");
  }
  if (master->getSymmetryY()) {
    symmetry.emplace_back("Y");
  }
  if (master->getSymmetryR90()) {
    symmetry.emplace_back("R90");
  }
  props.push_back({"Symmetry", symmetry});

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
  props.push_back({"Origin", master->getOrigin()});

  populateODBProperties(props, master);

  auto liberty
      = sta_->getDbNetwork()->findLibertyCell(master->getName().c_str());
  if (liberty) {
    props.push_back({"Liberty", gui->makeSelected(liberty)});
  }

  return props;
}

Selected DbMasterDescriptor::makeSelected(std::any object) const
{
  if (auto master = std::any_cast<odb::dbMaster*>(&object)) {
    return Selected(*master, this);
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
  sta::LibertyLibraryIterator* lib_iter = network->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    libs.push_back(lib);
  }
  delete lib_iter;
  sta->makeEquivCells(&libs, nullptr);

  sta::Cell* cell = network->dbToSta(master);
  if (!cell) {
    return;
  }
  sta::LibertyCell* liberty_cell = network->libertyCell(cell);
  auto equiv_cells = sta->equivCells(liberty_cell);
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
void DbMasterDescriptor::getInstances(odb::dbMaster* master,
                                      std::set<odb::dbInst*>& insts) const
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
    objects.insert(makeSelected(master));
  }
  return true;
}

//////////////////////////////////////////////////

DbNetDescriptor::DbNetDescriptor(odb::dbDatabase* db,
                                 sta::dbSta* sta,
                                 const std::set<odb::dbNet*>& focus_nets,
                                 const std::set<odb::dbNet*>& guide_nets,
                                 const std::set<odb::dbNet*>& tracks_nets)
    : db_(db),
      sta_(sta),
      focus_nets_(focus_nets),
      guide_nets_(guide_nets),
      tracks_nets_(tracks_nets)
{
}

std::string DbNetDescriptor::getName(std::any object) const
{
  return getNet(object)->getName();
}

std::string DbNetDescriptor::getTypeName() const
{
  return "Net";
}

bool DbNetDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto net = getNet(object);
  auto wire = net->getWire();
  bool has_box = false;
  bbox.mergeInit();
  if (wire) {
    const auto opt_bbox = wire->getBBox();
    if (opt_bbox) {
      bbox.merge(opt_bbox.value());
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
  auto get_graph_iterm_targets = [](odb::dbMTerm* mterm,
                                    const odb::dbTransform& transform,
                                    std::vector<GraphTarget>& targets) {
    for (auto* mpin : mterm->getMPins()) {
      for (auto* box : mpin->getGeometry()) {
        if (box->isVia()) {
          odb::dbTechVia* tech_via = box->getTechVia();
          if (tech_via == nullptr) {
            continue;
          }

          const odb::dbTransform via_transform(box->getViaXY());
          for (auto* via_box : tech_via->getBoxes()) {
            odb::Rect box_rect = via_box->getBox();
            via_transform.apply(box_rect);
            transform.apply(box_rect);
            targets.emplace_back(box_rect, via_box->getTechLayer());
          }
        } else {
          odb::Rect rect = box->getBox();
          transform.apply(rect);
          targets.emplace_back(rect, box->getTechLayer());
        }
      }
    }
  };

  // gets all the shapes that make up the bterm
  auto get_graph_bterm_targets
      = [](odb::dbBTerm* bterm, std::vector<GraphTarget>& targets) {
          for (auto* bpin : bterm->getBPins()) {
            for (auto* box : bpin->getBoxes()) {
              odb::Rect rect = box->getBox();
              targets.emplace_back(rect, box->getTechLayer());
            }
          }
        };

  // find sources and sinks on this net
  for (auto* iterm : net->getITerms()) {
    if (iterm == sink) {
      const odb::dbTransform transform = iterm->getInst()->getTransform();
      get_graph_iterm_targets(iterm->getMTerm(), transform, sinks);
      continue;
    }

    auto iotype = iterm->getIoType();
    if (iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT) {
      const odb::dbTransform transform = iterm->getInst()->getTransform();
      get_graph_iterm_targets(iterm->getMTerm(), transform, sources);
    }
  }
  for (auto* bterm : net->getBTerms()) {
    if (bterm == sink) {
      get_graph_bterm_targets(bterm, sinks);
      continue;
    }

    auto iotype = bterm->getIoType();
    if (iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT
        || iotype == odb::dbIoType::FEEDTHRU) {
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

void DbNetDescriptor::drawPathSegment(odb::dbNet* net,
                                      const odb::dbObject* sink,
                                      Painter& painter) const
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

void DbNetDescriptor::buildNodeMap(odb::dbWireGraph* graph,
                                   NodeMap& node_map) const
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

  struct DistNode
  {
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
        path.emplace_back(x, y);
        current = came_from[current];
      }
      current->xy(x, y);
      path.emplace_back(x, y);
      return;
    }

    const int current_g_score = g_score[current];
    for (const auto& neighbor : graph[current]) {
      const int possible_g_score
          = current_g_score + distance(current, neighbor);
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
void DbNetDescriptor::highlight(std::any object, Painter& painter) const
{
  odb::dbObject* sink_object = getSink(object);
  auto net = getNet(object);

  auto* iterm_descriptor = Gui::get()->getDescriptor<odb::dbITerm*>();
  auto* bterm_descriptor = Gui::get()->getDescriptor<odb::dbBTerm*>();

  const bool is_supply = net->getSigType().isSupply();
  const bool is_routed_special
      = net->isSpecial() && net->getFirstSWire() != nullptr;
  auto should_draw_term = [sink_object](const odb::dbObject* term) -> bool {
    if (sink_object == nullptr) {
      return true;
    }
    return sink_object == term;
  };

  auto is_source_iterm = [](odb::dbITerm* iterm) -> bool {
    const auto iotype = iterm->getIoType();
    return iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT;
  };
  auto is_sink_iterm = [](odb::dbITerm* iterm) -> bool {
    const auto iotype = iterm->getIoType();
    return iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT;
  };

  auto is_source_bterm = [](odb::dbBTerm* bterm) -> bool {
    const auto iotype = bterm->getIoType();
    return iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT
           || iotype == odb::dbIoType::FEEDTHRU;
  };
  auto is_sink_bterm = [](odb::dbBTerm* bterm) -> bool {
    const auto iotype = bterm->getIoType();
    return iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT
           || iotype == odb::dbIoType::FEEDTHRU;
  };

  // Draw regular routing
  if (!is_supply) {  // don't draw iterms on supply nets
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

  odb::dbWire* wire = net->getWire();
  if (wire) {
    if (sink_object != nullptr) {
      drawPathSegment(net, sink_object, painter);
    }

    odb::dbWireShapeItr it;
    it.begin(wire);
    odb::dbShape shape;
    while (it.next(shape)) {
      painter.drawRect(shape.getBox());
    }
  } else if (!is_supply && !is_routed_special) {
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
        odb::Rect rect = bbox->getBox();
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
      for (const auto& driver : driver_locs) {
        for (const auto& sink : sink_locs) {
          painter.drawLine(driver, sink);
        }
      }
      painter.restoreState();
    }
  }

  // Draw special (i.e. geometric) routing
  for (auto swire : net->getSWires()) {
    for (auto sbox : swire->getWires()) {
      if (sbox->getDirection() == odb::dbSBox::OCTILINEAR) {
        painter.drawOctagon(sbox->getOct());
      } else {
        odb::Rect rect = sbox->getBox();
        painter.drawRect(rect);
      }
    }
  }
}

bool DbNetDescriptor::isSlowHighlight(std::any object) const
{
  auto net = getNet(object);
  return net->getSigType().isSupply();
}

bool DbNetDescriptor::isNet(std::any object) const
{
  return true;
}

Descriptor::Properties DbNetDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto net = getNet(object);
  Properties props({{"Block", gui->makeSelected(net->getBlock())},
                    {"Signal type", net->getSigType().getString()},
                    {"Source type", net->getSourceType().getString()},
                    {"Wire type", net->getWireType().getString()},
                    {"Special", net->isSpecial()},
                    {"Dont Touch", net->isDoNotTouch()}});
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
  props.push_back({"ITerms", std::move(iterm_item)});
  SelectionSet bterms;
  for (auto bterm : net->getBTerms()) {
    bterms.insert(gui->makeSelected(bterm));
  }
  props.push_back({"BTerms", bterms});

  auto* ndr = net->getNonDefaultRule();
  if (ndr != nullptr) {
    props.push_back({"Non-default rule", gui->makeSelected(ndr)});
  }

  if (BufferTree::isAggregate(net)) {
    props.push_back({"Buffer tree", gui->makeSelected(BufferTree(net))});
  }

  populateODBProperties(props, net);

  return props;
}

Descriptor::Editors DbNetDescriptor::getEditors(std::any object) const
{
  auto net = getNet(object);
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
  editors.insert({"Dont Touch", makeEditor([net](std::any value) {
                    net->setDoNotTouch(std::any_cast<bool>(value));
                    return true;
                  })});
  return editors;
}

Descriptor::Actions DbNetDescriptor::getActions(std::any object) const
{
  auto net = getNet(object);

  auto* gui = Gui::get();
  Descriptor::Actions actions;
  if (focus_nets_.count(net) == 0) {
    actions.push_back(Descriptor::Action{"Focus", [this, gui, net]() {
                                           gui->addFocusNet(net);
                                           return makeSelected(net);
                                         }});
  } else {
    actions.push_back(Descriptor::Action{"De-focus", [this, gui, net]() {
                                           gui->removeFocusNet(net);
                                           return makeSelected(net);
                                         }});
  }

  if (!net->getSigType().isSupply()) {
    actions.push_back(
        {"Timing", [this, gui, net]() {
           auto* network = sta_->getDbNetwork();
           auto* drivers = network->drivers(network->dbToSta(net));

           if (!drivers->empty()) {
             std::set<Gui::odbTerm> terms;

             for (auto* driver : *drivers) {
               odb::dbITerm* iterm = nullptr;
               odb::dbBTerm* bterm = nullptr;
               odb::dbModITerm* moditerm = nullptr;
               odb::dbModBTerm* modbterm = nullptr;

               network->staToDb(driver, iterm, bterm, moditerm, modbterm);
               if (iterm != nullptr) {
                 terms.insert(iterm);
               } else {
                 terms.insert(bterm);
               }
             }

             gui->timingPathsThrough(terms);
           }
           return makeSelected(net);
         }});
  }
  if (!net->getGuides().empty()) {
    actions.push_back(Descriptor::Action{"Route Guides", [this, gui, net]() {
                                           if (guide_nets_.count(net) == 0) {
                                             gui->addRouteGuides(net);
                                           } else {
                                             gui->removeRouteGuides(net);
                                           }
                                           return makeSelected(net);
                                         }});
  }
  if (!net->getTracks().empty()) {
    actions.push_back(Descriptor::Action{"Tracks", [this, gui, net]() {
                                           if (tracks_nets_.count(net) == 0) {
                                             gui->addNetTracks(net);
                                           } else {
                                             gui->removeNetTracks(net);
                                           }
                                           return makeSelected(net);
                                         }});
  }
  return actions;
}

Selected DbNetDescriptor::makeSelected(std::any object) const
{
  if (auto net = std::any_cast<odb::dbNet*>(&object)) {
    return Selected(*net, this);
  }
  if (auto net = std::any_cast<NetWithSink>(&object)) {
    return Selected(*net, this);
  }
  return Selected();
}

bool DbNetDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_net = getNet(l);
  auto r_net = getNet(r);
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
    objects.insert(makeSelected(net));
  }
  return true;
}

odb::dbNet* DbNetDescriptor::getNet(const std::any& object) const
{
  odb::dbNet* const* net = std::any_cast<odb::dbNet*>(&object);
  if (net != nullptr) {
    return *net;
  }
  return std::any_cast<NetWithSink>(object).net;
}

odb::dbObject* DbNetDescriptor::getSink(const std::any& object) const
{
  const NetWithSink* net_sink = std::any_cast<NetWithSink>(&object);
  if (net_sink != nullptr) {
    return net_sink->sink;
  }
  return nullptr;
}

//////////////////////////////////////////////////

DbITermDescriptor::DbITermDescriptor(
    odb::dbDatabase* db,
    std::function<bool(void)> usingPolyDecompView)
    : db_(db), usingPolyDecompView_(std::move(usingPolyDecompView))
{
}

std::string DbITermDescriptor::getName(std::any object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  return iterm->getName();
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

void DbITermDescriptor::highlight(std::any object, Painter& painter) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);

  if (!iterm->getInst()->getPlacementStatus().isPlaced()) {
    return;
  }

  const odb::dbTransform inst_xfm = iterm->getInst()->getTransform();

  auto mterm = iterm->getMTerm();
  for (auto mpin : mterm->getMPins()) {
    if (usingPolyDecompView_()) {
      for (auto box : mpin->getGeometry()) {
        odb::Rect rect = box->getBox();
        inst_xfm.apply(rect);
        painter.drawRect(rect);
      }
    } else {
      for (auto box : mpin->getPolygonGeometry()) {
        odb::Polygon poly = box->getPolygon();
        inst_xfm.apply(poly);
        painter.drawPolygon(poly);
      }
      for (auto box : mpin->getGeometry(false)) {
        odb::Rect rect = box->getBox();
        inst_xfm.apply(rect);
        painter.drawRect(rect);
      }
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
  for (const auto& [mpin, ap_vec] : iterm->getAccessPoints()) {
    for (const auto& ap : ap_vec) {
      DbTermAccessPoint iap{ap, iterm};
      aps.insert(gui->makeSelected(iap));
    }
  }
  Properties props{{"Instance", gui->makeSelected(iterm->getInst())},
                   {"Net", std::move(net_value)},
                   {"Special", iterm->isSpecial()},
                   {"MTerm", gui->makeSelected(iterm->getMTerm())},
                   {"Access Points", aps}};

  populateODBProperties(props, iterm);

  return props;
}

Descriptor::Actions DbITermDescriptor::getActions(std::any object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);

  Descriptor::Actions actions;
  addTimingActions<odb::dbITerm*>(iterm, this, actions);

  return actions;
}

Selected DbITermDescriptor::makeSelected(std::any object) const
{
  if (auto iterm = std::any_cast<odb::dbITerm*>(&object)) {
    return Selected(*iterm, this);
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
    objects.insert(makeSelected(term));
  }
  return true;
}

//////////////////////////////////////////////////

DbBTermDescriptor::DbBTermDescriptor(odb::dbDatabase* db) : db_(db)
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
  return !bbox.isInverted();
}

void DbBTermDescriptor::highlight(std::any object, Painter& painter) const
{
  auto* bterm = std::any_cast<odb::dbBTerm*>(object);
  for (auto bpin : bterm->getBPins()) {
    for (auto box : bpin->getBoxes()) {
      odb::Rect rect = box->getBox();
      painter.drawRect(rect);
    }
  }
}

Descriptor::Properties DbBTermDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto bterm = std::any_cast<odb::dbBTerm*>(object);
  SelectionSet aps;
  for (auto* pin : bterm->getBPins()) {
    for (auto ap : pin->getAccessPoints()) {
      DbTermAccessPoint bap{ap, bterm};
      aps.insert(gui->makeSelected(bap));
    }
  }
  Properties props{{"Block", gui->makeSelected(bterm->getBlock())},
                   {"Net", gui->makeSelected(bterm->getNet())},
                   {"Signal type", bterm->getSigType().getString()},
                   {"IO type", bterm->getIoType().getString()},
                   {"Access Points", aps}};

  populateODBProperties(props, bterm);

  return props;
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
  addTimingActions<odb::dbBTerm*>(bterm, this, actions);

  return actions;
}

Selected DbBTermDescriptor::makeSelected(std::any object) const
{
  if (auto bterm = std::any_cast<odb::dbBTerm*>(&object)) {
    return Selected(*bterm, this);
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
    objects.insert(makeSelected(term));
  }
  return true;
}

//////////////////////////////////////////////////

DbMTermDescriptor::DbMTermDescriptor(
    odb::dbDatabase* db,
    std::function<bool(void)> usingPolyDecompView)
    : db_(db), usingPolyDecompView_(std::move(usingPolyDecompView))
{
}

std::string DbMTermDescriptor::getName(std::any object) const
{
  auto mterm = std::any_cast<odb::dbMTerm*>(object);
  return mterm->getMaster()->getName() + "/" + mterm->getName();
}

std::string DbMTermDescriptor::getShortName(std::any object) const
{
  auto mterm = std::any_cast<odb::dbMTerm*>(object);
  return mterm->getName();
}

std::string DbMTermDescriptor::getTypeName() const
{
  return "MTerm";
}

bool DbMTermDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto mterm = std::any_cast<odb::dbMTerm*>(object);
  bbox = mterm->getBBox();
  return true;
}

void DbMTermDescriptor::highlight(std::any object, Painter& painter) const
{
  auto mterm = std::any_cast<odb::dbMTerm*>(object);

  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  std::vector<odb::Polygon> mterm_polys;

  for (auto mpin : mterm->getMPins()) {
    if (usingPolyDecompView_()) {
      for (auto box : mpin->getGeometry()) {
        mterm_polys.emplace_back(box->getBox());
      }
    } else {
      for (auto box : mpin->getPolygonGeometry()) {
        mterm_polys.push_back(box->getPolygon());
      }
      for (auto box : mpin->getGeometry(false)) {
        mterm_polys.emplace_back(box->getBox());
      }
    }
  }
  for (auto* iterm : block->getITerms()) {
    if (iterm->getMTerm() == mterm) {
      if (!iterm->getInst()->getPlacementStatus().isPlaced()) {
        continue;
      }
      const odb::dbTransform inst_xfm = iterm->getInst()->getTransform();

      for (odb::Polygon poly : mterm_polys) {
        inst_xfm.apply(poly);
        painter.drawPolygon(poly);
      }
    }
  }
}

Descriptor::Properties DbMTermDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto mterm = std::any_cast<odb::dbMTerm*>(object);
  SelectionSet layers;
  for (auto* mpin : mterm->getMPins()) {
    for (auto* geom : mpin->getGeometry()) {
      auto* layer = geom->getTechLayer();
      if (layer != nullptr) {
        layers.insert(gui->makeSelected(layer));
      }
    }
  }
  Properties props{{"Master", gui->makeSelected(mterm->getMaster())},
                   {"IO type", mterm->getIoType().getString()},
                   {"Signal type", mterm->getSigType().getString()},
                   {"# Pins", mterm->getMPins().size()},
                   {"Layers", layers}};

  populateODBProperties(props, mterm);

  return props;
}

Selected DbMTermDescriptor::makeSelected(std::any object) const
{
  if (auto mterm = std::any_cast<odb::dbMTerm*>(&object)) {
    return Selected(*mterm, this);
  }
  return Selected();
}

bool DbMTermDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_mterm = std::any_cast<odb::dbMTerm*>(l);
  auto r_mterm = std::any_cast<odb::dbMTerm*>(r);
  return l_mterm->getId() < r_mterm->getId();
}

bool DbMTermDescriptor::getAllObjects(SelectionSet& objects) const
{
  for (auto* lib : db_->getLibs()) {
    for (auto* master : lib->getMasters()) {
      for (auto* mterm : master->getMTerms()) {
        objects.insert(makeSelected(mterm));
      }
    }
  }

  return true;
}

//////////////////////////////////////////////////

DbViaDescriptor::DbViaDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbViaDescriptor::getName(std::any object) const
{
  auto via = std::any_cast<odb::dbVia*>(object);
  return via->getName();
}

std::string DbViaDescriptor::getTypeName() const
{
  return "Block Via";
}

bool DbViaDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void DbViaDescriptor::highlight(std::any object, Painter& painter) const
{
}

Descriptor::Properties DbViaDescriptor::getProperties(std::any object) const
{
  auto via = std::any_cast<odb::dbVia*>(object);
  auto gui = Gui::get();

  Properties props({{"Block", gui->makeSelected(via->getBlock())}});

  if (!via->getPattern().empty()) {
    props.push_back({"Pattern", via->getPattern()});
  }

  props.push_back(
      {"Tech Via Generate Rule", gui->makeSelected(via->getViaGenerateRule())});

  if (via->hasParams()) {
    const odb::dbViaParams via_params = via->getViaParams();

    props.push_back(
        {"Cut Size",
         fmt::format("X={}, Y={}",
                     Property::convert_dbu(via_params.getXCutSize(), true),
                     Property::convert_dbu(via_params.getYCutSize(), true))});

    props.push_back(
        {"Cut Spacing",
         fmt::format(
             "X={}, Y={}",
             Property::convert_dbu(via_params.getXCutSpacing(), true),
             Property::convert_dbu(via_params.getYCutSpacing(), true))});

    props.push_back(
        {"Top Enclosure",
         fmt::format(
             "X={}, Y={}",
             Property::convert_dbu(via_params.getXTopEnclosure(), true),
             Property::convert_dbu(via_params.getYTopEnclosure(), true))});

    props.push_back(
        {"Bottom Enclosure",
         fmt::format(
             "X={}, Y={}",
             Property::convert_dbu(via_params.getXBottomEnclosure(), true),
             Property::convert_dbu(via_params.getYBottomEnclosure(), true))});

    props.push_back({"Number of Cut Rows", via_params.getNumCutRows()});
    props.push_back({"Number of Cut Columns", via_params.getNumCutCols()});

    props.push_back(
        {"Origin",
         fmt::format("X={}, Y={}",
                     Property::convert_dbu(via_params.getXOrigin(), true),
                     Property::convert_dbu(via_params.getYOrigin(), true))});

    props.push_back(
        {"Top Offset",
         fmt::format("X={}, Y={}",
                     Property::convert_dbu(via_params.getXTopOffset(), true),
                     Property::convert_dbu(via_params.getYTopOffset(), true))});

    props.push_back(
        {"Bottom Offset",
         fmt::format(
             "X={}, Y={}",
             Property::convert_dbu(via_params.getXBottomOffset(), true),
             Property::convert_dbu(via_params.getYBottomOffset(), true))});

    PropertyList shapes;
    for (auto box : via->getBoxes()) {
      auto layer = box->getTechLayer();
      auto rect = box->getBox();
      shapes.push_back({gui->makeSelected(layer), rect});
    }
    props.push_back({"Shapes", shapes});
  } else {
    PropertyList shapes;
    for (auto box : via->getBoxes()) {
      auto layer = box->getTechLayer();
      auto rect = box->getBox();
      shapes.push_back({gui->makeSelected(layer), rect});
    }
    props.push_back({"Shapes", shapes});
  }

  props.push_back({"Is Rotated", via->isViaRotated()});

  if (via->isViaRotated()) {
    props.push_back({"Orientation", via->getOrient().getString()});
    props.push_back({"Tech Via", gui->makeSelected(via->getTechVia())});
    props.push_back({"Block Via", gui->makeSelected(via->getBlockVia())});
  }

  props.push_back({"Is Default", via->isDefault()});

  return props;
}

Selected DbViaDescriptor::makeSelected(std::any object) const
{
  if (auto via = std::any_cast<odb::dbVia*>(&object)) {
    return Selected(*via, this);
  }
  return Selected();
}

bool DbViaDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_via = std::any_cast<odb::dbVia*>(l);
  auto r_via = std::any_cast<odb::dbVia*>(r);
  return l_via->getId() < r_via->getId();
}

bool DbViaDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }

  auto block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto via : block->getVias()) {
    objects.insert(makeSelected(via));
  }

  return true;
}

//////////////////////////////////////////////////

DbBlockageDescriptor::DbBlockageDescriptor(odb::dbDatabase* db) : db_(db)
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
  bbox = box->getBox();
  return true;
}

void DbBlockageDescriptor::highlight(std::any object, Painter& painter) const
{
  odb::Rect rect;
  getBBox(std::move(object), rect);
  painter.drawRect(rect);
}

Descriptor::Properties DbBlockageDescriptor::getProperties(
    std::any object) const
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
  odb::Rect rect = blockage->getBBox()->getBox();
  Properties props{
      {"Block", gui->makeSelected(blockage->getBlock())},
      {"Instance", std::move(inst_value)},
      {"X", Property::convert_dbu(rect.xMin(), true)},
      {"Y", Property::convert_dbu(rect.yMin(), true)},
      {"Width", Property::convert_dbu(rect.dx(), true)},
      {"Height", Property::convert_dbu(rect.dy(), true)},
      {"Soft", blockage->isSoft()},
      {"Max density", std::to_string(blockage->getMaxDensity()) + "%"}};

  populateODBProperties(props, blockage);

  return props;
}

Descriptor::Editors DbBlockageDescriptor::getEditors(std::any object) const
{
  auto blockage = std::any_cast<odb::dbBlockage*>(object);
  Editors editors;
  editors.insert({"Max density", makeEditor([blockage](std::any any_value) {
                    std::string value = std::any_cast<std::string>(any_value);
                    std::regex density_regex(
                        "(1?[0-9]?[0-9]?(\\.[0-9]*)?)\\s*%?");
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

Selected DbBlockageDescriptor::makeSelected(std::any object) const
{
  if (auto blockage = std::any_cast<odb::dbBlockage*>(&object)) {
    return Selected(*blockage, this);
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
    objects.insert(makeSelected(blockage));
  }
  return true;
}

//////////////////////////////////////////////////

DbObstructionDescriptor::DbObstructionDescriptor(odb::dbDatabase* db) : db_(db)
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
  bbox = box->getBox();
  return true;
}

void DbObstructionDescriptor::highlight(std::any object, Painter& painter) const
{
  odb::Rect rect;
  getBBox(std::move(object), rect);
  painter.drawRect(rect);
}

Descriptor::Properties DbObstructionDescriptor::getProperties(
    std::any object) const
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
  odb::Rect rect = obs->getBBox()->getBox();
  Properties props(
      {{"Block", gui->makeSelected(obs->getBlock())},
       {"Instance", std::move(inst_value)},
       {"Layer", gui->makeSelected(obs->getBBox()->getTechLayer())},
       {"X", Property::convert_dbu(rect.xMin(), true)},
       {"Y", Property::convert_dbu(rect.yMin(), true)},
       {"Width", Property::convert_dbu(rect.dx(), true)},
       {"Height", Property::convert_dbu(rect.dy(), true)},
       {"Slot", obs->isSlotObstruction()},
       {"Fill", obs->isFillObstruction()}});
  if (obs->hasEffectiveWidth()) {
    props.push_back({"Effective width",
                     Property::convert_dbu(obs->getEffectiveWidth(), true)});
  }

  if (obs->hasMinSpacing()) {
    props.push_back(
        {"Min spacing", Property::convert_dbu(obs->getMinSpacing(), true)});
  }

  populateODBProperties(props, obs);

  return props;
}

Descriptor::Actions DbObstructionDescriptor::getActions(std::any object) const
{
  auto obs = std::any_cast<odb::dbObstruction*>(object);
  return Actions(
      {{"Copy to layer",
        [obs]() {
          odb::dbBox* box = obs->getBBox();
          odb::dbTechLayer* layer = getLayerSelection(
              obs->getBlock()->getDataBase()->getTech(), box->getTechLayer());
          auto gui = gui::Gui::get();
          if (layer == nullptr) {
            // select old layer again
            return gui->makeSelected(obs);
          }
          auto new_obs = odb::dbObstruction::create(obs->getBlock(),
                                                    layer,
                                                    box->xMin(),
                                                    box->yMin(),
                                                    box->xMax(),
                                                    box->yMax());
          // does not copy other parameters
          return gui->makeSelected(new_obs);
        }},
       {"Delete", [obs]() {
          odb::dbObstruction::destroy(obs);
          return Selected();  // unselect since this object is now gone
        }}});
}

Selected DbObstructionDescriptor::makeSelected(std::any object) const
{
  if (auto obs = std::any_cast<odb::dbObstruction*>(&object)) {
    return Selected(*obs, this);
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
    objects.insert(makeSelected(obs));
  }
  return true;
}

//////////////////////////////////////////////////

DbTechLayerDescriptor::DbTechLayerDescriptor(odb::dbDatabase* db) : db_(db)
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

void DbTechLayerDescriptor::highlight(std::any object, Painter& painter) const
{
}

Descriptor::Properties DbTechLayerDescriptor::getProperties(
    std::any object) const
{
  auto* gui = Gui::get();
  auto layer = std::any_cast<odb::dbTechLayer*>(object);
  Properties props({{"Technology", gui->makeSelected(layer->getTech())},
                    {"Direction", layer->getDirection().getString()},
                    {"Layer type", layer->getType().getString()}});
  if (layer->getLef58Type() != odb::dbTechLayer::NONE) {
    props.push_back({"LEF58 type", layer->getLef58TypeString()});
  }
  props.push_back({"Layer number", layer->getNumber()});
  if (layer->getType() == odb::dbTechLayerType::ROUTING) {
    props.push_back({"Routing layer", layer->getRoutingLevel()});
  }
  if (layer->hasXYPitch()) {
    if (layer->getPitchX() != 0) {
      props.push_back(
          {"Pitch X", Property::convert_dbu(layer->getPitchX(), true)});
    }
    if (layer->getPitchY() != 0) {
      props.push_back(
          {"Pitch Y", Property::convert_dbu(layer->getPitchY(), true)});
    }
  } else {
    if (layer->getPitch() != 0) {
      props.push_back(
          {"Pitch", Property::convert_dbu(layer->getPitch(), true)});
    }
  }
  if (layer->getWidth() != 0) {
    props.push_back(
        {"Default width", Property::convert_dbu(layer->getWidth(), true)});
  }
  if (layer->getMinWidth() != 0) {
    props.push_back(
        {"Minimum width", Property::convert_dbu(layer->getMinWidth(), true)});
  }
  if (layer->hasMaxWidth()) {
    props.push_back(
        {"Max width", Property::convert_dbu(layer->getMaxWidth(), true)});
  }
  if (layer->getSpacing() != 0) {
    props.push_back(
        {"Minimum spacing", Property::convert_dbu(layer->getSpacing(), true)});
  }
  if (layer->hasArea()) {
    props.push_back(
        {"Minimum area",
         convertUnits(layer->getArea() * 1e-6 * 1e-6, true) + "m²"});
  }
  if (layer->getResistance() != 0.0) {
    props.push_back(
        {"Resistance", convertUnits(layer->getResistance()) + "Ω/sq"});
  }
  if (layer->getCapacitance() != 0.0) {
    props.push_back({"Capacitance",
                     convertUnits(layer->getCapacitance() * 1e-12) + "F/μm²"});
  }
  if (layer->getEdgeCapacitance() != 0.0) {
    props.push_back(
        {"Edge capacitance",
         convertUnits(layer->getEdgeCapacitance() * 1e-12) + "F/μm"});
  }

  for (auto* width_table : layer->getTechLayerWidthTableRules()) {
    std::string title = "Width table";
    if (width_table->isWrongDirection()) {
      title += " - wrong direction";
    }
    if (width_table->isOrthogonal()) {
      title += " - orthogonal";
    }

    std::vector<std::any> widths;
    for (auto width : width_table->getWidthTable()) {
      widths.emplace_back(Property::convert_dbu(width, true));
    }
    props.push_back({std::move(title), widths});
  }

  PropertyList cutclasses;
  for (auto* cutclass : layer->getTechLayerCutClassRules()) {
    std::string text
        = Property::convert_dbu(cutclass->getWidth(), true) + " x ";
    if (cutclass->isLengthValid()) {
      text += Property::convert_dbu(cutclass->getLength(), true);
    } else {
      text += Property::convert_dbu(cutclass->getWidth(), true);
    }

    if (cutclass->isCutsValid()) {
      text += " - " + std::to_string(cutclass->getNumCuts()) + " cuts";
    }

    cutclasses.emplace_back(cutclass->getName(), text);
  }
  if (!cutclasses.empty()) {
    props.push_back({"Cut classes", cutclasses});
  }

  PropertyList cut_enclosures;
  for (auto* enc_rule : layer->getTechLayerCutEnclosureRules()) {
    std::string text = Property::convert_dbu(enc_rule->getMinWidth(), true);

    if (enc_rule->getCutClass() != nullptr) {
      text += " - ";
      text += enc_rule->getCutClass()->getName();
    }
    if (enc_rule->isAbove()) {
      text += " - above";
    }
    if (enc_rule->isBelow()) {
      text += " - below";
    }

    const std::string enc0
        = Property::convert_dbu(enc_rule->getFirstOverhang(), true);
    const std::string enc1
        = Property::convert_dbu(enc_rule->getSecondOverhang(), true);
    std::stringstream enclosure;
    switch (enc_rule->getType()) {
      case odb::dbTechLayerCutEnclosureRule::DEFAULT:
        enclosure << enc0 << " x " << enc1;
        break;
      case odb::dbTechLayerCutEnclosureRule::EOL:
        enclosure << "EOL: " << enc0 << " x " << enc1;
        break;
      case odb::dbTechLayerCutEnclosureRule::ENDSIDE:
        enclosure << "End: " << enc0 << " x Side: " << enc1;
        break;
      case odb::dbTechLayerCutEnclosureRule::HORZ_AND_VERT:
        enclosure << "Horizontal: " << enc0 << " x Vertical: " << enc1;
        break;
    }

    cut_enclosures.emplace_back(text, enclosure.str());
  }
  if (!cut_enclosures.empty()) {
    props.push_back({"Cut enclosures", cut_enclosures});
  }

  PropertyList minimum_cuts;
  for (auto* min_cut_rule : layer->getMinCutRules()) {
    uint numcuts;
    uint rule_width;
    min_cut_rule->getMinimumCuts(numcuts, rule_width);

    std::string text = Property::convert_dbu(rule_width, true);

    if (min_cut_rule->isAboveOnly()) {
      text += " - above only";
    }
    if (min_cut_rule->isBelowOnly()) {
      text += " - below only";
    }

    uint length;
    uint distance;
    if (min_cut_rule->getLengthForCuts(length, distance)) {
      text += fmt::format(" LENGTH {} WITHIN {}",
                          Property::convert_dbu(length, true),
                          Property::convert_dbu(distance, true));
    }

    minimum_cuts.emplace_back(text, static_cast<int>(numcuts));
  }
  if (!minimum_cuts.empty()) {
    props.push_back({"Minimum cuts", minimum_cuts});
  }

  PropertyList lef58_minimum_cuts;
  for (auto* min_cut_rule : layer->getTechLayerMinCutRules()) {
    std::string text = Property::convert_dbu(min_cut_rule->getWidth(), true);

    if (min_cut_rule->isFromAbove()) {
      text += " - from above";
    }
    if (min_cut_rule->isFromBelow()) {
      text += " - from below";
    }

    for (const auto& [cutclass, min_cut] : min_cut_rule->getCutClassCutsMap()) {
      lef58_minimum_cuts.emplace_back(text + " - " + cutclass, min_cut);
    }
  }
  if (!lef58_minimum_cuts.empty()) {
    props.push_back({"LEF58 minimum cuts", lef58_minimum_cuts});
  }

  if (layer->getType() == odb::dbTechLayerType::CUT) {
    auto* tech = layer->getTech();

    SelectionSet generate_vias;
    for (auto* via : tech->getViaGenerateRules()) {
      for (uint l = 0; l < via->getViaLayerRuleCount(); l++) {
        auto* rule = via->getViaLayerRule(l);
        if (rule->getLayer() == layer) {
          generate_vias.insert(gui->makeSelected(via));
          break;
        }
      }
    }
    props.push_back({"Generate vias", generate_vias});

    SelectionSet tech_vias;
    for (auto* via : tech->getVias()) {
      for (auto* box : via->getBoxes()) {
        if (box->getTechLayer() == layer) {
          tech_vias.insert(gui->makeSelected(via));
          break;
        }
      }
    }
    props.push_back({"Tech vias", tech_vias});
  }

  populateODBProperties(props, layer);

  return props;
}

Selected DbTechLayerDescriptor::makeSelected(std::any object) const
{
  if (auto layer = std::any_cast<odb::dbTechLayer*>(&object)) {
    return Selected(*layer, this);
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
    objects.insert(makeSelected(layer));
  }
  return true;
}

//////////////////////////////////////////////////

DbTermAccessPointDescriptor::DbTermAccessPointDescriptor(odb::dbDatabase* db)
    : db_(db)
{
}

std::string DbTermAccessPointDescriptor::getName(std::any object) const
{
  auto iterm_ap = std::any_cast<DbTermAccessPoint>(object);
  auto ap = iterm_ap.ap;
  std::string name(ap->getLowType().getString());
  name += std::string("/") + ap->getHighType().getString();
  return name;
}

std::string DbTermAccessPointDescriptor::getTypeName() const
{
  return "Access Point";
}

bool DbTermAccessPointDescriptor::getBBox(std::any object,
                                          odb::Rect& bbox) const
{
  auto iterm_ap = std::any_cast<DbTermAccessPoint>(object);
  odb::Point pt = iterm_ap.ap->getPoint();
  if (iterm_ap.iterm) {
    int x, y;
    iterm_ap.iterm->getInst()->getLocation(x, y);
    odb::dbTransform xform({x, y});
    xform.apply(pt);
  }
  bbox = {pt, pt};
  return true;
}

void DbTermAccessPointDescriptor::highlight(std::any object,
                                            Painter& painter) const
{
  auto iterm_ap = std::any_cast<DbTermAccessPoint>(object);
  odb::Point pt = iterm_ap.ap->getPoint();
  if (iterm_ap.iterm) {
    int x, y;
    iterm_ap.iterm->getInst()->getLocation(x, y);
    odb::dbTransform xform({x, y});
    xform.apply(pt);
  }
  const int shape_size = 100;
  painter.drawX(pt.x(), pt.y(), shape_size);
}

Descriptor::Properties DbTermAccessPointDescriptor::getProperties(
    std::any object) const
{
  auto iterm_ap = std::any_cast<DbTermAccessPoint>(object);
  auto ap = iterm_ap.ap;

  std::vector<odb::dbDirection> accesses;
  ap->getAccesses(accesses);

  std::string directions;
  for (const auto& dir : accesses) {
    if (!directions.empty()) {
      directions += ", ";
    }
    directions += dir.getString();
  }

  auto gui = Gui::get();
  Properties props({{"Low Type", ap->getLowType().getString()},
                    {"High Type", ap->getHighType().getString()},
                    {"Directions", directions},
                    {"Layer", gui->makeSelected(ap->getLayer())}});

  auto vias_by_cuts = ap->getVias();
  for (int cuts = 0; cuts < vias_by_cuts.size(); ++cuts) {
    Descriptor::PropertyList vias_property;
    int cnt = 1;
    for (auto via : vias_by_cuts[cuts]) {
      std::string name;
      if (via->getObjectType() == odb::dbTechViaObj) {
        name = static_cast<odb::dbTechVia*>(via)->getName();
      } else {
        name = static_cast<odb::dbVia*>(via)->getName();
      }
      vias_property.push_back({cnt++, name});
    }
    props.push_back({fmt::format("{} cut vias", cuts + 1), vias_property});
  }

  populateODBProperties(props, ap);
  return props;
}

Selected DbTermAccessPointDescriptor::makeSelected(std::any object) const
{
  if (object.type() == typeid(DbTermAccessPoint)) {
    auto iterm_ap = std::any_cast<DbTermAccessPoint>(object);
    return Selected(iterm_ap, this);
  }
  return Selected();
}

bool DbTermAccessPointDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_term_ap = std::any_cast<DbTermAccessPoint>(l);
  auto r_term_ap = std::any_cast<DbTermAccessPoint>(r);
  if (l_term_ap.iterm != r_term_ap.iterm && l_term_ap.iterm != nullptr
      && r_term_ap.iterm != nullptr) {
    return l_term_ap.iterm->getId() < r_term_ap.iterm->getId();
  }
  if (l_term_ap.bterm != r_term_ap.bterm && l_term_ap.bterm != nullptr
      && r_term_ap.bterm != nullptr) {
    return l_term_ap.bterm->getId() < r_term_ap.bterm->getId();
  }
  return l_term_ap.ap->getId() < r_term_ap.ap->getId();
}

bool DbTermAccessPointDescriptor::getAllObjects(SelectionSet& objects) const
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
        objects.insert(makeSelected(DbTermAccessPoint{ap, iterm}));
      }
    }
  }

  for (auto* bterm : block->getBTerms()) {
    for (auto* pin : bterm->getBPins()) {
      for (auto* ap : pin->getAccessPoints()) {
        objects.insert(makeSelected(DbTermAccessPoint{ap, bterm}));
      }
    }
  }
  return true;
}

//////////////////////////////////////////////////

DbGroupDescriptor::DbGroupDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbGroupDescriptor::getName(std::any object) const
{
  auto* group = std::any_cast<odb::dbGroup*>(object);
  return group->getName();
}

std::string DbGroupDescriptor::getTypeName() const
{
  return "Group";
}

bool DbGroupDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto* group = std::any_cast<odb::dbGroup*>(object);
  auto* region = group->getRegion();
  if (region != nullptr && region->getBoundaries().size() == 1) {
    bbox = region->getBoundaries().begin()->getBox();
    return true;
  }
  return false;
}

void DbGroupDescriptor::highlight(std::any object, Painter& painter) const
{
  auto* group = std::any_cast<odb::dbGroup*>(object);
  auto* inst_descriptor = Gui::get()->getDescriptor<odb::dbInst*>();
  for (auto* inst : group->getInsts()) {
    inst_descriptor->highlight(inst, painter);
  }
  for (auto* subgroup : group->getGroups()) {
    highlight(subgroup, painter);
  }
}

Descriptor::Properties DbGroupDescriptor::getProperties(std::any object) const
{
  auto* group = std::any_cast<odb::dbGroup*>(object);

  auto* gui = Gui::get();

  Properties props;
  auto* parent = group->getParentGroup();
  if (parent != nullptr) {
    props.push_back({"Parent", gui->makeSelected(parent)});
  }

  auto* region = group->getRegion();
  if (region != nullptr) {
    props.push_back({"Region", gui->makeSelected(region)});
  }

  SelectionSet groups;
  for (auto* subgroup : group->getGroups()) {
    groups.insert(gui->makeSelected(subgroup));
  }
  if (!groups.empty()) {
    props.push_back({"Groups", groups});
  }

  SelectionSet insts;
  for (auto* inst : group->getInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.push_back({"Instances", insts});

  props.push_back({"Group Type", group->getType().getString()});

  SelectionSet pwr;
  for (auto* net : group->getPowerNets()) {
    pwr.insert(gui->makeSelected(net));
  }
  props.push_back({"Power Nets", pwr});

  SelectionSet gnd;
  for (auto* net : group->getGroundNets()) {
    gnd.insert(gui->makeSelected(net));
  }
  props.push_back({"Ground Nets", gnd});

  populateODBProperties(props, group);

  return props;
}

Selected DbGroupDescriptor::makeSelected(std::any object) const
{
  if (auto group = std::any_cast<odb::dbGroup*>(&object)) {
    return Selected(*group, this);
  }
  return Selected();
}

bool DbGroupDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_layer = std::any_cast<odb::dbGroup*>(l);
  auto r_layer = std::any_cast<odb::dbGroup*>(r);
  return l_layer->getId() < r_layer->getId();
}

bool DbGroupDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* group : block->getGroups()) {
    objects.insert(makeSelected(group));
  }
  return true;
}

//////////////////////////////////////////////////

DbRegionDescriptor::DbRegionDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbRegionDescriptor::getName(std::any object) const
{
  auto* region = std::any_cast<odb::dbRegion*>(object);
  return region->getName();
}

std::string DbRegionDescriptor::getTypeName() const
{
  return "Region";
}

bool DbRegionDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto* region = std::any_cast<odb::dbRegion*>(object);
  auto boxes = region->getBoundaries();
  if (boxes.empty()) {
    return false;
  }
  bbox.mergeInit();
  for (auto* box : boxes) {
    odb::Rect box_rect = box->getBox();
    bbox.merge(box_rect);
  }
  return true;
}

void DbRegionDescriptor::highlight(std::any object, Painter& painter) const
{
  auto* region = std::any_cast<odb::dbRegion*>(object);

  for (auto box : region->getBoundaries()) {
    painter.drawRect(box->getBox());
  }

  auto* group_descriptor = Gui::get()->getDescriptor<odb::dbGroup*>();
  for (auto* child : region->getGroups()) {
    group_descriptor->highlight(child, painter);
  }

  auto* inst_descriptor = Gui::get()->getDescriptor<odb::dbInst*>();
  for (auto* inst : region->getRegionInsts()) {
    inst_descriptor->highlight(inst, painter);
  }
}

Descriptor::Properties DbRegionDescriptor::getProperties(std::any object) const
{
  auto* region = std::any_cast<odb::dbRegion*>(object);

  auto* gui = Gui::get();

  Properties props({{"Block", gui->makeSelected(region->getBlock())},
                    {"Region Type", region->getRegionType().getString()}});
  SelectionSet children;
  for (auto* child : region->getGroups()) {
    children.insert(gui->makeSelected(child));
  }
  if (!children.empty()) {
    props.push_back({"Groups", children});
  }

  SelectionSet insts;
  for (auto* inst : region->getRegionInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.push_back({"Instances", insts});

  populateODBProperties(props, region);

  return props;
}

Selected DbRegionDescriptor::makeSelected(std::any object) const
{
  if (auto region = std::any_cast<odb::dbRegion*>(&object)) {
    return Selected(*region, this);
  }
  return Selected();
}

bool DbRegionDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_layer = std::any_cast<odb::dbRegion*>(l);
  auto r_layer = std::any_cast<odb::dbRegion*>(r);
  return l_layer->getId() < r_layer->getId();
}

bool DbRegionDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* region : block->getRegions()) {
    objects.insert(makeSelected(region));
  }
  return true;
}

//////////////////////////////////////////////////

DbModuleDescriptor::DbModuleDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbModuleDescriptor::getShortName(std::any object) const
{
  auto* module = std::any_cast<odb::dbModule*>(object);
  return module->getName();
}

std::string DbModuleDescriptor::getName(std::any object) const
{
  auto* module = std::any_cast<odb::dbModule*>(object);
  return module->getHierarchicalName();
}

std::string DbModuleDescriptor::getTypeName() const
{
  return "Module";
}

bool DbModuleDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto* module = std::any_cast<odb::dbModule*>(object);
  bbox.mergeInit();
  for (auto* child : module->getChildren()) {
    odb::Rect child_bbox;
    if (getBBox(child->getMaster(), child_bbox)) {
      bbox.merge(child_bbox);
    }
  }

  for (auto* inst : module->getInsts()) {
    auto* box = inst->getBBox();
    odb::Rect box_rect = box->getBox();
    bbox.merge(box_rect);
  }

  return !bbox.isInverted();
}

void DbModuleDescriptor::highlight(std::any object, Painter& painter) const
{
  auto* module = std::any_cast<odb::dbModule*>(object);

  auto* inst_descriptor = Gui::get()->getDescriptor<odb::dbInst*>();
  for (auto* inst : module->getInsts()) {
    inst_descriptor->highlight(inst, painter);
  }

  const int level_alpha_scale = 2;
  painter.saveState();
  auto pen_color = painter.getPenColor();
  pen_color.a /= level_alpha_scale;
  painter.setPen(pen_color, true);
  for (auto* children : module->getChildren()) {
    highlight(children->getMaster(), painter);
  }
  painter.restoreState();
}

Descriptor::Properties DbModuleDescriptor::getProperties(std::any object) const
{
  auto* module = std::any_cast<odb::dbModule*>(object);
  auto* mod_inst = module->getModInst();

  auto* gui = Gui::get();

  Properties props;
  if (mod_inst != nullptr) {
    auto* parent = mod_inst->getParent();
    if (parent != nullptr) {
      props.push_back({"Parent", gui->makeSelected(parent)});
    }

    auto* group = mod_inst->getGroup();
    if (group != nullptr) {
      props.push_back({"Group", gui->makeSelected(group)});
    }
  }

  SelectionSet children;
  for (auto* child : module->getChildren()) {
    children.insert(gui->makeSelected(child->getMaster()));
  }
  if (!children.empty()) {
    props.push_back({"Children", children});
  }

  SelectionSet insts;
  for (auto* inst : module->getInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.push_back({"Instances", insts});

  populateODBProperties(props, module);
  if (mod_inst != nullptr) {
    populateODBProperties(props, mod_inst, "Instance");
  }

  return props;
}

Selected DbModuleDescriptor::makeSelected(std::any object) const
{
  if (auto module = std::any_cast<odb::dbModule*>(&object)) {
    return Selected(*module, this);
  }
  return Selected();
}

bool DbModuleDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_layer = std::any_cast<odb::dbModule*>(l);
  auto r_layer = std::any_cast<odb::dbModule*>(r);
  return l_layer->getId() < r_layer->getId();
}

bool DbModuleDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  getModules(block->getTopModule(), objects);

  return true;
}

void DbModuleDescriptor::getModules(odb::dbModule* module,
                                    SelectionSet& objects) const
{
  objects.insert(makeSelected(module));

  for (auto* mod_inst : module->getChildren()) {
    getModules(mod_inst->getMaster(), objects);
  }
}

//////////////////////////////////////////////////

DbTechViaDescriptor::DbTechViaDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbTechViaDescriptor::getName(std::any object) const
{
  auto* via = std::any_cast<odb::dbTechVia*>(object);
  return via->getName();
}

std::string DbTechViaDescriptor::getTypeName() const
{
  return "Tech Via";
}

bool DbTechViaDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void DbTechViaDescriptor::highlight(std::any object, Painter& painter) const
{
}

Descriptor::Properties DbTechViaDescriptor::getProperties(std::any object) const
{
  auto* via = std::any_cast<odb::dbTechVia*>(object);
  auto* gui = Gui::get();

  Properties props({{"Tech", gui->makeSelected(via->getTech())}});

  std::map<odb::dbTechLayer*, odb::Rect> shapes;
  odb::dbTechLayer* cut_layer = nullptr;
  for (auto* box : via->getBoxes()) {
    auto* box_layer = box->getTechLayer();
    if (box_layer->getType() == odb::dbTechLayerType::CUT) {
      cut_layer = box_layer;
    }
    odb::Rect shape = box->getBox();
    shapes[box_layer] = shape;
  }

  PropertyList layers;
  auto make_layer = [gui, &shapes, &layers](odb::dbTechLayer* layer) {
    const auto& shape = shapes[layer];
    std::string shape_text
        = fmt::format("({}, {}), ({}, {})",
                      Property::convert_dbu(shape.xMin(), false),
                      Property::convert_dbu(shape.yMin(), false),
                      Property::convert_dbu(shape.xMax(), false),
                      Property::convert_dbu(shape.yMax(), false));
    layers.push_back({gui->makeSelected(layer), shape_text});
  };
  make_layer(via->getBottomLayer());
  if (cut_layer != nullptr) {
    make_layer(cut_layer);
  }
  make_layer(via->getTopLayer());
  props.push_back({"Layers", layers});

  props.push_back({"Is default", via->isDefault()});
  props.push_back({"Is top of stack", via->isTopOfStack()});

  if (via->getResistance() != 0.0) {
    props.push_back(
        {"Resistance", convertUnits(via->getResistance()) + "Ω/sq"});
  }

  auto* ndr = via->getNonDefaultRule();
  if (ndr != nullptr) {
    props.push_back({"Non-default Rule", gui->makeSelected(ndr)});
  }

  populateODBProperties(props, via);

  return props;
}

Selected DbTechViaDescriptor::makeSelected(std::any object) const
{
  if (auto via = std::any_cast<odb::dbTechVia*>(&object)) {
    return Selected(*via, this);
  }
  return Selected();
}

bool DbTechViaDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_via = std::any_cast<odb::dbTechVia*>(l);
  auto r_via = std::any_cast<odb::dbTechVia*>(r);
  return l_via->getId() < r_via->getId();
}

bool DbTechViaDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* tech = db_->getTech();

  for (auto* via : tech->getVias()) {
    objects.insert(makeSelected(via));
  }

  return true;
}
//////////////////////////////////////////////////

DbTechViaRuleDescriptor::DbTechViaRuleDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbTechViaRuleDescriptor::getName(std::any object) const
{
  auto via_rule = std::any_cast<odb::dbTechViaRule*>(object);
  return via_rule->getName();
}

std::string DbTechViaRuleDescriptor::getTypeName() const
{
  return "Tech Via Rule";
}

bool DbTechViaRuleDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void DbTechViaRuleDescriptor::highlight(std::any object, Painter& painter) const
{
}

Descriptor::Properties DbTechViaRuleDescriptor::getProperties(
    std::any object) const
{
  auto via_rule = std::any_cast<odb::dbTechViaRule*>(object);
  auto gui = Gui::get();

  Properties props;

  SelectionSet vias;
  for (uint via_index = 0; via_index < via_rule->getViaCount(); via_index++) {
    vias.insert(gui->makeSelected(via_rule->getVia(via_index)));
  }
  props.push_back({"Tech Vias", vias});

  SelectionSet layer_rules;
  for (uint rule_index = 0; rule_index < via_rule->getViaLayerRuleCount();
       rule_index++) {
    layer_rules.insert(
        gui->makeSelected(via_rule->getViaLayerRule(rule_index)));
  }
  props.push_back({"Tech Via-Layer Rules", layer_rules});

  return props;
}

Selected DbTechViaRuleDescriptor::makeSelected(std::any object) const
{
  if (auto via_rule = std::any_cast<odb::dbTechViaRule*>(&object)) {
    return Selected(*via_rule, this);
  }

  return Selected();
}

bool DbTechViaRuleDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_via_rule = std::any_cast<odb::dbTechViaRule*>(l);
  auto r_via_rule = std::any_cast<odb::dbTechViaRule*>(r);
  return l_via_rule->getId() < r_via_rule->getId();
}

bool DbTechViaRuleDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* tech = db_->getTech();

  for (auto via_rule : tech->getViaRules()) {
    objects.insert(makeSelected(via_rule));
  }

  return true;
}

//////////////////////////////////////////////////

DbTechViaLayerRuleDescriptor::DbTechViaLayerRuleDescriptor(odb::dbDatabase* db)
    : db_(db)
{
}

std::string DbTechViaLayerRuleDescriptor::getName(std::any object) const
{
  auto via_layer_rule = std::any_cast<odb::dbTechViaLayerRule*>(object);
  std::string rule_name = via_layer_rule->getLayer()->getName() + "_rule";
  return rule_name;
}

std::string DbTechViaLayerRuleDescriptor::getTypeName() const
{
  return "Tech Via-Layer Rule";
}

bool DbTechViaLayerRuleDescriptor::getBBox(std::any object,
                                           odb::Rect& bbox) const
{
  return false;
}

void DbTechViaLayerRuleDescriptor::highlight(std::any object,
                                             Painter& painter) const
{
}

Descriptor::Properties DbTechViaLayerRuleDescriptor::getProperties(
    std::any object) const
{
  auto via_layer_rule = std::any_cast<odb::dbTechViaLayerRule*>(object);
  auto gui = Gui::get();

  Properties props({{"Layer", gui->makeSelected(via_layer_rule->getLayer())},
                    {"Direction", via_layer_rule->getDirection().getString()}});

  if (via_layer_rule->hasWidth()) {
    int minWidth = 0;
    int maxWidth = 0;

    via_layer_rule->getWidth(minWidth, maxWidth);

    std::string width_range
        = fmt::format("{} to {}",
                      Property::convert_dbu(minWidth, true),
                      Property::convert_dbu(maxWidth, true));

    props.push_back({"Width", width_range});
  }

  if (via_layer_rule->hasEnclosure()) {
    int overhang_1 = 0;
    int overhang_2 = 0;

    via_layer_rule->getEnclosure(overhang_1, overhang_2);

    std::string enclosure_rule
        = fmt::format("{} x {}",
                      Property::convert_dbu(overhang_1, true),
                      Property::convert_dbu(overhang_2, true));

    props.push_back({"Enclosure", enclosure_rule});
  }

  if (via_layer_rule->hasOverhang()) {
    props.push_back(
        {"Overhang",
         Property::convert_dbu(via_layer_rule->getOverhang(), true)});
  }

  if (via_layer_rule->hasMetalOverhang()) {
    props.push_back(
        {"Metal Overhang",
         Property::convert_dbu(via_layer_rule->getMetalOverhang(), true)});
  }

  if (via_layer_rule->hasRect()) {
    odb::Rect rect_rule;
    via_layer_rule->getRect(rect_rule);

    props.push_back({"Rectangle", rect_rule});
  }

  if (via_layer_rule->hasSpacing()) {
    int x_spacing = 0;
    int y_spacing = 0;

    via_layer_rule->getSpacing(x_spacing, y_spacing);

    props.push_back({"Spacing",
                     fmt::format("{} x {}",
                                 Property::convert_dbu(x_spacing, true),
                                 Property::convert_dbu(y_spacing, true))});
  }

  if (via_layer_rule->hasResistance()) {
    props.push_back(
        {"Resistance", convertUnits(via_layer_rule->getResistance()) + "Ω/sq"});
  }

  return props;
}

Selected DbTechViaLayerRuleDescriptor::makeSelected(std::any object) const
{
  if (auto via_layer_rule = std::any_cast<odb::dbTechViaLayerRule*>(&object)) {
    return Selected(*via_layer_rule, this);
  }

  return Selected();
}

bool DbTechViaLayerRuleDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_via_layer_rule = std::any_cast<odb::dbTechViaLayerRule*>(l);
  auto r_via_layer_rule = std::any_cast<odb::dbTechViaLayerRule*>(r);

  return l_via_layer_rule->getId() < r_via_layer_rule->getId();
}

bool DbTechViaLayerRuleDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto tech = db_->getTech();

  for (auto via_rule : tech->getViaRules()) {
    for (uint via_layer_index = 0;
         via_layer_index < via_rule->getViaLayerRuleCount();
         via_layer_index++) {
      objects.insert(makeSelected(via_rule->getViaLayerRule(via_layer_index)));
    }
  }

  return true;
}

//////////////////////////////////////////////////

DbMetalWidthViaMapDescriptor::DbMetalWidthViaMapDescriptor(odb::dbDatabase* db)
    : db_(db)
{
}

std::string DbMetalWidthViaMapDescriptor::getName(std::any object) const
{
  auto via_map = std::any_cast<odb::dbMetalWidthViaMap*>(object);
  std::string map_name = via_map->getViaName() + "_width_map";
  return map_name;
}

std::string DbMetalWidthViaMapDescriptor::getTypeName() const
{
  return "Metal Width Via Map Rule";
}

bool DbMetalWidthViaMapDescriptor::getBBox(std::any object,
                                           odb::Rect& bbox) const
{
  return false;
}

void DbMetalWidthViaMapDescriptor::highlight(std::any object,
                                             Painter& painter) const
{
}

Descriptor::Properties DbMetalWidthViaMapDescriptor::getProperties(
    std::any object) const
{
  auto via_map = std::any_cast<odb::dbMetalWidthViaMap*>(object);

  Properties props(
      {{"Is via cut class", via_map->isViaCutClass()},
       {"Below Layer Low Width",
        Property::convert_dbu(via_map->getBelowLayerWidthLow(), true)},
       {"Above Layer Low Width",
        Property::convert_dbu(via_map->getAboveLayerWidthLow(), true)},
       {"Below Layer High Width",
        Property::convert_dbu(via_map->getBelowLayerWidthHigh(), true)},
       {"Above Layer High Width",
        Property::convert_dbu(via_map->getAboveLayerWidthHigh(), true)}});

  return props;
}

Selected DbMetalWidthViaMapDescriptor::makeSelected(std::any object) const
{
  if (auto via_map = std::any_cast<odb::dbMetalWidthViaMap*>(&object)) {
    return Selected(*via_map, this);
  }

  return Selected();
}

bool DbMetalWidthViaMapDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_via_map = std::any_cast<odb::dbMetalWidthViaMap*>(l);
  auto r_via_map = std::any_cast<odb::dbMetalWidthViaMap*>(r);
  return l_via_map->getId() < r_via_map->getId();
}

bool DbMetalWidthViaMapDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* tech = db_->getTech();

  for (auto* map : tech->getMetalWidthViaMap()) {
    objects.insert(makeSelected(map));
  }

  return true;
}

//////////////////////////////////////////////////

DbGenerateViaDescriptor::DbGenerateViaDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbGenerateViaDescriptor::getName(std::any object) const
{
  auto* via = std::any_cast<odb::dbTechViaGenerateRule*>(object);
  return via->getName();
}

std::string DbGenerateViaDescriptor::getTypeName() const
{
  return "Generate Via Rule";
}

bool DbGenerateViaDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void DbGenerateViaDescriptor::highlight(std::any object, Painter& painter) const
{
}

Descriptor::Properties DbGenerateViaDescriptor::getProperties(
    std::any object) const
{
  auto* via = std::any_cast<odb::dbTechViaGenerateRule*>(object);
  auto* gui = Gui::get();

  Properties props;

  SelectionSet via_layer_rules;
  PropertyList layers;
  for (uint l = 0; l < via->getViaLayerRuleCount(); l++) {
    auto* rule = via->getViaLayerRule(l);
    auto* layer = rule->getLayer();
    std::string shape_text;
    if (layer->getType() == odb::dbTechLayerType::CUT) {
      odb::Rect shape;
      rule->getRect(shape);
      shape_text = fmt::format("({}, {}), ({}, {})",
                               Property::convert_dbu(shape.xMin(), false),
                               Property::convert_dbu(shape.yMin(), false),
                               Property::convert_dbu(shape.xMax(), false),
                               Property::convert_dbu(shape.yMax(), false));
    } else {
      int enc0, enc1;
      rule->getEnclosure(enc0, enc1);
      shape_text = fmt::format("Enclosure: {} x {}",
                               Property::convert_dbu(enc0, true),
                               Property::convert_dbu(enc1, true));
    }
    layers.push_back({gui->makeSelected(layer), shape_text});
    via_layer_rules.insert(gui->makeSelected(rule));
  }
  props.push_back({"Tech Via-Layer Rules", via_layer_rules});
  props.push_back({"Layers", layers});

  props.push_back({"Is default", via->isDefault()});

  populateODBProperties(props, via);

  return props;
}

Selected DbGenerateViaDescriptor::makeSelected(std::any object) const
{
  if (auto via = std::any_cast<odb::dbTechViaGenerateRule*>(&object)) {
    return Selected(*via, this);
  }
  return Selected();
}

bool DbGenerateViaDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_via = std::any_cast<odb::dbTechViaGenerateRule*>(l);
  auto r_via = std::any_cast<odb::dbTechViaGenerateRule*>(r);
  return l_via->getId() < r_via->getId();
}

bool DbGenerateViaDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* tech = db_->getTech();

  for (auto* via : tech->getViaGenerateRules()) {
    objects.insert(makeSelected(via));
  }

  return true;
}

//////////////////////////////////////////////////

DbNonDefaultRuleDescriptor::DbNonDefaultRuleDescriptor(odb::dbDatabase* db)
    : db_(db)
{
}

std::string DbNonDefaultRuleDescriptor::getName(std::any object) const
{
  auto* rule = std::any_cast<odb::dbTechNonDefaultRule*>(object);
  return rule->getName();
}

std::string DbNonDefaultRuleDescriptor::getTypeName() const
{
  return "Non-default Rule";
}

bool DbNonDefaultRuleDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void DbNonDefaultRuleDescriptor::highlight(std::any object,
                                           Painter& painter) const
{
}

Descriptor::Properties DbNonDefaultRuleDescriptor::getProperties(
    std::any object) const
{
  auto* rule = std::any_cast<odb::dbTechNonDefaultRule*>(object);
  auto* gui = Gui::get();

  Properties props({{"Tech", gui->makeSelected(db_->getTech())}});

  std::vector<odb::dbTechLayerRule*> rule_layers;
  rule->getLayerRules(rule_layers);
  SelectionSet layers;
  for (auto* layer : rule_layers) {
    layers.insert(gui->makeSelected(layer));
  }
  props.push_back({"Layer rules", layers});

  std::vector<odb::dbTechVia*> rule_vias;
  rule->getVias(rule_vias);
  SelectionSet vias;
  for (auto* via : rule_vias) {
    vias.insert(gui->makeSelected(via));
  }
  props.push_back({"Tech vias", vias});

  std::vector<odb::dbTechSameNetRule*> rule_samenets;
  rule->getSameNetRules(rule_samenets);
  SelectionSet samenet_rules;
  for (auto* samenet : rule_samenets) {
    samenet_rules.insert(gui->makeSelected(samenet));
  }
  props.push_back({"Same net rules", samenet_rules});

  props.push_back({"Is block rule", rule->isBlockRule()});

  populateODBProperties(props, rule);

  return props;
}

Selected DbNonDefaultRuleDescriptor::makeSelected(std::any object) const
{
  if (auto rule = std::any_cast<odb::dbTechNonDefaultRule*>(&object)) {
    return Selected(*rule, this);
  }
  return Selected();
}

bool DbNonDefaultRuleDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_rule = std::any_cast<odb::dbTechNonDefaultRule*>(l);
  auto r_rule = std::any_cast<odb::dbTechNonDefaultRule*>(r);
  return l_rule->getId() < r_rule->getId();
}

bool DbNonDefaultRuleDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* rule : db_->getTech()->getNonDefaultRules()) {
    objects.insert(makeSelected(rule));
  }

  for (auto* rule : block->getNonDefaultRules()) {
    objects.insert(makeSelected(rule));
  }

  return true;
}

//////////////////////////////////////////////////

DbTechSameNetRuleDescriptor::DbTechSameNetRuleDescriptor(odb::dbDatabase* db)
    : db_(db)
{
}

std::string DbTechLayerRuleDescriptor::getName(std::any object) const
{
  auto* rule = std::any_cast<odb::dbTechLayerRule*>(object);
  return rule->getLayer()->getName();
}

std::string DbTechLayerRuleDescriptor::getTypeName() const
{
  return "Tech layer rule";
}

bool DbTechLayerRuleDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void DbTechLayerRuleDescriptor::highlight(std::any object,
                                          Painter& painter) const
{
}

Descriptor::Properties DbTechLayerRuleDescriptor::getProperties(
    std::any object) const
{
  auto* rule = std::any_cast<odb::dbTechLayerRule*>(object);
  auto* gui = Gui::get();

  Properties props;

  props.push_back({"Layer", gui->makeSelected(rule->getLayer())});
  props.push_back(
      {"Non-default Rule", gui->makeSelected(rule->getNonDefaultRule())});
  props.push_back({"Is block rule", rule->isBlockRule()});

  props.push_back({"Width", Property::convert_dbu(rule->getWidth(), true)});
  props.push_back({"Spacing", Property::convert_dbu(rule->getSpacing(), true)});

  populateODBProperties(props, rule);

  return props;
}

Selected DbTechLayerRuleDescriptor::makeSelected(std::any object) const
{
  if (auto rule = std::any_cast<odb::dbTechLayerRule*>(&object)) {
    return Selected(*rule, this);
  }
  return Selected();
}

bool DbTechLayerRuleDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_rule = std::any_cast<odb::dbTechLayerRule*>(l);
  auto r_rule = std::any_cast<odb::dbTechLayerRule*>(r);
  return l_rule->getId() < r_rule->getId();
}

bool DbTechLayerRuleDescriptor::getAllObjects(SelectionSet& objects) const
{
  return false;
}

//////////////////////////////////////////////////

std::string DbTechSameNetRuleDescriptor::getName(std::any object) const
{
  auto* rule = std::any_cast<odb::dbTechSameNetRule*>(object);
  return rule->getLayer1()->getName() + " - " + rule->getLayer2()->getName();
}

std::string DbTechSameNetRuleDescriptor::getTypeName() const
{
  return "Tech same net rule";
}

bool DbTechSameNetRuleDescriptor::getBBox(std::any object,
                                          odb::Rect& bbox) const
{
  return false;
}

void DbTechSameNetRuleDescriptor::highlight(std::any object,
                                            Painter& painter) const
{
}

Descriptor::Properties DbTechSameNetRuleDescriptor::getProperties(
    std::any object) const
{
  auto* rule = std::any_cast<odb::dbTechSameNetRule*>(object);
  auto* gui = Gui::get();

  Properties props({{"Tech", gui->makeSelected(db_->getTech())}});

  props.push_back({"Layer 1", gui->makeSelected(rule->getLayer1())});
  props.push_back({"Layer 2", gui->makeSelected(rule->getLayer2())});

  props.push_back({"Spacing", Property::convert_dbu(rule->getSpacing(), true)});
  props.push_back({"Allow via stacking", rule->getAllowStackedVias()});

  populateODBProperties(props, rule);

  return props;
}

Selected DbTechSameNetRuleDescriptor::makeSelected(std::any object) const
{
  if (auto rule = std::any_cast<odb::dbTechSameNetRule*>(&object)) {
    return Selected(*rule, this);
  }
  return Selected();
}

bool DbTechSameNetRuleDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_rule = std::any_cast<odb::dbTechSameNetRule*>(l);
  auto r_rule = std::any_cast<odb::dbTechSameNetRule*>(r);
  return l_rule->getId() < r_rule->getId();
}

bool DbTechSameNetRuleDescriptor::getAllObjects(SelectionSet& objects) const
{
  return false;
}

//////////////////////////////////////////////////

DbSiteDescriptor::DbSiteDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbSiteDescriptor::getName(std::any object) const
{
  auto* site = getSite(object);
  return site->getName();
}

std::string DbSiteDescriptor::getTypeName() const
{
  return "Site";
}

bool DbSiteDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  if (isSpecificSite(object)) {
    bbox = getRect(object);
    return true;
  }

  return false;
}

void DbSiteDescriptor::highlight(std::any object, Painter& painter) const
{
  if (isSpecificSite(object)) {
    painter.drawRect(getRect(object));
  }
}

Descriptor::Properties DbSiteDescriptor::getProperties(std::any object) const
{
  auto* site = getSite(object);

  Properties props;

  props.push_back({"Width", Property::convert_dbu(site->getWidth(), true)});
  props.push_back({"Height", Property::convert_dbu(site->getHeight(), true)});

  props.push_back({"Site class", site->getClass().getString()});

  std::vector<std::any> symmetry;
  if (site->getSymmetryX()) {
    symmetry.emplace_back("X");
  }
  if (site->getSymmetryY()) {
    symmetry.emplace_back("Y");
  }
  if (site->getSymmetryR90()) {
    symmetry.emplace_back("R90");
  }
  props.push_back({"Symmetry", symmetry});

  populateODBProperties(props, site);

  return props;
}

Selected DbSiteDescriptor::makeSelected(std::any object) const
{
  if (auto site = std::any_cast<odb::dbSite*>(&object)) {
    return Selected(*site, this);
  }
  if (auto site = std::any_cast<SpecificSite>(&object)) {
    return Selected(*site, this);
  }
  return Selected();
}

bool DbSiteDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_site = getSite(l);
  auto r_site = getSite(r);
  if (l_site->getId() < r_site->getId()) {
    return true;
  }

  const odb::Rect l_rect = getRect(l);
  const odb::Rect r_rect = getRect(r);
  return l_rect < r_rect;
}

bool DbSiteDescriptor::getAllObjects(SelectionSet& objects) const
{
  for (auto* lib : db_->getLibs()) {
    for (auto* site : lib->getSites()) {
      objects.insert(makeSelected(site));
    }
  }

  return true;
}

odb::dbSite* DbSiteDescriptor::getSite(const std::any& object) const
{
  odb::dbSite* const* site = std::any_cast<odb::dbSite*>(&object);
  if (site != nullptr) {
    return *site;
  }
  SpecificSite ss = std::any_cast<SpecificSite>(object);
  return ss.site;
}

odb::Rect DbSiteDescriptor::getRect(const std::any& object) const
{
  const SpecificSite* ss = std::any_cast<SpecificSite>(&object);
  if (ss != nullptr) {
    return ss->rect;
  }
  return odb::Rect();
}

bool DbSiteDescriptor::isSpecificSite(const std::any& object) const
{
  return std::any_cast<SpecificSite>(&object) != nullptr;
}

//////////////////////////////////////////////////

DbRowDescriptor::DbRowDescriptor(odb::dbDatabase* db) : db_(db)
{
}

std::string DbRowDescriptor::getName(std::any object) const
{
  auto* row = std::any_cast<odb::dbRow*>(object);
  return row->getName();
}

std::string DbRowDescriptor::getTypeName() const
{
  return "Row";
}

bool DbRowDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto* row = std::any_cast<odb::dbRow*>(object);
  bbox = row->getBBox();
  return true;
}

void DbRowDescriptor::highlight(std::any object, Painter& painter) const
{
  auto* row = std::any_cast<odb::dbRow*>(object);
  painter.drawRect(row->getBBox());
}

Descriptor::Properties DbRowDescriptor::getProperties(std::any object) const
{
  auto* row = std::any_cast<odb::dbRow*>(object);
  auto* gui = Gui::get();

  Properties props({{"Block", gui->makeSelected(row->getBlock())},
                    {"Site", gui->makeSelected(row->getSite())}});
  odb::Point origin_pt = row->getOrigin();
  PropertyList origin;
  origin.push_back({"X", Property::convert_dbu(origin_pt.x(), true)});
  origin.push_back({"Y", Property::convert_dbu(origin_pt.y(), true)});
  props.push_back({"Origin", origin});

  props.push_back({"Orientation", row->getOrient().getString()});
  props.push_back({"Direction", row->getDirection().getString()});

  props.push_back({"Site count", row->getSiteCount()});
  props.push_back(
      {"Site spacing", Property::convert_dbu(row->getSpacing(), true)});

  populateODBProperties(props, row);

  return props;
}

Selected DbRowDescriptor::makeSelected(std::any object) const
{
  if (auto row = std::any_cast<odb::dbRow*>(&object)) {
    return Selected(*row, this);
  }
  return Selected();
}

bool DbRowDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_row = std::any_cast<odb::dbRow*>(l);
  auto r_row = std::any_cast<odb::dbRow*>(r);

  return l_row->getId() < r_row->getId();
}

bool DbRowDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* block = db_->getChip()->getBlock();

  for (auto* row : block->getRows()) {
    objects.insert(makeSelected(row));
  }

  return true;
}

}  // namespace gui
