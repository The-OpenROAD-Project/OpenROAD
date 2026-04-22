// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%module syn_py

%{
#include "ord/OpenRoad.hh"
#include "syn/synthesis.h"

namespace ord {
// Defined in OpenRoad.i
odb::dbDatabase* getDb();
}

// Opaque forward decls so SWIG doesn't need to walk db_sta / odb headers for
// the Synthesis constructor parameters.
namespace odb { class dbDatabase; }
namespace sta { class dbSta; }
namespace utl { class Logger; }
%}

%include <std_string.i>
%include <std_vector.i>

%import "odb.i"  // Provides vector_str template for vector<std::string>.
%include "../../Exception-py.i"

// Hide the sta::dbStaState base class (SWIG would otherwise try to walk it).
%ignore sta::dbStaState;

// Skip the constructor and destructor -- callers retrieve the singleton via
// ord::OpenRoad::getSynthesis() rather than constructing new Synthesis objects.
%ignore syn::Synthesis::Synthesis;
%ignore syn::Synthesis::~Synthesis;
// graph() returns a Graph* whose header isn't wrapped; not needed from Python.
%ignore syn::Synthesis::graph;
%ignore syn::livenessOpt;
%ignore syn::abcRoundtrip;
// Free-function flow entry points reference Graph&, which is forward-declared
// only -- not needed from Python anyway, so ignore them.
%ignore syn::mapSequentials;
%ignore syn::abcRoundtrip;

%include "syn/synthesis.h"

%inline %{
namespace syn {
syn::Synthesis* get_synthesis()
{
  return ord::OpenRoad::openRoad()->getSynthesis();
}
}
%}
