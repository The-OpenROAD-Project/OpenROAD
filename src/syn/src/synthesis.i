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
sv_elaborate_cmd(const char* slang_args)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();

  // TODO: remove and accept list of arguments directly
  // Split the argument string into individual args.
  std::vector<std::string> args;
  std::istringstream iss(slang_args);
  std::string token;
  while (iss >> token)
    args.push_back(token);

  synthesis->svElaborate(args);
}

void
remove_ports_cmd(const char* pattern)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->removePorts(pattern);
}

void
parse_cmd(const char* filename)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->parse(filename);
}

void
dump_fanin_cone_cmd(const char* net_ref, int max_depth, bool stop_at_stateful)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->dumpFaninCone(net_ref, max_depth, stop_at_stateful);
}

void
dump_fanout_cone_cmd(const char* net_ref, int max_depth, bool stop_at_stateful)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->dumpFanoutCone(net_ref, max_depth, stop_at_stateful);
}

void
dump_cmd(const char* filename = nullptr)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  if (filename && filename[0])
    synthesis->dump(filename);
  else
    synthesis->dump();
}

void
stats_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->stats();
}

void
memory_usage_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->memoryUsage();
}

void
make_names_tentative_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->makeNamesTentative();
}

void
map_sequentials_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->mapSequentials();
}

void
import_targets_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->importTargets();
}

void
normalize_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->normalize();
}

void
opt_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->opt();
}

void
bitblast_cmd(bool blast_arith=true)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->bitblast(blast_arith);
}

void
map_combinationals_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->mapCombinationals();
}

void
abc_roundtrip_cmd(const char* commands)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->abcRoundtrip(commands);
}

void
gate_fuse_opt_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->gateFuseOpt();
}

void
liveness_opt_cmd(bool replace_combinational = false)
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->livenessOpt(replace_combinational);
}

void
export_to_odb_cmd()
{
  syn::Synthesis* synthesis = ord::OpenRoad::openRoad()->getSynthesis();
  synthesis->exportToOdb();
}

} // namespace syn

%} // inline
