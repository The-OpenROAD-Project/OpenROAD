// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "dbDescriptors.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <any>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "boost/algorithm/string.hpp"
#include "bufferTreeDescriptor.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "insertBufferDialog.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/dbWireGraph.h"
#include "odb/geom.h"
#include "options.h"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace gui {

static void populateODBProperties(Descriptor::Properties& props,
                                  odb::dbObject* object,
                                  const std::string& prefix = "")
{
  std::optional<int> src_file_id;
  std::optional<int> src_file_line;
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
    // Look for the file name properties from Verilog2db::storeLineInfo
    if (prop->getName() == "src_file_id") {
      src_file_id = std::any_cast<int>(value);
    } else if (prop->getName() == "src_file_line") {
      src_file_line = std::any_cast<int>(value);
    } else {
      prop_list.emplace_back(prop->getName(), value);
    }
  }

  if (src_file_id && src_file_line) {
    auto block = object->getDb()->getChip()->getBlock();
    const auto src_file = fmt::format("src_file_{}", src_file_id.value());
    const auto file_name_prop
        = odb::dbStringProperty::find(block, src_file.c_str());
    if (file_name_prop) {
      const auto info = fmt::format(
          "{}:{}", file_name_prop->getValue(), src_file_line.value());
      prop_list.emplace_back("Source", info);
    }
  }

  if (!prop_list.empty()) {
    std::string prop_name = "Properties";
    if (!prefix.empty()) {
      prop_name = prefix + " " + prop_name;
    }
    props.emplace_back(std::move(prop_name), prop_list);
  }
}

std::string Descriptor::convertUnits(const double value,
                                     const bool area,
                                     const int digits)
{
  auto format = [area, value, digits](int log_units) {
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
    } else if (log_units <= 9) {
      unit_scale = 1e-9;
      unit = "G";
    }
    if (area) {
      unit_scale *= unit_scale;
    }

    auto str = utl::to_numeric_string(value * unit_scale, digits);
    return std::make_pair(str, unit);
  };

  double log_value = value;
  if (area) {
    log_value = std::sqrt(log_value);
  }
  // Try both ways and see what produces the better result.
  auto [s1, u1] = format(std::trunc(std::log10(log_value) / 3.0) * 3);
  auto [s2, u2] = format(std::floor(std::log10(log_value) / 3.0) * 3);

  // Don't include the unit size in the comparison as the micron
  // symbol counts as two characters.
  if (s1.size() < s2.size()) {
    return s1 + " " + u1;
  }
  return s2 + " " + u2;
}

// renames an object
template <typename T>
static void addRenameEditor(T obj, Descriptor::Editors& editor)
{
  editor.insert(
      {"Name", Descriptor::makeEditor([obj](const std::any& value) {
         const std::string new_name = std::any_cast<std::string>(value);
         // check if empty
         if (new_name.empty()) {
           return false;
         }
         // check for illegal characters
         for (const char ch : {obj->getBlock()->getHierarchyDelimiter()}) {
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
      {std::string(Descriptor::kDeselectAction), [obj, desc, gui]() {
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

//////////////////////////////////////////////////

template <typename T>
BaseDbDescriptor<T>::BaseDbDescriptor(odb::dbDatabase* db) : db_(db)
{
}

template <typename T>
Descriptor::Properties BaseDbDescriptor<T>::getProperties(
    const std::any& object) const
{
  T* obj = getObject(object);

  Properties props = getDBProperties(obj);

  populateODBProperties(props, obj);

  return props;
}

template <typename T>
Selected BaseDbDescriptor<T>::makeSelected(const std::any& object) const
{
  if (auto obj = std::any_cast<T*>(&object)) {
    return Selected(*obj, this);
  }
  return Selected();
}

template <typename T>
bool BaseDbDescriptor<T>::lessThan(const std::any& l, const std::any& r) const
{
  T* l_obj = std::any_cast<T*>(l);
  T* r_obj = std::any_cast<T*>(r);
  return odb::compare_by_id(l_obj, r_obj);
}

template <typename T>
T* BaseDbDescriptor<T>::getObject(const std::any& object) const
{
  return std::any_cast<T*>(object);
}

////////

DbTechDescriptor::DbTechDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTech>(db)
{
}

std::string DbTechDescriptor::getName(const std::any& object) const
{
  return "Default";
}

std::string DbTechDescriptor::getTypeName() const
{
  return "Tech";
}

bool DbTechDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void DbTechDescriptor::highlight(const std::any& object, Painter& painter) const
{
}

Descriptor::Properties DbTechDescriptor::getDBProperties(
    odb::dbTech* tech) const
{
  auto gui = Gui::get();

  Properties props({{"DbUnits per Micron", tech->getDbUnitsPerMicron()},
                    {"LEF Units", tech->getLefUnits()},
                    {"LEF Version", tech->getLefVersionStr()}});

  if (tech->hasManufacturingGrid()) {
    props.emplace_back(
        "Manufacturing Grid",
        Property::convert_dbu(tech->getManufacturingGrid(), true));
  }

  SelectionSet tech_layers;
  for (auto tech_layer : tech->getLayers()) {
    tech_layers.insert(gui->makeSelected(tech_layer));
  }
  props.emplace_back("Tech Layers", tech_layers);

  SelectionSet tech_vias;
  for (auto tech_via : tech->getVias()) {
    tech_vias.insert(gui->makeSelected(tech_via));
  }
  props.emplace_back("Tech Vias", tech_vias);

  SelectionSet via_rules;
  for (auto via_rule : tech->getViaRules()) {
    via_rules.insert(gui->makeSelected(via_rule));
  }
  props.emplace_back("Tech Via Rules", via_rules);

  SelectionSet generate_vias;
  for (auto via : tech->getViaGenerateRules()) {
    generate_vias.insert(gui->makeSelected(via));
  }
  props.emplace_back("Tech Via Generate Rules", generate_vias);

  SelectionSet via_maps;
  for (auto map : tech->getMetalWidthViaMap()) {
    via_maps.insert(gui->makeSelected(map));
  }
  props.emplace_back("Metal Width Via Map Rules", via_maps);

  std::vector<odb::dbTechSameNetRule*> rule_samenets;
  tech->getSameNetRules(rule_samenets);
  SelectionSet samenet_rules;
  for (auto samenet : rule_samenets) {
    samenet_rules.insert(gui->makeSelected(samenet));
  }
  props.emplace_back("Same Net Rules", samenet_rules);

  SelectionSet nondefault_rules;
  for (auto nondefault : tech->getNonDefaultRules()) {
    nondefault_rules.insert(gui->makeSelected(nondefault));
  }
  props.emplace_back("Non-Default Rules", nondefault_rules);

  return props;
}

void DbTechDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto tech = db_->getTech();
  if (tech == nullptr) {
    return;
  }
  func({tech, this});
}

//////////////////////////////////////////////////

DbBlockDescriptor::DbBlockDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbBlock>(db)
{
}

std::string DbBlockDescriptor::getName(const std::any& object) const
{
  auto block = std::any_cast<odb::dbBlock*>(object);
  return block->getName();
}

std::string DbBlockDescriptor::getTypeName() const
{
  return "Block";
}

bool DbBlockDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto block = std::any_cast<odb::dbBlock*>(object);
  bbox = block->getBBox()->getBox();
  return !bbox.isInverted();
}

void DbBlockDescriptor::highlight(const std::any& object,
                                  Painter& painter) const
{
  auto block = std::any_cast<odb::dbBlock*>(object);

  odb::dbBox* bbox = block->getBBox();
  odb::Rect rect = bbox->getBox();
  if (!rect.isInverted()) {
    painter.drawRect(rect);
  }
}

Descriptor::Properties DbBlockDescriptor::getDBProperties(
    odb::dbBlock* block) const
{
  auto gui = Gui::get();

  Properties props;
  SelectionSet children;
  for (auto child : block->getChildren()) {
    children.insert(gui->makeSelected(child));
  }
  props.emplace_back("Child Blocks", children);

  SelectionSet modules;
  for (auto module : block->getModules()) {
    modules.insert(gui->makeSelected(module));
  }
  props.emplace_back("Modules", modules);

  props.emplace_back("Top Module", gui->makeSelected(block->getTopModule()));

  SelectionSet bterms;
  for (auto bterm : block->getBTerms()) {
    bterms.insert(gui->makeSelected(bterm));
  }
  props.emplace_back("BTerms", bterms);

  SelectionSet vias;
  for (auto via : block->getVias()) {
    vias.insert(gui->makeSelected(via));
  }
  props.emplace_back("Block Vias", vias);

  SelectionSet nets;
  for (auto net : block->getNets()) {
    nets.insert(gui->makeSelected(net));
  }
  props.emplace_back("Nets", nets);

  SelectionSet regions;
  for (auto region : block->getRegions()) {
    regions.insert(gui->makeSelected(region));
  }
  props.emplace_back("Regions", regions);

  SelectionSet insts;
  for (auto inst : block->getInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.emplace_back("Instances", insts);

  SelectionSet blockages;
  for (auto blockage : block->getBlockages()) {
    blockages.insert(gui->makeSelected(blockage));
  }
  props.emplace_back("Blockages", blockages);

  SelectionSet obstructions;
  for (auto obstruction : block->getObstructions()) {
    obstructions.insert(gui->makeSelected(obstruction));
  }
  props.emplace_back("Obstructions", obstructions);

  SelectionSet rows;
  for (auto row : block->getRows()) {
    rows.insert(gui->makeSelected(row));
  }
  props.emplace_back("Rows", rows);

  SelectionSet markers;
  for (auto marker : block->getMarkerCategories()) {
    markers.insert(gui->makeSelected(marker));
  }
  if (!markers.empty()) {
    props.emplace_back("Markers", markers);
  }

  props.emplace_back("Core Area", block->getCoreArea());
  props.emplace_back("Die Area", block->getDieArea());

  return props;
}

void DbBlockDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto block = chip->getBlock();
  func({block, this});
}

//////////////////////////////////////////////////

DbInstDescriptor::DbInstDescriptor(odb::dbDatabase* db, sta::dbSta* sta)
    : BaseDbDescriptor<odb::dbInst>(db), sta_(sta)
{
}

std::string DbInstDescriptor::getName(const std::any& object) const
{
  return std::any_cast<odb::dbInst*>(object)->getName();
}

std::string DbInstDescriptor::getTypeName() const
{
  return "Inst";
}

bool DbInstDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  bbox = inst->getBBox()->getBox();
  return !bbox.isInverted();
}

void DbInstDescriptor::highlight(const std::any& object, Painter& painter) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  if (!inst->getPlacementStatus().isPlaced()) {
    return;
  }

  odb::dbBox* bbox = inst->getBBox();
  odb::Rect rect = bbox->getBox();
  painter.drawRect(rect);
}

bool DbInstDescriptor::isInst(const std::any& object) const
{
  return true;
}

Descriptor::Properties DbInstDescriptor::getDBProperties(
    odb::dbInst* inst) const
{
  auto gui = Gui::get();
  auto placed = inst->getPlacementStatus();
  auto* module = inst->getModule();
  Properties props;
  props.emplace_back("Block", gui->makeSelected(inst->getBlock()));
  if (module != nullptr) {
    props.emplace_back("Module", gui->makeSelected(module));
  }
  props.emplace_back("Master", gui->makeSelected(inst->getMaster()));

  props.emplace_back("Description",
                     sta_->getInstanceTypeText(sta_->getInstanceType(inst)));
  props.emplace_back("Placement status", placed.getString());
  props.emplace_back("Source type", inst->getSourceType().getString());
  props.emplace_back("Dont Touch", inst->isDoNotTouch());
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
    iterms.emplace_back(gui->makeSelected(iterm), net_value);
  }
  props.emplace_back("ITerms", iterms);

  auto* group = inst->getGroup();
  if (group != nullptr) {
    props.emplace_back("Group", gui->makeSelected(group));
  }

  auto* region = inst->getRegion();
  if (region != nullptr) {
    props.emplace_back("Region", gui->makeSelected(region));
  }

  auto* halo = inst->getHalo();
  if (halo != nullptr) {
    props.emplace_back("Halo", gui->makeSelected(halo));
  }

  sta::Instance* sta_inst = sta_->getDbNetwork()->dbToSta(inst);
  if (sta_inst != nullptr) {
    props.emplace_back("Timing/Power", gui->makeSelected(sta_inst));
  }

  odb::dbScanInst* scan_inst = inst->getScanInst();
  if (scan_inst != nullptr) {
    props.emplace_back("Scan Inst", gui->makeSelected(scan_inst));
  }

  Descriptor::PropertyList obs_layers;
  const auto xform = inst->getTransform();
  for (auto* obs : inst->getMaster()->getObstructions()) {
    if (auto* layer = obs->getTechLayer()) {
      obs_layers.emplace_back(
          gui->makeSelected(layer),
          gui->makeSelected(DbBoxDescriptor::BoxWithTransform{obs, xform}));
    } else if (auto* via = obs->getTechVia()) {
      obs_layers.emplace_back(
          gui->makeSelected(via),
          gui->makeSelected(DbBoxDescriptor::BoxWithTransform{obs, xform}));
    }
  }
  if (!obs_layers.empty()) {
    props.emplace_back("Obstructions", obs_layers);
  }

  return props;
}

Descriptor::Actions DbInstDescriptor::getActions(const std::any& object) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  return Actions(
      {{"Delete", [inst]() {
          odb::dbInst::destroy(inst);
          return Selected();  // unselect since this object is now gone
        }}});
}

Descriptor::Editors DbInstDescriptor::getEditors(const std::any& object) const
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
                        [inst](const std::any& value) {
                          inst->swapMaster(
                              std::any_cast<odb::dbMaster*>(value));
                          return true;
                        },
                        master_options)});
  }
  editors.insert({"Orientation",
                  makeEditor(
                      [inst](const std::any& value) {
                        inst->setLocationOrient(
                            std::any_cast<odb::dbOrientType>(value));
                        return true;
                      },
                      orient_options)});
  editors.insert({"Placement status",
                  makeEditor(
                      [inst](const std::any& value) {
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

void DbInstDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* inst : block->getInsts()) {
    func({inst, this});
  }
}

//////////////////////////////////////////////////

DbMasterDescriptor::DbMasterDescriptor(odb::dbDatabase* db, sta::dbSta* sta)
    : BaseDbDescriptor<odb::dbMaster>(db), sta_(sta)
{
}

std::string DbMasterDescriptor::getName(const std::any& object) const
{
  return std::any_cast<odb::dbMaster*>(object)->getName();
}

std::string DbMasterDescriptor::getTypeName() const
{
  return "Master";
}

bool DbMasterDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto master = std::any_cast<odb::dbMaster*>(object);
  master->getPlacementBoundary(bbox);
  return true;
}

void DbMasterDescriptor::highlight(const std::any& object,
                                   Painter& painter) const
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

Descriptor::Properties DbMasterDescriptor::getDBProperties(
    odb::dbMaster* master) const
{
  Properties props({{"Master type", master->getType().getString()}});
  auto gui = Gui::get();
  auto site = master->getSite();
  if (site != nullptr) {
    props.emplace_back("Site", gui->makeSelected(site));
  }
  SelectionSet mterms;
  for (auto mterm : master->getMTerms()) {
    mterms.insert(gui->makeSelected(mterm));
  }
  props.emplace_back("MTerms", mterms);

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
  props.emplace_back("Symmetry", symmetry);

  SelectionSet equivalent;
  std::set<odb::dbMaster*> equivalent_masters;
  getMasterEquivalent(sta_, master, equivalent_masters);
  for (auto other_master : equivalent_masters) {
    if (other_master != master) {
      equivalent.insert(gui->makeSelected(other_master));
    }
  }
  props.emplace_back("Equivalent", equivalent);
  SelectionSet instances;
  std::set<odb::dbInst*> insts;
  getInstances(master, insts);
  for (auto inst : insts) {
    instances.insert(gui->makeSelected(inst));
  }
  props.emplace_back("Instances", instances);
  props.emplace_back("Origin", master->getOrigin());

  std::vector<std::any> edge_types;
  for (auto* edge : master->getEdgeTypes()) {
    edge_types.emplace_back(gui->makeSelected(edge));
  }
  if (!edge_types.empty()) {
    props.emplace_back("Edge types", edge_types);
  }

  auto liberty
      = sta_->getDbNetwork()->findLibertyCell(master->getName().c_str());
  if (liberty) {
    props.emplace_back("Liberty", gui->makeSelected(liberty));
  }

  return props;
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
                                      std::set<odb::dbInst*>& insts)
{
  for (auto inst : master->getDb()->getChip()->getBlock()->getInsts()) {
    if (inst->getMaster() == master) {
      insts.insert(inst);
    }
  }
}

void DbMasterDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  for (auto* lib : db_->getLibs()) {
    for (auto* master : lib->getMasters()) {
      func({master, this});
    }
  }
}

