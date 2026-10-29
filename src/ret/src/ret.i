// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "ret/SequentialUnmapper.h"
%}

%include "../../Exception.i"

%inline %{

namespace ret {

bool
unmap_sequential_cells_cmd()
{
  ret::SequentialUnmapper* unmapper
      = ord::OpenRoad::openRoad()->getSequentialUnmapper();
  return unmapper->unmap();
}

} // namespace ret

%} // inline
