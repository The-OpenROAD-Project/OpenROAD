// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "staDescriptors.h"

#include <QInputDialog>
#include <QStringList>
#include <algorithm>
#include <any>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "boost/algorithm/string.hpp"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/ClkNetwork.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace gui {

static void capitalize(std::string& str)
{
  str[0] = std::toupper(str[0]);
}

static void add_if_true(Descriptor::Properties& props,
                        const char* name,
                        const bool value)
{
  if (value) {
    props.push_back({name, value});
  }
}

// T may be MinMax or RiseFall
template <typename T>
using LimitFunc = std::function<
    void(sta::LibertyPort* port, const T* t, float& limit, bool& exists)>;

template <typename T>
static void add_limit(Descriptor::Properties& props,
                      sta::LibertyPort* port,
                      const char* label,
                      const LimitFunc<T>& func,
                      const char* suffix)
{
  for (const auto* elem : T::range()) {
    bool exists;
    float limit;
    func(port, elem, limit, exists);
    if (exists) {
      std::string elem_str = elem->to_string();
      capitalize(elem_str);
      props.push_back({fmt::format("{} {}", elem_str, label),
                       Descriptor::convertUnits(limit) + suffix});
    }
  }
};

//////////////////////////////////////////////////

LibertyLibraryDescriptor::LibertyLibraryDescriptor(sta::dbSta* sta) : sta_(sta)
{
}

std::string LibertyLibraryDescriptor::getName(const std::any& object) const
{
  return std::any_cast<sta::LibertyLibrary*>(object)->name();
}

std::string LibertyLibraryDescriptor::getTypeName() const
{
  return "Liberty library";
}

bool LibertyLibraryDescriptor::getBBox(const std::any& object,
                                       odb::Rect& bbox) const
{
  return false;
}

void LibertyLibraryDescriptor::highlight(const std::any& object,
                                         Painter& painter) const
{
  auto library = std::any_cast<sta::LibertyLibrary*>(object);
  auto network = sta_->getDbNetwork();

  sta::LibertyCellIterator cell_iter(library);
  std::set<odb::dbMaster*> masters;
  while (cell_iter.hasNext()) {
    auto* master = network->staToDb(cell_iter.next());
    if (master != nullptr) {
      masters.insert(master);
    }
  }

  auto* master_desc = Gui::get()->getDescriptor<odb::dbMaster*>();
  for (auto* master : masters) {
    master_desc->highlight(master, painter);
  }
}

Descriptor::Properties LibertyLibraryDescriptor::getProperties(
    const std::any& object) const
{
  auto library = std::any_cast<sta::LibertyLibrary*>(object);

  auto gui = Gui::get();
  Properties props;

  const char* delay_model = "";
  switch (library->delayModelType()) {
    case sta::DelayModelType::cmos2:
      delay_model = "cmos2";
      break;
    case sta::DelayModelType::cmos_linear:
      delay_model = "cmos linear";
      break;
    case sta::DelayModelType::cmos_pwl:
      delay_model = "cmos pwl";
      break;
    case sta::DelayModelType::table:
      delay_model = "table";
      break;
    case sta::DelayModelType::polynomial:
      delay_model = "polynomial";
      break;
    case sta::DelayModelType::dcm:
      delay_model = "dcm";
      break;
  }
  props.push_back({"Delay model", delay_model});

  auto format_unit = [](float value, const sta::Unit* unit) -> std::string {
    return fmt::format(
        "{} {}", unit->asString(value), unit->scaleAbbrevSuffix());
  };

  props.push_back(
      {"Nominal voltage",
       format_unit(library->nominalVoltage(), sta_->units()->voltageUnit())});
  props.push_back({"Nominal temperature",
                   fmt::format("{} deg C", library->nominalTemperature())});
  props.push_back({"Nominal process", library->nominalProcess()});

  bool exists;
  float cap, fanout, load, slew;
  library->defaultMaxSlew(slew, exists);
  if (exists) {
    props.push_back(
        {"Default max slew", format_unit(slew, sta_->units()->timeUnit())});
  }
  library->defaultMaxCapacitance(cap, exists);
  if (exists) {
    props.push_back({"Default max capacitance",
                     format_unit(cap, sta_->units()->capacitanceUnit())});
  }
  library->defaultMaxFanout(fanout, exists);
  if (exists) {
    props.push_back({"Default max fanout", fanout});
  }
  library->defaultFanoutLoad(load, exists);
  if (exists) {
    props.push_back({"Default fanout load", load});
  }

  SelectionSet scenes;
  for (auto* scene : sta_->scenes()) {
    for (const sta::MinMax* min_max :
         {sta::MinMax::min(), sta::MinMax::max()}) {
      const auto& libs = scene->libertyLibraries(min_max);
      if (std::ranges::find(libs, library) != libs.end()) {
        scenes.insert(gui->makeSelected(scene));
      }
    }
  }
  props.push_back({"Scenes", scenes});

  SelectionSet cells;
  sta::LibertyCellIterator cell_iter(library);
  while (cell_iter.hasNext()) {
    cells.insert(gui->makeSelected(cell_iter.next()));
  }
  props.push_back({"Cells", cells});

  return props;
}

