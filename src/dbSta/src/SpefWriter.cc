// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "db_sta/SpefWriter.hh"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Scene.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace sta {

using std::map;
using utl::ORD;

SpefWriter::SpefWriter(utl::Logger* logger,
                       dbSta* sta,
                       std::map<Scene*, std::ostream*>& spef_streams)
    : logger_(logger),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      spef_streams_(spef_streams)
{
  writeHeader();
  writePorts();
}

static std::string escapeSpecial(const std::string& name)
{
  std::string result = name;
  size_t pos = 0;
  while ((pos = result.find('$', pos)) != std::string::npos) {
    result.replace(pos, 1, "\\$");
    pos += 2;
  }
  return result;
}

static std::string escapeSpecial(const char* name)
{
  if (!name) {
    return "";
  }
  return escapeSpecial(std::string(name));
}

// Quick fix for wrong pin delimiter.
// TODO: save the parasitics data to odb and use the existing write_spef
// mechanism to produce the spef files from estimate_parasitics.
static std::string fixPinDelimiter(const std::string& name)
{
  const char delimiter = '/';
  std::string result = name;
  size_t pos = result.find_last_of(delimiter);
  if (pos != std::string::npos) {
    result.replace(pos, 1, ":");
  }
  return result;
}

void SpefWriter::writeHeader()
{
  for (auto [_, it] : spef_streams_) {
    std::ostream& stream = *it;
    stream << "*SPEF \"ieee 1481-1999\"" << '\n';
    stream << "*DESIGN \"" << escapeSpecial(network_->block()->getName())
           << "\"" << '\n';
    stream << "*DATE \"11:11:11 Fri 11 11, 1111\"" << '\n';
    stream << "*VENDOR \"The OpenROAD Project\"" << '\n';
    stream << "*PROGRAM \"OpenROAD\"" << '\n';
    stream << "*VERSION \"1.0\"" << '\n';
    stream << "*DESIGN_FLOW \"NAME_SCOPE LOCAL\" \"PIN_CAP NONE\"" << '\n';
    stream << "*DIVIDER /" << '\n';
    stream << "*DELIMITER :" << '\n';
    stream << "*BUS_DELIMITER []" << '\n';

    auto units = network_->units();
    std::string time_unit = std::string(units->timeUnit()->scaleAbbrevSuffix());
    std::string cap_unit
        = std::string(units->capacitanceUnit()->scaleAbbrevSuffix());
    std::string res_unit
        = std::string(units->resistanceUnit()->scaleAbbrevSuffix());
    std::ranges::transform(time_unit, time_unit.begin(), ::toupper);
    std::ranges::transform(cap_unit, cap_unit.begin(), ::toupper);
    std::ranges::transform(res_unit, res_unit.begin(), ::toupper);

    stream << "*T_UNIT 1 " << time_unit << '\n';
    stream << "*C_UNIT 1 " << cap_unit << '\n';
    stream << "*R_UNIT 1 " << res_unit << '\n';
    stream << "*L_UNIT 1 HENRY" << '\n';
    stream << '\n';
  }
}

static char getIoDirectionText(const odb::dbIoType& io_type)
{
  if (io_type == odb::dbIoType::INPUT) {
    return 'I';
  }
  if (io_type == odb::dbIoType::OUTPUT) {
    return 'O';
  }
  return 'B';
}

void SpefWriter::writePorts()
{
  for (auto [_, it] : spef_streams_) {
    std::ostream& stream = *it;

    std::unique_ptr<InstancePinIterator> pin_iter(
        network_->pinIterator(network_->topInstance()));

    stream << "*PORTS" << '\n';
    while (pin_iter->hasNext()) {
      Pin* pin = pin_iter->next();
      odb::dbITerm* iterm = nullptr;
      odb::dbBTerm* bterm = nullptr;
      odb::dbModITerm* moditerm = nullptr;
      network_->staToDb(pin, iterm, bterm, moditerm);

      if (iterm != nullptr) {
        stream << escapeSpecial(iterm->getName()) << " ";
        stream << getIoDirectionText(iterm->getIoType());
        stream << '\n';
      } else if (bterm != nullptr) {
        stream << escapeSpecial(bterm->getName()) << " ";
        stream << getIoDirectionText(bterm->getIoType());
        stream << '\n';
      } else {
        logger_->error(ORD,
                       10,
                       "Got a modTerm instead of iTerm or bTerm while writing "
                       "SPEF ports.");
      }
    }
    stream << '\n';
  }
}

