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

#pragma once

#include <stdio.h>
#include <unistd.h>

#include <list>
#include <map>
#include <string>
#include <vector>

#include "db.h"
#include "odb.h"

namespace upf {

bool create_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name);

bool update_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name,
                         const char* element);

bool create_logic_port(utl::Logger* logger,
                       odb::dbBlock* block,
                       const char* name,
                       const char* direction);

bool create_power_switch(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name,
                         const char* power_domain,
                         const char* out_port,
                         const char* in_port);

bool update_power_switch_control(utl::Logger* logger,
                                 odb::dbBlock* block,
                                 const char* name,
                                 const char* control_port);

bool update_power_switch_on(utl::Logger* logger,
                            odb::dbBlock* block,
                            const char* name,
                            const char* on_state);

bool set_isolation(utl::Logger* logger,
                   odb::dbBlock* block,
                   const char* name,
                   const char* power_domain,
                   bool update,
                   const char* applies_to,
                   const char* clamp_value,
                   const char* signal,
                   const char* sense,
                   const char* location);

bool use_interface_cell(utl::Logger* logger,
                        odb::dbBlock* block,
                        const char* power_domain,
                        const char* strategy,
                        const char* cell);

bool set_domain_area(utl::Logger* logger,
                     odb::dbBlock* block,
                     char* domain,
                     float x1,
                     float y1,
                     float x2,
                     float y2);

bool eval_upf(utl::Logger* logger, odb::dbBlock* block);

}  // namespace upf
