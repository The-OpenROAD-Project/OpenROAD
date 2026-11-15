// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/Pex.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "fsv/Itf.h"
#include "fsv/Stack.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "pex/Extractor.h"
#include "pex/Model.h"
#include "pex/ModelBuilder.h"
#include "pex/ModelJson.h"
#include "pex/SpefWriter.h"
#include "utl/Logger.h"

namespace pex {

namespace {

double secondsSince(const std::chrono::steady_clock::time_point start)
{
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
      .count();
}

}  // namespace

Pex::Pex(odb::dbDatabase* db, utl::Logger* logger) : db_(db), logger_(logger)
{
}

Pex::~Pex() = default;

odb::dbBlock* Pex::block() const
{
  odb::dbChip* chip = db_->getChip();
  if (chip == nullptr || chip->getBlock() == nullptr) {
    logger_->error(utl::PEX, 1, "No design is loaded.");
  }
  return chip->getBlock();
}

void Pex::buildModel(const std::string& itf,
                     const std::string& out,
                     const std::vector<std::string>& layers,
                     const double tol,
                     const int threads)
{
  fsv::Stack stack;
  try {
    stack = fsv::readItfFile(itf);
  } catch (const std::exception& e) {
    logger_->error(utl::PEX, 2, "Cannot read {}: {}", itf, e.what());
  }
  BuildOptions options;
  options.layers = layers;
  if (options.layers.empty()) {
    odb::dbTech* tech = db_->getTech();
    if (tech == nullptr) {
      logger_->error(
          utl::PEX, 3, "Give -layers, or read a technology LEF first.");
    }
    for (odb::dbTechLayer* layer : tech->getLayers()) {
      if (layer->getType() == odb::dbTechLayerType::ROUTING
          && stack.isConductor(stack.layer(layer->getName()))) {
        options.layers.push_back(layer->getName());
      }
    }
    if (options.layers.empty()) {
      logger_->error(utl::PEX,
                     40,
                     "No routing layer of the technology is a conductor of "
                     "{}; give -layers.",
                     itf);
    }
  }
  for (const std::string& name : options.layers) {
    if (!stack.isConductor(stack.layer(name))) {
      logger_->error(utl::PEX, 4, "{} has no conductor layer {}.", itf, name);
    }
  }
  options.tol = tol;
  options.threads = threads;
  const ModelSource source{
      .technology = stack.technology(),
      .file = itf,
      .fingerprint = buildFingerprint(stack, options),
      .stack = stackFingerprint(stack),
  };
  Model model;
  if (std::filesystem::exists(out)) {
    ModelSource held;
    try {
      model = readModelFile(out, &held);
    } catch (const std::exception& e) {
      logger_->error(utl::PEX, 5, "Cannot read {}: {}", out, e.what());
    }
    if (held.fingerprint != source.fingerprint) {
      logger_->error(utl::PEX,
                     6,
                     "{} was built from another stack or with other "
                     "settings; remove it or name another file.",
                     out);
    }
    logger_->info(utl::PEX, 7, "Resuming the model in {}.", out);
  }
  const auto start = std::chrono::steady_clock::now();
  try {
    for (const std::string& name : options.layers) {
      const auto layer_start = std::chrono::steady_clock::now();
      int solves = 0;
      options.progress = [&](int, int) { solves++; };
      const fsv::Stack::Layer layer = stack.layer(name);
      const std::optional<CoverPlan> held = heldCovers(stack, model, layer);
      buildLayer(stack,
                 layer,
                 held ? *held : planCovers(stack, layer, options),
                 options,
                 model);
      writeModelFile(model, source, out);
      logger_->info(utl::PEX,
                    8,
                    "Layer {}: {} solves in {:.0f} s.",
                    name,
                    solves,
                    secondsSince(layer_start));
    }
  } catch (const std::exception& e) {
    logger_->error(utl::PEX, 9, "Building the model failed: {}", e.what());
  }
  logger_->info(utl::PEX,
                10,
                "Wrote {}: {} lateral, {} end, {} crossing and {} diagonal "
                "tables in {:.0f} s.",
                out,
                model.laterals().size(),
                model.ends().size(),
                model.crossings().size(),
                model.diagonals().size(),
                secondsSince(start));
}

void Pex::readModel(const std::string& itf, const std::string& path)
{
  auto stack = std::make_unique<fsv::Stack>();
  auto model = std::make_unique<Model>();
  ModelSource source;
  try {
    *stack = fsv::readItfFile(itf);
  } catch (const std::exception& e) {
    logger_->error(utl::PEX, 11, "Cannot read {}: {}", itf, e.what());
  }
  try {
    *model = readModelFile(path, &source);
  } catch (const std::exception& e) {
    logger_->error(utl::PEX, 12, "Cannot read {}: {}", path, e.what());
  }
  if (source.stack.empty()) {
    logger_->warn(utl::PEX,
                  13,
                  "{} does not record its stack, so it cannot be checked "
                  "against {}.",
                  path,
                  itf);
  } else if (source.stack != stackFingerprint(*stack)) {
    logger_->error(utl::PEX,
                   14,
                   "{} was built from another stack than {} ({}).",
                   path,
                   itf,
                   source.file);
  }
  logger_->info(utl::PEX,
                15,
                "Read model {} of {}: {} lateral, {} end, {} crossing and {} "
                "diagonal tables.",
                path,
                source.technology,
                model->laterals().size(),
                model->ends().size(),
                model->crossings().size(),
                model->diagonals().size());
  stack_ = std::move(stack);
  model_ = std::move(model);
}

void Pex::extract(const ExtractOptions& options)
{
  if (model_ == nullptr) {
    logger_->error(utl::PEX, 16, "Read a model with read_pex_model first.");
  }
  odb::dbBlock* design = block();
  ExtractStats stats;
  try {
    Extractor extractor(design, *stack_, *model_, logger_, options);
    stats = extractor.extract();
  } catch (const std::exception& e) {
    logger_->error(utl::PEX, 17, "Extraction failed: {}", e.what());
  }
  logger_->info(utl::PEX,
                18,
                "Extracted {} nets from {} boxes: {} net pairs coupled, {} "
                "grounded below the thresholds.",
                stats.nets,
                stats.boxes,
                stats.coupled_pairs,
                stats.grounded_pairs);
  if (!stats.unknown_layers.empty()) {
    std::string layers;
    for (const std::string& layer : stats.unknown_layers) {
      layers += (layers.empty() ? "" : ", ") + layer;
    }
    logger_->warn(utl::PEX,
                  37,
                  "The stack has no conductor for routing layers {}; their "
                  "shapes are left out.",
                  layers);
  }
  if (stats.unreached > 0) {
    logger_->warn(utl::PEX,
                  38,
                  "{} pins are on no wire of their net and have no node in "
                  "its parasitics.",
                  stats.unreached);
  }
  if (stats.unplaced > 0) {
    logger_->warn(utl::PEX,
                  19,
                  "{} pin or special-wire shapes are on no node of their "
                  "net; their capacitance is on the net's first node.",
                  stats.unplaced);
  }
  if (stats.loops > 0) {
    logger_->warn(utl::PEX,
                  39,
                  "{} wire runs that start and end on the same node are left "
                  "out of the resistance.",
                  stats.loops);
  }
}

void Pex::writeSpef(const std::string& path,
                    const std::vector<std::string>& nets)
{
  odb::dbBlock* design = block();
  SpefOptions options;
  for (const std::string& name : nets) {
    odb::dbNet* net = design->findNet(name.c_str());
    if (net == nullptr) {
      logger_->error(utl::PEX, 20, "Net {} is not in the design.", name);
    }
    if (!net->isRCgraph()) {
      logger_->error(utl::PEX, 21, "Net {} has no parasitics.", name);
    }
    options.nets.push_back(net);
  }
  std::ofstream out(path);
  if (!out) {
    logger_->error(utl::PEX, 22, "Cannot write {}.", path);
  }
  pex::writeSpef(design, out, options);
  out.close();
  if (!out) {
    logger_->error(utl::PEX, 23, "Writing {} failed.", path);
  }
}

}  // namespace pex
