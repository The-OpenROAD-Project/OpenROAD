// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%module utl_py

%{

#include "utl/Logger.h"
#include "LoggerCommon.h"

namespace ord {
// Defined in OpenRoad.i
utl::Logger *
getLogger();
}

using utl::ToolId;
using utl::Logger;
using ord::getLogger;

using namespace utl;

%}

#define __attribute__(x)

// this maps ToolId to unsigned long
%include typemaps.i
%apply unsigned long { ToolId };

%include "../../Exception-py.i"

%ignore utl::Logger::progress;
%ignore utl::Logger::swapProgress;

%include "utl/Logger.h"
%include "LoggerCommon.h"