void SpefWriter::writeNet(Scene* scene,
                          const Net* net,
                          Parasitic* parasitic,
                          Parasitics* parasitics)
{
  auto it = spef_streams_.find(scene);
  if (it == spef_streams_.end()) {
    logger_->error(
        ORD, 20, "Tried to write net SPEF info for scene that was not set");
  }
  std::ostream& stream = *it->second;

  auto units = network_->units();
  float cap_scale = units->capacitanceUnit()->scale();
  float res_scale = units->resistanceUnit()->scale();

  stream << "*D_NET " << escapeSpecial(network_->staToDb(net)->getName())
         << " ";
  stream << parasitics->capacitance(parasitic) / cap_scale << '\n';

  stream << "*CONN" << '\n';
  for (auto node : parasitics->nodes(parasitic)) {
    auto pin = parasitics->pin(node);
    if (pin != nullptr) {
      odb::dbITerm* iterm = nullptr;
      odb::dbBTerm* bterm = nullptr;
      odb::dbModITerm* moditerm = nullptr;
      network_->staToDb(pin, iterm, bterm, moditerm);

      if (iterm != nullptr) {
        stream << "*I "
               << escapeSpecial(fixPinDelimiter(parasitics->name(node))) << " ";
        stream << getIoDirectionText(iterm->getIoType());
        stream << " *D " << iterm->getInst()->getMaster()->getName();
        stream << '\n';
      } else if (bterm != nullptr) {
        stream << "*P " << escapeSpecial(parasitics->name(node)) << " ";
        stream << getIoDirectionText(bterm->getIoType());
        stream << '\n';
      } else {
        logger_->error(ORD,
                       9,
                       "Got a modTerm instead of iTerm or bTerm while writing "
                       "SPEF net connections.");
      }
    }
  }

  int count = 1;
  bool label = false;
  for (auto node : parasitics->nodes(parasitic)) {
    if (parasitics->pin(node) == nullptr) {
      if (!label) {
        label = true;
        stream << "*CAP" << '\n';
      }

      stream << count++ << " ";
      stream << escapeSpecial(parasitics->name(node)) << " "
             << parasitics->nodeGndCap(node) / cap_scale;
      stream << '\n';
    }
  }
  for (auto cap : parasitics->capacitors(parasitic)) {
    if (!label) {
      label = true;
      stream << "*CAP" << '\n';
    }
    stream << count++ << " ";

    auto n1 = parasitics->node1(cap);
    stream << escapeSpecial(parasitics->name(n1)) << " ";
    auto n2 = parasitics->node2(cap);
    stream << escapeSpecial(parasitics->name(n2)) << " ";
    stream << parasitics->value(cap) / cap_scale << '\n';
  }

  count = 1;
  label = false;
  for (auto res : parasitics->resistors(parasitic)) {
    if (!label) {
      label = true;
      stream << "*RES" << '\n';
    }
    stream << count++ << " ";

    auto n1 = parasitics->node1(res);
    auto n2 = parasitics->node2(res);

    odb::dbITerm* iterm = nullptr;
    odb::dbBTerm* bterm = nullptr;
    odb::dbModITerm* moditerm = nullptr;

    std::string node1_name = parasitics->name(n1);
    auto pin1 = parasitics->pin(n1);
    if (pin1 != nullptr) {
      network_->staToDb(pin1, iterm, bterm, moditerm);
      if (iterm != nullptr) {
        node1_name = fixPinDelimiter(node1_name);
      }
    }
    node1_name = escapeSpecial(node1_name);

    std::string node2_name = parasitics->name(n2);
    auto pin2 = parasitics->pin(n2);
    if (pin2 != nullptr) {
      network_->staToDb(pin2, iterm, bterm, moditerm);
      if (iterm != nullptr) {
        node2_name = fixPinDelimiter(node2_name);
      }
    }
    node2_name = escapeSpecial(node2_name);

    stream << node1_name << " ";
    stream << node2_name << " ";
    stream << parasitics->value(res) / res_scale << '\n';
  }

  stream << "*END" << '\n' << '\n';
}

}  // namespace sta
