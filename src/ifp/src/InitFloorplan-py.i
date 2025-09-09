// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "ifp/InitFloorplan.hh"
%}

%include "../../Exception-py.i"

%include <std_string.i>
%include <std_set.i>
%include <std_vector.i>
%import "dbtypes.i"

%typemap(typecheck,precedence=SWIG_TYPECHECK_STRING) ifp::RowParity {
  const char *str = PyUnicode_AsUTF8($input);
  $1 = strcasecmp(str, "NONE") != 0 || strcasecmp(str, "EVEN") != 0 || strcasecmp(str, "ODD") != 0;
}

%typemap(in) ifp::RowParity {
  const char *str = PyUnicode_AsUTF8($input);
  if (strcasecmp(str, "NONE") == 0) {
    $1 = ifp::RowParity::NONE;
  } else if (strcasecmp(str, "EVEN") == 0) {
    $1 = ifp::RowParity::EVEN;
  } else if (strcasecmp(str, "ODD") == 0) {
    $1 = ifp::RowParity::ODD;
  } else {
    $1 = ifp::RowParity::NONE;
  }
}

// These are needed to coax swig into sending or returning Python 
// lists, arrays, or sets (as appropriate) of opaque pointers. Note 
// that you must %include (not %import) <std_vector.i> and <std_array.i> 
// before these definitions
namespace std {
  %template(site_list)    std::vector<odb::dbSite*>;
  %template(site_set)     std::set<odb::dbSite*>;
}

%include "ifp/InitFloorplan.hh"