Selected LibertyLibraryDescriptor::makeSelected(const std::any& object) const
{
  if (auto library = std::any_cast<sta::LibertyLibrary*>(&object)) {
    return Selected(*library, this);
  }
  return Selected();
}

bool LibertyLibraryDescriptor::lessThan(const std::any& l,
                                        const std::any& r) const
{
  auto l_library = std::any_cast<sta::LibertyLibrary*>(l);
  auto r_library = std::any_cast<sta::LibertyLibrary*>(r);
  return l_library->id() < r_library->id();
}

void LibertyLibraryDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
      network->libertyLibraryIterator()};

  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* library = lib_iter->next();
    func({library, this});
  }
}

//////////////////////////////////////////////////

LibertyCellDescriptor::LibertyCellDescriptor(sta::dbSta* sta) : sta_(sta)
{
}

std::string LibertyCellDescriptor::getName(const std::any& object) const
{
  return std::any_cast<sta::LibertyCell*>(object)->name();
}

std::string LibertyCellDescriptor::getTypeName() const
{
  return "Liberty cell";
}

bool LibertyCellDescriptor::getBBox(const std::any& object,
                                    odb::Rect& bbox) const
{
  return false;
}

void LibertyCellDescriptor::highlight(const std::any& object,
                                      Painter& painter) const
{
  auto cell = std::any_cast<sta::LibertyCell*>(object);
  auto* network = sta_->getDbNetwork();
  odb::dbMaster* master = network->staToDb(cell);

  auto* master_desc = Gui::get()->getDescriptor<odb::dbMaster*>();
  master_desc->highlight(master, painter);
}

