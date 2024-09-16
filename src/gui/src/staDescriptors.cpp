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
#include <queue>
#include <regex>
#include <sstream>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
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
}

Descriptor::Properties LibertyLibraryDescriptor::getProperties(
    std::any object) const
{
  auto library = std::any_cast<sta::LibertyLibrary*>(object);

  Properties props;

  auto gui = Gui::get();
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
}

Descriptor::Properties LibertyCellDescriptor::getProperties(
    std::any object) const
{
  auto cell = std::any_cast<sta::LibertyCell*>(object);

  auto gui = Gui::get();
  Properties props;

  sta::LibertyLibrary* library = cell->libertyLibrary();

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
}

Descriptor::Properties LibertyPortDescriptor::getProperties(
    std::any object) const
{
  auto port = std::any_cast<sta::LibertyPort*>(object);

  Properties props;
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

}  // namespace gui