//////////////////////////////////////////////////

DbNetDescriptor::DbNetDescriptor(odb::dbDatabase* db,
                                 sta::dbSta* sta,
                                 const std::set<odb::dbNet*>& focus_nets,
                                 const std::set<odb::dbNet*>& guide_nets,
                                 const std::set<odb::dbNet*>& tracks_nets)
    : BaseDbDescriptor<odb::dbNet>(db),
      sta_(sta),
      focus_nets_(focus_nets),
      guide_nets_(guide_nets),
      tracks_nets_(tracks_nets)
{
}

std::string DbNetDescriptor::getName(const std::any& object) const
{
  return getObject(object)->getName();
}

std::string DbNetDescriptor::getTypeName() const
{
  return "Net";
}

bool DbNetDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto net = getObject(object);
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
  if (!has_box) {
    // a wire bbox was not found, try using guides
    for (odb::dbGuide* guide : net->getGuides()) {
      bbox.merge(guide->getBox());
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

void DbNetDescriptor::findSourcesAndSinks(
    odb::dbNet* net,
    const odb::dbObject* sink,
    std::vector<std::set<GraphTarget>>& sources,
    std::vector<std::set<GraphTarget>>& sinks) const
{
  // gets all the shapes that make up the iterm
  auto get_graph_iterm_targets
      = [](odb::dbMTerm* mterm,
           const odb::dbTransform& transform,
           std::vector<std::set<GraphTarget>>& targets) {
          for (auto* mpin : mterm->getMPins()) {
            std::set<GraphTarget> term_targets;
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
                  term_targets.emplace(box_rect, via_box->getTechLayer());
                }
              } else {
                odb::Rect rect = box->getBox();
                transform.apply(rect);
                term_targets.emplace(rect, box->getTechLayer());
              }
            }

            if (!term_targets.empty()) {
              targets.push_back(std::move(term_targets));
            }
          }
        };

  // gets all the shapes that make up the bterm
  auto get_graph_bterm_targets
      = [](odb::dbBTerm* bterm, std::vector<std::set<GraphTarget>>& targets) {
          for (auto* bpin : bterm->getBPins()) {
            std::set<GraphTarget> term_targets;
            for (auto* box : bpin->getBoxes()) {
              odb::Rect rect = box->getBox();
              term_targets.emplace(rect, box->getTechLayer());
            }
            if (!term_targets.empty()) {
              targets.push_back(std::move(term_targets));
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

void DbNetDescriptor::findSourcesAndSinksInGraph(
    odb::dbNet* net,
    const odb::dbObject* sink,
    odb::dbWireGraph* graph,
    std::set<NodeList>& source_nodes,
    std::set<NodeList>& sink_nodes) const
{
  // find sources and sinks on this net
  std::vector<std::set<GraphTarget>> sources;
  std::vector<std::set<GraphTarget>> sinks;
  findSourcesAndSinks(net, sink, sources, sinks);

  // Preserve mapping of nodes to pin sets
  std::vector<NodeList> sources_nodes;
  sources_nodes.resize(sources.size());
  std::vector<NodeList> sinks_nodes;
  sinks_nodes.resize(sinks.size());

  // find the nodes on the wire graph that intersect the sinks identified
  for (auto itr = graph->begin_nodes(); itr != graph->end_nodes(); itr++) {
    const auto* node = *itr;
    int x, y;
    node->xy(x, y);
    const odb::Point node_pt(x, y);
    const odb::dbTechLayer* node_layer = node->layer();

    for (std::size_t i = 0; i < sources.size(); i++) {
      for (const auto& [source_rect, source_layer] : sources[i]) {
        if (source_rect.intersects(node_pt) && source_layer == node_layer) {
          sources_nodes[i].insert(node);
        }
      }
    }

    for (std::size_t i = 0; i < sinks.size(); i++) {
      for (const auto& [sink_rect, sink_layer] : sinks[i]) {
        if (sink_rect.intersects(node_pt) && sink_layer == node_layer) {
          sinks_nodes[i].insert(node);
        }
      }
    }
  }

  source_nodes.insert(sources_nodes.begin(), sources_nodes.end());
  sink_nodes.insert(sinks_nodes.begin(), sinks_nodes.end());
}

void DbNetDescriptor::drawPathSegmentWithGraph(odb::dbNet* net,
                                               const odb::dbObject* sink,
                                               Painter& painter) const
{
  odb::dbWireGraph graph;
  graph.decode(net->getWire());

  // find the nodes on the wire graph that intersect the sinks identified
  std::set<NodeList> source_nodes;
  std::set<NodeList> sink_nodes;
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

  std::vector<std::pair<odb::Point, odb::Point>> flywires;

  for (const auto& src_nodes : source_nodes) {
    for (const auto& sink_node_set : sink_nodes) {
      bool drawn = false;
      std::vector<std::pair<odb::Point, odb::Point>> flywires_pair;
      for (const auto* source_node : src_nodes) {
        for (const auto* sink_node : sink_node_set) {
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
            drawn = true;
          } else {
            // unable to find path so just draw a fly-wire
            int x, y;
            source_node->xy(x, y);
            const odb::Point source_pt(x, y);
            sink_node->xy(x, y);
            const odb::Point sink_pt(x, y);
            flywires_pair.emplace_back(source_pt, sink_pt);
          }
        }
      }
      if (!drawn) {
        flywires.insert(
            flywires.end(), flywires_pair.begin(), flywires_pair.end());
      }
    }
  }

  for (const auto& [source_pt, sink_pt] : flywires) {
    painter.drawLine(source_pt, sink_pt);
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

void DbNetDescriptor::findPath(PointMap& graph,
                               const odb::Point& source,
                               const odb::Point& sink,
                               std::vector<odb::Point>& path) const
{
  // find path from source to sink using A*
  // https://en.wikipedia.org/w/index.php?title=A*_search_algorithm&oldid=1050302256

  auto distance = [](const odb::Point& node0, const odb::Point& node1) -> int {
    return odb::Point::manhattanDistance(node0, node1);
  };

  std::map<odb::Point, odb::Point> came_from;
  std::map<odb::Point, int> g_score;
  std::map<odb::Point, int> f_score;

  struct DistNode
  {
    odb::Point node;
    int dist;

   public:
    // used for priority queue
    bool operator<(const DistNode& other) const { return dist > other.dist; }
  };
  std::priority_queue<DistNode> open_set;
  std::set<odb::Point> open_set_nodes;
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
      while (current != source) {
        path.emplace_back(current);
        current = came_from[current];
      }
      path.emplace_back(current);
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

std::set<odb::Line> DbNetDescriptor::convertGuidesToLines(
    odb::dbNet* net,
    DbTargets& sources,
    DbTargets& sinks) const
{
  sources.clear();
  sinks.clear();

  auto guides = net->getGuides();
  if (guides.empty()) {
    return {};
  }

  std::set<odb::Line> lines;

  struct DbIO
  {
    bool is_sink;
    bool is_source;
  };
  std::map<odb::dbObject*, DbIO> io_map;

  std::map<odb::dbTechLayer*, std::map<odb::dbObject*, std::set<odb::Rect>>>
      terms;
  for (odb::dbITerm* term : net->getITerms()) {
    if (!term->getInst()->isPlaced()) {
      continue;
    }
    const auto iotype = term->getIoType();
    const bool is_sink
        = iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT;
    const bool is_source
        = iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT;
    io_map[term] = {is_sink, is_source};
    for (const auto& [layer, itermbox] : term->getGeometries()) {
      terms[layer][term].insert(itermbox);
    }
  }
  for (odb::dbBTerm* term : net->getBTerms()) {
    const auto iotype = term->getIoType();
    const bool is_sink
        = iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT;
    const bool is_source
        = iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT;
    io_map[term] = {is_sink, is_source};

    for (odb::dbBPin* pin : term->getBPins()) {
      if (!pin->getPlacementStatus().isPlaced()) {
        continue;
      }
      for (odb::dbBox* box : pin->getBoxes()) {
        terms[box->getTechLayer()][term].insert(box->getBox());
      }
    }
  }

  // Build gcell grid centers
  std::vector<int> grid;
  net->getBlock()->getGCellGrid()->getGridX(grid);
  grid.push_back(net->getBlock()->getDieArea().xMax());
  std::vector<int> x_grid;
  x_grid.reserve(grid.size() - 1);
  for (int i = 1; i < grid.size(); i++) {
    x_grid.push_back((grid[i - 1] + grid[i]) / 2);
  }

  grid.clear();
  net->getBlock()->getGCellGrid()->getGridY(grid);
  grid.push_back(net->getBlock()->getDieArea().yMax());
  std::vector<int> y_grid;
  y_grid.reserve(grid.size() - 1);
  for (int i = 1; i < grid.size(); i++) {
    y_grid.push_back((grid[i - 1] + grid[i]) / 2);
  }

  for (const auto* guide : guides) {
    const auto& box = guide->getBox();

    std::vector<odb::Point> guide_pts;
    for (const auto& x : x_grid) {
      if (x < box.xMin()) {
        continue;
      }
      if (x > box.xMax()) {
        break;
      }

      for (const auto& y : y_grid) {
        if (y < box.yMin()) {
          continue;
        }
        if (y > box.yMax()) {
          break;
        }

        guide_pts.emplace_back(x, y);
      }
    }

    std::ranges::sort(guide_pts);
    for (int i = 1; i < guide_pts.size(); i++) {
      lines.emplace(guide_pts[i - 1], guide_pts[i]);
    }

    if (!guide_pts.empty() && guide->isConnectedToTerm()) {
      auto find_term_connection
          = [&guide_pts, &lines, &io_map, &sources, &sinks](
                odb::dbObject* dbterm, const odb::Point& term) {
              // draw shortest flywire
              std::ranges::stable_sort(
                  guide_pts,

                  [&term](const odb::Point& pt0, const odb::Point& pt1) {
                    return odb::Point::manhattanDistance(term, pt0)
                           < odb::Point::manhattanDistance(term, pt1);
                  });
              lines.emplace(term, guide_pts[0]);
              if (io_map[dbterm].is_sink) {
                sinks[dbterm].insert(term);
              }
              if (io_map[dbterm].is_source) {
                sources[dbterm].insert(term);
              }
            };

      for (const auto& [obj, objbox] : terms[guide->getLayer()]) {
        std::vector<const odb::Rect*> candidates;
        for (const auto& termbox : objbox) {
          if (termbox.intersects(box)) {
            candidates.push_back(&termbox);
          }
        }
        bool found = false;
        for (const auto* termbox : candidates) {
          if (termbox->overlaps(box)) {
            find_term_connection(obj, termbox->center());
            found = true;
            break;
          }
        }
        if (!found && !candidates.empty()) {
          find_term_connection(obj, candidates[0]->center());
        }
      }
    }
  }

  return lines;
}

void DbNetDescriptor::drawPathSegmentWithGuides(
    const std::set<odb::Line>& lines,
    DbTargets& sources,
    DbTargets& sinks,
    const odb::dbObject* sink,
    Painter& painter) const
{
  PointMap pointmap;
  for (const auto& line : lines) {
    pointmap[line.pt0()].insert(line.pt1());
    pointmap[line.pt1()].insert(line.pt0());
  }

  for (const auto& [obj, srcs] : sources) {
    for (const auto& src_pt : srcs) {
      for (const auto& dst_pt : sinks[sink]) {
        std::vector<odb::Point> path;
        findPath(pointmap, src_pt, dst_pt, path);

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
          painter.drawLine(src_pt, dst_pt);
        }
      }
    }
  }
}

// additional_data is used define the related sink for this net
// this will limit the fly-wires to just those related to that sink
// if nullptr, all flywires will be drawn
void DbNetDescriptor::highlight(const std::any& object, Painter& painter) const
{
  odb::dbObject* sink_object = getSink(object);
  auto net = getObject(object);

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

  bool draw_flywires = true;

  if (!painter.getOptions()->isFlywireHighlightOnly()) {
    odb::dbWire* wire = net->getWire();
    if (wire) {
      draw_flywires = false;
      if (sink_object != nullptr) {
        drawPathSegmentWithGraph(net, sink_object, painter);
      }

      auto* wire_descriptor = Gui::get()->getDescriptor<odb::dbWire*>();
      wire_descriptor->highlight(wire, painter);
    } else {
      auto guides = net->getGuides();
      if (!guides.empty()) {
        draw_flywires = false;

        if (guides.begin()->getBox().minDXDY() * painter.getPixelsPerDBU()
            >= kMinGuidePixelWidth) {
          // draw outlines of guides, dont draw if less that kMinGuidePixelWidth
          // pixels
          std::vector<odb::Rect> guide_rects;
          guide_rects.reserve(guides.size());
          for (const auto* guide : guides) {
            guide_rects.push_back(guide->getBox());
          }
          painter.saveState();
          painter.setBrush(painter.getPenColor(), gui::Painter::Brush::kNone);
          for (const odb::Polygon& outline : odb::Polygon::merge(guide_rects)) {
            painter.drawPolygon(outline);
          }
          painter.restoreState();
        }

        painter.saveState();
        Painter::Color highlight_color = painter.getPenColor();
        highlight_color.a = 255;
        painter.setPen(highlight_color, true, 2);

        DbTargets sources;
        DbTargets sinks;
        std::set<odb::Line> lines = convertGuidesToLines(net, sources, sinks);

        if (sink_object != nullptr) {
          drawPathSegmentWithGuides(
              lines, sources, sinks, sink_object, painter);
        } else {
          for (const auto& line : lines) {
            painter.drawLine(line);
          }
        }

        painter.restoreState();
      }
    }
  }

  if (draw_flywires && !is_supply && !is_routed_special) {
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
  auto* swire_descriptor = Gui::get()->getDescriptor<odb::dbSWire*>();
  for (auto swire : net->getSWires()) {
    swire_descriptor->highlight(swire, painter);
  }
}

bool DbNetDescriptor::isSlowHighlight(const std::any& object) const
{
  auto net = getObject(object);
  return net->getSigType().isSupply();
}

bool DbNetDescriptor::isNet(const std::any& object) const
{
  return true;
}

Descriptor::Properties DbNetDescriptor::getDBProperties(odb::dbNet* net) const
{
  auto gui = Gui::get();

  Properties props({{"Block", gui->makeSelected(net->getBlock())},
                    {"Signal type", net->getSigType().getString()},
                    {"Source type", net->getSourceType().getString()},
                    {"Wire type", net->getWireType().getString()},
                    {"Special", net->isSpecial()},
                    {"Dont Touch", net->isDoNotTouch()}});
  int iterm_size = net->getITerms().size();
  std::any iterm_item;
  if (iterm_size > kMaxIterms) {
    iterm_item = std::to_string(iterm_size) + " items";
  } else {
    SelectionSet iterms;
    for (auto iterm : net->getITerms()) {
      iterms.insert(gui->makeSelected(iterm));
    }
    iterm_item = iterms;
  }
  props.emplace_back("ITerms", std::move(iterm_item));
  SelectionSet bterms;
  for (auto bterm : net->getBTerms()) {
    bterms.insert(gui->makeSelected(bterm));
  }
  props.emplace_back("BTerms", bterms);

  std::set<odb::dbModNet*> modnet_set;
  if (net->findRelatedModNets(modnet_set)) {
    SelectionSet mod_nets;
    for (odb::dbModNet* mod_net : modnet_set) {
      mod_nets.insert(gui->makeSelected(mod_net));
    }
    props.emplace_back("ModNets", mod_nets);
  }

  auto* ndr = net->getNonDefaultRule();
  if (ndr != nullptr) {
    props.emplace_back("Non-default rule", gui->makeSelected(ndr));
  }

  if (BufferTree::isAggregate(net)) {
    props.emplace_back("Buffer tree", gui->makeSelected(BufferTree(net)));
  }

  odb::dbWire* wire = net->getWire();
  if (wire != nullptr) {
    props.emplace_back("Wire", gui->makeSelected(wire));
  }
  SelectionSet swires;
  for (auto* swire : net->getSWires()) {
    swires.insert(gui->makeSelected(swire));
  }
  if (!swires.empty()) {
    props.emplace_back("Special wires", swires);
  }

  return props;
}

Descriptor::Editors DbNetDescriptor::getEditors(const std::any& object) const
{
  auto net = getObject(object);
  Editors editors;
  addRenameEditor(net, editors);
  editors.insert({"Special", makeEditor([net](const std::any& value) {
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
  editors.insert({"Dont Touch", makeEditor([net](const std::any& value) {
                    net->setDoNotTouch(std::any_cast<bool>(value));
                    return true;
                  })});
  return editors;
}

Descriptor::Actions DbNetDescriptor::getActions(const std::any& object) const
{
  auto net = getObject(object);

  auto* gui = Gui::get();
  Descriptor::Actions actions;
  if (!focus_nets_.contains(net)) {
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
    actions.push_back({"Timing", [this, gui, net]() {
                         auto* network = sta_->getDbNetwork();
                         auto* drivers
                             = network->drivers(network->dbToSta(net));

                         if (!drivers->empty()) {
                           std::set<Gui::Term> terms;

                           for (auto* driver : *drivers) {
                             odb::dbITerm* iterm = nullptr;
                             odb::dbBTerm* bterm = nullptr;
                             odb::dbModITerm* moditerm = nullptr;

                             network->staToDb(driver, iterm, bterm, moditerm);
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
    if (!guide_nets_.contains(net)) {
      actions.push_back(
          Descriptor::Action{"Show Route Guides", [this, gui, net]() {
                               gui->addRouteGuides(net);
                               return makeSelected(net);
                             }});
    } else {
      actions.push_back(
          Descriptor::Action{"Hide Route Guides", [this, gui, net]() {
                               gui->removeRouteGuides(net);
                               return makeSelected(net);
                             }});
    }
  }
  if (!net->getTracks().empty()) {
    if (!tracks_nets_.contains(net)) {
      actions.push_back(Descriptor::Action{"Show Tracks", [this, gui, net]() {
                                             gui->addNetTracks(net);
                                             return makeSelected(net);
                                           }});
    } else {
      actions.push_back(Descriptor::Action{"Hide Tracks", [this, gui, net]() {
                                             gui->removeNetTracks(net);
                                             return makeSelected(net);
                                           }});
    }
  }
  int drivers = 0;
  for (auto* iterm : net->getITerms()) {
    const auto iotype = iterm->getIoType();
    if (iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT) {
      drivers++;
    }
  }
  for (auto* bterm : net->getBTerms()) {
    const auto iotype = bterm->getIoType();
    if (iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT
        || iotype == odb::dbIoType::FEEDTHRU) {
      drivers++;
    }
  }

  if (drivers <= 1) {
    actions.push_back(
        {"Insert Buffer", [this, net]() {
           InsertBufferDialog dialog(net, sta_, nullptr);
           if (dialog.exec() == QDialog::Accepted) {
             odb::dbMaster* master = dialog.getSelectedMaster();
             odb::dbObject* driver = nullptr;
             std::set<odb::dbObject*> loads;
             dialog.getSelection(driver, loads);

             std::string buf_name = dialog.getBufferName().toStdString();
             std::string net_name = dialog.getNetName().toStdString();
             const char* buf_p
                 = buf_name.empty() ? kDefaultBufBaseName : buf_name.c_str();
             const char* net_p
                 = net_name.empty() ? kDefaultNetBaseName : net_name.c_str();

             try {
               odb::dbInst* buffer_inst = nullptr;
               if (driver) {
                 buffer_inst = net->insertBufferAfterDriver(
                     driver,
                     master,
                     nullptr,
                     buf_p,
                     net_p,
                     odb::dbNameUniquifyType::IF_NEEDED);
               } else if (!loads.empty()) {
                 buffer_inst = net->insertBufferBeforeLoads(
                     loads,
                     master,
                     nullptr,
                     buf_p,
                     net_p,
                     odb::dbNameUniquifyType::IF_NEEDED);
               }
               Gui::get()->redraw();
               if (buffer_inst) {
                 return Gui::get()->makeSelected(buffer_inst);
               }
             } catch (const std::exception& e) {
               QMessageBox::critical(nullptr, "Error", e.what());
             }
           }
           return makeSelected(net);
         }});
  }
  return actions;
}

Selected DbNetDescriptor::makeSelected(const std::any& object) const
{
  Selected net_selected = BaseDbDescriptor::makeSelected(object);
  if (net_selected) {
    return net_selected;
  }

  if (auto net = std::any_cast<NetWithSink>(&object)) {
    return Selected(*net, this);
  }
  return Selected();
}

bool DbNetDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_net = getObject(l);
  auto r_net = getObject(r);
  return BaseDbDescriptor::lessThan(l_net, r_net);
}

void DbNetDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* net : block->getNets()) {
    func({net, this});
  }
}

odb::dbNet* DbNetDescriptor::getObject(const std::any& object) const
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
    std::function<bool()> using_poly_decomp_view)
    : BaseDbDescriptor<odb::dbITerm>(db),
      using_poly_decomp_view_(std::move(using_poly_decomp_view))
{
}

std::string DbITermDescriptor::getName(const std::any& object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  return iterm->getName();
}

std::string DbITermDescriptor::getShortName(const std::any& object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  return iterm->getMTerm()->getName();
}

std::string DbITermDescriptor::getTypeName() const
{
  return "ITerm";
}

bool DbITermDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  if (iterm->getInst()->getPlacementStatus().isPlaced()) {
    bbox = iterm->getBBox();
    return true;
  }
  return false;
}

void DbITermDescriptor::highlight(const std::any& object,
                                  Painter& painter) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);

  if (!iterm->getInst()->getPlacementStatus().isPlaced()) {
    return;
  }

  const odb::dbTransform inst_xfm = iterm->getInst()->getTransform();

  auto mterm = iterm->getMTerm();
  for (auto mpin : mterm->getMPins()) {
    if (using_poly_decomp_view_()) {
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

Descriptor::Properties DbITermDescriptor::getDBProperties(
    odb::dbITerm* iterm) const
{
  auto gui = Gui::get();
  auto net = iterm->getNet();
  std::any net_value;
  if (net != nullptr) {
    net_value = gui->makeSelected(net);
  } else {
    net_value = "<none>";
  }
  auto mod_net = iterm->getModNet();
  std::any mod_net_value;
  if (mod_net != nullptr) {
    mod_net_value = gui->makeSelected(mod_net);
  } else {
    mod_net_value = "<none>";
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
                   {"ModNet", std::move(mod_net_value)},
                   {"Special", iterm->isSpecial()},
                   {"MTerm", gui->makeSelected(iterm->getMTerm())},
                   {"Access Points", aps}};

  return props;
}

Descriptor::Actions DbITermDescriptor::getActions(const std::any& object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);

  Descriptor::Actions actions;
  addTimingActions<odb::dbITerm*>(iterm, this, actions);

  return actions;
}

void DbITermDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* term : block->getITerms()) {
    func({term, this});
  }
}

//////////////////////////////////////////////////

DbBTermDescriptor::DbBTermDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbBTerm>(db)
{
}

std::string DbBTermDescriptor::getName(const std::any& object) const
{
  return std::any_cast<odb::dbBTerm*>(object)->getName();
}

std::string DbBTermDescriptor::getTypeName() const
{
  return "BTerm";
}

bool DbBTermDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto* bterm = std::any_cast<odb::dbBTerm*>(object);
  bbox = bterm->getBBox();
  return !bbox.isInverted();
}

void DbBTermDescriptor::highlight(const std::any& object,
                                  Painter& painter) const
{
  auto* bterm = std::any_cast<odb::dbBTerm*>(object);
  for (auto bpin : bterm->getBPins()) {
    for (auto box : bpin->getBoxes()) {
      odb::Rect rect = box->getBox();
      painter.drawRect(rect);
    }
  }
}

Descriptor::Properties DbBTermDescriptor::getDBProperties(
    odb::dbBTerm* bterm) const
{
  auto gui = Gui::get();
  SelectionSet aps;
  for (auto* pin : bterm->getBPins()) {
    for (auto ap : pin->getAccessPoints()) {
      DbTermAccessPoint bap{ap, bterm};
      aps.insert(gui->makeSelected(bap));
    }
  }
  auto mod_net = bterm->getModNet();
  std::any mod_net_value;
  if (mod_net != nullptr) {
    mod_net_value = gui->makeSelected(mod_net);
  } else {
    mod_net_value = "<none>";
  }
  Properties props{{"Block", gui->makeSelected(bterm->getBlock())},
                   {"Net", gui->makeSelected(bterm->getNet())},
                   {"ModNet", std::move(mod_net_value)},
                   {"Signal type", bterm->getSigType().getString()},
                   {"IO type", bterm->getIoType().getString()},
                   {"Access Points", aps}};

  std::optional<odb::Rect> constraint = bterm->getConstraintRegion();
  if (constraint) {
    props.emplace_back("Constraint Region", constraint.value());
  }

  props.emplace_back("Is Mirrored", bterm->isMirrored());
  if (odb::dbBTerm* mirrored = bterm->getMirroredBTerm()) {
    props.emplace_back("Mirrored", gui->makeSelected(mirrored));
  }

  SelectionSet pins;
  for (auto* pin : bterm->getBPins()) {
    pins.insert(gui->makeSelected(pin));
  }
  if (!pins.empty()) {
    props.emplace_back("Pins", pins);
  }

  return props;
}

Descriptor::Editors DbBTermDescriptor::getEditors(const std::any& object) const
{
  auto bterm = std::any_cast<odb::dbBTerm*>(object);
  Editors editors;
  addRenameEditor(bterm, editors);
  return editors;
}

Descriptor::Actions DbBTermDescriptor::getActions(const std::any& object) const
{
  auto bterm = std::any_cast<odb::dbBTerm*>(object);

  Descriptor::Actions actions;
  addTimingActions<odb::dbBTerm*>(bterm, this, actions);

  return actions;
}

void DbBTermDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* term : block->getBTerms()) {
    func({term, this});
  }
}

//////////////////////////////////////////////////

DbBPinDescriptor::DbBPinDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbBPin>(db)
{
}

std::string DbBPinDescriptor::getName(const std::any& object) const
{
  odb::dbBPin* pin = std::any_cast<odb::dbBPin*>(object);
  return pin->getBTerm()->getName();
}

std::string DbBPinDescriptor::getTypeName() const
{
  return "BPin";
}

bool DbBPinDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto* bpin = std::any_cast<odb::dbBPin*>(object);
  bbox = bpin->getBBox();
  return !bbox.isInverted();
}

void DbBPinDescriptor::highlight(const std::any& object, Painter& painter) const
{
  auto* bpin = std::any_cast<odb::dbBPin*>(object);
  for (auto box : bpin->getBoxes()) {
    odb::Rect rect = box->getBox();
    painter.drawRect(rect);
  }
}

Descriptor::Properties DbBPinDescriptor::getDBProperties(
    odb::dbBPin* bpin) const
{
  auto gui = Gui::get();
  SelectionSet aps;
  for (auto ap : bpin->getAccessPoints()) {
    DbTermAccessPoint bap{ap, bpin->getBTerm()};
    aps.insert(gui->makeSelected(bap));
  }
  Properties props{{"BTerm", gui->makeSelected(bpin->getBTerm())},
                   {"Placement status", bpin->getPlacementStatus().getString()},
                   {"Access points", aps}};

  PropertyList boxes;
  for (auto* box : bpin->getBoxes()) {
    auto* layer = box->getTechLayer();
    if (layer != nullptr) {
      boxes.emplace_back(gui->makeSelected(box->getTechLayer()),
                         gui->makeSelected(box));
    }
  }
  props.emplace_back("Boxes", boxes);

  if (bpin->hasEffectiveWidth()) {
    props.emplace_back("Effective width",
                       convertUnits(bpin->getEffectiveWidth()));
  }

  if (bpin->hasMinSpacing()) {
    props.emplace_back("Min spacing", convertUnits(bpin->getMinSpacing()));
  }

  return props;
}

void DbBPinDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* term : block->getBTerms()) {
    for (auto* pin : term->getBPins()) {
      func({pin, this});
    }
  }
}

//////////////////////////////////////////////////

DbMTermDescriptor::DbMTermDescriptor(
    odb::dbDatabase* db,
    std::function<bool()> using_poly_decomp_view)
    : BaseDbDescriptor<odb::dbMTerm>(db),
      using_poly_decomp_view_(std::move(using_poly_decomp_view))
{
}

std::string DbMTermDescriptor::getName(const std::any& object) const
{
  auto mterm = std::any_cast<odb::dbMTerm*>(object);
  return mterm->getMaster()->getName() + "/" + mterm->getName();
}

std::string DbMTermDescriptor::getShortName(const std::any& object) const
{
  auto mterm = std::any_cast<odb::dbMTerm*>(object);
  return mterm->getName();
}

std::string DbMTermDescriptor::getTypeName() const
{
  return "MTerm";
}

bool DbMTermDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto mterm = std::any_cast<odb::dbMTerm*>(object);
  bbox = mterm->getBBox();
  return true;
}

