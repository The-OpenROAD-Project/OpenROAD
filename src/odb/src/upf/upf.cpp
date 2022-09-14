/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "odb/upf.h"

#include <tcl.h>

#include "utl/Logger.h"

namespace upf {

bool create_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name)
{
  if (odb::dbPowerDomain::create(block, name) == nullptr) {
    logger->warn(utl::ODB, 10001, "Creation of '%s' power domain failed", name);
    return false;
  }

  return true;
}

bool update_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name,
                         const char* elements)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(name);
  if (pd != nullptr) {
    pd->addElement(std::string(elements));
  } else {
    logger->warn(
        utl::ODB,
        10002,
        "Couldn't retrieve power domain '%s' while adding element '%s'",
        name,
        elements);
    return false;
  }
  return true;
}

bool create_logic_port(utl::Logger* logger,
                       odb::dbBlock* block,
                       const char* name,
                       const char* direction)
{
  if (odb::dbLogicPort::create(block, name, std::string(direction))
      == nullptr) {
    logger->warn(utl::ODB, 10003, "Creation of '%s' logic port failed", name);
    return false;
  }
  return true;
}

bool create_power_switch(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name,
                         const char* power_domain,
                         const char* out_port,
                         const char* in_port)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain);
  if (pd == nullptr) {
    logger->warn(
        utl::ODB,
        10004,
        "Couldn't retrieve power domain '%s' while creating power switch '%s'",
        power_domain,
        name);
    return false;
  }

  odb::dbPowerSwitch* ps = odb::dbPowerSwitch::create(block, name);
  if (ps == nullptr) {
    logger->warn(utl::ODB, 10005, "Creation of '%s' power switch failed", name);
    return false;
  }

  ps->setInSupplyPort(std::string(in_port));
  ps->setOutSupplyPort(std::string(out_port));
  ps->setPowerDomain(pd);
  pd->addPowerSwitch(ps);
  return true;
}

bool update_power_switch_control(utl::Logger* logger,
                                 odb::dbBlock* block,
                                 const char* name,
                                 const char* control_port)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name);
  if (ps == nullptr) {
    logger->warn(
        utl::ODB,
        10006,
        "Couldn't retrieve power switch '%s' while adding control port '%s'",
        name,
        control_port);
    return false;
  }

  ps->addControlPort(std::string(control_port));
  return true;
}

bool update_power_switch_on(utl::Logger* logger,
                            odb::dbBlock* block,
                            const char* name,
                            const char* on_state)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name);
  if (ps == nullptr) {
    logger->warn(
        utl::ODB,
        10007,
        "Couldn't retrieve power switch '%s' while adding on state '%s'",
        name,
        on_state);
    return false;
  }
  ps->addOnState(on_state);
  return true;
}

bool set_isolation(utl::Logger* logger,
                   odb::dbBlock* block,
                   const char* name,
                   const char* power_domain,
                   bool update,
                   const char* applies_to,
                   const char* clamp_value,
                   const char* signal,
                   const char* sense,
                   const char* location)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain);
  if (pd == nullptr) {
    logger->warn(utl::ODB,
                 10008,
                 "Couldn't retrieve power domain '%s' while creating/updating "
                 "isolation '%s'",
                 power_domain,
                 name);
    return false;
  }

  odb::dbIsolation* iso = block->findIsolation(name);
  if (iso == nullptr && update) {
    logger->warn(
        utl::ODB, 10009, "Couldn't update a non existing isolation %s", name);
    return false;
  }

  if (iso == nullptr) {
    iso = odb::dbIsolation::create(block, name);
  }

  if (!update) {
    iso->setPowerDomain(pd);
    pd->addIsolation(iso);
  }

  if (strlen(applies_to) > 0) {
    iso->setAppliesTo(applies_to);
  }

  if (strlen(clamp_value) > 0) {
    iso->setClampValue(clamp_value);
  }

  if (strlen(signal) > 0) {
    iso->setIsolationSignal(signal);
  }

  if (strlen(sense) > 0) {
    iso->setIsolationSense(sense);
  }

  if (strlen(location) > 0) {
    iso->setLocation(location);
  }

  return true;
}

}  // namespace upf
