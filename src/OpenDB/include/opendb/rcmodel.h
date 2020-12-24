///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#pragma once

#include "odb.h"
#include "arnoldi1.h"

namespace odb {

//
// This is the rcmodel, without Rd.
// n is the number of terms
// The vectors U[j] are of size n
//
class rcmodel : public arnoldi1
{
 public:
  void**   itermV;  // [n]
  void**   btermV;  // [n]
  rcmodel* rise_cap_model;
  rcmodel* next;

 public:
  rcmodel()
  {
    itermV         = NULL;
    btermV         = NULL;
    rise_cap_model = NULL;
    next           = NULL;
  }
  ~rcmodel() {}
};

//
// rcmodel for a simple line
//
rcmodel* get_line_rcmodel(int n, double* r, double* c, int max_order, int jout);

//
// use this with get_line_rcmodel
//
void rcmodelDestroy(rcmodel*);

//
// get total cap from rcmodel
//
double rcmodel_ctot(rcmodel* mod);

//
// get max elmore from rcmodel
// this is not the whole elmore delay,
// does not include Rdrive*ctot
//
double rcmodel_max_elmore(rcmodel* mod);

}  // namespace odb

