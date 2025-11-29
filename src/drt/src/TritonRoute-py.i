// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%ignore drt::TritonRoute::init;

%{

#include "ord/OpenRoad.hh"
#include "triton_route/TritonRoute.h"
#include "utl/Logger.h"
%}

%ignore drt::TritonRoute::initGraphics;

%include <std_string.i>
%include <std_vector.i>

%template(ApAbsEdges) std::vector<drt::trApAbsoluteEdge>;
%template(StrVec) std::vector<std::string>;
%template(RectVec) std::vector<odb::Rect>;

%include "../../Exception-py.i"
%include "triton_route/TritonRoute.h"

