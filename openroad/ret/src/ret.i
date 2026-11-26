// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "ret/Retimer.h"
%}

%include "../../Exception.i"

%inline %{

namespace ret {

void
retime_cmd(int max_moves,
           double margin)
{
  ret::RetimeOptions options;
  options.max_moves = max_moves;
  options.margin = margin;
  ord::OpenRoad::openRoad()->getRetimer()->retime(options);
}

} // namespace ret

%} // inline
