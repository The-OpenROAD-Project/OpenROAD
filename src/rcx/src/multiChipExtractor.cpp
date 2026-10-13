// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/multiChipExtractor.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/wOrder.h"

namespace rcx {

MultiChipExtractor::MultiChipExtractor(odb::dbDatabase* db, utl::Logger* logger)
    : db_{db}, logger_{logger}
{
}

void MultiChipExtractor::run(const ExtractOptions& options)
{
  loadRules();

  for (odb::dbChip* chip : db_->getChips()) {
    if (chip->getChipType() == odb::dbChip::ChipType::DIE) {
      odb::orderWires(logger_, chip->getBlock());
      extractChipParasitics(chip, options);
    }
  }

  extractInterChipParasitics();
}

void MultiChipExtractor::setExtractionRulesFile(
    odb::dbTech* tech,
    const std::string& extraction_rules_file)
{
  extraction_rules_files_[tech] = extraction_rules_file;
}

void MultiChipExtractor::setAssemblyExtractionRulesFile(
    const std::string& assembly_extraction_rules_file)
{
  assembly_extraction_rules_file_ = assembly_extraction_rules_file;
}

void MultiChipExtractor::loadRules()
{
  if (assembly_extraction_rules_file_.empty()) {
    logger_->error(utl::RCX,
                   20,
                   "No assembly rules file defined; set one with "
                   "set_extraction_rules_file -assembly.");
  }

  // The rules parser requires an instantiated process corner table,
  // so we use an auxiliary extractor to generate it.
  auto auxiliary_extractor = std::make_unique<extMain>();
  auxiliary_extractor->init(db_, logger_);

  for (odb::dbChip* chip : db_->getChips()) {
    const odb::dbChip::ChipType type = chip->getChipType();

    if (type == odb::dbChip::ChipType::RDL) {
      logger_->error(
          utl::RCX,
          22,
          "RDL chips are not supported in this version of 3D extraction.");
    }

    if (type == odb::dbChip::ChipType::DIE) {
      auxiliary_extractor->setBlockFromChip(chip);
      auxiliary_extractor->addRCCorner(corner_name_.c_str(), corner_index_);

      odb::dbTech* tech = chip->getTech();

      if (tech_to_rules_model_.contains(tech)) {
        continue;
      }

      const std::string& rules_file = extraction_rules_files_.at(tech);
      const Array1D<extCorner*>* corner_table
          = auxiliary_extractor->getProcessCornerTable();

      std::unique_ptr<extRCModel> rules_model
          = parseRules(tech, rules_file, corner_table, false, logger_);

      tech_to_rules_model_[tech] = std::move(rules_model);
    }
  }

  inter_chip_model_
      = parseInterChipRules(assembly_extraction_rules_file_, logger_);

  // Avoid leaving the block pointing at the extractor we destroy.
  odb::dbBlock* block = auxiliary_extractor->getBlock();
  if (block) {
    block->setExtmi(nullptr);
  }
}

void MultiChipExtractor::extractChipParasitics(odb::dbChip* chip,
                                               const ExtractOptions& options)
{
  logger_->info(utl::RCX,
                23,
                "Running parasitics extraction for chip {}.",
                chip->getName());

  auto block_extractor = std::make_unique<extMain>();

  block_extractor->init(db_, logger_);
  block_extractor->setExtractionOptions(options);

  // The block must be set before adding the corner to it.
  block_extractor->setBlockFromChip(chip);
  block_extractor->addRCCorner(corner_name_.c_str(), corner_index_);

  // In a single-chip extraction, the extractor owns the model, so here
  // we ensure that the ownership is kept with the multi chip extractor
  // rather than the block extractor.
  block_extractor->setDeleteModelAtExtraction(false);

  const auto& rules_model = tech_to_rules_model_.at(chip->getTech());
  block_extractor->registerRulesModel(rules_model.get());

  block_extractor->run();

  // Avoid leaving the block pointing at the extractor we destroy.
  chip->getBlock()->setExtmi(nullptr);
}

void MultiChipExtractor::extractInterChipParasitics()
{
  odb::dbChip* top_chip = db_->getChip();

  int bond_count = 0;
  for (odb::dbChipNet* chip_net : top_chip->getChipNets()) {
    const uint32_t bump_count = chip_net->getNumBumpInsts();

    // Skip chip nets whose bump does not connect to a bump in the other chip.
    if (bump_count < 2) {
      continue;
    }

    if (bump_count > 2) {
      logger_->error(utl::RCX,
                     532,
                     "Inter-chip net {} bonds {} bumps; nets bonding more "
                     "than two bumps are not supported.",
                     chip_net->getName(),
                     bump_count);
    }

    // Model the bond as a resistor between a cap node at each bump landing.
    // Each cap node references its bump so the network stitches to that die's
    // net. For this initial version, we consider negligible capacitive
    // contribution from the bumps.
    std::vector<odb::dbChipInst*> path;
    odb::dbChipCapNode* source = odb::dbChipCapNode::create(chip_net);
    source->setChipBumpInst(chip_net->getBumpInst(0, path));

    odb::dbChipCapNode* target = odb::dbChipCapNode::create(chip_net);
    target->setChipBumpInst(chip_net->getBumpInst(1, path));

    odb::dbChipRSeg* r_seg = odb::dbChipRSeg::create(chip_net, source, target);
    r_seg->setResistance(inter_chip_model_.resistance);

    bond_count++;
  }

  logger_->info(utl::RCX, 533, "Extracted {} inter-chip bonds.", bond_count);
}

}  // namespace rcx
