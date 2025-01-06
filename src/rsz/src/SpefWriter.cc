/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, Precision Innovations Inc.
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include "rsz/SpefWriter.hh"

#include <fstream>
#include <iostream>

#include "db_sta/dbNetwork.hh"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

using std::map;
using utl::RSZ;

SpefWriter::SpefWriter(Logger* logger,
                       dbSta* sta,
                       std::map<Corner*, std::ostream*>& spef_streams)
    : logger_(logger),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      parasitics_(sta_->parasitics()),
      spef_streams_(spef_streams)
{
  writeHeader();
  writePorts();
}

std::string escapeSpecial(const std::string& name)
{
  std::string result = name;
  size_t pos = 0;
  while ((pos = result.find('$', pos)) != std::string::npos) {
    result.replace(pos, 1, "\\$");
    pos += 2;
  }
  pos = 0;
  while ((pos = result.find('/', pos)) != std::string::npos) {
    result.replace(pos, 1, "\\/");
    pos += 2;
  }
  return result;
}

std::string escapeSpecial(const char* name)
{
  if (!name) {
    return "";
  }
  return escapeSpecial(std::string(name));
}

// Quick fix for wrong pin delimiter.
// TODO: save the parasitics data to odb and use the existing write_spef
// mechanism to produce the spef files from estimate_parasitics.
std::string fixPinDelimiter(const std::string& name)
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
    std::string time_unit = std::string(units->timeUnit()->scaledSuffix());
    std::string cap_unit
        = std::string(units->capacitanceUnit()->scaledSuffix());
    std::string res_unit = std::string(units->resistanceUnit()->scaledSuffix());
    std::transform(
        time_unit.begin(), time_unit.end(), time_unit.begin(), ::toupper);
    std::transform(
        cap_unit.begin(), cap_unit.end(), cap_unit.begin(), ::toupper);
    std::transform(
        res_unit.begin(), res_unit.end(), res_unit.begin(), ::toupper);

    stream << "*T_UNIT 1 " << time_unit << '\n';
    stream << "*C_UNIT 1 " << cap_unit << '\n';
    stream << "*R_UNIT 1 " << res_unit << '\n';
    stream << "*L_UNIT 1 HENRY" << '\n';
    stream << '\n';
  }
}

char getIoDirectionText(const odb::dbIoType& ioType)
{
  if (ioType == odb::dbIoType::INPUT) {
    return 'I';
  }
  if (ioType == odb::dbIoType::OUTPUT) {
    return 'O';
  }
  return 'B';
}

void SpefWriter::writePorts()
{
  for (auto [_, it] : spef_streams_) {
    std::ostream& stream = *it;

    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network_->pinIterator(network_->topInstance()));

    stream << "*PORTS" << '\n';
    while (pin_iter->hasNext()) {
      sta::Pin* pin = pin_iter->next();
      odb::dbITerm* iterm = nullptr;
      odb::dbBTerm* bterm = nullptr;
      odb::dbModITerm* moditerm = nullptr;
      odb::dbModBTerm* modbterm = nullptr;
      network_->staToDb(pin, iterm, bterm, moditerm, modbterm);

      if (iterm != nullptr) {
        stream << escapeSpecial(iterm->getName()) << " ";
        stream << getIoDirectionText(iterm->getIoType());
        stream << '\n';
      } else if (bterm != nullptr) {
        stream << escapeSpecial(bterm->getName()) << " ";
        stream << getIoDirectionText(bterm->getIoType());
        stream << '\n';
      } else {
        logger_->error(RSZ,
                       8,
                       "Got a modTerm instead of iTerm or bTerm while writing "
                       "SPEF ports.");
      }
    }
    stream << '\n';
  }
}

