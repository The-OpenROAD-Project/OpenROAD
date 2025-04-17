// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <string>

struct Tcl_Interp;

namespace utl {

std::string base64_decode(const std::string& encoded_string);
std::string base64_decode(const char* encoded_strings[]);

void evalTclInit(Tcl_Interp* interp, const char* inits[]);

}  // namespace utl
