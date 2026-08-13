// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "cts/TritonCTS.h"
#include "TechChar.h"
#include "ord/OpenRoad.hh"

using namespace cts;
%}

%include "stdint.i"

%include "../../Exception-py.i"

%include <std_string.i>
%include <std_vector.i>

%ignore cts::TritonCTS::setObserver;
%ignore cts::TritonCTS::getObserver;

%include "cts/TritonCTS.h"
