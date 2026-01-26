// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#ifdef BAZEL
%module(package="src.gpl") gpl
#else
%module gpl
#endif

%{
#include "gpl/Replace.h"
#include "odb/db.h"

namespace ord {
gpl::Replace*
getReplace();

}

using ord::getReplace;
using gpl::Replace;

%}

%include "typemaps.i"
%include "std_vector.i"
%include "../../Exception-py.i"
%include "gpl/Replace.h"

namespace std {
#ifndef SWIG_VECTOR_INT
#define SWIG_VECTOR_INT
    %template(IntVector) vector<int>;
#endif
}