Descriptor::Properties LibertyCellDescriptor::getProperties(
    const std::any& object) const
{
  auto cell = std::any_cast<sta::LibertyCell*>(object);

  auto* network = sta_->getDbNetwork();
  auto gui = Gui::get();
  Properties props;

  sta::LibertyLibrary* library = cell->libertyLibrary();

  auto* db_master = network->staToDb(cell);
  if (db_master != nullptr) {
    props.push_back({"Master", gui->makeSelected(db_master)});
  }

  if (auto area = cell->area()) {
    props.push_back({"Area", fmt::format("{} μm²", area)});
  }
  add_if_true(props, "Dont Use", cell->dontUse());
  props.push_back({"Filename", cell->filename()});
  add_if_true(props, "Has Sequentials", cell->hasSequentials());
  add_if_true(props, "Is Always On", cell->alwaysOn());
  add_if_true(props, "Is Buffer", cell->isBuffer());
  add_if_true(props, "Is Clock Cell", cell->isClockCell());
  add_if_true(props, "Is Inverter", cell->isInverter());
  add_if_true(props, "Is Isolation Cell", cell->isIsolationCell());
  add_if_true(props, "Is Level Shifter", cell->isLevelShifter());
  add_if_true(props, "Is Macro", cell->isMacro());
  add_if_true(props, "Is Memory", cell->isMemory());
  add_if_true(props, "Is Pad", cell->isPad());
  props.push_back({"Library", gui->makeSelected(library)});

  std::array<SelectionSet, 8> ports;
  sta::LibertyCellPortIterator port_iter(cell);
  SelectionSet pg_ports;
  while (port_iter.hasNext()) {
    auto port = port_iter.next();
    if (port->isPwrGnd()) {
      pg_ports.insert(gui->makeSelected(port));
    } else {
      ports[port->direction()->index()].insert(gui->makeSelected(port));
    }
  }
  for (auto dir : {sta::PortDirection::input(),
                   sta::PortDirection::output(),
                   sta::PortDirection::tristate(),
                   sta::PortDirection::bidirect(),
                   sta::PortDirection::internal(),
                   sta::PortDirection::ground(),
                   sta::PortDirection::power(),
                   sta::PortDirection::unknown()}) {
    if (!ports[dir->index()].empty()) {
      std::string direction_str = dir->name();
      capitalize(direction_str);
      props.push_back(
          {fmt::format("{} Ports", direction_str), ports[dir->index()]});
    }
  }

  props.push_back({"PG Ports", pg_ports});

  SelectionSet insts;
  std::unique_ptr<sta::LeafInstanceIterator> lib_iter(
      network->leafInstanceIterator());
  while (lib_iter->hasNext()) {
    auto* inst = lib_iter->next();
    if (network->libertyCell(inst) == cell) {
      insts.insert(gui->makeSelected(inst));
    }
  }
  props.push_back({"Instances", insts});

  return props;
}

Selected LibertyCellDescriptor::makeSelected(const std::any& object) const
{
  if (auto cell = std::any_cast<sta::LibertyCell*>(&object)) {
    return Selected(*cell, this);
  }
  return Selected();
}

bool LibertyCellDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_cell = std::any_cast<sta::LibertyCell*>(l);
  auto r_cell = std::any_cast<sta::LibertyCell*>(r);
  return l_cell->id() < r_cell->id();
}

void LibertyCellDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
      network->libertyLibraryIterator()};

  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* library = lib_iter->next();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      func({cell, this});
    }
  }
}

//////////////////////////////////////////////////

LibertyPortDescriptor::LibertyPortDescriptor(sta::dbSta* sta) : sta_(sta)
{
}

std::string LibertyPortDescriptor::getName(const std::any& object) const
{
  return std::any_cast<sta::LibertyPort*>(object)->name();
}

std::string LibertyPortDescriptor::getTypeName() const
{
  return "Liberty port";
}

bool LibertyPortDescriptor::getBBox(const std::any& object,
                                    odb::Rect& bbox) const
{
  return false;
}

void LibertyPortDescriptor::highlight(const std::any& object,
                                      Painter& painter) const
{
  auto port = std::any_cast<sta::LibertyPort*>(object);
  auto* network = sta_->getDbNetwork();
  odb::dbMTerm* mterm = network->staToDb(port);

  auto* mterm_desc = Gui::get()->getDescriptor<odb::dbMTerm*>();
  if (mterm != nullptr) {
    mterm_desc->highlight(mterm, painter);
  }
}

