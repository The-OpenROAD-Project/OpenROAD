// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/multiChipSpefWriter.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "odb/db.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

namespace {

std::string stripSuffix(const std::string& target, const std::string& suffix)
{
  if (target.ends_with(suffix)) {
    return target.substr(0, target.size() - suffix.size());
  }

  return target;
}

odb::dbITerm* findBumpTerminal(odb::dbChipCapNode* cap_node,
                               utl::Logger* logger)
{
  odb::dbITerm* bump_terminal = nullptr;
  odb::dbChipBump* chip_bump = cap_node->getChipBumpInst()->getChipBump();
  odb::dbNet* net = chip_bump->getNet();

  if (net) {
    for (odb::dbITerm* iterm : chip_bump->getInst()->getITerms()) {
      if (iterm->getNet() == net) {
        bump_terminal = iterm;
        break;
      }
    }
  }

  if (!bump_terminal) {
    logger->error(utl::RCX,
                  535,
                  "Inter-chip net {} lands on a bump with no terminal; "
                  "cannot write its SPEF node.",
                  cap_node->getChipNet()->getName());
  }

  return bump_terminal;
}

char pinDirection(odb::dbITerm* iterm)
{
  char direction = 'B';

  switch (iterm->getIoType().getValue()) {
    case odb::dbIoType::INPUT:
      direction = 'I';
      break;
    case odb::dbIoType::OUTPUT:
      direction = 'O';
      break;
    case odb::dbIoType::INOUT:
    case odb::dbIoType::FEEDTHRU:
      break;
  }

  return direction;
}

}  // namespace

MultiChipSpefWriter::MultiChipSpefWriter(odb::dbDatabase* db,
                                         utl::Logger* logger,
                                         const std::string& spef_version)
    : db_{db}, logger_{logger}
{
  spef_header_.version = spef_version;
}

void MultiChipSpefWriter::run(const SpefOptions& options)
{
  file_base_name_ = stripSuffix(options.file, ".spef");

  setUnits(options);

  for (odb::dbChip* chip : db_->getChips()) {
    if (chip->getChipType() == odb::dbChip::ChipType::DIE) {
      writeChipSpef(chip, options);
    }
  }

  writeInterChipSpef();
}

void MultiChipSpefWriter::setUnits(const SpefOptions& options)
{
  spef_header_.capacitance_unit_word = options.cap_units;
  spef_header_.resistance_unit_word = options.res_units;

  scale_factors_ = extSpef::computeScaleFactors(
      spef_header_.capacitance_unit_word, spef_header_.resistance_unit_word);
}

void MultiChipSpefWriter::writeChipSpef(odb::dbChip* chip,
                                        const SpefOptions& options)
{
  const std::string file_path
      = file_base_name_ + "." + chip->getName() + ".spef";

  // The block spef writer was originally conceived to live within the block
  // extractor, that's why we use the latter here. In the future, we can
  // modify the writer so that it does not require an extractor.
  auto block_extractor = std::make_unique<extMain>();
  block_extractor->init(db_, logger_);
  block_extractor->setBlockFromChip(chip);

  block_extractor->writeSPEF((char*) file_path.c_str(),
                             (char*) options.nets,
                             options.no_name_map,
                             (char*) options.N,
                             options.term_junction_xy,
                             options.cap_units,
                             options.res_units,
                             options.gz,
                             options.stop_after_map,
                             options.w_clock,
                             options.w_conn,
                             options.w_cap,
                             options.w_cc_cap,
                             options.w_res,
                             options.no_c_num,
                             false,
                             options.single_pi,
                             options.no_backslash,
                             options.corner,
                             options.ext_corner_name,
                             spef_header_.version.c_str(),
                             options.parallel);

  logger_->info(utl::RCX,
                534,
                "Wrote parasitics for chip {} to {}.",
                chip->getName(),
                file_path);
}

std::string MultiChipSpefWriter::bondNodeName(odb::dbChipCapNode* cap_node)
{
  odb::dbChipNet* chip_net = cap_node->getChipNet();
  odb::dbChipBumpInst* bump_inst = cap_node->getChipBumpInst();

  std::vector<odb::dbChipInst*> chip_inst_path;
  for (uint32_t i = 0; i < chip_net->getNumBumpInsts(); ++i) {
    std::vector<odb::dbChipInst*> candidate_path;
    if (chip_net->getBumpInst(i, candidate_path) == bump_inst) {
      chip_inst_path = candidate_path;
      break;
    }
  }

  odb::dbITerm* iterm = findBumpTerminal(cap_node, logger_);

  std::string bond_node_name;
  for (odb::dbChipInst* chip_inst : chip_inst_path) {
    if (!bond_node_name.empty()) {
      bond_node_name += '/';
    }

    bond_node_name += chip_inst->getName();
  }

  bond_node_name += '/';
  bond_node_name += iterm->getName(':');

  return bond_node_name;
}

void MultiChipSpefWriter::writeInterChipSpef()
{
  const std::string file_path = file_base_name_ + ".bonds.spef";
  std::ofstream out(file_path);

  if (!out) {
    logger_->error(utl::RCX,
                   536,
                   "Can't open file {} to write inter-chip spef.",
                   file_path);
  }

  odb::dbChip* top_chip = db_->getChip();
  spef_header_.design_name = top_chip->getName();

  out << spef_header_.string(logger_);

  int bond_count = 0;
  for (odb::dbChipNet* chip_net : top_chip->getChipNets()) {
    const std::string chip_net_spef = chipNetSpefString(chip_net);

    if (!chip_net_spef.empty()) {
      out << chip_net_spef;
      bond_count++;
    }
  }

  logger_->info(
      utl::RCX, 537, "Wrote {} inter-chip bonds to {}.", bond_count, file_path);
}

std::string MultiChipSpefWriter::chipNetSpefString(odb::dbChipNet* chip_net)
{
  std::ostringstream out;
  odb::dbSet<odb::dbChipRSeg> rsegs = chip_net->getChipRSegs();

  if (!rsegs.empty()) {
    out << "\n*D_NET " << chip_net->getName() << " "
        << chip_net->getTotalCapacitance() * scale_factors_.capacitance << "\n";

    out << "*CONN\n";
    for (odb::dbChipCapNode* cap_node : chip_net->getChipCapNodes()) {
      odb::dbITerm* bump_terminal = findBumpTerminal(cap_node, logger_);

      out << "*I " << bondNodeName(cap_node) << " "
          << pinDirection(bump_terminal) << "\n";
    }

    out << "*RES\n";
    int res_id = 1;
    for (odb::dbChipRSeg* rseg : rsegs) {
      out << res_id++ << " " << bondNodeName(rseg->getSourceCapNode()) << " "
          << bondNodeName(rseg->getTargetCapNode()) << " "
          << rseg->getResistance() * scale_factors_.resistance << "\n";
    }

    out << "*END\n";
  }

  return out.str();
}

}  // namespace rcx