void SpefWriter::writeNet(Corner* corner, const Net* net, Parasitic* parasitic)
{
  auto it = spef_streams_.find(corner);
  if (it == spef_streams_.end()) {
    logger_->error(
        RSZ, 14, "Tried to write net SPEF info for corner that was not set");
  }
  std::ostream& stream = *it->second;

  auto units = network_->units();
  float cap_scale = units->capacitanceUnit()->scale();
  float res_scale = units->resistanceUnit()->scale();

  stream << "*D_NET " << escapeSpecial(network_->staToDb(net)->getName())
         << " ";
  stream << parasitics_->capacitance(parasitic) / cap_scale << '\n';

  stream << "*CONN" << '\n';
  for (auto node : parasitics_->nodes(parasitic)) {
    auto pin = parasitics_->pin(node);
    if (pin != nullptr) {
      odb::dbITerm* iterm = nullptr;
      odb::dbBTerm* bterm = nullptr;
      odb::dbModITerm* moditerm = nullptr;
      odb::dbModBTerm* modbterm = nullptr;
      network_->staToDb(pin, iterm, bterm, moditerm, modbterm);

      if (iterm != nullptr) {
        stream << "*I "
               << escapeSpecial(fixPinDelimiter(parasitics_->name(node)))
               << " ";
        stream << getIoDirectionText(iterm->getIoType());
        stream << " *D " << iterm->getInst()->getMaster()->getName();
        stream << '\n';
      } else if (bterm != nullptr) {
        stream << "*P " << escapeSpecial(parasitics_->name(node)) << " ";
        stream << getIoDirectionText(bterm->getIoType());
        stream << '\n';
      } else {
        logger_->error(RSZ,
                       9,
                       "Got a modTerm instead of iTerm or bTerm while writing "
                       "SPEF net connections.");
      }
    }
  }

  int count = 1;
  bool label = false;
  for (auto node : parasitics_->nodes(parasitic)) {
    if (parasitics_->pin(node) == nullptr) {
      if (!label) {
        label = true;
        stream << "*CAP" << '\n';
      }

      stream << count++ << " ";
      stream << escapeSpecial(parasitics_->name(node)) << " "
             << parasitics_->nodeGndCap(node) / cap_scale;
      stream << '\n';
    }
  }
  for (auto cap : parasitics_->capacitors(parasitic)) {
    if (!label) {
      label = true;
      stream << "*CAP" << '\n';
    }
    stream << count++ << " ";

    auto n1 = parasitics_->node1(cap);
    stream << escapeSpecial(parasitics_->name(n1)) << " ";
    auto n2 = parasitics_->node2(cap);
    stream << escapeSpecial(parasitics_->name(n2)) << " ";
    stream << parasitics_->value(cap) / cap_scale << '\n';
  }

  count = 1;
  label = false;
  for (auto res : parasitics_->resistors(parasitic)) {
    if (!label) {
      label = true;
      stream << "*RES" << '\n';
    }
    stream << count++ << " ";

    auto n1 = parasitics_->node1(res);
    auto n2 = parasitics_->node2(res);

    odb::dbITerm* iterm = nullptr;
    odb::dbBTerm* bterm = nullptr;
    odb::dbModITerm* moditerm = nullptr;
    odb::dbModBTerm* modbterm = nullptr;

    std::string node1_name = parasitics_->name(n1);
    auto pin1 = parasitics_->pin(n1);
    if (pin1 != nullptr) {
      network_->staToDb(pin1, iterm, bterm, moditerm, modbterm);
      if (iterm != nullptr) {
        node1_name = fixPinDelimiter(node1_name);
      }
    }
    node1_name = escapeSpecial(node1_name);

    std::string node2_name = parasitics_->name(n2);
    auto pin2 = parasitics_->pin(n2);
    if (pin2 != nullptr) {
      network_->staToDb(pin2, iterm, bterm, moditerm, modbterm);
      if (iterm != nullptr) {
        node2_name = fixPinDelimiter(node2_name);
      }
    }
    node2_name = escapeSpecial(node2_name);

    stream << node1_name << " ";
    stream << node2_name << " ";
    stream << parasitics_->value(res) / res_scale << '\n';
  }

  stream << "*END" << '\n' << '\n';
}

}  // namespace rsz