Descriptor::Properties LibertyPortDescriptor::getProperties(
    const std::any& object) const
{
  auto port = std::any_cast<sta::LibertyPort*>(object);
  auto* network = sta_->getDbNetwork();

  Properties props;

  auto gui = Gui::get();
  odb::dbMTerm* mterm = network->staToDb(port);
  if (mterm != nullptr) {
    props.push_back({"MTerm", gui->makeSelected(network->staToDb(port))});
  }

  props.push_back({"Direction", port->direction()->name()});
  if (auto function = port->function()) {
    props.push_back({"Function", function->to_string()});
  }
  if (auto function = port->tristateEnable()) {
    props.push_back({"Tristate Enable", function->to_string()});
  }
  add_if_true(props, "Is Clock", port->isClock());
  add_if_true(props, "Is Clock Gate Clock", port->isClockGateClock());
  add_if_true(props, "Is Clock Gate Enable", port->isClockGateEnable());
  add_if_true(props, "Is Clock Gate Out", port->isClockGateOut());
  add_if_true(props, "Is PLL Feedback", port->isPllFeedback());
  add_if_true(props, "Is Switch", port->isSwitch());
  add_if_true(props, "Is Isolation Cell Data", port->isolationCellData());
  add_if_true(props, "Is Isolation Cell Enable", port->isolationCellEnable());
  add_if_true(props, "Is Level Shifter Enable", port->levelShifterData());

  auto capacitance = port->capacitance();
  if (capacitance != 0) {
    props.push_back({"Capacitance", convertUnits(capacitance) + "F"});
  }
  auto drive_resistance = port->driveResistance();
  if (drive_resistance != 0) {
    props.push_back({"Drive Resistance", convertUnits(drive_resistance) + "Ω"});
  }

  add_limit<sta::MinMax>(
      props, port, "Slew Limit", &sta::LibertyPort::slewLimit, "S");
  add_limit<sta::MinMax>(props,
                         port,
                         "Capacitance Limit",
                         &sta::LibertyPort::capacitanceLimit,
                         "F");
  add_limit<sta::MinMax>(
      props, port, "Fanout Limit", &sta::LibertyPort::fanoutLimit, "");
  add_limit<sta::RiseFall>(
      props, port, "Min Pulse Width", &sta::LibertyPort::minPulseWidth, "s");

  if (port->hasMembers()) {
    SelectionSet members;
    for (auto* member : *port->memberPorts()) {
      auto lib_port = member->libertyPort();
      if (lib_port != nullptr) {
        members.insert(makeSelected(lib_port));
      }
    }
    props.push_back({"Member ports", members});
  }

  std::any power_pin;
  std::any ground_pin;
  const char* power_pin_name = port->relatedPowerPin();
  const char* ground_pin_name = port->relatedGroundPin();
  sta::LibertyCellPortIterator pg_port_iter(port->libertyCell());
  while (pg_port_iter.hasNext()) {
    auto* pg_port = pg_port_iter.next();
    if (!pg_port->isPwrGnd()) {
      continue;
    }
    if (power_pin_name != nullptr
        && strcmp(pg_port->name(), power_pin_name) == 0) {
      power_pin = gui->makeSelected(pg_port);
    } else if (ground_pin_name != nullptr
               && strcmp(pg_port->name(), ground_pin_name) == 0) {
      ground_pin = gui->makeSelected(pg_port);
    }
  }
  if (!power_pin.has_value() && power_pin_name != nullptr) {
    power_pin = power_pin_name;
  }
  if (power_pin.has_value()) {
    props.push_back({"Related power pin", std::move(power_pin)});
  }
  if (!ground_pin.has_value() && ground_pin_name != nullptr) {
    ground_pin = ground_pin_name;
  }
  if (ground_pin.has_value()) {
    props.push_back({"Related ground pin", std::move(ground_pin)});
  }

  return props;
}

Selected LibertyPortDescriptor::makeSelected(const std::any& object) const
{
  if (auto port = std::any_cast<sta::LibertyPort*>(&object)) {
    return Selected(*port, this);
  }
  return Selected();
}

bool LibertyPortDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_port = std::any_cast<sta::LibertyPort*>(l);
  auto r_port = std::any_cast<sta::LibertyPort*>(r);
  return l_port->id() < r_port->id();
}

void LibertyPortDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
      network->libertyLibraryIterator()};

  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* library = lib_iter->next();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();
        func({port, this});
      }
    }
  }
}

