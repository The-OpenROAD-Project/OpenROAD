// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Stubs for symbols that pdn's full library references (through SWIG-
// generated Tcl glue) but that aren't needed by these unit tests. Without
// these stubs the cc_test fails to link because pdn's shared library has
// undefined references to ord::OpenRoad::openRoad() and ord::getPdnGen().

#include "ord/OpenRoad.hh"

namespace pdn {
class PdnGen;
}

namespace ord {

OpenRoad::OpenRoad() = default;

OpenRoad* OpenRoad::openRoad()
{
  return nullptr;
}

pdn::PdnGen* getPdnGen()
{
  return nullptr;
}

}  // namespace ord
