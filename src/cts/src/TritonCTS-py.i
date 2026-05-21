// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "src/cts/include/cts/TritonCTS.h"
#include "src/cts/src/CtsOptions.h"
#include "src/cts/src/TechChar.h"
#include "include/ord/OpenRoad.hh"

using namespace cts;
%}

%include "stdint.i"

%include "../../Exception-py.i"

%include <std_string.i>
%include <std_vector.i>

%ignore cts::CtsOptions::setObserver;
%ignore cts::CtsOptions::getObserver;

%include "CtsOptions.h"
%include "cts/TritonCTS.h"
