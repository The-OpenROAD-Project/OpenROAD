// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Tcl side of options structs: a Tcl dict becomes the dictionary a schema
// applies, and a dictionary becomes a Tcl dict.

#pragma once

#include <tcl.h>

#include <boost/json.hpp>
#include <functional>

#include "utl/Validate.h"

namespace utl {

class Logger;

// Converts a value in the user's units to the SI unit the C++ API takes.
using UnitScaler = std::function<double(Unit unit, double value)>;

// Converts a Tcl dict to a dictionary, guided by the schema: the value of a
// group is itself a dict, the value of a list field is a Tcl list, and any
// other value is kept as a string for the schema to convert.  A dotted key
// is resolved through the groups.  A value with a unit is scaled.  An
// unknown key is passed through so that the schema reports it.
boost::json::object tclToJson(Tcl_Interp* interp,
                              Tcl_Obj* dict,
                              const SchemaBase& schema,
                              const UnitScaler& scaler,
                              Logger* logger);

// Scales the string values of a flat dictionary of dotted keys (as
// legacyToJson builds) whose field has a unit, the way tclToJson does.
void scaleUnits(boost::json::object& options,
                const SchemaBase& schema,
                const UnitScaler& scaler);

// Converts a dictionary to a Tcl dict (nested dicts and lists).  The result
// has a zero reference count.
Tcl_Obj* jsonToTcl(Tcl_Interp* interp, const boost::json::value& value);

// The string form of jsonToTcl().
std::string jsonToTclString(Tcl_Interp* interp,
                            const boost::json::value& value);

}  // namespace utl
