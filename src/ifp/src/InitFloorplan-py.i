/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
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
