// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{

#include "dpo/Optdp.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

using namespace dpo;
%}

%include "../../Exception-py.i"

%import "odb.i"
%import "odb/dbTypes.h"

%include "Optdp.h"
