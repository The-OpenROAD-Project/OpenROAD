///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, Precision Innovations Inc.
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

#include "staDescriptors.h"

#include <QInputDialog>
#include <QStringList>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <limits>
#include <memory>
#include <queue>
#include <regex>
#include <sstream>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Corner.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
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
                      LimitFunc<T> func,
                      const char* suffix)
{
  for (const auto* elem : T::range()) {
    bool exists;
    float limit;
    func(port, elem, limit, exists);
    if (exists) {
      std::string elem_str = elem->asString();
      capitalize(elem_str);
      props.push_back({fmt::format("{} {}", elem_str, label),
                       Descriptor::convertUnits(limit) + suffix});
    }
  }
};

//////////////////////////////////////////////////

LibertyLibraryDescriptor::LibertyLibraryDescriptor(odb::dbDatabase* db,
                                                   sta::dbSta* sta)
    : db_(db), sta_(sta)
{
}

std::string LibertyLibraryDescriptor::getName(std::any object) const
{
  return std::any_cast<sta::LibertyLibrary*>(object)->name();
}

std::string LibertyLibraryDescriptor::getTypeName() const
{
  return "Liberty library";
}

bool LibertyLibraryDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void LibertyLibraryDescriptor::highlight(std::any object,
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
    std::any object) const
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
    return fmt::format("{} {}", unit->asString(value), unit->scaledSuffix());
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

  SelectionSet corners;
  for (auto* corner : *sta_->corners()) {
    for (const sta::MinMax* min_max :
         {sta::MinMax::min(), sta::MinMax::max()}) {
      const auto libs = corner->libertyLibraries(min_max);
      if (std::find(libs.begin(), libs.end(), library) != libs.end()) {
        corners.insert(gui->makeSelected(corner));
      }
    }
  }
  props.push_back({"Corners", corners});

  SelectionSet cells;
  sta::LibertyCellIterator cell_iter(library);
  while (cell_iter.hasNext()) {
    cells.insert(gui->makeSelected(cell_iter.next()));
  }
  props.push_back({"Cells", cells});

  return props;
}

Selected LibertyLibraryDescriptor::makeSelected(std::any object) const
{
  if (auto library = std::any_cast<sta::LibertyLibrary*>(&object)) {
    return Selected(*library, this);
  }
  return Selected();
}

bool LibertyLibraryDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_library = std::any_cast<sta::LibertyLibrary*>(l);
  auto r_library = std::any_cast<sta::LibertyLibrary*>(r);
  return l_library->id() < r_library->id();
}

bool LibertyLibraryDescriptor::getAllObjects(SelectionSet& objects) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
      network->libertyLibraryIterator()};

  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* library = lib_iter->next();
    objects.insert(makeSelected(library));
  }

  return true;
}

//////////////////////////////////////////////////

LibertyCellDescriptor::LibertyCellDescriptor(odb::dbDatabase* db,
                                             sta::dbSta* sta)
    : db_(db), sta_(sta)
{
}

std::string LibertyCellDescriptor::getName(std::any object) const
{
  return std::any_cast<sta::LibertyCell*>(object)->name();
}

std::string LibertyCellDescriptor::getTypeName() const
{
  return "Liberty cell";
}

bool LibertyCellDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void LibertyCellDescriptor::highlight(std::any object, Painter& painter) const
{
  auto cell = std::any_cast<sta::LibertyCell*>(object);
  auto* network = sta_->getDbNetwork();
  odb::dbMaster* master = network->staToDb(cell);

  auto* master_desc = Gui::get()->getDescriptor<odb::dbMaster*>();
  master_desc->highlight(master, painter);
}

