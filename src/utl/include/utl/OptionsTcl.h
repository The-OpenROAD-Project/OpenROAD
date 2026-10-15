// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Tcl side of options structs: the switches a command received as the argv
// list parseArgs() takes, a Tcl dict as the dictionary a schema applies
// (the value of -options), and a dictionary back as a Tcl dict.

#pragma once

#include <tcl.h>

#include <boost/json.hpp>
#include <string>
#include <vector>

#include "utl/OptionsArgs.h"
#include "utl/Validate.h"

namespace utl {

class Logger;

// The elements of a Tcl list as strings.
std::vector<std::string> argvFromTcl(Tcl_Interp* interp,
                                     Tcl_Obj* list,
                                     Logger* logger);

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

// tclToJson as the parser parseArgs() applies to the value of -options.
DictParser tclDictParser(Tcl_Interp* interp,
                         const SchemaBase& schema,
                         const UnitScaler& scaler,
                         Logger* logger);

// Converts a dictionary to a Tcl dict (nested dicts and lists).  The result
// has a zero reference count.
Tcl_Obj* jsonToTcl(Tcl_Interp* interp, const boost::json::value& value);

// The string form of jsonToTcl().
std::string jsonToTclString(Tcl_Interp* interp,
                            const boost::json::value& value);

}  // namespace utl