void DbMTermDescriptor::highlight(const std::any& object,
                                  Painter& painter) const
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
    if (using_poly_decomp_view_()) {
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

Descriptor::Properties DbMTermDescriptor::getDBProperties(
    odb::dbMTerm* mterm) const
{
  auto gui = Gui::get();
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

  return props;
}

void DbMTermDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  for (auto* lib : db_->getLibs()) {
    for (auto* master : lib->getMasters()) {
      for (auto* mterm : master->getMTerms()) {
        func({mterm, this});
      }
    }
  }
}

//////////////////////////////////////////////////

DbViaDescriptor::DbViaDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbVia>(db)
{
}

std::string DbViaDescriptor::getName(const std::any& object) const
{
  auto via = std::any_cast<odb::dbVia*>(object);
  return via->getName();
}

std::string DbViaDescriptor::getTypeName() const
{
  return "Block Via";
}

bool DbViaDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void DbViaDescriptor::highlight(const std::any& object, Painter& painter) const
{
}

Descriptor::Properties DbViaDescriptor::getDBProperties(odb::dbVia* via) const
{
  auto gui = Gui::get();

  Properties props({{"Block", gui->makeSelected(via->getBlock())}});

  if (!via->getPattern().empty()) {
    props.emplace_back("Pattern", via->getPattern());
  }

  props.emplace_back("Tech Via Generate Rule",
                     gui->makeSelected(via->getViaGenerateRule()));

  if (via->hasParams()) {
    const odb::dbViaParams via_params = via->getViaParams();

    props.emplace_back(
        "Cut Size",
        fmt::format("X={}, Y={}",
                    Property::convert_dbu(via_params.getXCutSize(), true),
                    Property::convert_dbu(via_params.getYCutSize(), true)));

    props.emplace_back(
        "Cut Spacing",
        fmt::format("X={}, Y={}",
                    Property::convert_dbu(via_params.getXCutSpacing(), true),
                    Property::convert_dbu(via_params.getYCutSpacing(), true)));

    props.emplace_back(
        "Top Enclosure",
        fmt::format(
            "X={}, Y={}",
            Property::convert_dbu(via_params.getXTopEnclosure(), true),
            Property::convert_dbu(via_params.getYTopEnclosure(), true)));

    props.emplace_back(
        "Bottom Enclosure",
        fmt::format(
            "X={}, Y={}",
            Property::convert_dbu(via_params.getXBottomEnclosure(), true),
            Property::convert_dbu(via_params.getYBottomEnclosure(), true)));

    props.emplace_back("Number of Cut Rows", via_params.getNumCutRows());
    props.emplace_back("Number of Cut Columns", via_params.getNumCutCols());

    props.emplace_back(
        "Origin",
        fmt::format("X={}, Y={}",
                    Property::convert_dbu(via_params.getXOrigin(), true),
                    Property::convert_dbu(via_params.getYOrigin(), true)));

    props.emplace_back(
        "Top Offset",
        fmt::format("X={}, Y={}",
                    Property::convert_dbu(via_params.getXTopOffset(), true),
                    Property::convert_dbu(via_params.getYTopOffset(), true)));

    props.emplace_back(
        "Bottom Offset",
        fmt::format(
            "X={}, Y={}",
            Property::convert_dbu(via_params.getXBottomOffset(), true),
            Property::convert_dbu(via_params.getYBottomOffset(), true)));

    PropertyList shapes;
    for (auto box : via->getBoxes()) {
      auto layer = box->getTechLayer();
      auto rect = box->getBox();
      shapes.emplace_back(gui->makeSelected(layer), rect);
    }
    props.emplace_back("Shapes", shapes);
  } else {
    PropertyList shapes;
    for (auto box : via->getBoxes()) {
      auto layer = box->getTechLayer();
      auto rect = box->getBox();
      shapes.emplace_back(gui->makeSelected(layer), rect);
    }
    props.emplace_back("Shapes", shapes);
  }

  props.emplace_back("Is Rotated", via->isViaRotated());

  if (via->isViaRotated()) {
    props.emplace_back("Orientation", via->getOrient().getString());
    props.emplace_back("Tech Via", gui->makeSelected(via->getTechVia()));
    props.emplace_back("Block Via", gui->makeSelected(via->getBlockVia()));
  }

  props.emplace_back("Is Default", via->isDefault());

  return props;
}

void DbViaDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }

  auto block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto via : block->getVias()) {
    func({via, this});
  }
}

//////////////////////////////////////////////////

DbBlockageDescriptor::DbBlockageDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbBlockage>(db)
{
}

std::string DbBlockageDescriptor::getName(const std::any& object) const
{
  return "Blockage";
}

std::string DbBlockageDescriptor::getTypeName() const
{
  return "Blockage";
}

bool DbBlockageDescriptor::getBBox(const std::any& object,
                                   odb::Rect& bbox) const
{
  auto* blockage = std::any_cast<odb::dbBlockage*>(object);
  odb::dbBox* box = blockage->getBBox();
  bbox = box->getBox();
  return true;
}

void DbBlockageDescriptor::highlight(const std::any& object,
                                     Painter& painter) const
{
  odb::Rect rect;
  getBBox(object, rect);
  painter.drawRect(rect);
}

Descriptor::Properties DbBlockageDescriptor::getDBProperties(
    odb::dbBlockage* blockage) const
{
  auto gui = Gui::get();
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

  return props;
}

Descriptor::Actions DbBlockageDescriptor::getActions(
    const std::any& object) const
{
  auto blk = std::any_cast<odb::dbBlockage*>(object);
  return Actions(
      {{"Delete", [blk]() {
          odb::dbBlockage::destroy(blk);
          return Selected();  // unselect since this object is now gone
        }}});
}

Descriptor::Editors DbBlockageDescriptor::getEditors(
    const std::any& object) const
{
  auto blockage = std::any_cast<odb::dbBlockage*>(object);
  Editors editors;
  editors.insert(
      {"Max density", makeEditor([blockage](const std::any& any_value) {
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
           } catch (std::out_of_range&) {  // NOLINT(bugprone-empty-catch)
             // catch poorly formatted string
           } catch (std::logic_error&) {  // NOLINT(bugprone-empty-catch)
             // catch poorly formatted string
           }
         }
         return false;
       })});
  return editors;
}

void DbBlockageDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* blockage : block->getBlockages()) {
    func({blockage, this});
  }
}

//////////////////////////////////////////////////

DbObstructionDescriptor::DbObstructionDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbObstruction>(db)
{
}

std::string DbObstructionDescriptor::getName(const std::any& object) const
{
  auto obs = std::any_cast<odb::dbObstruction*>(object);
  return "Obstruction: " + obs->getBBox()->getTechLayer()->getName();
}

std::string DbObstructionDescriptor::getTypeName() const
{
  return "Obstruction";
}

bool DbObstructionDescriptor::getBBox(const std::any& object,
                                      odb::Rect& bbox) const
{
  auto obs = std::any_cast<odb::dbObstruction*>(object);
  odb::dbBox* box = obs->getBBox();
  bbox = box->getBox();
  return true;
}

void DbObstructionDescriptor::highlight(const std::any& object,
                                        Painter& painter) const
{
  odb::Rect rect;
  getBBox(object, rect);
  painter.drawRect(rect);
}

Descriptor::Properties DbObstructionDescriptor::getDBProperties(
    odb::dbObstruction* obs) const
{
  auto gui = Gui::get();
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
    props.emplace_back("Effective width",
                       Property::convert_dbu(obs->getEffectiveWidth(), true));
  }

  if (obs->hasMinSpacing()) {
    props.emplace_back("Min spacing",
                       Property::convert_dbu(obs->getMinSpacing(), true));
  }

  return props;
}

Descriptor::Actions DbObstructionDescriptor::getActions(
    const std::any& object) const
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

void DbObstructionDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* obs : block->getObstructions()) {
    func({obs, this});
  }
}

//////////////////////////////////////////////////

DbTechLayerDescriptor::DbTechLayerDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTechLayer>(db)
{
}

std::string DbTechLayerDescriptor::getName(const std::any& object) const
{
  auto layer = std::any_cast<odb::dbTechLayer*>(object);
  return layer->getConstName();
}

std::string DbTechLayerDescriptor::getTypeName() const
{
  return "Tech layer";
}

bool DbTechLayerDescriptor::getBBox(const std::any& object,
                                    odb::Rect& bbox) const
{
  return false;
}

void DbTechLayerDescriptor::highlight(const std::any& object,
                                      Painter& painter) const
{
}

Descriptor::Properties DbTechLayerDescriptor::getDBProperties(
    odb::dbTechLayer* layer) const
{
  auto* gui = Gui::get();
  Properties props({{"Technology", gui->makeSelected(layer->getTech())},
                    {"Direction", layer->getDirection().getString()},
                    {"Layer type", layer->getType().getString()}});
  if (layer->getLef58Type() != odb::dbTechLayer::NONE) {
    props.emplace_back("LEF58 type", layer->getLef58TypeString());
  }
  props.emplace_back("Layer number", layer->getNumber());
  if (layer->getType() == odb::dbTechLayerType::ROUTING) {
    props.emplace_back("Routing layer", layer->getRoutingLevel());
  }
  if (layer->hasXYPitch()) {
    if (layer->getPitchX() != 0) {
      props.emplace_back("Pitch X",
                         Property::convert_dbu(layer->getPitchX(), true));
    }
    if (layer->getPitchY() != 0) {
      props.emplace_back("Pitch Y",
                         Property::convert_dbu(layer->getPitchY(), true));
    }
  } else {
    if (layer->getPitch() != 0) {
      props.emplace_back("Pitch",
                         Property::convert_dbu(layer->getPitch(), true));
    }
  }
  if (layer->getWidth() != 0) {
    props.emplace_back("Default width",
                       Property::convert_dbu(layer->getWidth(), true));
  }
  if (layer->getMinWidth() != 0) {
    props.emplace_back("Minimum width",
                       Property::convert_dbu(layer->getMinWidth(), true));
  }
  if (layer->getWrongWayMinWidth() != 0) {
    props.emplace_back(
        "Wrong way minimum width",
        Property::convert_dbu(layer->getWrongWayMinWidth(), true));
  }
  if (layer->hasMaxWidth()) {
    props.emplace_back("Max width",
                       Property::convert_dbu(layer->getMaxWidth(), true));
  }
  if (layer->getSpacing() != 0) {
    props.emplace_back("Minimum spacing",
                       Property::convert_dbu(layer->getSpacing(), true));
  }
  if (layer->hasArea()) {
    props.emplace_back(
        "Minimum area",
        convertUnits(layer->getArea() * 1e-6 * 1e-6, true) + "m²");
  }
  if (layer->getResistance() != 0.0) {
    props.emplace_back("Resistance",
                       convertUnits(layer->getResistance()) + "Ω/sq");
  }
  if (layer->getCapacitance() != 0.0) {
    props.emplace_back("Capacitance",
                       convertUnits(layer->getCapacitance() * 1e-12) + "F/μm²");
  }
  if (layer->getEdgeCapacitance() != 0.0) {
    props.emplace_back(
        "Edge capacitance",
        convertUnits(layer->getEdgeCapacitance() * 1e-12) + "F/μm");
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
    props.emplace_back(std::move(title), widths);
  }

  if (layer->hasTwoWidthsSpacingRules()) {
    const int widths = layer->getTwoWidthsSpacingTableNumWidths();

    PropertyTable table(widths, widths);
    for (int i = 0; i < widths; i++) {
      const std::string prefix_title = Property::convert_dbu(
          layer->getTwoWidthsSpacingTableWidth(i), true);
      const std::string prl_title
          = layer->getTwoWidthsSpacingTableHasPRL(i)
                ? "\nPRL "
                      + Property::convert_dbu(
                          layer->getTwoWidthsSpacingTablePRL(i), true)
                : "";
      table.setRowHeader(i, prefix_title + prl_title);
      table.setColumnHeader(i, prefix_title + prl_title);
      for (int j = 0; j < widths; j++) {
        table.setData(i,
                      j,
                      Property::convert_dbu(
                          layer->getTwoWidthsSpacingTableEntry(i, j), true));
      }
    }

    if (!table.empty()) {
      props.emplace_back("Two width spacing rules", table);
    }
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
    props.emplace_back("Cut classes", cutclasses);
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
    props.emplace_back("Cut enclosures", cut_enclosures);
  }

  PropertyList minimum_cuts;
  for (auto* min_cut_rule : layer->getMinCutRules()) {
    uint32_t numcuts;
    uint32_t rule_width;
    min_cut_rule->getMinimumCuts(numcuts, rule_width);

    std::string text = Property::convert_dbu(rule_width, true);

    if (min_cut_rule->isAboveOnly()) {
      text += " - above only";
    }
    if (min_cut_rule->isBelowOnly()) {
      text += " - below only";
    }

    uint32_t length;
    uint32_t distance;
    if (min_cut_rule->getLengthForCuts(length, distance)) {
      text += fmt::format(" LENGTH {} WITHIN {}",
                          Property::convert_dbu(length, true),
                          Property::convert_dbu(distance, true));
    }

    minimum_cuts.emplace_back(text, static_cast<int>(numcuts));
  }
  if (!minimum_cuts.empty()) {
    props.emplace_back("Minimum cuts", minimum_cuts);
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
      lef58_minimum_cuts.emplace_back(fmt::format("{} - {}", text, cutclass),
                                      min_cut);
    }
  }
  if (!lef58_minimum_cuts.empty()) {
    props.emplace_back("LEF58 minimum cuts", lef58_minimum_cuts);
  }

  if (layer->getType() == odb::dbTechLayerType::CUT) {
    auto* tech = layer->getTech();

    SelectionSet generate_vias;
    for (auto* via : tech->getViaGenerateRules()) {
      for (uint32_t l = 0; l < via->getViaLayerRuleCount(); l++) {
        auto* rule = via->getViaLayerRule(l);
        if (rule->getLayer() == layer) {
          generate_vias.insert(gui->makeSelected(via));
          break;
        }
      }
    }
    props.emplace_back("Generate vias", generate_vias);

    SelectionSet tech_vias;
    for (auto* via : tech->getVias()) {
      for (auto* box : via->getBoxes()) {
        if (box->getTechLayer() == layer) {
          tech_vias.insert(gui->makeSelected(via));
          break;
        }
      }
    }
    props.emplace_back("Tech vias", tech_vias);
  }

  for (auto* spacing_table : layer->getTechLayerVoltageSpacings()) {
    std::string title = "Voltage spacing";
    if (spacing_table->isTocutAbove() && spacing_table->isTocutBelow()) {
      title += " - tocut";
    } else if (spacing_table->isTocutAbove()) {
      title += " - tocut above";
    } else if (spacing_table->isTocutBelow()) {
      title += " - tocut below";
    }

    PropertyList voltagetable;
    for (const auto& [voltage, spacing] : spacing_table->getTable()) {
      voltagetable.emplace_back(fmt::format("{:.3f}V", voltage),
                                Property::convert_dbu(spacing, true));
    }
    props.emplace_back(std::move(title), voltagetable);
  }

  return props;
}

void DbTechLayerDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* tech = db_->getTech();
  if (tech == nullptr) {
    return;
  }

  for (auto* layer : tech->getLayers()) {
    func({layer, this});
  }
}

//////////////////////////////////////////////////

DbTermAccessPointDescriptor::DbTermAccessPointDescriptor(odb::dbDatabase* db)
    : db_(db)
{
}

std::string DbTermAccessPointDescriptor::getName(const std::any& object) const
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

bool DbTermAccessPointDescriptor::getBBox(const std::any& object,
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

void DbTermAccessPointDescriptor::highlight(const std::any& object,
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
    const std::any& object) const
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
      vias_property.emplace_back(cnt++, name);
    }
    props.emplace_back(fmt::format("{} cut vias", cuts + 1), vias_property);
  }

  populateODBProperties(props, ap);
  return props;
}

Selected DbTermAccessPointDescriptor::makeSelected(const std::any& object) const
{
  if (object.type() == typeid(DbTermAccessPoint)) {
    auto iterm_ap = std::any_cast<DbTermAccessPoint>(object);
    return Selected(iterm_ap, this);
  }
  return Selected();
}

bool DbTermAccessPointDescriptor::lessThan(const std::any& l,
                                           const std::any& r) const
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

void DbTermAccessPointDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* iterm : block->getITerms()) {
    for (const auto& [mpin, aps] : iterm->getAccessPoints()) {
      for (auto* ap : aps) {
        func({DbTermAccessPoint{ap, iterm}, this});
      }
    }
  }

  for (auto* bterm : block->getBTerms()) {
    for (auto* pin : bterm->getBPins()) {
      for (auto* ap : pin->getAccessPoints()) {
        func({DbTermAccessPoint{ap, bterm}, this});
      }
    }
  }
}

//////////////////////////////////////////////////

DbGroupDescriptor::DbGroupDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbGroup>(db)
{
}

std::string DbGroupDescriptor::getName(const std::any& object) const
{
  auto* group = std::any_cast<odb::dbGroup*>(object);
  return group->getName();
}

std::string DbGroupDescriptor::getTypeName() const
{
  return "Group";
}

bool DbGroupDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto* group = std::any_cast<odb::dbGroup*>(object);
  auto* region = group->getRegion();
  if (region != nullptr && region->getBoundaries().size() == 1) {
    bbox = region->getBoundaries().begin()->getBox();
    return true;
  }
  return false;
}

void DbGroupDescriptor::highlight(const std::any& object,
                                  Painter& painter) const
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