Descriptor::Properties LibertyCellDescriptor::getProperties(
    std::any object) const
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
  while (port_iter.hasNext()) {
    auto port = port_iter.next();
    ports[port->direction()->index()].insert(gui->makeSelected(port));
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

  SelectionSet pg_ports;
  sta::LibertyCellPgPortIterator pg_port_iter(cell);
  while (pg_port_iter.hasNext()) {
    pg_ports.insert(gui->makeSelected(pg_port_iter.next()));
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

Selected LibertyCellDescriptor::makeSelected(std::any object) const
{
  if (auto cell = std::any_cast<sta::LibertyCell*>(&object)) {
    return Selected(*cell, this);
  }
  return Selected();
}

bool LibertyCellDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_cell = std::any_cast<sta::LibertyCell*>(l);
  auto r_cell = std::any_cast<sta::LibertyCell*>(r);
  return l_cell->id() < r_cell->id();
}

bool LibertyCellDescriptor::getAllObjects(SelectionSet& objects) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
      network->libertyLibraryIterator()};

  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* library = lib_iter->next();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      objects.insert(makeSelected(cell));
    }
  }

  return true;
}

//////////////////////////////////////////////////

LibertyPortDescriptor::LibertyPortDescriptor(odb::dbDatabase* db,
                                             sta::dbSta* sta)
    : db_(db), sta_(sta)
{
}

std::string LibertyPortDescriptor::getName(std::any object) const
{
  return std::any_cast<sta::LibertyPort*>(object)->name();
}

std::string LibertyPortDescriptor::getTypeName() const
{
  return "Liberty port";
}

bool LibertyPortDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void LibertyPortDescriptor::highlight(std::any object, Painter& painter) const
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
    std::any object) const
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
    props.push_back({"Function", function->asString()});
  }
  if (auto function = port->tristateEnable()) {
    props.push_back({"Tristate Enable", function->asString()});
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
  sta::LibertyCellPgPortIterator pg_port_iter(port->libertyCell());
  while (pg_port_iter.hasNext()) {
    auto* pg_port = pg_port_iter.next();
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

Selected LibertyPortDescriptor::makeSelected(std::any object) const
{
  if (auto port = std::any_cast<sta::LibertyPort*>(&object)) {
    return Selected(*port, this);
  }
  return Selected();
}

bool LibertyPortDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_port = std::any_cast<sta::LibertyPort*>(l);
  auto r_port = std::any_cast<sta::LibertyPort*>(r);
  return l_port->id() < r_port->id();
}

bool LibertyPortDescriptor::getAllObjects(SelectionSet& objects) const
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
        objects.insert(makeSelected(port));
      }
    }
  }

  return true;
}

//////////////////////////////////////////////////

static const char* typeNameStr(sta::LibertyPgPort::PgType type)
{
  switch (type) {
    case sta::LibertyPgPort::unknown:
      return "unknown";
    case sta::LibertyPgPort::primary_power:
      return "primary_power";
    case sta::LibertyPgPort::primary_ground:
      return "primary_ground";
    case sta::LibertyPgPort::backup_power:
      return "backup_power";
    case sta::LibertyPgPort::backup_ground:
      return "backup_ground";
    case sta::LibertyPgPort::internal_power:
      return "internal_power";
    case sta::LibertyPgPort::internal_ground:
      return "internal_ground";
    case sta::LibertyPgPort::nwell:
      return "nwell";
    case sta::LibertyPgPort::pwell:
      return "pwell";
    case sta::LibertyPgPort::deepnwell:
      return "deepnwell";
    case sta::LibertyPgPort::deeppwell:
      return "deeppwell";
  }
  return "<unexpected>";
}

LibertyPgPortDescriptor::LibertyPgPortDescriptor(odb::dbDatabase* db,
                                                 sta::dbSta* sta)
    : db_(db), sta_(sta)
{
}

std::string LibertyPgPortDescriptor::getName(std::any object) const
{
  return std::any_cast<sta::LibertyPgPort*>(object)->name();
}

std::string LibertyPgPortDescriptor::getTypeName() const
{
  return "Liberty PG port";
}

