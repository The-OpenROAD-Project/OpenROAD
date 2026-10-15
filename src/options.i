// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

// Tcl side of options structs (utl/Validate.h, utl/OptionsArgs.h).  A
// command's Tcl proc parses its own switches with sta::parse_key_args and
// passes the rest of $args to a %inline function that builds the struct
// with optionsFromArgs<S>().  The switches are generated from the struct's
// schema; the command's older switch names are registered as aliases.

%{
#include <map>
#include <string>
#include <vector>

#include "ord/OpenRoad.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
#include "utl/OptionsArgs.h"
#include "utl/OptionsRegistry.h"
#include "utl/OptionsTcl.h"
#include "utl/Validate.h"

#if TCL_MAJOR_VERSION < 9 && !defined(Tcl_Size)
  typedef int Tcl_Size;
#endif

// Converts a value in the user's units (set_cmd_units) to the SI unit the
// C++ API takes.
static utl::UnitScaler staUnitScaler()
{
  return [](const utl::Unit unit, const double value) {
    sta::Units* units = sta::Sta::sta()->units();
    switch (unit) {
      case utl::Unit::kDistance:
        return units->distanceUnit()->userToSta(value);
      case utl::Unit::kTime:
        return units->timeUnit()->userToSta(value);
      case utl::Unit::kCapacitance:
        return units->capacitanceUnit()->userToSta(value);
      case utl::Unit::kResistance:
        return units->resistanceUnit()->userToSta(value);
      case utl::Unit::kNone:
        break;
    }
    return value;
  };
}

// Builds the options struct of a registered command from the switches the
// Tcl proc left in $args.  The struct is validated.
template <typename S>
S optionsFromArgs(Tcl_Interp* interp, Tcl_Obj* args, const char* command)
{
  utl::Logger* logger = ord::OpenRoad::openRoad()->getLogger();
  const utl::OptionsEntry* entry = utl::OptionsRegistry::get().find(command);
  if (entry == nullptr || entry->schema != &S::schema()) {
    logger->error(utl::UTL, 112, "{} is not registered with its options.", command);
  }
  S result;
  S::schema().apply(
      result,
      utl::parseArgs(
          utl::argvFromTcl(interp, args, logger),
          S::schema(),
          entry->aliases,
          staUnitScaler(),
          utl::tclDictParser(interp, S::schema(), staUnitScaler(), logger),
          logger),
      logger);
  return result;
}

%}

%include "std_string.i"
%include "std_map.i"

// Used to convert the keys & flags from tcl into a map:
//   swig_wrapper_cmd [array get keys] [array get flags]
%typemap(in) const std::map<std::string, std::string>& (std::map<std::string, std::string> temp) {
    Tcl_Size len;
    Tcl_Obj **elemObjv;

    // 1. Check if input is a valid list
    if (Tcl_ListObjGetElements(interp, $input, &len, &elemObjv) == TCL_ERROR) {
        SWIG_fail;
    }

    // 2. Check for even number of elements (Key + Value)
    if (len % 2 != 0) {
        Tcl_SetResult(interp, (char*)"List must have even number of elements", TCL_STATIC);
        SWIG_fail;
    }

    // 3. Iterate and populate the temp map
    for (Tcl_Size i = 0; i < len; i += 2) {
        char *key = Tcl_GetString(elemObjv[i]);
        char *value = Tcl_GetString(elemObjv[i+1]);
        temp[key] = value;
    }

    $1 = &temp;
}

namespace std {
    %template(StringMap) map<string, string>;
}

// A %inline function parameter named `interp` is filled in by SWIG and is
// not an argument of the Tcl command.
%typemap(in, numinputs=0) Tcl_Interp* interp {
  $1 = interp;
}