Descriptor::Properties DbGroupDescriptor::getDBProperties(
    odb::dbGroup* group) const
{
  auto* gui = Gui::get();

  Properties props;
  auto* parent = group->getParentGroup();
  if (parent != nullptr) {
    props.emplace_back("Parent", gui->makeSelected(parent));
  }

  auto* region = group->getRegion();
  if (region != nullptr) {
    props.emplace_back("Region", gui->makeSelected(region));
  }

  SelectionSet groups;
  for (auto* subgroup : group->getGroups()) {
    groups.insert(gui->makeSelected(subgroup));
  }
  if (!groups.empty()) {
    props.emplace_back("Groups", groups);
  }

  SelectionSet insts;
  for (auto* inst : group->getInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.emplace_back("Instances", insts);

  props.emplace_back("Group Type", group->getType().getString());

  SelectionSet pwr;
  for (auto* net : group->getPowerNets()) {
    pwr.insert(gui->makeSelected(net));
  }
  props.emplace_back("Power Nets", pwr);

  SelectionSet gnd;
  for (auto* net : group->getGroundNets()) {
    gnd.insert(gui->makeSelected(net));
  }
  props.emplace_back("Ground Nets", gnd);

  return props;
}

void DbGroupDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* group : block->getGroups()) {
    func({group, this});
  }
}

//////////////////////////////////////////////////

DbRegionDescriptor::DbRegionDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbRegion>(db)
{
}

std::string DbRegionDescriptor::getName(const std::any& object) const
{
  auto* region = std::any_cast<odb::dbRegion*>(object);
  return region->getName();
}

std::string DbRegionDescriptor::getTypeName() const
{
  return "Region";
}

bool DbRegionDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
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

void DbRegionDescriptor::highlight(const std::any& object,
                                   Painter& painter) const
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

Descriptor::Properties DbRegionDescriptor::getDBProperties(
    odb::dbRegion* region) const
{
  auto* gui = Gui::get();

  Properties props({{"Block", gui->makeSelected(region->getBlock())},
                    {"Region Type", region->getRegionType().getString()}});
  SelectionSet children;
  for (auto* child : region->getGroups()) {
    children.insert(gui->makeSelected(child));
  }
  if (!children.empty()) {
    props.emplace_back("Groups", children);
  }

  SelectionSet insts;
  for (auto* inst : region->getRegionInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.emplace_back("Instances", insts);

  return props;
}

void DbRegionDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* region : block->getRegions()) {
    func({region, this});
  }
}

//////////////////////////////////////////////////

DbModuleDescriptor::DbModuleDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbModule>(db)
{
}

std::string DbModuleDescriptor::getShortName(const std::any& object) const
{
  auto* module = std::any_cast<odb::dbModule*>(object);
  return module->getName();
}

std::string DbModuleDescriptor::getName(const std::any& object) const
{
  auto* module = std::any_cast<odb::dbModule*>(object);
  return module->getHierarchicalName();
}

std::string DbModuleDescriptor::getTypeName() const
{
  return "Module";
}

bool DbModuleDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
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

void DbModuleDescriptor::highlight(const std::any& object,
                                   Painter& painter) const
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

Descriptor::Properties DbModuleDescriptor::getDBProperties(
    odb::dbModule* module) const
{
  auto* mod_inst = module->getModInst();

  auto* gui = Gui::get();

  Properties props;
  if (mod_inst != nullptr) {
    props.emplace_back("ModInst", gui->makeSelected(mod_inst));
    auto* parent = mod_inst->getParent();
    if (parent != nullptr) {
      props.emplace_back("Parent", gui->makeSelected(parent));
    }

    auto* group = mod_inst->getGroup();
    if (group != nullptr) {
      props.emplace_back("Group", gui->makeSelected(group));
    }
  }

  SelectionSet children;
  for (auto* child : module->getChildren()) {
    children.insert(gui->makeSelected(child->getMaster()));
  }
  if (!children.empty()) {
    props.emplace_back("Children", children);
  }

  SelectionSet insts;
  for (auto* inst : module->getInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.emplace_back("Instances", insts);

  SelectionSet modnets;
  for (auto* modnet : module->getModNets()) {
    modnets.insert(gui->makeSelected(modnet));
  }
  props.emplace_back("ModNets", modnets);

  SelectionSet ports;
  for (auto* port : module->getPorts()) {
    ports.insert(gui->makeSelected(port));
  }
  props.emplace_back("Ports", ports);

  SelectionSet modbterms;
  for (auto* modbterm : module->getModBTerms()) {
    modbterms.insert(gui->makeSelected(modbterm));
  }
  props.emplace_back("ModBTerms", modbterms);

  if (mod_inst != nullptr) {
    populateODBProperties(props, mod_inst, "Instance");
  }

  return props;
}

void DbModuleDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  getModules(block->getTopModule(), func);
}

void DbModuleDescriptor::getModules(
    odb::dbModule* module,
    const std::function<void(const Selected&)>& func) const
{
  func({module, this});

  for (auto* mod_inst : module->getChildren()) {
    getModules(mod_inst->getMaster(), func);
  }
}

//////////////////////////////////////////////////

DbModBTermDescriptor::DbModBTermDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbModBTerm>(db)
{
}

std::string DbModBTermDescriptor::getShortName(const std::any& object) const
{
  auto* modBTerm = std::any_cast<odb::dbModBTerm*>(object);
  return modBTerm->getName();
}

std::string DbModBTermDescriptor::getName(const std::any& object) const
{
  auto* modBTerm = std::any_cast<odb::dbModBTerm*>(object);
  return modBTerm->getHierarchicalName();
}

std::string DbModBTermDescriptor::getTypeName() const
{
  return "ModBTerm";
}

bool DbModBTermDescriptor::getBBox(const std::any& object,
                                   odb::Rect& bbox) const
{
  return false;
}

void DbModBTermDescriptor::highlight(const std::any& object,
                                     Painter& painter) const
{
}

Descriptor::Properties DbModBTermDescriptor::getDBProperties(
    odb::dbModBTerm* modbterm) const
{
  auto* gui = Gui::get();

  Properties props;
  auto* parent = modbterm->getParent();
  if (parent != nullptr) {
    props.emplace_back("Parent", gui->makeSelected(parent));
  }

  auto* moditerm = modbterm->getParentModITerm();
  if (moditerm != nullptr) {
    props.emplace_back("Parent ModITerm", gui->makeSelected(moditerm));
  }

  auto* modnet = modbterm->getModNet();
  if (modnet != nullptr) {
    props.emplace_back("ModNet", gui->makeSelected(modnet));
  }

  auto signal = modbterm->getSigType().getString();
  if (signal) {
    props.emplace_back("Signal type", signal);
  }

  auto iotype = modbterm->getIoType().getString();
  if (iotype != nullptr) {
    props.emplace_back("IO type", iotype);
  }

  auto* bus = modbterm->getBusPort();
  if (bus) {
    SelectionSet ports;
    for (auto port : bus->getBusPortMembers()) {
      ports.insert(gui->makeSelected(port));
    }

    props.emplace_back("Bus port", ports);
  } else {
    props.emplace_back("Is bus port", false);
  }

  return props;
}

void DbModBTermDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  getModBTerms(block->getTopModule(), func);
}

void DbModBTermDescriptor::getModBTerms(
    odb::dbModule* module,
    const std::function<void(const Selected&)>& func) const
{
  for (auto* modbterm : module->getModBTerms()) {
    func({modbterm, this});
  }

  for (auto* mod_inst : module->getChildren()) {
    getModBTerms(mod_inst->getMaster(), func);
  }
}

//////////////////////////////////////////////////

DbModITermDescriptor::DbModITermDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbModITerm>(db)
{
}

std::string DbModITermDescriptor::getShortName(const std::any& object) const
{
  auto* modITerm = std::any_cast<odb::dbModITerm*>(object);
  return modITerm->getName();
}

std::string DbModITermDescriptor::getName(const std::any& object) const
{
  auto* modITerm = std::any_cast<odb::dbModITerm*>(object);
  return modITerm->getHierarchicalName();
}

std::string DbModITermDescriptor::getTypeName() const
{
  return "ModITerm";
}

bool DbModITermDescriptor::getBBox(const std::any& object,
                                   odb::Rect& bbox) const
{
  return false;
}

void DbModITermDescriptor::highlight(const std::any& object,
                                     Painter& painter) const
{
}

Descriptor::Properties DbModITermDescriptor::getDBProperties(
    odb::dbModITerm* moditerm) const
{
  auto* gui = Gui::get();

  Properties props;
  auto* parent = moditerm->getParent();
  if (parent != nullptr) {
    props.emplace_back("Parent", gui->makeSelected(parent));
  }

  auto* modnet = moditerm->getModNet();
  if (modnet != nullptr) {
    props.emplace_back("ModNet", gui->makeSelected(modnet));
  }

  auto* modbterm = moditerm->getChildModBTerm();
  if (modbterm != nullptr) {
    props.emplace_back("Child ModBTerm", gui->makeSelected(modbterm));
  }

  return props;
}

void DbModITermDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  getModITerms(block->getTopModule(), func);
}

void DbModITermDescriptor::getModITerms(
    odb::dbModule* module,
    const std::function<void(const Selected&)>& func) const
{
  auto mod_inst = module->getModInst();
  if (mod_inst) {
    for (auto* modbterm : mod_inst->getModITerms()) {
      func({modbterm, this});
    }
  }

  for (auto* mod_inst : module->getChildren()) {
    getModITerms(mod_inst->getMaster(), func);
  }
}

//////////////////////////////////////////////////

DbModInstDescriptor::DbModInstDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbModInst>(db)
{
}

std::string DbModInstDescriptor::getShortName(const std::any& object) const
{
  auto* modInst = std::any_cast<odb::dbModInst*>(object);
  return modInst->getName();
}

std::string DbModInstDescriptor::getName(const std::any& object) const
{
  auto* modInst = std::any_cast<odb::dbModInst*>(object);
  return modInst->getHierarchicalName();
}

std::string DbModInstDescriptor::getTypeName() const
{
  return "ModInst";
}

bool DbModInstDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void DbModInstDescriptor::highlight(const std::any& object,
                                    Painter& painter) const
{
}

Descriptor::Properties DbModInstDescriptor::getDBProperties(
    odb::dbModInst* modinst) const
{
  auto* gui = Gui::get();

  Properties props;
  auto* parent = modinst->getParent();
  if (parent != nullptr) {
    props.emplace_back("Parent", gui->makeSelected(parent));
  }

  auto* master = modinst->getMaster();
  if (master != nullptr) {
    props.emplace_back("Master", gui->makeSelected(master));
  }

  auto* group = modinst->getGroup();
  if (group != nullptr) {
    props.emplace_back("Group", gui->makeSelected(group));
  }

  SelectionSet moditerms;
  for (auto* moditerm : modinst->getModITerms()) {
    moditerms.insert(gui->makeSelected(moditerm));
  }
  props.emplace_back("ModITerms", moditerms);

  return props;
}

void DbModInstDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  getModInsts(block->getTopModule(), func);
}

void DbModInstDescriptor::getModInsts(
    odb::dbModule* module,
    const std::function<void(const Selected&)>& func) const
{
  auto mod_inst = module->getModInst();
  if (mod_inst) {
    func({mod_inst, this});
  }

  for (auto* mod_inst : module->getChildren()) {
    getModInsts(mod_inst->getMaster(), func);
  }
}
//////////////////////////////////////////////////

DbModNetDescriptor::DbModNetDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbModNet>(db)
{
}

std::string DbModNetDescriptor::getShortName(const std::any& object) const
{
  auto* modnet = std::any_cast<odb::dbModNet*>(object);
  return modnet->getName();
}

std::string DbModNetDescriptor::getName(const std::any& object) const
{
  auto* modnet = std::any_cast<odb::dbModNet*>(object);
  return modnet->getHierarchicalName();
}

std::string DbModNetDescriptor::getTypeName() const
{
  return "ModNet";
}

bool DbModNetDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void DbModNetDescriptor::highlight(const std::any& object,
                                   Painter& painter) const
{
}

Descriptor::Properties DbModNetDescriptor::getDBProperties(
    odb::dbModNet* modnet) const
{
  auto* gui = Gui::get();

  Properties props;
  auto* parent = modnet->getParent();
  if (parent != nullptr) {
    props.emplace_back("Parent", gui->makeSelected(parent));
  }

  SelectionSet moditerms;
  for (auto* moditerm : modnet->getModITerms()) {
    moditerms.insert(gui->makeSelected(moditerm));
  }
  props.emplace_back("ModITerms", moditerms);

  SelectionSet modbterms;
  for (auto* modbterm : modnet->getModBTerms()) {
    modbterms.insert(gui->makeSelected(modbterm));
  }
  props.emplace_back("ModBTerms", modbterms);

  SelectionSet iterms;
  for (auto* iterm : modnet->getITerms()) {
    iterms.insert(gui->makeSelected(iterm));
  }
  props.emplace_back("ITerms", iterms);

  SelectionSet bterms;
  for (auto* bterm : modnet->getBTerms()) {
    bterms.insert(gui->makeSelected(bterm));
  }
  props.emplace_back("BTerms", bterms);
  props.emplace_back("Net", gui->makeSelected(modnet->findRelatedNet()));

  return props;
}

void DbModNetDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  getModNets(block->getTopModule(), func);
}

void DbModNetDescriptor::getModNets(
    odb::dbModule* module,
    const std::function<void(const Selected&)>& func) const
{
  for (auto* modnet : module->getModNets()) {
    func({modnet, this});
  }

  for (auto* mod_inst : module->getChildren()) {
    getModNets(mod_inst->getMaster(), func);
  }
}

//////////////////////////////////////////////////

DbTechViaDescriptor::DbTechViaDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTechVia>(db)
{
}

std::string DbTechViaDescriptor::getName(const std::any& object) const
{
  auto* via = std::any_cast<odb::dbTechVia*>(object);
  return via->getName();
}

std::string DbTechViaDescriptor::getTypeName() const
{
  return "Tech Via";
}

bool DbTechViaDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void DbTechViaDescriptor::highlight(const std::any& object,
                                    Painter& painter) const
{
}

Descriptor::Properties DbTechViaDescriptor::getDBProperties(
    odb::dbTechVia* via) const
{
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
    layers.emplace_back(gui->makeSelected(layer), shape);
  };
  make_layer(via->getBottomLayer());
  if (cut_layer != nullptr) {
    make_layer(cut_layer);
  }
  make_layer(via->getTopLayer());
  props.emplace_back("Layers", layers);

  props.emplace_back("Is default", via->isDefault());
  props.emplace_back("Is top of stack", via->isTopOfStack());

  if (via->getResistance() != 0.0) {
    props.emplace_back("Resistance",
                       convertUnits(via->getResistance()) + "Ω/sq");
  }

  auto* ndr = via->getNonDefaultRule();
  if (ndr != nullptr) {
    props.emplace_back("Non-default Rule", gui->makeSelected(ndr));
  }

  return props;
}

void DbTechViaDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* tech = db_->getTech();

  for (auto* via : tech->getVias()) {
    func({via, this});
  }
}
//////////////////////////////////////////////////

DbTechViaRuleDescriptor::DbTechViaRuleDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTechViaRule>(db)
{
}

std::string DbTechViaRuleDescriptor::getName(const std::any& object) const
{
  auto via_rule = std::any_cast<odb::dbTechViaRule*>(object);
  return via_rule->getName();
}

std::string DbTechViaRuleDescriptor::getTypeName() const
{
  return "Tech Via Rule";
}

bool DbTechViaRuleDescriptor::getBBox(const std::any& object,
                                      odb::Rect& bbox) const
{
  return false;
}

void DbTechViaRuleDescriptor::highlight(const std::any& object,
                                        Painter& painter) const
{
}

Descriptor::Properties DbTechViaRuleDescriptor::getDBProperties(
    odb::dbTechViaRule* via_rule) const
{
  auto gui = Gui::get();

  Properties props;

  SelectionSet vias;
  for (uint32_t via_index = 0; via_index < via_rule->getViaCount();
       via_index++) {
    vias.insert(gui->makeSelected(via_rule->getVia(via_index)));
  }
  props.emplace_back("Tech Vias", vias);

  SelectionSet layer_rules;
  for (uint32_t rule_index = 0; rule_index < via_rule->getViaLayerRuleCount();
       rule_index++) {
    layer_rules.insert(
        gui->makeSelected(via_rule->getViaLayerRule(rule_index)));
  }
  props.emplace_back("Tech Via-Layer Rules", layer_rules);

  return props;
}

void DbTechViaRuleDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* tech = db_->getTech();

  for (auto via_rule : tech->getViaRules()) {
    func({via_rule, this});
  }
}

//////////////////////////////////////////////////

DbTechViaLayerRuleDescriptor::DbTechViaLayerRuleDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTechViaLayerRule>(db)
{
}

std::string DbTechViaLayerRuleDescriptor::getName(const std::any& object) const
{
  auto via_layer_rule = std::any_cast<odb::dbTechViaLayerRule*>(object);
  std::string rule_name = via_layer_rule->getLayer()->getName() + "_rule";
  return rule_name;
}

std::string DbTechViaLayerRuleDescriptor::getTypeName() const
{
  return "Tech Via-Layer Rule";
}

bool DbTechViaLayerRuleDescriptor::getBBox(const std::any& object,
                                           odb::Rect& bbox) const
{
  return false;
}

void DbTechViaLayerRuleDescriptor::highlight(const std::any& object,
                                             Painter& painter) const
{
}

Descriptor::Properties DbTechViaLayerRuleDescriptor::getDBProperties(
    odb::dbTechViaLayerRule* via_layer_rule) const
{
  auto gui = Gui::get();

  Properties props({{"Layer", gui->makeSelected(via_layer_rule->getLayer())},
                    {"Direction", via_layer_rule->getDirection().getString()}});

  if (via_layer_rule->hasWidth()) {
    int min_width = 0;
    int max_width = 0;

    via_layer_rule->getWidth(min_width, max_width);

    std::string width_range
        = fmt::format("{} to {}",
                      Property::convert_dbu(min_width, true),
                      Property::convert_dbu(max_width, true));

    props.emplace_back("Width", width_range);
  }

  if (via_layer_rule->hasEnclosure()) {
    int overhang_1 = 0;
    int overhang_2 = 0;

    via_layer_rule->getEnclosure(overhang_1, overhang_2);

    std::string enclosure_rule
        = fmt::format("{} x {}",
                      Property::convert_dbu(overhang_1, true),
                      Property::convert_dbu(overhang_2, true));

    props.emplace_back("Enclosure", enclosure_rule);
  }

  if (via_layer_rule->hasOverhang()) {
    props.emplace_back(
        "Overhang", Property::convert_dbu(via_layer_rule->getOverhang(), true));
  }

  if (via_layer_rule->hasMetalOverhang()) {
    props.emplace_back(
        "Metal Overhang",
        Property::convert_dbu(via_layer_rule->getMetalOverhang(), true));
  }

  if (via_layer_rule->hasRect()) {
    odb::Rect rect_rule;
    via_layer_rule->getRect(rect_rule);

    props.emplace_back("Rectangle", rect_rule);
  }

  if (via_layer_rule->hasSpacing()) {
    int x_spacing = 0;
    int y_spacing = 0;

    via_layer_rule->getSpacing(x_spacing, y_spacing);

    props.emplace_back("Spacing",
                       fmt::format("{} x {}",
                                   Property::convert_dbu(x_spacing, true),
                                   Property::convert_dbu(y_spacing, true)));
  }

  if (via_layer_rule->hasResistance()) {
    props.emplace_back("Resistance",
                       convertUnits(via_layer_rule->getResistance()) + "Ω/sq");
  }

  return props;
}

void DbTechViaLayerRuleDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto tech = db_->getTech();

  for (auto via_rule : tech->getViaRules()) {
    for (uint32_t via_layer_index = 0;
         via_layer_index < via_rule->getViaLayerRuleCount();
         via_layer_index++) {
      func({via_rule->getViaLayerRule(via_layer_index), this});
    }
  }
}

//////////////////////////////////////////////////

DbMetalWidthViaMapDescriptor::DbMetalWidthViaMapDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbMetalWidthViaMap>(db)
{
}

std::string DbMetalWidthViaMapDescriptor::getName(const std::any& object) const
{
  auto via_map = std::any_cast<odb::dbMetalWidthViaMap*>(object);
  std::string map_name = via_map->getViaName() + "_width_map";
  return map_name;
}

std::string DbMetalWidthViaMapDescriptor::getTypeName() const
{
  return "Metal Width Via Map Rule";
}

bool DbMetalWidthViaMapDescriptor::getBBox(const std::any& object,
                                           odb::Rect& bbox) const
{
  return false;
}

void DbMetalWidthViaMapDescriptor::highlight(const std::any& object,
                                             Painter& painter) const
{
}

Descriptor::Properties DbMetalWidthViaMapDescriptor::getDBProperties(
    odb::dbMetalWidthViaMap* via_map) const
{
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

void DbMetalWidthViaMapDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* tech = db_->getTech();

  for (auto* map : tech->getMetalWidthViaMap()) {
    func({map, this});
  }
}

//////////////////////////////////////////////////

DbGenerateViaDescriptor::DbGenerateViaDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTechViaGenerateRule>(db)
{
}

std::string DbGenerateViaDescriptor::getName(const std::any& object) const
{
  auto* via = std::any_cast<odb::dbTechViaGenerateRule*>(object);
  return via->getName();
}

std::string DbGenerateViaDescriptor::getTypeName() const
{
  return "Generate Via Rule";
}

bool DbGenerateViaDescriptor::getBBox(const std::any& object,
                                      odb::Rect& bbox) const
{
  return false;
}

void DbGenerateViaDescriptor::highlight(const std::any& object,
                                        Painter& painter) const
{
}

Descriptor::Properties DbGenerateViaDescriptor::getDBProperties(
    odb::dbTechViaGenerateRule* via) const
{
  auto* gui = Gui::get();

  Properties props;

  SelectionSet via_layer_rules;
  PropertyList layers;
  for (uint32_t l = 0; l < via->getViaLayerRuleCount(); l++) {
    auto* rule = via->getViaLayerRule(l);
    auto* layer = rule->getLayer();
    if (layer->getType() == odb::dbTechLayerType::CUT) {
      odb::Rect shape;
      rule->getRect(shape);
      layers.emplace_back(gui->makeSelected(layer), shape);
    } else {
      int enc0, enc1;
      rule->getEnclosure(enc0, enc1);
      std::string shape_text = fmt::format("Enclosure: {} x {}",
                                           Property::convert_dbu(enc0, true),
                                           Property::convert_dbu(enc1, true));
      layers.emplace_back(gui->makeSelected(layer), shape_text);
    }
    via_layer_rules.insert(gui->makeSelected(rule));
  }
  props.emplace_back("Tech Via-Layer Rules", via_layer_rules);
  props.emplace_back("Layers", layers);

  props.emplace_back("Is default", via->isDefault());

  return props;
}

void DbGenerateViaDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* tech = db_->getTech();

  for (auto* via : tech->getViaGenerateRules()) {
    func({via, this});
  }
}

//////////////////////////////////////////////////

DbNonDefaultRuleDescriptor::DbNonDefaultRuleDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTechNonDefaultRule>(db)
{
}

std::string DbNonDefaultRuleDescriptor::getName(const std::any& object) const
{
  auto* rule = std::any_cast<odb::dbTechNonDefaultRule*>(object);
  return rule->getName();
}

std::string DbNonDefaultRuleDescriptor::getTypeName() const
{
  return "Non-default Rule";
}

bool DbNonDefaultRuleDescriptor::getBBox(const std::any& object,
                                         odb::Rect& bbox) const
{
  return false;
}

void DbNonDefaultRuleDescriptor::highlight(const std::any& object,
                                           Painter& painter) const
{
}

Descriptor::Properties DbNonDefaultRuleDescriptor::getDBProperties(
    odb::dbTechNonDefaultRule* rule) const
{
  auto* gui = Gui::get();

  Properties props({{"Tech", gui->makeSelected(db_->getTech())}});

  std::vector<odb::dbTechLayerRule*> rule_layers;
  rule->getLayerRules(rule_layers);
  SelectionSet layers;
  for (auto* layer : rule_layers) {
    layers.insert(gui->makeSelected(layer));
  }
  props.emplace_back("Layer rules", layers);

  std::vector<odb::dbTechVia*> rule_vias;
  rule->getVias(rule_vias);
  SelectionSet vias;
  for (auto* via : rule_vias) {
    vias.insert(gui->makeSelected(via));
  }
  props.emplace_back("Tech vias", vias);

  std::vector<odb::dbTechSameNetRule*> rule_samenets;
  rule->getSameNetRules(rule_samenets);
  SelectionSet samenet_rules;
  for (auto* samenet : rule_samenets) {
    samenet_rules.insert(gui->makeSelected(samenet));
  }
  props.emplace_back("Same net rules", samenet_rules);

  props.emplace_back("Is block rule", rule->isBlockRule());

  return props;
}

void DbNonDefaultRuleDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* rule : db_->getTech()->getNonDefaultRules()) {
    func({rule, this});
  }

  for (auto* rule : block->getNonDefaultRules()) {
    func({rule, this});
  }
}

//////////////////////////////////////////////////

DbTechLayerRuleDescriptor::DbTechLayerRuleDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTechLayerRule>(db)
{
}

std::string DbTechLayerRuleDescriptor::getName(const std::any& object) const
{
  auto* rule = std::any_cast<odb::dbTechLayerRule*>(object);
  return rule->getLayer()->getName();
}

std::string DbTechLayerRuleDescriptor::getTypeName() const
{
  return "Tech layer rule";
}

bool DbTechLayerRuleDescriptor::getBBox(const std::any& object,
                                        odb::Rect& bbox) const
{
  return false;
}

void DbTechLayerRuleDescriptor::highlight(const std::any& object,
                                          Painter& painter) const
{
}

Descriptor::Properties DbTechLayerRuleDescriptor::getDBProperties(
    odb::dbTechLayerRule* rule) const
{
  auto* gui = Gui::get();

  Properties props;

  props.emplace_back("Layer", gui->makeSelected(rule->getLayer()));
  props.emplace_back("Non-default Rule",
                     gui->makeSelected(rule->getNonDefaultRule()));
  props.emplace_back("Is block rule", rule->isBlockRule());

  props.emplace_back("Width", Property::convert_dbu(rule->getWidth(), true));
  props.emplace_back("Spacing",
                     Property::convert_dbu(rule->getSpacing(), true));

  return props;
}

void DbTechLayerRuleDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
}

//////////////////////////////////////////////////

DbTechSameNetRuleDescriptor::DbTechSameNetRuleDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbTechSameNetRule>(db)
{
}

std::string DbTechSameNetRuleDescriptor::getName(const std::any& object) const
{
  auto* rule = std::any_cast<odb::dbTechSameNetRule*>(object);
  return rule->getLayer1()->getName() + " - " + rule->getLayer2()->getName();
}

std::string DbTechSameNetRuleDescriptor::getTypeName() const
{
  return "Tech same net rule";
}

bool DbTechSameNetRuleDescriptor::getBBox(const std::any& object,
                                          odb::Rect& bbox) const
{
  return false;
}

void DbTechSameNetRuleDescriptor::highlight(const std::any& object,
                                            Painter& painter) const
{
}

Descriptor::Properties DbTechSameNetRuleDescriptor::getDBProperties(
    odb::dbTechSameNetRule* rule) const
{
  auto* gui = Gui::get();

  Properties props({{"Tech", gui->makeSelected(db_->getTech())}});

  props.emplace_back("Layer 1", gui->makeSelected(rule->getLayer1()));
  props.emplace_back("Layer 2", gui->makeSelected(rule->getLayer2()));

  props.emplace_back("Spacing",
                     Property::convert_dbu(rule->getSpacing(), true));
  props.emplace_back("Allow via stacking", rule->getAllowStackedVias());

  return props;
}

void DbTechSameNetRuleDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
}

//////////////////////////////////////////////////

DbSiteDescriptor::DbSiteDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbSite>(db)
{
}

std::string DbSiteDescriptor::getName(const std::any& object) const
{
  return getObject(object)->getName();
}

std::string DbSiteDescriptor::getTypeName() const
{
  return "Site";
}

bool DbSiteDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  if (isSpecificSite(object)) {
    bbox = getRect(object);
    return true;
  }

  return false;
}

void DbSiteDescriptor::highlight(const std::any& object, Painter& painter) const
{
  if (isSpecificSite(object)) {
    painter.drawRect(getRect(object));
  }
}

Descriptor::Properties DbSiteDescriptor::getProperties(
    const std::any& object) const
{
  Properties props = BaseDbDescriptor::getProperties(object);

  if (auto site = std::any_cast<SpecificSite>(&object)) {
    props.emplace_back("Index", site->index_in_row);
  }

  return props;
}

Descriptor::Properties DbSiteDescriptor::getDBProperties(
    odb::dbSite* site) const
{
  Properties props;

  props.emplace_back("Width", Property::convert_dbu(site->getWidth(), true));
  props.emplace_back("Height", Property::convert_dbu(site->getHeight(), true));

  props.emplace_back("Site class", site->getClass().getString());

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
  props.emplace_back("Symmetry", symmetry);

  auto* gui = Gui::get();
  SelectionSet masters;
  for (auto* lib : db_->getLibs()) {
    for (auto* master : lib->getMasters()) {
      if (master->getSite() == site) {
        masters.insert(gui->makeSelected(master));
      }
    }
  }
  if (!masters.empty()) {
    props.emplace_back("Masters", masters);
  }

  return props;
}

Selected DbSiteDescriptor::makeSelected(const std::any& object) const
{
  Selected site_selected = BaseDbDescriptor::makeSelected(object);
  if (site_selected) {
    return site_selected;
  }

  if (auto site = std::any_cast<SpecificSite>(&object)) {
    return Selected(*site, this);
  }
  return Selected();
}

bool DbSiteDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  if (!isSpecificSite(l) && !isSpecificSite(r)) {
    return BaseDbDescriptor::lessThan(l, r);
  }

  const odb::Rect l_rect = getRect(l);
  const odb::Rect r_rect = getRect(r);
  return l_rect < r_rect;
}

void DbSiteDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  for (auto* lib : db_->getLibs()) {
    for (auto* site : lib->getSites()) {
      func({site, this});
    }
  }
}

odb::dbSite* DbSiteDescriptor::getObject(const std::any& object) const
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

DbRowDescriptor::DbRowDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbRow>(db)
{
}

std::string DbRowDescriptor::getName(const std::any& object) const
{
  auto* row = std::any_cast<odb::dbRow*>(object);
  return row->getName();
}

std::string DbRowDescriptor::getTypeName() const
{
  return "Row";
}

bool DbRowDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto* row = std::any_cast<odb::dbRow*>(object);
  bbox = row->getBBox();
  return true;
}

void DbRowDescriptor::highlight(const std::any& object, Painter& painter) const
{
  auto* row = std::any_cast<odb::dbRow*>(object);
  painter.drawRect(row->getBBox());
}

Descriptor::Properties DbRowDescriptor::getDBProperties(odb::dbRow* row) const
{
  auto* gui = Gui::get();

  Properties props({{"Block", gui->makeSelected(row->getBlock())},
                    {"Site", gui->makeSelected(row->getSite())}});
  odb::Point origin_pt = row->getOrigin();
  PropertyList origin;
  origin.emplace_back("X", Property::convert_dbu(origin_pt.x(), true));
  origin.emplace_back("Y", Property::convert_dbu(origin_pt.y(), true));

  props.emplace_back("Origin", origin);

  props.emplace_back("Orientation", row->getOrient().getString());
  props.emplace_back("Direction", row->getDirection().getString());

  props.emplace_back("Site count", row->getSiteCount());
  props.emplace_back("Site spacing",
                     Property::convert_dbu(row->getSpacing(), true));

  return props;
}

void DbRowDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* block = db_->getChip()->getBlock();

  for (auto* row : block->getRows()) {
    func({row, this});
  }
}

//////////////////////////////////////////////////

DbMarkerCategoryDescriptor::DbMarkerCategoryDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbMarkerCategory>(db)
{
}

