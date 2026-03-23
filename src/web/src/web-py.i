// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%module web_py

%{
#include "web/web.h"
#include "ord/OpenRoad.hh"
%}

%include "../../Exception-py.i"

%include "web/web.h"
