// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

%module pdngen
%{
#include "pdn/PdnGen.hh"
#include "odb/db.h"
#include <array>
#include <string>
#include <memory>
#include <vector>

using namespace pdn;
%}

%include <std_vector.i>
%include <std_array.i>
%include <std_map.i>
%include <std_pair.i>
%include <std_set.i>
%include <std_string.i>

%import "odb.i"
%clear int & x, int & y; // defined in dbtypes.i, must be cleared here.

// These are needed to coax swig into sending or returning Python 
// lists, arrays, or sets (as appropriate) of opaque pointers. Note 
// that you must %include (not %import) <std_vector.i> and <std_array.i> 
// before these definitions
namespace std {
  %template() std::pair<int, bool>;
#ifndef SWIG_VECTOR_INT
#define SWIG_VECTOR_INT
  %template(split_cuts_stuff) std::vector<int>;
#endif
  %template(stuff)        std::array<int, 4>;
  %template(split_map)    std::map<odb::dbTechLayer *, std::pair<int, bool>>;
  %template(grid_list)    std::vector<pdn::Grid *>;
  %template(domain_list)  std::vector<pdn::VoltageDomain *>;
  %template(net_list)     std::vector<odb::dbNet *>;
  %template(viagen_list)  std::vector<odb::dbTechViaGenerateRule *>;
  %template(techvia_list) std::vector<odb::dbTechVia *>;
  %template(layer_list)   std::vector<odb::dbTechLayer *>;
  %template(net_set)      std::set<odb::dbNet*>;
}

%include "../../Exception-py.i"
%include "pdn/PdnGen.hh"
