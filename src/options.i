// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

%{
#include <map>
#include <string>
%}

%include "std_string.i"
%include "std_map.i"

// Used to convert the keys & flags from tcl into a map:
//   swig_wrapper_cmd [array get keys] [array get flags]
%typemap(in) const std::map<std::string, std::string>& (std::map<std::string, std::string> temp) {
    int len;
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
    for (int i = 0; i < len; i += 2) {
        char *key = Tcl_GetString(elemObjv[i]);
        char *value = Tcl_GetString(elemObjv[i+1]);
        temp[key] = value;
    }

    $1 = &temp;
}

namespace std {
    %template(StringMap) map<string, string>;
}