bool LibertyPgPortDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void LibertyPgPortDescriptor::highlight(std::any object, Painter& painter) const
{
  odb::dbMTerm* mterm = getMTerm(object);

  if (mterm != nullptr) {
    auto* mterm_desc = Gui::get()->getDescriptor<odb::dbMTerm*>();
    mterm_desc->highlight(mterm, painter);
  }
}

Descriptor::Properties LibertyPgPortDescriptor::getProperties(
    std::any object) const
{
  auto port = std::any_cast<sta::LibertyPgPort*>(object);

  auto gui = Gui::get();

  Properties props;
  props.push_back({"Cell", gui->makeSelected(port->cell())});
  props.push_back({"Type", typeNameStr(port->pgType())});
  props.push_back({"Voltage name", port->voltageName()});

  odb::dbMTerm* mterm = getMTerm(object);
  if (mterm != nullptr) {
    props.push_back({"Terminal", gui->makeSelected(mterm)});
  }

  return props;
}

Selected LibertyPgPortDescriptor::makeSelected(std::any object) const
{
  if (auto port = std::any_cast<sta::LibertyPgPort*>(&object)) {
    return Selected(*port, this);
  }
  return Selected();
}

bool LibertyPgPortDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_port = std::any_cast<sta::LibertyPgPort*>(l);
  auto r_port = std::any_cast<sta::LibertyPgPort*>(r);
  return strcmp(l_port->name(), r_port->name()) < 0;
}

bool LibertyPgPortDescriptor::getAllObjects(SelectionSet& objects) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
      network->libertyLibraryIterator()};

  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* library = lib_iter->next();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      sta::LibertyCellPgPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        sta::LibertyPgPort* port = port_iter.next();
        objects.insert(makeSelected(port));
      }
    }
  }

  return true;
}

odb::dbMTerm* LibertyPgPortDescriptor::getMTerm(const std::any& object) const
{
  auto port = std::any_cast<sta::LibertyPgPort*>(object);
  odb::dbMaster* master = sta_->getDbNetwork()->staToDb(port->cell());
  odb::dbMTerm* mterm = master->findMTerm(port->name());

  return mterm;
}

CornerDescriptor::CornerDescriptor(odb::dbDatabase* db, sta::dbSta* sta)
    : db_(db), sta_(sta)
{
}

std::string CornerDescriptor::getName(std::any object) const
{
  return std::any_cast<sta::Corner*>(object)->name();
}

std::string CornerDescriptor::getTypeName() const
{
  return "Timing corner";
}

bool CornerDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void CornerDescriptor::highlight(std::any object, Painter& painter) const
{
}

Descriptor::Properties CornerDescriptor::getProperties(std::any object) const
{
  auto corner = std::any_cast<sta::Corner*>(object);

  auto gui = Gui::get();

  Properties props;

  SelectionSet libs;
  for (auto* min_max : {sta::MinMax::min(), sta::MinMax::max()}) {
    for (auto* lib : corner->libertyLibraries(min_max)) {
      libs.insert(gui->makeSelected(lib));
    }
  }
  props.push_back({"Libraries", libs});

  return props;
}

Selected CornerDescriptor::makeSelected(std::any object) const
{
  if (auto corner = std::any_cast<sta::Corner*>(&object)) {
    return Selected(*corner, this);
  }
  return Selected();
}

bool CornerDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_corner = std::any_cast<sta::Corner*>(l);
  auto r_corner = std::any_cast<sta::Corner*>(r);
  return strcmp(l_corner->name(), r_corner->name()) < 0;
}

bool CornerDescriptor::getAllObjects(SelectionSet& objects) const
{
  for (auto* corner : *sta_->corners()) {
    objects.insert(makeSelected(corner));
  }

  return true;
}

StaInstanceDescriptor::StaInstanceDescriptor(odb::dbDatabase* db,
                                             sta::dbSta* sta)
    : db_(db), sta_(sta)
{
}