//////////////////////////////////////////////////

SceneDescriptor::SceneDescriptor(sta::dbSta* sta) : sta_(sta)
{
}

std::string SceneDescriptor::getName(const std::any& object) const
{
  return std::any_cast<sta::Scene*>(object)->name();
}

std::string SceneDescriptor::getTypeName() const
{
  return "Timing scene";
}

bool SceneDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void SceneDescriptor::highlight(const std::any& object, Painter& painter) const
{
}

Descriptor::Properties SceneDescriptor::getProperties(
    const std::any& object) const
{
  auto scene = std::any_cast<sta::Scene*>(object);

  auto gui = Gui::get();

  Properties props({{.name = "Mode", .value = scene->mode()->name()}});

  SelectionSet libs;
  for (auto* min_max : {sta::MinMax::min(), sta::MinMax::max()}) {
    for (auto* lib : scene->libertyLibraries(min_max)) {
      libs.insert(gui->makeSelected(lib));
    }
  }
  props.push_back({"Libraries", libs});

  return props;
}

Selected SceneDescriptor::makeSelected(const std::any& object) const
{
  if (auto scene = std::any_cast<sta::Scene*>(&object)) {
    return Selected(*scene, this);
  }
  return Selected();
}

bool SceneDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_scene = std::any_cast<sta::Scene*>(l);
  auto r_scene = std::any_cast<sta::Scene*>(r);
  return l_scene->name() < r_scene->name();
}

void SceneDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  for (auto* scene : sta_->scenes()) {
    func({scene, this});
  }
}

StaInstanceDescriptor::StaInstanceDescriptor(sta::dbSta* sta) : sta_(sta)
{
}

std::string StaInstanceDescriptor::getName(const std::any& object) const
{
  return sta_->network()->name(std::any_cast<sta::Instance*>(object));
}

std::string StaInstanceDescriptor::getTypeName() const
{
  return "Timing/Power";
}

bool StaInstanceDescriptor::getBBox(const std::any& object,
                                    odb::Rect& bbox) const
{
  return false;
}

void StaInstanceDescriptor::highlight(const std::any& object,
                                      Painter& painter) const
{
  auto inst = std::any_cast<sta::Instance*>(object);
  odb::dbInst* db_inst = sta_->getDbNetwork()->staToDb(inst);

  auto* inst_desc = Gui::get()->getDescriptor<odb::dbInst*>();
  inst_desc->highlight(db_inst, painter);
}

