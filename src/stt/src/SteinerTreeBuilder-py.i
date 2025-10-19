// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{

#include "stt/SteinerTreeBuilder.h"
#include "stt/pd.h"
#include "stt/flute.h"
#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include <vector>

%}

%include "../../Exception-py.i"

%include <std_vector.i>

namespace std {
%template(xy) vector<int>;
}

%include "stt/SteinerTreeBuilder.h"
%include "stt/flute.h"
%include "stt/pd.h"
