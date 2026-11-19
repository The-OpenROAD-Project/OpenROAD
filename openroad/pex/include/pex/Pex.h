// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "pex/ExtractOptions.h"

namespace fsv {
class Stack;
}  // namespace fsv

namespace odb {
class dbBlock;
class dbDatabase;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace pex {

class Model;

// The state behind the pex commands: a model with the ITF stack it was
// built from, and the extraction of the design into ODB. Errors are
// reported through the logger.
class Pex
{
 public:
  Pex(odb::dbDatabase* db, utl::Logger* logger);
  ~Pex();

  // Builds a model of `layers` (the design's routing layers when empty)
  // from an ITF stack into `out`, saving it after each layer; a model
  // already in `out` that was built with the same stack and settings is
  // finished rather than rebuilt.
  void buildModel(const std::string& itf,
                  const std::string& out,
                  const std::vector<std::string>& layers,
                  double tol,
                  int threads);
  // Reads a model and the ITF it was built from, which must be the same
  // stack.
  void readModel(const std::string& itf, const std::string& path);
  // Extracts the design's signal nets into ODB, replacing its parasitics.
  void extract(const ExtractOptions& options);
  // Writes the parasitics in ODB as SPEF: `nets`, or every extracted net.
  void writeSpef(const std::string& path, const std::vector<std::string>& nets);
  // Reports the extracted totals and couplings of `names`, or of `sample`
  // nets taken through the block's net order (sampleNets), against the
  // field solver's.
  void compare(const std::vector<std::string>& names,
               int sample,
               double halo,
               double tol,
               int threads);

 private:
  odb::dbBlock* block() const;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  std::unique_ptr<fsv::Stack> stack_;
  std::unique_ptr<Model> model_;
};

}  // namespace pex