std::string DbMarkerCategoryDescriptor::getName(const std::any& object) const
{
  auto* category = std::any_cast<odb::dbMarkerCategory*>(object);
  return category->getName();
}

std::string DbMarkerCategoryDescriptor::getTypeName() const
{
  return "Marker Category";
}

bool DbMarkerCategoryDescriptor::getBBox(const std::any& object,
                                         odb::Rect& bbox) const
{
  auto* category = std::any_cast<odb::dbMarkerCategory*>(object);
  bbox.mergeInit();
  bool has_bbox = false;
  for (odb::dbMarker* marker : category->getAllMarkers()) {
    bbox.merge(marker->getBBox());
    has_bbox = true;
  }
  return has_bbox;
}

void DbMarkerCategoryDescriptor::highlight(const std::any& object,
                                           Painter& painter) const
{
  auto* category = std::any_cast<odb::dbMarkerCategory*>(object);

  const Descriptor* desc = Gui::get()->getDescriptor<odb::dbMarker*>();
  for (odb::dbMarker* marker : category->getAllMarkers()) {
    desc->highlight(marker, painter);
  }
}

Descriptor::Properties DbMarkerCategoryDescriptor::getDBProperties(
    odb::dbMarkerCategory* category) const
{
  auto* gui = Gui::get();

  Properties props;

  props.emplace_back("Description", category->getDescription());
  props.emplace_back("Source", category->getSource());
  props.emplace_back("Max markers", category->getMaxMarkers());

  odb::dbMarkerCategory* top = category->getTopCategory();
  if (category != top) {
    props.emplace_back("Top category", gui->makeSelected(top));
  }

  odb::dbObject* parent = category->getParent();
  if (parent != top) {
    if (parent->getObjectType() == odb::dbObjectType::dbChipObj) {
      // TODO: fix this
      props.emplace_back(
          "Parent",
          gui->makeSelected(static_cast<odb::dbChip*>(parent)->getBlock()));
    } else {
      props.emplace_back(
          "Parent",
          gui->makeSelected(static_cast<odb::dbMarkerCategory*>(parent)));
    }
  }

  SelectionSet subcategories;
  for (odb::dbMarkerCategory* subcat : category->getMarkerCategories()) {
    subcategories.insert(gui->makeSelected(subcat));
  }
  if (!subcategories.empty()) {
    props.emplace_back("Categories", subcategories);
  }

  SelectionSet markers;
  for (odb::dbMarker* marker : category->getMarkers()) {
    markers.insert(gui->makeSelected(marker));
  }
  if (!markers.empty()) {
    props.emplace_back("Markers", markers);
  }

  return props;
}

void DbMarkerCategoryDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* block = db_->getChip()->getBlock();

  for (auto* category : block->getMarkerCategories()) {
    func({category, this});
  }
}

//////////////////////////////////////////////////

DbMarkerDescriptor::DbMarkerDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbMarker>(db)
{
}

std::string DbMarkerDescriptor::getName(const std::any& object) const
{
  auto* marker = std::any_cast<odb::dbMarker*>(object);
  return marker->getName();
}

std::string DbMarkerDescriptor::getTypeName() const
{
  return "Marker";
}

bool DbMarkerDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto* marker = std::any_cast<odb::dbMarker*>(object);
  bbox = marker->getBBox();
  return true;
}

void DbMarkerDescriptor::highlight(const std::any& object,
                                   Painter& painter) const
{
  auto* marker = std::any_cast<odb::dbMarker*>(object);
  paintMarker(marker, painter);
}

Descriptor::Properties DbMarkerDescriptor::getDBProperties(
    odb::dbMarker* marker) const
{
  auto* gui = Gui::get();

  Properties props;

  props.emplace_back("Category", gui->makeSelected(marker->getCategory()));

  props.emplace_back("Visited", marker->isVisited());
  props.emplace_back("Waived", marker->isWaived());

  auto layer = marker->getTechLayer();
  if (layer != nullptr) {
    props.emplace_back("Layer", gui->makeSelected(layer));
  }

  SelectionSet sources;
  for (odb::dbObject* src : marker->getSources()) {
    Selected select;
    switch (src->getObjectType()) {
      case odb::dbNetObj:
        select = gui->makeSelected(static_cast<odb::dbNet*>(src));
        break;
      case odb::dbInstObj:
        select = gui->makeSelected(static_cast<odb::dbInst*>(src));
        break;
      case odb::dbITermObj:
        select = gui->makeSelected(static_cast<odb::dbITerm*>(src));
        break;
      case odb::dbBTermObj:
        select = gui->makeSelected(static_cast<odb::dbBTerm*>(src));
        break;
      case odb::dbObstructionObj:
        select = gui->makeSelected(static_cast<odb::dbObstruction*>(src));
        break;
      default:
        break;
    }
    if (select) {
      sources.insert(select);
    }
  }
  if (!sources.empty()) {
    props.emplace_back("Sources", sources);
  }

  const auto& comment = marker->getComment();
  if (!comment.empty()) {
    props.emplace_back("Comment", comment);
  }

  int line_number = marker->getLineNumber();
  if (line_number > 0) {
    props.emplace_back("Line number:", line_number);
  }

  return props;
}

void DbMarkerDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* block = db_->getChip()->getBlock();

  for (auto* category : block->getMarkerCategories()) {
    for (odb::dbMarker* marker : category->getAllMarkers()) {
      func({marker, this});
    }
  }
}

void DbMarkerDescriptor::paintMarker(odb::dbMarker* marker,
                                     Painter& painter) const
{
  const int min_box = 20.0 / painter.getPixelsPerDBU();

  const odb::Rect& box = marker->getBBox();
  if (box.maxDXDY() < min_box) {
    // box is too small to be useful, so draw X instead
    odb::Point center(box.xMin() + box.dx() / 2, box.yMin() + box.dy() / 2);
    painter.drawX(center.x(), center.y(), min_box);
  } else {
    for (const auto& shape : marker->getShapes()) {
      if (std::holds_alternative<odb::Point>(shape)) {
        const odb::Point pt = std::get<odb::Point>(shape);
        painter.drawX(pt.x(), pt.y(), min_box);
      } else if (std::holds_alternative<odb::Line>(shape)) {
        const odb::Line line = std::get<odb::Line>(shape);
        painter.drawLine(line.pt0(), line.pt1());
      } else if (std::holds_alternative<odb::Rect>(shape)) {
        painter.drawRect(std::get<odb::Rect>(shape));
      } else if (std::holds_alternative<odb::Polygon>(shape)) {
        painter.drawPolygon(std::get<odb::Polygon>(shape));
      } else if (std::holds_alternative<odb::Cuboid>(shape)) {
        const odb::Cuboid cuboid = std::get<odb::Cuboid>(shape);
        const odb::Rect rect = cuboid.getEnclosingRect();
        painter.drawRect(rect);
      }
    }
  }
}

//////////////////////////////////////////////////

DbScanInstDescriptor::DbScanInstDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbScanInst>(db)
{
}

std::string DbScanInstDescriptor::getName(const std::any& object) const
{
  auto* scan_inst = std::any_cast<odb::dbScanInst*>(object);
  auto* inst = scan_inst->getInst();
  return "(Scan) " + inst->getName();
}

std::string DbScanInstDescriptor::getTypeName() const
{
  return "Scan Inst";
}

bool DbScanInstDescriptor::getBBox(const std::any& object,
                                   odb::Rect& bbox) const
{
  auto* scan_inst = std::any_cast<odb::dbScanInst*>(object);
  auto* inst = scan_inst->getInst();
  bbox = inst->getBBox()->getBox();
  return true;
}

void DbScanInstDescriptor::highlight(const std::any& object,
                                     Painter& painter) const
{
  auto* scan_inst = std::any_cast<odb::dbScanInst*>(object);
  auto* inst_descriptor = Gui::get()->getDescriptor<odb::dbInst*>();
  inst_descriptor->highlight(scan_inst->getInst(), painter);
}

void DbScanInstDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* block = db_->getChip()->getBlock();
  auto* db_dft = block->getDft();

  for (auto* scan_chain : db_dft->getScanChains()) {
    for (auto* scan_partition : scan_chain->getScanPartitions()) {
      for (auto* scan_list : scan_partition->getScanLists()) {
        for (auto* scan_inst : scan_list->getScanInsts()) {
          func({scan_inst, this});
        }
      }
    }
  }
}

Descriptor::Properties DbScanInstDescriptor::getDBProperties(
    odb::dbScanInst* scan_inst) const
{
  Properties props;

  props.emplace_back("Scan Clock", scan_inst->getScanClock());
  props.emplace_back("Clock Edge", scan_inst->getClockEdgeString());
  props.emplace_back(getScanPinProperty("Enable", scan_inst->getScanEnable()));

  PropertyList access_pins_props;
  odb::dbScanInst::AccessPins access_pins = scan_inst->getAccessPins();
  Property scan_in_prop = getScanPinProperty("In", access_pins.scan_in);
  access_pins_props.emplace_back(scan_in_prop.name, scan_in_prop.value);
  Property scan_out_prop = getScanPinProperty("Out", access_pins.scan_out);
  access_pins_props.emplace_back(scan_out_prop.name, scan_out_prop.value);
  props.emplace_back("Access Pins", access_pins_props);

  auto gui = Gui::get();
  props.emplace_back("Inst", gui->makeSelected(scan_inst->getInst()));

  return props;
}

/* static */
Descriptor::Property DbScanInstDescriptor::getScanPinProperty(
    const std::string& name,
    const std::variant<odb::dbBTerm*, odb::dbITerm*>& pin)
{
  Descriptor::Property property;
  property.name = name;

  auto gui = Gui::get();
  std::visit([&](auto* term) { property.value = gui->makeSelected(term); },
             pin);

  return property;
}

//////////////////////////////////////////////////

DbScanListDescriptor::DbScanListDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbScanList>(db)
{
}

std::string DbScanListDescriptor::getName(const std::any& object) const
{
  return "Scan List";
}

std::string DbScanListDescriptor::getTypeName() const
{
  return "Scan List";
}

bool DbScanListDescriptor::getBBox(const std::any& object,
                                   odb::Rect& bbox) const
{
  auto scan_list = getObject(object);
  bbox.mergeInit();

  for (odb::dbScanInst* scan_inst : scan_list->getScanInsts()) {
    odb::dbInst* inst = scan_inst->getInst();
    if (inst->getPlacementStatus().isPlaced()) {
      bbox.merge(inst->getBBox()->getBox());
    }
  }

  return !bbox.isInverted();
}

void DbScanListDescriptor::highlight(const std::any& object,
                                     Painter& painter) const
{
  auto scan_list = getObject(object);

  for (odb::dbScanInst* scan_inst : scan_list->getScanInsts()) {
    odb::dbInst* inst = scan_inst->getInst();
    if (inst->getPlacementStatus().isPlaced()) {
      auto* inst_descriptor = Gui::get()->getDescriptor<odb::dbInst*>();
      inst_descriptor->highlight(scan_inst->getInst(), painter);
    }
  }
}

void DbScanListDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* block = db_->getChip()->getBlock();
  auto* db_dft = block->getDft();

  for (auto* scan_chain : db_dft->getScanChains()) {
    for (auto* scan_partition : scan_chain->getScanPartitions()) {
      for (auto* scan_list : scan_partition->getScanLists()) {
        func({scan_list, this});
      }
    }
  }
}

Descriptor::Properties DbScanListDescriptor::getDBProperties(
    odb::dbScanList* scan_list) const
{
  Properties props;

  auto gui = Gui::get();

  SelectionSet scan_insts;
  for (odb::dbScanInst* scan_inst : scan_list->getScanInsts()) {
    scan_insts.insert(gui->makeSelected(scan_inst));
  }
  props.emplace_back("Scan Insts", scan_insts);

  return props;
}

//////////////////////////////////////////////////

DbScanPartitionDescriptor::DbScanPartitionDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbScanPartition>(db)
{
}

std::string DbScanPartitionDescriptor::getName(const std::any& object) const
{
  auto scan_partition = getObject(object);
  return scan_partition->getName();
}

std::string DbScanPartitionDescriptor::getTypeName() const
{
  return "Scan Partition";
}

bool DbScanPartitionDescriptor::getBBox(const std::any& object,
                                        odb::Rect& bbox) const
{
  auto scan_partition = getObject(object);
  auto* scan_list_descriptor = Gui::get()->getDescriptor<odb::dbScanList*>();
  bbox.mergeInit();

  for (auto* scan_list : scan_partition->getScanLists()) {
    odb::Rect scan_list_bbox;
    if (scan_list_descriptor->getBBox(scan_list, scan_list_bbox)) {
      bbox.merge(scan_list_bbox);
    }
  }

  return !bbox.isInverted();
}

void DbScanPartitionDescriptor::highlight(const std::any& object,
                                          Painter& painter) const
{
  auto scan_partition = getObject(object);

  for (auto* scan_list : scan_partition->getScanLists()) {
    auto* scan_list_descriptor = Gui::get()->getDescriptor<odb::dbScanList*>();
    scan_list_descriptor->highlight(scan_list, painter);
  }
}

void DbScanPartitionDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* block = db_->getChip()->getBlock();
  auto* db_dft = block->getDft();

  for (auto* scan_chain : db_dft->getScanChains()) {
    for (auto* scan_partition : scan_chain->getScanPartitions()) {
      func({scan_partition, this});
    }
  }
}

Descriptor::Properties DbScanPartitionDescriptor::getDBProperties(
    odb::dbScanPartition* scan_partition) const
{
  Properties props;

  auto gui = Gui::get();

  SelectionSet scan_lists;
  for (odb::dbScanList* scan_list : scan_partition->getScanLists()) {
    scan_lists.insert(gui->makeSelected(scan_list));
  }
  props.emplace_back("Scan Lists", scan_lists);

  return props;
}

//////////////////////////////////////////////////

DbScanChainDescriptor::DbScanChainDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbScanChain>(db)
{
}

std::string DbScanChainDescriptor::getName(const std::any& object) const
{
  auto scan_chain = getObject(object);
  return scan_chain->getName();
}

std::string DbScanChainDescriptor::getTypeName() const
{
  return "Scan Chain";
}

bool DbScanChainDescriptor::getBBox(const std::any& object,
                                    odb::Rect& bbox) const
{
  auto scan_chain = getObject(object);
  auto* scan_partition_descriptor
      = Gui::get()->getDescriptor<odb::dbScanPartition*>();
  bbox.mergeInit();

  for (auto* scan_partition : scan_chain->getScanPartitions()) {
    odb::Rect scan_partition_bbox;
    if (scan_partition_descriptor->getBBox(scan_partition,
                                           scan_partition_bbox)) {
      bbox.merge(scan_partition_bbox);
    }
  }

  return !bbox.isInverted();
}

void DbScanChainDescriptor::highlight(const std::any& object,
                                      Painter& painter) const
{
  auto scan_chain = getObject(object);

  for (auto* scan_partition : scan_chain->getScanPartitions()) {
    auto* scan_partition_descriptor
        = Gui::get()->getDescriptor<odb::dbScanPartition*>();
    scan_partition_descriptor->highlight(scan_partition, painter);
  }
}

void DbScanChainDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* block = db_->getChip()->getBlock();
  auto* db_dft = block->getDft();

  for (auto* scan_chain : db_dft->getScanChains()) {
    func({scan_chain, this});
  }
}

Descriptor::Properties DbScanChainDescriptor::getDBProperties(
    odb::dbScanChain* scan_chain) const
{
  Properties props;

  props.emplace_back(
      DbScanInstDescriptor::getScanPinProperty("In", scan_chain->getScanIn()));
  props.emplace_back(DbScanInstDescriptor::getScanPinProperty(
      "Out", scan_chain->getScanOut()));
  props.emplace_back(DbScanInstDescriptor::getScanPinProperty(
      "Enable", scan_chain->getScanEnable()));

  auto gui = Gui::get();

  SelectionSet scan_partitions;
  for (auto* scan_partition : scan_chain->getScanPartitions()) {
    scan_partitions.insert(gui->makeSelected(scan_partition));
  }
  props.emplace_back("Scan Partitions", scan_partitions);

  return props;
}

