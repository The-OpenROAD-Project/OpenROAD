// BSD 3-Clause License

// Copyright (c) 2019, SCALE Lab, Brown University
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.

// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.

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

#ifndef __PSN_EXPORTS__
#define __PSN_EXPORTS__
#include <OpenPhySyn/DatabaseHandler.hpp>
#include <OpenPhySyn/SteinerTree.hpp>
#include <OpenPhySyn/Types.hpp>

namespace psn
{
void  version();
void  print_version();
void  help();
void  print_usage();
void  print_transforms();
void  print_license();
float design_area();
bool  has_transform(const char* transform_name);
int   set_wire_res_cap(float res_per_micon, float cap_per_micron);
int   set_wire_res_cap(const char* layer_name);
int   set_max_area(float area);
int   set_log(const char* level);
int   set_log_level(const char* level);
int   set_log_pattern(const char* pattern);
int   transform_internal(std::string              transform_name,
                         std::vector<std::string> args);

DatabaseHandler& get_handler();
DatabaseHandler& get_database_handler();
Database&        get_database();
SteinerTree*     make_steiner_tree(const char* pin_name);
SteinerTree*     make_steiner_tree(Net* net);
} // namespace psn

#endif
