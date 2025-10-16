// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "grt/GlobalRouter.h"

using namespace grt;

%}

%include "../../Exception-py.i"

%include <std_string.i>

%ignore grt::GlobalRouter::init;
%ignore grt::GlobalRouter::initDebugFastRoute;
%ignore grt::GlobalRouter::getDebugFastRoute;
%ignore grt::GlobalRouter::setRenderer;
%ignore grt::GlobalRouter::initGui;

%include "grt/GlobalRouter.h"
