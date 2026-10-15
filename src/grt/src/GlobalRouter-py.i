// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "grt/GlobalRouteOptions.h"
#include "grt/GlobalRouter.h"

using namespace grt;

%}

%include "../../Exception-py.i"
%include "../../options-py.i"

%include <std_string.i>

%ignore grt::GlobalRouter::init;
%ignore grt::GlobalRouter::initDebugFastRoute;
%ignore grt::GlobalRouter::getDebugFastRoute;
%ignore grt::GlobalRouter::initDebugCugr;
%ignore grt::GlobalRouter::getDebugCugr;
%ignore grt::GlobalRouter::setRenderer;
%ignore grt::GlobalRouter::initGui;

%include "grt/GlobalRouteOptions.h"
%options_struct(grt::GlobalRouteOptions, GlobalRouteOptions)

%include "grt/GlobalRouter.h"
