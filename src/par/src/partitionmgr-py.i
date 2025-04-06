// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "par/PartitionMgr.h"
%}

%include "../../Exception-py.i"

%include <std_vector.i>
%include <std_string.i>

namespace std
{
  %template(vector_int) std::vector<int>;
  %template(vector_float) std::vector<float>;
}

%include "par/PartitionMgr.h"
