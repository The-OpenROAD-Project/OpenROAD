// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "fin/Finale.h"

%}

%include "../../Exception.i"

%inline %{

void
set_density_fill_debug_cmd()
{
  auto *finale = ord::OpenRoad::openRoad()->getFinale();
  finale->setDebug();
}

void
density_fill_cmd(const char* rules_filename,
                 const odb::Rect& fill_area)
{
  auto *finale = ord::OpenRoad::openRoad()->getFinale();
  finale->densityFill(rules_filename, fill_area);
}

%} // inline

