// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

%{

#include "dpo/Optdp.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

%}

%include "../../Exception.i"

%inline %{
  namespace dpo {

  void improve_placement_cmd(int seed,
                             int max_displacement_x,
                             int max_displacement_y)
  {
    dpo::Optdp* optdp = ord::OpenRoad::openRoad()->getOptdp();
    optdp->improvePlacement(
        seed, max_displacement_x, max_displacement_y);
  }

  }  // namespace dpo

%}  // inline