Descriptor::Properties StaInstanceDescriptor::getProperties(
    const std::any& object) const
{
  auto inst = std::any_cast<sta::Instance*>(object);
  auto* network = sta_->getDbNetwork();

  auto gui = Gui::get();

  Properties props;

  odb::dbInst* db_inst = network->staToDb(inst);
  props.push_back({"Instance", gui->makeSelected(db_inst)});

  auto* lib_cell = network->libertyCell(inst);
  if (lib_cell != nullptr) {
    props.push_back({"Liberty cell", gui->makeSelected(lib_cell)});
  }

  auto is_inf = [](double value) -> bool {
    // mirrored from:
    // https://github.com/The-OpenROAD-Project/OpenSTA/blob/20925bb00965c1199c45aca0318c2baeb4042c5a/liberty/Units.cc#L153
    return abs(value) >= 0.1 * sta::INF;
  };

  bool has_sdc_constraint = false;

  for (sta::Mode* mode : sta_->modes()) {
    has_sdc_constraint |= mode->sdc()->isConstrained(inst);
  }

  PropertyList port_power_activity;
  PropertyList port_arrival_hold;
  PropertyList port_arrival_setup;
  SelectionSet clocks;
  std::unique_ptr<sta::CellPortBitIterator> port_itr(
      network->portBitIterator(network->cell(inst)));
  while (port_itr->hasNext()) {
    sta::Port* port = port_itr->next();
    sta::Pin* pin = network->findPin(inst, port);

    for (sta::Mode* mode : sta_->modes()) {
      has_sdc_constraint |= mode->sdc()->isConstrained(inst);
    }

    for (sta::Mode* mode : sta_->modes()) {
      for (auto* clock : sta_->clocks(pin, mode)) {
        clocks.insert(gui->makeSelected(clock));
      }
    }

    auto power = sta_->activity(pin, sta_->cmdScene());

    bool is_lib_port = false;
    std::any port_id;
    if (lib_cell != nullptr) {
      auto* lib_port = network->libertyPort(port);
      if (lib_port != nullptr) {
        is_lib_port = true;
        port_id = gui->makeSelected(lib_port);
      }
    }
    if (!port_id.has_value()) {
      port_id = network->name(port);
    }

    if (is_lib_port) {
      const std::string freq
          = Descriptor::convertUnits(power.density(), false, kFloatPrecision);
      const std::string activity_info = fmt::format("{:.2f}% at {}Hz from {}",
                                                    100 * power.duty(),
                                                    freq,
                                                    power.originName());
      port_power_activity.emplace_back(port_id, activity_info);

      const sta::Unit* timeunit = sta_->units()->timeUnit();
      const auto setup_arrival
          = sta_->arrival(pin, nullptr, sta::MinMax::max());
      const std::string setup_text
          = is_inf(setup_arrival)
                ? "None"
                : fmt::format(
                      "{} {}",
                      timeunit->asString(setup_arrival, kFloatPrecision),
                      timeunit->scaleAbbrevSuffix());
      port_arrival_setup.emplace_back(port_id, setup_text);
      const auto hold_arrival = sta_->arrival(
          pin, sta::RiseFallBoth::riseFall(), sta::MinMax::min());
      const std::string hold_text
          = is_inf(hold_arrival)
                ? "None"
                : fmt::format("{} {}",
                              timeunit->asString(hold_arrival, kFloatPrecision),
                              timeunit->scaleAbbrevSuffix());
      port_arrival_hold.emplace_back(port_id, hold_text);
    }
  }
  props.push_back({"Max arrival times", port_arrival_setup});
  props.push_back({"Min arrival times", port_arrival_hold});
  props.push_back({"Port power activity", port_power_activity});

  PropertyList power;
  for (auto* scene : sta_->scenes()) {
    const auto power_info = sta_->power(inst, scene);
    power.emplace_back(
        gui->makeSelected(scene),
        Descriptor::convertUnits(power_info.total(), false, kFloatPrecision)
            + "W");
  }
  props.push_back({"Total power", power});

  props.push_back({"Clocks", clocks});

  props.push_back({"Has SDC constraint", has_sdc_constraint});

  return props;
}

Selected StaInstanceDescriptor::makeSelected(const std::any& object) const
{
  if (auto inst = std::any_cast<sta::Instance*>(object)) {
    return Selected(inst, this);
  }
  return Selected();
}

bool StaInstanceDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto* network = sta_->getDbNetwork();
  auto l_inst = std::any_cast<sta::Instance*>(l);
  auto r_inst = std::any_cast<sta::Instance*>(r);
  return network->id(l_inst) < network->id(r_inst);
}

void StaInstanceDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LeafInstanceIterator> lib_iter(
      network->leafInstanceIterator());

  while (lib_iter->hasNext()) {
    func({lib_iter->next(), this});
  }
}

ClockDescriptor::ClockDescriptor(sta::dbSta* sta) : sta_(sta)
{
}

std::string ClockDescriptor::getName(const std::any& object) const
{
  return std::any_cast<sta::Clock*>(object)->name();
}

std::string ClockDescriptor::getTypeName() const
{
  return "Clock";
}