std::string StaInstanceDescriptor::getName(std::any object) const
{
  return sta_->network()->name(std::any_cast<sta::Instance*>(object));
}

std::string StaInstanceDescriptor::getTypeName() const
{
  return "Timing/Power";
}

bool StaInstanceDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void StaInstanceDescriptor::highlight(std::any object, Painter& painter) const
{
  auto inst = std::any_cast<sta::Instance*>(object);
  odb::dbInst* db_inst = sta_->getDbNetwork()->staToDb(inst);

  auto* inst_desc = Gui::get()->getDescriptor<odb::dbInst*>();
  inst_desc->highlight(db_inst, painter);
}

Descriptor::Properties StaInstanceDescriptor::getProperties(
    std::any object) const
{
  auto inst = std::any_cast<sta::Instance*>(object);
  auto* network = sta_->getDbNetwork();
  auto* sdc = sta_->sdc();

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

  bool has_sdc_constraint = sdc->isConstrained(inst);
  PropertyList port_power_activity;
  PropertyList port_arrival_hold;
  PropertyList port_arrival_setup;
  SelectionSet clocks;
  std::unique_ptr<sta::CellPortBitIterator> port_itr(
      network->portBitIterator(network->cell(inst)));
  while (port_itr->hasNext()) {
    sta::Port* port = port_itr->next();
    sta::Pin* pin = network->findPin(inst, port);
    has_sdc_constraint |= sdc->isConstrained(pin);

    for (auto* clock : sta_->clocks(pin)) {
      clocks.insert(gui->makeSelected(clock));
    }

    auto power = sta_->findClkedActivity(pin);

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
          = Descriptor::convertUnits(power.activity(), false, float_precision_);
      const std::string activity_info = fmt::format("{:.2f}% at {}Hz from {}",
                                                    100 * power.duty(),
                                                    freq,
                                                    power.originName());
      port_power_activity.push_back({port_id, activity_info});

      const sta::Unit* timeunit = sta_->units()->timeUnit();
      const auto setup_arrival
          = sta_->pinArrival(pin, nullptr, sta::MinMax::max());
      const std::string setup_text
          = is_inf(setup_arrival)
                ? "None"
                : fmt::format(
                    "{} {}",
                    timeunit->asString(setup_arrival, float_precision_),
                    timeunit->scaledSuffix());
      port_arrival_setup.push_back({port_id, setup_text});
      const auto hold_arrival
          = sta_->pinArrival(pin, nullptr, sta::MinMax::min());
      const std::string hold_text
          = is_inf(hold_arrival)
                ? "None"
                : fmt::format(
                    "{} {}",
                    timeunit->asString(hold_arrival, float_precision_),
                    timeunit->scaledSuffix());
      port_arrival_hold.push_back({port_id, hold_text});
    }
  }
  props.push_back({"Max arrival times", port_arrival_setup});
  props.push_back({"Min arrival times", port_arrival_hold});
  props.push_back({"Port power activity", port_power_activity});

  PropertyList power;
  for (auto* corner : *sta_->corners()) {
    const auto power_info = sta_->power(inst, corner);
    power.push_back(
        {gui->makeSelected(corner),
         Descriptor::convertUnits(power_info.total(), false, float_precision_)
             + "W"});
  }
  props.push_back({"Total power", power});

  props.push_back({"Clocks", clocks});

  props.push_back({"Has SDC constraint", has_sdc_constraint});

  return props;
}

Selected StaInstanceDescriptor::makeSelected(std::any object) const
{
  if (auto inst = std::any_cast<sta::Instance*>(&object)) {
    return Selected(*inst, this);
  }
  return Selected();
}

bool StaInstanceDescriptor::lessThan(std::any l, std::any r) const
{
  auto* network = sta_->getDbNetwork();
  auto l_inst = std::any_cast<sta::Instance*>(l);
  auto r_inst = std::any_cast<sta::Instance*>(r);
  return network->id(l_inst) < network->id(r_inst);
}

