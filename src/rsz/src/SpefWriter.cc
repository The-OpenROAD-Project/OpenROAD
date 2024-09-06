/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, Precision Innovations Inc.
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
                       std::map<Corner*, std::ostream*>& spef_files)
    : logger_(logger),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      parasitics_(sta_->parasitics()),
      spef_files_(spef_files)
{
}

void SpefWriter::writeSpefHeader(Corner* corner)
{
  auto it = spef_files_.find(corner);
  if (it == spef_files_.end()) {
    return;
  }
  std::ostream& stream = *it->second;
  stream << "*SPEF \"ieee 1481-1999\"" << '\n';
  stream << "*DESIGN \"" << network_->block()->getName() << "\"" << '\n';
  stream << "*DATE \"11:11:11 Fri 11 11, 1111\"" << '\n';
  stream << "*VENDOR \"The OpenROAD Project\"" << '\n';
  stream << "*PROGRAM \"OpenROAD\"" << '\n';
  stream << "*VERSION \"1.0\"" << '\n';
  stream << "*DESIGN_FLOW \"NAME_SCOPE LOCAL\" \"PIN_CAP NONE\"" << '\n';
  stream << "*DIVIDER /" << '\n';
  stream << "*DELIMITER :" << '\n';
  stream << "*BUS_DELIMITER []" << '\n';

  auto units = network_->units();
  std::string time_unit = std::string(units->timeUnit()->suffix());
  std::string cap_unit = std::string(units->capacitanceUnit()->suffix());
  std::string res_unit = std::string(units->resistanceUnit()->suffix());
  std::transform(
      time_unit.begin(), time_unit.end(), time_unit.begin(), ::toupper);
  std::transform(cap_unit.begin(), cap_unit.end(), cap_unit.begin(), ::toupper);
  std::transform(res_unit.begin(), res_unit.end(), res_unit.begin(), ::toupper);

  stream << "*T_UNIT 1 " << time_unit << '\n';
  stream << "*C_UNIT 1 " << cap_unit << '\n';
  stream << "*R_UNIT 1 " << res_unit << '\n';
  stream << '\n';
}

void SpefWriter::writeSpefPorts(Corner* corner)
{
  auto it = spef_files_.find(corner);
  if (it == spef_files_.end()) {
    return;
  }
  std::ostream& stream = *it->second;

  auto pin_iter = network_->pinIterator(network_->topInstance());
  stream << "*PORTS" << '\n';
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    odb::dbITerm* iterm = nullptr;
    odb::dbBTerm* bterm = nullptr;
    odb::dbModITerm* moditerm = nullptr;
    odb::dbModBTerm* modbterm = nullptr;
    network_->staToDb(pin, iterm, bterm, moditerm, modbterm);

    if (iterm != nullptr) {
      stream << iterm->getName() << " ";
      if (iterm->getIoType() == odb::dbIoType::INPUT) {
        stream << "I";
      } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        stream << "O";
      } else {
        stream << "B";
      }
      stream << '\n';
    } else if (bterm != nullptr) {
      stream << bterm->getName() << " ";
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        stream << "I";
      } else if (bterm->getIoType() == odb::dbIoType::OUTPUT) {
        stream << "O";
      } else {
        stream << "B";
      }
      stream << '\n';
    } else {
      logger_->error(
          RSZ,
          7,
          "Got a modTerm instead of iTerm or bTerm while writing SPEF ports.");
    }
  }
  stream << '\n';

  delete pin_iter;
}

void SpefWriter::writeSpefNet(Corner* corner,
                              const Net* net,
                              Parasitic* parasitic)
{
  auto it = spef_files_.find(corner);
  if (it == spef_files_.end()) {
    return;
  }
  std::ostream& stream = *it->second;

  stream << "*D_NET " << network_->staToDb(net)->getName() << " ";
  stream << parasitics_->capacitance(parasitic) << '\n';

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
        stream << "*I " << parasitics_->name(node) << " ";
        if (iterm->getIoType() == odb::dbIoType::INPUT) {
          stream << "I";
        } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
          stream << "O";
        } else {
          stream << "B";
        }
        stream << " *D " << iterm->getInst()->getMaster()->getName();
        stream << '\n';
      } else if (bterm != nullptr) {
        stream << "*P " << parasitics_->name(node) << " ";
        if (bterm->getIoType() == odb::dbIoType::INPUT) {
          stream << "I";
        } else if (bterm->getIoType() == odb::dbIoType::OUTPUT) {
          stream << "O";
        } else {
          stream << "B";
        }
        stream << '\n';
      } else {
        logger_->error(RSZ,
                       8,
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
      stream << parasitics_->name(node) << " " << parasitics_->nodeGndCap(node);
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
    stream << parasitics_->name(n1) << " ";
    auto n2 = parasitics_->node2(cap);
    stream << parasitics_->name(n2) << " ";
    stream << parasitics_->value(cap) << '\n';
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
    stream << parasitics_->name(n1) << " ";
    auto n2 = parasitics_->node2(res);
    stream << parasitics_->name(n2) << " ";
    stream << parasitics_->value(res) << '\n';
  }

  stream << "*END" << '\n' << '\n';
}

}  // namespace rsz