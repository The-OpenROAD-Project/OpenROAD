// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/synthesis.h"

#include <fnmatch.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "elab/driver.h"
#include "flow/constant_fold.h"
#include "flow/export.h"
#include "flow/import.h"
#include "flow/opt_gatefusion.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "syn/ir/Instance.h"
#include "utl/Logger.h"

namespace syn {

Synthesis::Synthesis(odb::dbDatabase* db,
                     sta::dbSta* sta,
                     rsz::Resizer* resizer,
                     utl::Logger* logger)
    : db_(db), resizer_(resizer), logger_(logger)
{
  dbStaState::init(sta);
}

Synthesis::~Synthesis() = default;

bool Synthesis::svElaborate(const std::vector<std::string>& args)
{
  auto result = syn::elaborate(logger_, args, sta_);
  if (!result) {
    logger_->error(utl::SYN, 1, "Elaboration failed.");
    return false;
  }
  graph_ = std::move(result);
  graph_->makeNamesTentative();
  graph_->normalize();
  syn::importTargets(*graph_, sta_->network(), logger_);
  graph_->normalize();

  constantFold(*graph_);
  graph_->normalize();

  return true;
}

void Synthesis::removePorts(const std::string& pattern)
{
  if (!graph_) {
    logger_->error(utl::SYN, 49, "No graph. Run syn::elaborate first.");
    return;
  }

  int removed = 0;
  std::vector<const Instance*> to_remove;
  graph_->forEachInstance([&](const Instance* inst) {
    const char* name = nullptr;
    if (auto* out = inst->try_as<Output>()) {
      name = out->name().c_str();
    } else if (auto* in = inst->try_as<Input>()) {
      name = in->name().c_str();
    }
    if (name && fnmatch(pattern.c_str(), name, 0) == 0) {
      to_remove.push_back(inst);
    }
  });
  for (auto* inst : to_remove) {
    graph_->removeInstance(const_cast<Instance*>(inst));
    removed++;
  }

  if (removed > 0) {
    graph_->normalize();
  }

  logger_->info(utl::SYN,
                55,
                "removePorts: removed {} ports matching '{}'",
                removed,
                pattern);
}

void Synthesis::parse(const std::string& filename)
{
  std::ifstream ifs(filename);
  if (!ifs) {
    logger_->error(
        utl::SYN, 42, "Cannot open file '{}' for reading.", filename);
    return;
  }
  graph_ = Graph::parse(ifs, sta_->network());
  graph_->setLogger(logger_);
  graph_->checkConsistency();
  syn::importTargets(*graph_, sta_->network(), logger_);
  logger_->info(utl::SYN, 43, "Loaded graph from '{}'.", filename);
}

void Synthesis::dump() const
{
  if (!graph_) {
    logger_->error(utl::SYN, 2, "No graph to dump. Run syn::elaborate first.");
    return;
  }
  graph_->dump(std::cout);
}

void Synthesis::dump(const std::string& filename) const
{
  if (!graph_) {
    logger_->error(utl::SYN, 51, "No graph to dump. Run syn::elaborate first.");
    return;
  }
  std::ofstream ofs(filename);
  if (!ofs) {
    logger_->error(
        utl::SYN, 25, "Cannot open file '{}' for writing.", filename);
    return;
  }
  graph_->dump(ofs);
}

Net Synthesis::resolveNetRef(const std::string& net_ref) const
{
  Net root = Net::sentinel();

  // Try numeric: strip leading '%' if present.
  std::string ref = net_ref;
  if (!ref.empty() && ref[0] == '%') {
    ref = ref.substr(1);
  }

  char* end = nullptr;
  uint64_t id = strtoul(ref.c_str(), &end, 10);
  if (end != ref.c_str() && *end == '\0' && id < graph_->tableSize()) {
    root = Graph::netFromId(id);
  } else {
    // Parse optional [offset] suffix (e.g. "data_out[5]").
    std::string lookup_name = net_ref;
    uint32_t bit_offset = 0;
    auto bracket = net_ref.rfind('[');
    if (bracket != std::string::npos && net_ref.back() == ']') {
      char* bend = nullptr;
      uint64_t off = strtoul(net_ref.c_str() + bracket + 1, &bend, 10);
      if (bend == net_ref.c_str() + net_ref.size() - 1) {
        lookup_name = net_ref.substr(0, bracket);
        bit_offset = off;
      }
    }

    // Look up by Name, mapping the HDL bit index through from/to.
    bool have_bracket = (lookup_name != net_ref);
    graph_->forEachInstance([&](const Instance* inst) {
      if (root != Net::sentinel()) {
        return;
      }
      if (auto* nm = inst->try_as<Name>()) {
        if (nm->nameStr() != lookup_name) {
          return;
        }
        if (!have_bracket) {
          if (nm->value().width() > 0) {
            root = nm->value()[0];
          }
          return;
        }
        // Map HDL bit index to bundle position.
        uint32_t from = nm->from();
        uint32_t to = nm->to();
        uint32_t pos;
        if (from <= to) {
          if (bit_offset >= from && bit_offset < to) {
            pos = bit_offset - from;
          } else {
            return;
          }
        } else {
          if (bit_offset <= from && bit_offset > to) {
            pos = from - bit_offset;
          } else {
            return;
          }
        }
        if (pos < nm->value().width()) {
          root = nm->value()[pos];
        }
      }
    });
  }

  return root;
}

void Synthesis::dumpFaninCone(const std::string& net_ref,
                              int max_depth,
                              bool stop_at_stateful) const
{
  if (!graph_) {
    logger_->error(utl::SYN, 47, "No graph. Run syn::elaborate first.");
    return;
  }

  Net root = resolveNetRef(net_ref);
  if (root == Net::sentinel()) {
    logger_->error(utl::SYN, 48, "Cannot find net '{}'.", net_ref);
    return;
  }

  graph_->dumpFaninCone(std::cout, root, max_depth, stop_at_stateful);
}

void Synthesis::dumpFanoutCone(const std::string& net_ref,
                               int max_depth,
                               bool stop_at_stateful) const
{
  if (!graph_) {
    logger_->error(utl::SYN, 53, "No graph. Run syn::elaborate first.");
    return;
  }

  Net root = resolveNetRef(net_ref);
  if (root == Net::sentinel()) {
    logger_->error(utl::SYN, 54, "Cannot find net '{}'.", net_ref);
    return;
  }

  graph_->dumpFanoutCone(std::cout, root, max_depth, stop_at_stateful);
}

void Synthesis::stats() const
{
  if (!graph_) {
    logger_->error(utl::SYN, 4, "No graph. Run syn::elaborate first.");
    return;
  }
  graph_->stats(std::cout);
}

void Synthesis::memoryUsage() const
{
  if (!graph_) {
    logger_->error(utl::SYN, 5, "No graph. Run syn::elaborate first.");
    return;
  }
  graph_->memoryUsage(std::cout);
}

void Synthesis::makeNamesTentative()
{
  if (!graph_) {
    logger_->error(utl::SYN, 3, "No graph. Run syn::elaborate first.");
    return;
  }
  graph_->makeNamesTentative();
}

void Synthesis::normalize()
{
  if (!graph_) {
    logger_->error(utl::SYN, 8, "No graph. Run syn::elaborate first.");
    return;
  }
  graph_->normalize();
}

bool Synthesis::dontUse(const sta::LibertyCell* cell) const
{
  return resizer_ && resizer_->dontUse(cell);
}

void Synthesis::mapSequentials()
{
  if (!graph_) {
    logger_->error(utl::SYN, 14, "No graph. Run syn::elaborate first.");
    return;
  }
  syn::mapSequentials(*graph_, sta_->network(), logger_, *this);
}

void Synthesis::importTargets()
{
  if (!graph_) {
    logger_->error(utl::SYN, 12, "No graph. Run syn::elaborate first.");
    return;
  }
  syn::importTargets(*graph_, sta_->network(), logger_);
}

void Synthesis::opt()
{
  if (!graph_) {
    logger_->error(utl::SYN, 9, "No graph. Run syn::elaborate first.");
    return;
  }
  constantFold(*graph_);
}

void Synthesis::bitblast(bool blast_arith)
{
  if (!graph_) {
    logger_->error(utl::SYN, 7, "No graph. Run syn::elaborate first.");
    return;
  }
  syn::bitblast(*graph_, blast_arith);
}

void Synthesis::mapCombinationals()
{
  if (!graph_) {
    logger_->error(utl::SYN, 15, "No graph. Run syn::elaborate first.");
    return;
  }
  syn::mapCombinationals(*graph_, sta_->network(), logger_, *this);
}

void Synthesis::abcRoundtrip(const std::string& commands)
{
  if (!graph_) {
    logger_->error(utl::SYN, 24, "No graph. Run syn::elaborate first.");
    return;
  }
  syn::abcRoundtrip(*graph_, commands, logger_);
}

void Synthesis::gateFuseOpt()
{
  if (!graph_) {
    logger_->error(utl::SYN, 52, "No graph. Run syn::elaborate first.");
    return;
  }
  syn::gateFusionOpt(*graph_, sta_->network(), logger_, *this);
}

void Synthesis::livenessOpt(bool replace_combinational)
{
  if (!graph_) {
    logger_->error(utl::SYN, 57, "No graph. Run syn::elaborate first.");
    return;
  }
  syn::livenessOpt(*graph_, logger_, replace_combinational);
}

void Synthesis::exportToOdb()
{
  if (!graph_) {
    logger_->error(utl::SYN, 32, "No graph. Run syn::elaborate first.");
    return;
  }
  syn::exportToOdb(*graph_, db_, sta_, logger_);
}

}  // namespace syn
