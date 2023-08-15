/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

%{
#include "ppl/IOPlacer.h"
#include "ord/OpenRoad.hh"
#include "odb/odb.h"
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