bool ClockDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void ClockDescriptor::highlight(const std::any& object, Painter& painter) const
{
  auto clock = std::any_cast<sta::Clock*>(object);
  auto* network = sta_->getDbNetwork();

  auto* iterm_desc = Gui::get()->getDescriptor<odb::dbITerm*>();
  auto* bterm_desc = Gui::get()->getDescriptor<odb::dbBTerm*>();
  for (auto* pin : getClockPins(clock)) {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;

    network->staToDb(pin, iterm, bterm, moditerm);

    if (iterm != nullptr) {
      iterm_desc->highlight(iterm, painter);
    } else if (bterm != nullptr) {
      bterm_desc->highlight(bterm, painter);
    }
  }
  for (auto* pin : clock->pins()) {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;

    network->staToDb(pin, iterm, bterm, moditerm);

    if (iterm != nullptr) {
      iterm_desc->highlight(iterm, painter);
    } else if (bterm != nullptr) {
      bterm_desc->highlight(bterm, painter);
    }
  }
}

std::set<const sta::Pin*> ClockDescriptor::getClockPins(sta::Clock* clock) const
{
  auto pins = sta_->cmdMode()->clkNetwork()->pins(clock);
  if (!pins) {
    return {};
  }
  return {pins->begin(), pins->end()};
}

Descriptor::Properties ClockDescriptor::getProperties(
    const std::any& object) const
{
  auto clock = std::any_cast<sta::Clock*>(object);
  auto* network = sta_->getDbNetwork();

  auto gui = Gui::get();

  Properties props;

  auto* timeunit = sta_->units()->timeUnit();
  props.push_back({"Period",
                   fmt::format("{} {}",
                               timeunit->asString(clock->period()),
                               timeunit->scaleAbbrevSuffix())});

  props.push_back({"Is virtual", clock->isVirtual()});
  props.push_back({"Is generated", clock->isGenerated()});
  props.push_back({"Is propagated", clock->isPropagated()});

  auto* master_clk = clock->masterClk();
  if (master_clk != nullptr) {
    props.push_back({"Master clock", gui->makeSelected(master_clk)});
  }

  if (clock->isGenerated()) {
    props.push_back(
        {"Duty cycle", fmt::format("{:.2f}%", clock->dutyCycle() * 100)});

    if (clock->divideBy() > 0) {
      props.push_back({"Divide by", clock->divideBy()});
    }
    if (clock->multiplyBy() > 0) {
      props.push_back({"Multiply by", clock->multiplyBy()});
    }
    props.push_back({"Is inverted", clock->invert()});

    sta::Pin* src_pin = clock->srcPin();
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;

    network->staToDb(src_pin, iterm, bterm, moditerm);

    std::any source;
    if (iterm != nullptr) {
      source = gui->makeSelected(iterm);
    } else if (bterm != nullptr) {
      source = gui->makeSelected(bterm);
    }
    if (source.has_value()) {
      props.push_back({"Source", std::move(source)});
    }
  }

  SelectionSet source_pins;
  for (auto* pin : clock->pins()) {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;

    network->staToDb(pin, iterm, bterm, moditerm);

    if (iterm != nullptr) {
      source_pins.insert(gui->makeSelected(iterm));
    } else if (bterm != nullptr) {
      source_pins.insert(gui->makeSelected(bterm));
    }
  }
  props.push_back({"Sources", source_pins});

  SelectionSet pins;
  for (auto* pin : getClockPins(clock)) {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;

    network->staToDb(pin, iterm, bterm, moditerm);

    if (iterm != nullptr) {
      pins.insert(gui->makeSelected(iterm));
    } else if (bterm != nullptr) {
      pins.insert(gui->makeSelected(bterm));
    }
  }
  props.push_back({"Pins", pins});

  return props;
}

Selected ClockDescriptor::makeSelected(const std::any& object) const
{
  if (auto clock = std::any_cast<sta::Clock*>(&object)) {
    return Selected(*clock, this);
  }
  return Selected();
}

bool ClockDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_clock = std::any_cast<sta::Clock*>(l);
  auto r_clock = std::any_cast<sta::Clock*>(r);
  return strcmp(l_clock->name(), r_clock->name()) < 0;
}

void ClockDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
}

}  // namespace gui
