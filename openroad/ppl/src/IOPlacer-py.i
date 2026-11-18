// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "ppl/IOPlacer.h"
#include "ord/OpenRoad.hh"
#include "odb/db.h"

using std::set;
using ppl::PinSet;    
using namespace ppl;

template <class TYPE>
set<TYPE> *
PyListSet(PyObject *const source,
          swig_type_info *swig_type)
{
  int sz;
  if (PyList_Check(source) && PyList_Size(source) > 0) {
    sz = PyList_Size(source);
    set<TYPE> *seq = new set<TYPE>;
    for (int i = 0; i < sz; i++) {
      void *obj;
      SWIG_ConvertPtr(PyList_GetItem(source, i), &obj, swig_type, false);
      seq->insert(reinterpret_cast<TYPE>(obj));
    }
    return seq;
  }
  else
    return nullptr;
}

%}

%typemap(in) ppl::PinSet* {
  $1 = PyListSet<odb::dbBTerm*>($input, SWIGTYPE_p_odb__dbBTerm);
}

%include "../../Exception-py.i"

%import "odb.i"

%ignore ppl::IOPlacer::getRenderer;
%ignore ppl::IOPlacer::setRenderer;

%include "ppl/Parameters.h"
%include "ppl/IOPlacer.h"