//////////////////////////////////////////////////

DbBoxDescriptor::DbBoxDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbBox>(db)
{
}

std::string DbBoxDescriptor::getName(const std::any& object) const
{
  odb::Rect box;
  getBBox(object, box);

  return fmt::format("Box of {}: {}",
                     getObject(object)->getOwnerType().getString(),
                     Property::toString(box));
}

std::string DbBoxDescriptor::getTypeName() const
{
  return "Box";
}

bool DbBoxDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  bbox = getObject(object)->getBox();
  const auto xform = getTransform(object);
  xform.apply(bbox);
  return true;
}

Selected DbBoxDescriptor::makeSelected(const std::any& object) const
{
  Selected box_selected = BaseDbDescriptor::makeSelected(object);
  if (box_selected) {
    return box_selected;
  }

  if (auto box = std::any_cast<BoxWithTransform>(&object)) {
    return Selected(*box, this);
  }
  return Selected();
}

void DbBoxDescriptor::highlight(const std::any& object, Painter& painter) const
{
  odb::Rect bbox = getObject(object)->getBox();
  const auto xform = getTransform(object);
  xform.apply(bbox);

  painter.drawRect(bbox);
}

void DbBoxDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
}

bool DbBoxDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_net = getObject(l);
  auto r_net = getObject(r);
  return BaseDbDescriptor::lessThan(l_net, r_net);
}

Descriptor::Properties DbBoxDescriptor::getDBProperties(odb::dbBox* box) const
{
  Properties props;
  populateProperties(box, props);
  return props;
}

void DbBoxDescriptor::populateProperties(odb::dbBox* box, Properties& props)
{
  auto* gui = Gui::get();

  switch (box->getOwnerType().getValue()) {
    case odb::dbBoxOwner::BLOCK:
      props.emplace_back("Owner",
                         gui->makeSelected((odb::dbBlock*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::INST:
      props.emplace_back("Owner",
                         gui->makeSelected((odb::dbInst*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::BTERM:
      props.emplace_back("Owner",
                         gui->makeSelected((odb::dbBTerm*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::BPIN:
      props.emplace_back("Owner",
                         gui->makeSelected((odb::dbBPin*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::VIA:
      props.emplace_back("Owner",
                         gui->makeSelected((odb::dbVia*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::OBSTRUCTION:
      props.emplace_back(
          "Owner", gui->makeSelected((odb::dbObstruction*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::BLOCKAGE:
      props.emplace_back(
          "Owner", gui->makeSelected((odb::dbBlockage*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::SWIRE:
      props.emplace_back("Owner",
                         gui->makeSelected((odb::dbSWire*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::MASTER:
      props.emplace_back(
          "Owner", gui->makeSelected((odb::dbMaster*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::MPIN:
      props.emplace_back("Owner",
                         gui->makeSelected((odb::dbMPin*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::PBOX:
      props.emplace_back("Owner", "PBOX");
      break;
    case odb::dbBoxOwner::TECH_VIA:
      props.emplace_back(
          "Owner", gui->makeSelected((odb::dbTechVia*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::REGION:
      props.emplace_back(
          "Owner", gui->makeSelected((odb::dbRegion*) box->getBoxOwner()));
      break;
    case odb::dbBoxOwner::UNKNOWN:
      props.emplace_back("Owner", "Unknown");
      break;
  }

  if (auto* layer = box->getTechLayer()) {
    props.emplace_back("Layer", gui->makeSelected(layer));
    if (box->getLayerMask() > 0) {
      props.emplace_back("Mask", box->getLayerMask());
    }
    if (box->getDesignRuleWidth() >= 0) {
      props.emplace_back(
          "Design rule width",
          Property::convert_dbu(box->getDesignRuleWidth(), true));
    }
  } else if (auto* via = box->getTechVia()) {
    props.emplace_back("Tech via", gui->makeSelected(via));
  } else if (auto* via = box->getBlockVia()) {
    props.emplace_back("Block via", gui->makeSelected(via));
  }
}

odb::dbBox* DbBoxDescriptor::getObject(const std::any& object) const
{
  odb::dbBox* const* box = std::any_cast<odb::dbBox*>(&object);
  if (box != nullptr) {
    return *box;
  }
  return std::any_cast<BoxWithTransform>(object).box;
}

odb::dbTransform DbBoxDescriptor::getTransform(const std::any& object) const
{
  const BoxWithTransform* box_xform = std::any_cast<BoxWithTransform>(&object);
  if (box_xform != nullptr) {
    return box_xform->xform;
  }
  return odb::dbTransform();
}

//////////////////////////////////////////////////

DbSBoxDescriptor::DbSBoxDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbSBox>(db)
{
}

std::string DbSBoxDescriptor::getName(const std::any& object) const
{
  odb::Rect box;
  getBBox(object, box);

  return fmt::format("SBox of {}: {}",
                     getObject(object)->getOwnerType().getString(),
                     Property::toString(box));
}

std::string DbSBoxDescriptor::getTypeName() const
{
  return "SBox";
}

bool DbSBoxDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  bbox = getObject(object)->getBox();
  return true;
}

void DbSBoxDescriptor::highlight(const std::any& object, Painter& painter) const
{
  auto* box = getObject(object);

  if (box->getDirection() == odb::dbSBox::OCTILINEAR) {
    painter.drawOctagon(box->getOct());
  } else {
    odb::Rect rect = box->getBox();
    painter.drawRect(rect);
  }
}

void DbSBoxDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
}

Descriptor::Properties DbSBoxDescriptor::getDBProperties(odb::dbSBox* box) const
{
  Properties props;

  auto* gui = Gui::get();

  DbBoxDescriptor::populateProperties(box, props);

  props.emplace_back("SWire", gui->makeSelected(box->getSWire()));
  props.emplace_back("Shape type", box->getWireShapeType().getString());
  std::string direction;
  switch (box->getDirection()) {
    case odb::dbSBox::UNDEFINED:
      direction = "Undefined";
      break;
    case odb::dbSBox::HORIZONTAL:
      direction = "Horizontal";
      break;
    case odb::dbSBox::VERTICAL:
      direction = "Vertical";
      break;
    case odb::dbSBox::OCTILINEAR:
      direction = "Octilinear";
      break;
  }
  props.emplace_back("Direction", direction);

  return props;
}

//////////////////////////////////////////////////

DbMasterEdgeTypeDescriptor::DbMasterEdgeTypeDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbMasterEdgeType>(db)
{
}

std::string DbMasterEdgeTypeDescriptor::getName(const std::any& object) const
{
  return getObject(object)->getEdgeType();
}

std::string DbMasterEdgeTypeDescriptor::getTypeName() const
{
  return "MasterEdgeType";
}

bool DbMasterEdgeTypeDescriptor::getBBox(const std::any& object,
                                         odb::Rect& bbox) const
{
  return false;
}

void DbMasterEdgeTypeDescriptor::highlightEdge(
    odb::dbMaster* master,
    odb::dbMasterEdgeType* edge,
    Painter& painter,
    const std::optional<int>& pen_width)
{
  if (pen_width) {
    painter.saveState();
    painter.setPenWidth(pen_width.value());
  }

  std::set<odb::dbInst*> insts;
  DbMasterDescriptor::getInstances(master, insts);
  for (auto inst : insts) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }

    const odb::Rect rect = inst->getBBox()->getBox();

    switch (edge->getEdgeDir()) {
      case odb::dbMasterEdgeType::TOP:
        painter.drawLine(rect.ur(), rect.ul());
        break;
      case odb::dbMasterEdgeType::RIGHT:
        painter.drawLine(rect.lr(), rect.ur());
        break;
      case odb::dbMasterEdgeType::LEFT:
        painter.drawLine(rect.ll(), rect.ul());
        break;
      case odb::dbMasterEdgeType::BOTTOM:
        painter.drawLine(rect.lr(), rect.ll());
        break;
    }
  }

  if (pen_width) {
    painter.restoreState();
  }
}

void DbMasterEdgeTypeDescriptor::highlight(const std::any& object,
                                           Painter& painter) const
{
  auto* edge = getObject(object);
  for (auto* lib : db_->getLibs()) {
    for (auto* master : lib->getMasters()) {
      for (auto* master_edge : master->getEdgeTypes()) {
        if (master_edge == edge) {
          DbMasterEdgeTypeDescriptor::highlightEdge(master, edge, painter);
        }
      }
    }
  }
}

void DbMasterEdgeTypeDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  for (auto* lib : db_->getLibs()) {
    for (auto* master : lib->getMasters()) {
      for (auto* edge : master->getEdgeTypes()) {
        func({edge, this});
      }
    }
  }
}

Descriptor::Properties DbMasterEdgeTypeDescriptor::getDBProperties(
    odb::dbMasterEdgeType* edge) const
{
  Properties props;

  auto* gui = Gui::get();

  SelectionSet rules;
  for (auto* tech : db_->getTechs()) {
    for (auto* rule : tech->getCellEdgeSpacingTable()) {
      if (rule->getFirstEdgeType() == edge->getEdgeType()) {
        rules.insert(gui->makeSelected(rule));
      } else if (rule->getSecondEdgeType() == edge->getEdgeType()) {
        rules.insert(gui->makeSelected(rule));
      }
    }
  }
  if (!rules.empty()) {
    props.emplace_back("Rules", rules);
  }

  if (edge->getCellRow() != -1) {
    props.emplace_back("Cell row", edge->getCellRow());
  }
  if (edge->getHalfRow() != -1) {
    props.emplace_back("Half row", edge->getHalfRow());
  }

  PropertyList range;
  if (edge->getRangeBegin() != -1) {
    range.emplace_back("Begin", edge->getRangeBegin());
  }
  if (edge->getRangeEnd() != -1) {
    range.emplace_back("End", edge->getRangeEnd());
  }
  if (!range.empty()) {
    props.emplace_back("Range", range);
  }

  std::string edgedir;
  switch (edge->getEdgeDir()) {
    case odb::dbMasterEdgeType::TOP:
      edgedir = "top";
      break;
    case odb::dbMasterEdgeType::RIGHT:
      edgedir = "right";
      break;
    case odb::dbMasterEdgeType::LEFT:
      edgedir = "left";
      break;
    case odb::dbMasterEdgeType::BOTTOM:
      edgedir = "bottom";
      break;
  }
  props.emplace_back("Edge direction", edgedir);

  return props;
}

//////////////////////////////////////////////////

DbCellEdgeSpacingDescriptor::DbCellEdgeSpacingDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbCellEdgeSpacing>(db)
{
}

std::string DbCellEdgeSpacingDescriptor::getName(const std::any& object) const
{
  auto* obj = getObject(object);
  return obj->getFirstEdgeType() + " - " + obj->getSecondEdgeType();
}

std::string DbCellEdgeSpacingDescriptor::getTypeName() const
{
  return "CellEdgeSpacingRule";
}

bool DbCellEdgeSpacingDescriptor::getBBox(const std::any& object,
                                          odb::Rect& bbox) const
{
  return false;
}

void DbCellEdgeSpacingDescriptor::highlight(const std::any& object,
                                            Painter& painter) const
{
  auto rule = getObject(object);
  for (auto* lib : db_->getLibs()) {
    for (auto* master : lib->getMasters()) {
      for (auto* edge : master->getEdgeTypes()) {
        if (rule->getFirstEdgeType() == edge->getEdgeType()) {
          DbMasterEdgeTypeDescriptor::highlightEdge(master, edge, painter, 1);
        } else if (rule->getSecondEdgeType() == edge->getEdgeType()) {
          DbMasterEdgeTypeDescriptor::highlightEdge(master, edge, painter, 2);
        }
      }
    }
  }
}

void DbCellEdgeSpacingDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  for (auto* tech : db_->getTechs()) {
    for (auto* rule : tech->getCellEdgeSpacingTable()) {
      func({rule, this});
    }
  }
}

Descriptor::Properties DbCellEdgeSpacingDescriptor::getDBProperties(
    odb::dbCellEdgeSpacing* rule) const
{
  Properties props;

  props.emplace_back("First edge", rule->getFirstEdgeType());
  props.emplace_back("Second edge", rule->getSecondEdgeType());

  props.emplace_back("Spacing",
                     Property::convert_dbu(rule->getSpacing(), true));
  props.emplace_back("Except abutted", rule->isExceptAbutted());
  props.emplace_back("Except non filler in between",
                     rule->isExceptNonFillerInBetween());
  props.emplace_back("Optional", rule->isOptional());
  props.emplace_back("Soft", rule->isSoft());
  props.emplace_back("Exact", rule->isExact());

  return props;
}

//////////////////////////////////////////////////

DbWireDescriptor::DbWireDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbWire>(db)
{
}

std::string DbWireDescriptor::getName(const std::any& object) const
{
  auto* obj = getObject(object);
  return obj->getNet()->getName();
}

std::string DbWireDescriptor::getTypeName() const
{
  return "Net Wire";
}

bool DbWireDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto* obj = getObject(object);
  const auto box = obj->getBBox();
  if (box.has_value()) {
    bbox = *box;
    return true;
  }
  return false;
}

void DbWireDescriptor::highlight(const std::any& object, Painter& painter) const
{
  auto* wire = getObject(object);

  odb::dbWireShapeItr it;
  it.begin(wire);
  odb::dbShape shape;
  while (it.next(shape)) {
    painter.drawRect(shape.getBox());
  }
}

void DbWireDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* net : block->getNets()) {
    odb::dbWire* wire = net->getWire();
    if (wire != nullptr) {
      func({wire, this});
    }
  }
}

Descriptor::Properties DbWireDescriptor::getDBProperties(
    odb::dbWire* wire) const
{
  Properties props;
  auto* gui = Gui::get();

  props.emplace_back("Net", gui->makeSelected(wire->getNet()));
  props.emplace_back("Is global", wire->isGlobalWire());
  props.emplace_back("Count", wire->count());
  props.emplace_back("Entries", wire->length());
  props.emplace_back("Length", Property::convert_dbu(wire->getLength(), true));

  return props;
}

//////////////////////////////////////////////////

DbSWireDescriptor::DbSWireDescriptor(odb::dbDatabase* db)
    : BaseDbDescriptor<odb::dbSWire>(db)
{
}

std::string DbSWireDescriptor::getName(const std::any& object) const
{
  auto* obj = getObject(object);
  return obj->getNet()->getName();
}

std::string DbSWireDescriptor::getTypeName() const
{
  return "Net SWire";
}

bool DbSWireDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto* obj = getObject(object);
  if (obj->getWires().empty()) {
    return false;
  }
  bbox.mergeInit();
  for (auto* box : obj->getWires()) {
    bbox.merge(box->getBox());
  }
  return true;
}

void DbSWireDescriptor::highlight(const std::any& object,
                                  Painter& painter) const
{
  auto* wire = getObject(object);

  auto* sbox_descriptor = Gui::get()->getDescriptor<odb::dbSBox*>();

  for (auto* box : wire->getWires()) {
    sbox_descriptor->highlight(box, painter);
  }
}

void DbSWireDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* net : block->getNets()) {
    for (auto* swire : net->getSWires()) {
      func({swire, this});
    }
  }
}

Descriptor::Properties DbSWireDescriptor::getDBProperties(
    odb::dbSWire* wire) const
{
  Properties props;
  auto* gui = Gui::get();

  props.emplace_back("Net", gui->makeSelected(wire->getNet()));
  props.emplace_back("Type", wire->getWireType().getString());
  if (wire->getShield() != nullptr) {
    props.emplace_back("Sheild net", gui->makeSelected(wire->getShield()));
  }
  if (wire->getWires().size() > kMaxBoxes) {
    props.emplace_back("Boxes", wire->getWires().size());
  } else {
    SelectionSet boxes;
    for (odb::dbSBox* box : wire->getWires()) {
      boxes.insert(gui->makeSelected(box));
    }
    props.emplace_back("Boxes", boxes);
  }

  return props;
}

}  // namespace gui
