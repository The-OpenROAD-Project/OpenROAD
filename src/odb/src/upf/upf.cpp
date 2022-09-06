/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

namespace odb {

bool upf::create_power_domain(utl::Logger* logger,
                              dbBlock* block,
                              const char* name)
{
  return dbPowerDomain::create(block, name) != nullptr;
}

bool upf::update_power_domain(utl::Logger* logger,
                              dbBlock* block,
                              const char* name,
                              const char* elements)
{
  dbPowerDomain* pd = block->findPowerDomain(name);
  if(pd != nullptr) {
    pd->addElement(elements);
  } else {
    return false;
  }
  return true;
}

bool upf::create_logic_port(utl::Logger* logger,
                            dbBlock* block,
                            const char* name,
                            const char* direction)
{
  return dbLogicPort::create(block, name, direction) != nullptr;
}

bool upf::create_power_switch(utl::Logger* logger,
                              dbBlock* block,
                              const char* name,
                              const char* power_domain,
                              const char* out_port,
                              const char* in_port)
{
    
  dbPowerDomain* pd = block->findPowerDomain(power_domain);
  if(pd == nullptr) return false;
  dbPowerSwitch* ps = dbPowerSwitch::create(block, name);
  if (ps == nullptr) return false;
  ps->setInSupplyPort(in_port);
  ps->setOutSupplyPort(out_port);
  ps->setPowerDomain(pd);
  pd->setPowerSwitch(ps);
  return true;
}

bool upf::update_power_switch_control(utl::Logger* logger,
                                      dbBlock* block,
                                      const char* name,
                                      const char* control_port)
{
  dbPowerSwitch* ps = block->findPowerSwitch(name);
  if(ps == nullptr) return false;
  ps->addControlPort(control_port);
  return true;
}

bool upf::update_power_switch_on(utl::Logger* logger,
                                 dbBlock* block,
                                 const char* name,
                                 const char* on_state)
{
  dbPowerSwitch* ps = block->findPowerSwitch(name);
  if(ps == nullptr) return false;
  ps->addOnState(on_state);
  return true;
}

bool upf::set_isolation(utl::Logger* logger,
                        dbBlock* block,
                        const char* name,
                        const char* power_domain,
                        bool update,
                        const char* applies_to,
                        const char* clamp_value,
                        const char* signal,
                        const char* sense,
                        const char* location)
{
  dbIsolation* iso = block->findIsolation(name);
  if(iso == nullptr && update) return false;
  if(iso == nullptr) {
    iso = dbIsolation::create(block, name);
  }

  dbPowerDomain* pd = block->findPowerDomain(power_domain);
  if(pd != nullptr) {
    iso->setPowerDomain(pd);
    pd->setIsolation(iso);
  }

  if(strlen(applies_to) > 0) {
    iso->setAppliesTo(applies_to);
  }

  if(strlen(clamp_value) > 0) {
    iso->setClampValue(clamp_value);
  }

  if(strlen(signal) > 0) {
    iso->setIsolationSignal(signal);
  }

  if(strlen(sense) > 0) {
    iso->setIsolationSense(sense);
  }

  if(strlen(location) > 0) {
    iso->setLocation(location);
  }

  return true;
}

}  // namespace odb
