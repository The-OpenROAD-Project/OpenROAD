// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

// Tcl side of options structs (utl/Validate.h).  A command passes its
// `-options` dict plus its legacy keys and flags to a %inline function that
// builds the struct with optionsFromTcl<S>().

%{
#include <map>
#include <string>

#include "ord/OpenRoad.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
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

// Builds an options struct from a command invocation: the -options dict
// first, then the legacy switches (which override it).  The struct is
// validated.
template <typename S>
S optionsFromTcl(Tcl_Interp* interp,
                 Tcl_Obj* options,
                 const std::map<std::string, std::string>& keys,
                 const std::map<std::string, std::string>& flags,
                 const utl::LegacyArgs& legacy)
{
  utl::Logger* logger = ord::OpenRoad::openRoad()->getLogger();
  S result;
  if (options != nullptr) {
    S::schema().applyObject(
        result,
        utl::tclToJson(interp, options, S::schema(), staUnitScaler(), logger),
        "",
        logger);
  }
  boost::json::object legacy_options = utl::legacyToJson(keys, flags, legacy);
  utl::scaleUnits(legacy_options, S::schema(), staUnitScaler());
  S::schema().applyObject(result, legacy_options, "", logger);
  S::schema().enforce(result, logger);
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
