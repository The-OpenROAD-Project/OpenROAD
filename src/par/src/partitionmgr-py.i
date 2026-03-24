// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "par/PartitionMgr.h"
%}

%include "../../Exception-py.i"

#ifndef BAZEL
%include <std_vector.i>
%include <std_string.i>

namespace std
{
#ifndef SWIG_VECTOR_INT
#define SWIG_VECTOR_INT
  %template(vector_int) std::vector<int>;
#endif
  %template(vector_float) std::vector<float>;
}
#endif

%include "par/PartitionMgr.h"
