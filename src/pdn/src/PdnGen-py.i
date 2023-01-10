/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, The Regents of the University of California
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
%module pdngen
%{

// These includes are needed by the SWIG generated CXX files mostly 
// for determining the size of Classes that they define. Since that are
// not actually wrapped, the classes defined in these files will appear as
// opaque pointers in Python.
#include "../src/connect.h"
#include "../src/domain.h"
#include "../src/grid.h"
#include "../src/power_cells.h"
#include "../src/renderer.h"

#include "pdn/PdnGen.hh"
#include "odb/db.h"
#include <array>
#include <string>
#include <memory>
#include <vector>

using pdn::ExtensionMode;
using namespace pdn;
 
%}

%include <std_vector.i>
%include <std_array.i>
%include <std_map.i>

// this maps unsigned long to the enum ExtensionMode
%include typemaps.i
%apply unsigned long { ExtensionMode };

%import "odb.i"
%import "dbtypes.i"
%include <std_string.i>

// These are needed to coax swig into sending or returning Python 
// lists, arrays, or sets (as appropriate) of opaque pointers. Note 
// that you must %include (not %import) <std_vector.i> and <std_array.i> 
// before these definitions
namespace std {
  %template(split_cuts_stuff) std::vector<int>;
  %template(stuff)        std::array<int, 4>;
  %template(split_map)    std::map<odb::dbTechLayer *, int>;  
  %template(grid_list)    std::vector<pdn::Grid *>;
  %template(domain_list)  std::vector<pdn::VoltageDomain *>;
  %template(net_list)     std::vector<odb::dbNet *>;
  %template(viagen_list)  std::vector<odb::dbTechViaGenerateRule *>;
  %template(techvia_list) std::vector<odb::dbTechVia *>;
  %template(layer_list)   std::vector<odb::dbTechLayer *>;
}

%include "../../Exception-py.i"
%include "pdn/PdnGen.hh"

%inline %{

namespace pdn {

// difficult to pass a set. repair_pdn_via takes a vector and then
// rebuilds the set in C++, so we just borrow that here.
void repair_pdn_vias(const std::vector<odb::dbNet*>& nets);
 
}  // namespace

%} //  %inline
