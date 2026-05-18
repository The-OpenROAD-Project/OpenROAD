// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "syn/synthesis.h"
%}

%include "../../Exception.i"

%inline %{

namespace syn {

void
elaborate_cmd(const char* slang_args)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();

  // Split the argument string into individual args.
  std::vector<std::string> args;
  std::istringstream iss(slang_args);
  std::string token;
  while (iss >> token)
    args.push_back(token);

  synthesis->elaborate(args);
}

} // namespace syn

%} // inline