bool StaInstanceDescriptor::getAllObjects(SelectionSet& objects) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unique_ptr<sta::LeafInstanceIterator> lib_iter(
      network->leafInstanceIterator());

  while (lib_iter->hasNext()) {
    objects.insert(makeSelected(lib_iter->next()));
  }

  return true;
}

ClockDescriptor::ClockDescriptor(odb::dbDatabase* db, sta::dbSta* sta)
    : db_(db), sta_(sta)
{
}

std::string ClockDescriptor::getName(std::any object) const
{
  return std::any_cast<sta::Clock*>(object)->name();
}

std::string ClockDescriptor::getTypeName() const
{
  return "Clock";
}

bool ClockDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void ClockDescriptor::highlight(std::any object, Painter& painter) const
{
  auto clock = std::any_cast<sta::Clock*>(object);
  auto* network = sta_->getDbNetwork();

  auto* iterm_desc = Gui::get()->getDescriptor<odb::dbITerm*>();
  auto* bterm_desc = Gui::get()->getDescriptor<odb::dbBTerm*>();
  for (auto* pin : getClockPins(clock)) {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;
    odb::dbModBTerm* modbterm;

    network->staToDb(pin, iterm, bterm, moditerm, modbterm);

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
    odb::dbModBTerm* modbterm;

    network->staToDb(pin, iterm, bterm, moditerm, modbterm);

    if (iterm != nullptr) {
      iterm_desc->highlight(iterm, painter);
    } else if (bterm != nullptr) {
      bterm_desc->highlight(bterm, painter);
    }
  }
}

std::set<const sta::Pin*> ClockDescriptor::getClockPins(sta::Clock* clock) const
{
  std::set<const sta::Pin*> pins;
  for (auto* pin : sta_->startpointPins()) {
    const auto pin_clocks = sta_->clocks(pin);
    if (std::find(pin_clocks.begin(), pin_clocks.end(), clock)
        != pin_clocks.end()) {
      pins.insert(pin);
    }
  }
  for (auto* pin : sta_->endpointPins()) {
    const auto pin_clocks = sta_->clocks(pin);
    if (std::find(pin_clocks.begin(), pin_clocks.end(), clock)
        != pin_clocks.end()) {
      pins.insert(pin);
    }
  }

  return pins;
}

Descriptor::Properties ClockDescriptor::getProperties(std::any object) const
{
  auto clock = std::any_cast<sta::Clock*>(object);
  auto* network = sta_->getDbNetwork();

  auto gui = Gui::get();

  Properties props;

  auto* timeunit = sta_->units()->timeUnit();
  props.push_back({"Period",
                   fmt::format("{} {}",
                               timeunit->asString(clock->period()),
                               timeunit->scaledSuffix())});

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
    odb::dbModBTerm* modbterm;

    network->staToDb(src_pin, iterm, bterm, moditerm, modbterm);

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
    odb::dbModBTerm* modbterm;

    network->staToDb(pin, iterm, bterm, moditerm, modbterm);

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
    odb::dbModBTerm* modbterm;

    network->staToDb(pin, iterm, bterm, moditerm, modbterm);

    if (iterm != nullptr) {
      pins.insert(gui->makeSelected(iterm));
    } else if (bterm != nullptr) {
      pins.insert(gui->makeSelected(bterm));
    }
  }
  props.push_back({"Pins", pins});

  return props;
}

Selected ClockDescriptor::makeSelected(std::any object) const
{
  if (auto clock = std::any_cast<sta::Clock*>(&object)) {
    return Selected(*clock, this);
  }
  return Selected();
}

bool ClockDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_clock = std::any_cast<sta::Clock*>(l);
  auto r_clock = std::any_cast<sta::Clock*>(r);
  return strcmp(l_clock->name(), r_clock->name()) < 0;
}

bool ClockDescriptor::getAllObjects(SelectionSet& objects) const
{
  return false;
}

}  // namespace gui
