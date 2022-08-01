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

#include "utl/Logger.h"

namespace odb {

bool upf::create_power_domain(utl::Logger* logger,
                              dbBlock* block,
                              const char* name,
                              const char* elements)
{
  dbModInst* inst = block->findModInst(elements);
  std::vector<dbModInst*> els{inst};
  return dbPowerDomain::create(block, name, els) != nullptr;
}

bool upf::create_logic_port(utl::Logger* logger,
                            dbBlock* block,
                            const char* name,
                            const char* direction)
{
  // TODO: determine what structure will hold this information
  return true;
}

bool upf::create_power_switch(utl::Logger* logger,
                              dbBlock* block,
                              const char* name,
                              const char* power_domain,
                              const char* out_port,
                              const char* in_port,
                              const char* control_port)
{
    
    dbPowerDomain* pd = block->findPowerDomain(power_domain);
    if(pd == nullptr) return false;
    dbPowerSwitch* ps =  dbPowerSwitch::create(block, name);
    if(ps == nullptr) return false;
    ps->setInSupplyPort(std::string(in_port));
    ps->setOutSupplyPort(std::string(out_port));
    ps->setPowerDomain(pd);
    ps->setControlPort(std::string(control_port));

    return true;
}

bool upf::set_isolation(utl::Logger* logger,
                        dbBlock* block,
                        const char* name,
                        const char* power_domain,
                        const char* applies_to,
                        const char* clamp_value,
                        const char* signal,
                        const char* sense,
                        const char* location)
{
  return true;
}

}  // namespace odb
