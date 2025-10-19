// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#ifdef BAZEL
%module(package="src.utl") utl
#else
%module utl_py
#endif

#ifdef BAZEL
%{
#include "utl/Logger.h"

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
#else
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
#endif

#define __attribute__(x)

// this maps ToolId to unsigned long
%include typemaps.i
%apply unsigned long { ToolId };

%include "../../Exception-py.i"
%include "stdint.i"

%ignore utl::Logger::progress;
%ignore utl::Logger::swapProgress;

%include "utl/Logger.h"
#ifndef BAZEL
%include "LoggerCommon.h"
#endif