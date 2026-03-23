// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors

#include "tst/IntegratedFixture.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Clock.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "sta/VerilogReader.hh"
#include "sta/VerilogWriter.hh"
#include "utl/Logger.h"

namespace tst {

IntegratedFixture::IntegratedFixture(Technology tech,
                                     const std::string& test_root_path)
    : stt_(db_.get(), &logger_),
      callback_handler_(&logger_),
      dp_(db_.get(), &logger_),
      ant_(db_.get(), &logger_),
      grt_(&logger_,
           &callback_handler_,
           &stt_,
           db_.get(),
           sta_.get(),
           &ant_,
           &dp_),
      ep_(&logger_, &callback_handler_, db_.get(), sta_.get(), &stt_, &grt_),
      resizer_(&logger_, db_.get(), sta_.get(), &stt_, &grt_, &dp_, &ep_),
      test_root_path_(test_root_path)
{
  switch (tech) {
    case Technology::Nangate45: {
      const std::string nangate45_tech_path = "_main/test/Nangate45/";
      readLiberty(getFilePath(nangate45_tech_path + "Nangate45_typ.lib"));
      lib_ = loadTechAndLib("Nangate45",
                            "Nangate45",
                            getFilePath(nangate45_tech_path + "Nangate45.lef"));
      break;
    }

    case Technology::Sky130hd: {
      const std::string sky130hd_tech_path = "_main/test/sky130hd/";
      readLiberty(getFilePath(sky130hd_tech_path + "sky130_fd_sc_hd_tt.lib"));
      lib_ = loadTechAndLib(
          "sky130",
          "sky130_fd_sc_hd",
          getFilePath(sky130hd_tech_path + "sky130_fd_sc_hd.tlef"));
      break;
    }
  }

  db_->setLogger(&logger_);
  db_network_ = sta_->getDbNetwork();
  db_network_->setHierarchy();
}

void IntegratedFixture::readVerilogAndSetup(const std::string& verilog_file,
                                            bool init_default_sdc)
{
  ord::dbVerilogNetwork verilog_network(sta_.get());
  sta::VerilogReader verilog_reader(&verilog_network);
  verilog_reader.read(
      getFilePath(test_root_path_ + "cpp/" + verilog_file).c_str());

  ord::dbLinkDesign(
      "top", &verilog_network, db_.get(), &logger_, true /*hierarchy = */);

  sta_->postReadDb(db_.get());

  block_ = db_->getChip()->getBlock();
  block_->setDefUnits(lib_->getTech()->getLefUnits());
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  sta_->postReadDef(block_);

  if (init_default_sdc) {
    initStaDefaultSdc();
  }
}

void IntegratedFixture::initStaDefaultSdc()
{
  // Timing setup
  sta::Cell* top_cell = db_network_->cell(db_network_->topInstance());
  ASSERT_NE(top_cell, nullptr);
  sta::Port* clk_port = db_network_->findPort(top_cell, "clk");
  if (clk_port != nullptr) {
    sta::Pin* clk_pin
        = db_network_->findPin(db_network_->topInstance(), clk_port);

    // STA frees the 'clk_pins' after use.
    // coverity[RESOURCE_LEAK: FALSE_POSITIVE]
    sta::PinSet* clk_pins = new sta::PinSet(db_network_);
    clk_pins->insert(clk_pin);

    // Clock period = 0.5ns
    double period = sta_->units()->timeUnit()->userToSta(0.5);
    // STA takes the ownership of 'waveform'.
    // coverity[RESOURCE_LEAK: FALSE_POSITIVE]
    sta::FloatSeq* waveform = new sta::FloatSeq;
    waveform->push_back(0);
    waveform->push_back(period / 2.0);

    sta_->makeClock("clk",
                    clk_pins,
                    /*add_to_pins=*/false,
                    /*period=*/period,
                    waveform,
                    /*comment=*/nullptr,
                    /*mode=*/sta_->cmdMode());

    sta::Sdc* sdc = sta_->cmdMode()->sdc();
    const sta::RiseFallBoth* rf = sta::RiseFallBoth::riseFall();
    sta::Clock* clk = sdc->findClock("clk");
    const sta::RiseFall* clk_rf = sta::RiseFall::rise();

    for (odb::dbBTerm* term : block_->getBTerms()) {
      sta::Pin* pin = db_network_->dbToSta(term);
      if (pin == nullptr) {
        continue;
      }
      if (sdc->isClock(pin)) {
        continue;
      }

      odb::dbIoType io_type = term->getIoType();
      if (io_type == odb::dbIoType::INPUT) {
        sta_->setInputDelay(pin,
                            rf,
                            clk,
                            clk_rf,
                            nullptr,
                            false,
                            false,
                            sta::MinMaxAll::all(),
                            true,
                            0.0,
                            sta_->cmdMode()->sdc());
      } else if (io_type == odb::dbIoType::OUTPUT) {
        sta_->setOutputDelay(pin,
                             rf,
                             clk,
                             clk_rf,
                             nullptr,
                             false,
                             false,
                             sta::MinMaxAll::all(),
                             true,
                             0.0,
                             sta_->cmdMode()->sdc());
      }
    }
  }

  sta_->ensureGraph();
  sta_->ensureLevelized();

  resizer_.initBlock();
  ep_.estimateWireParasitics();
}

void IntegratedFixture::dumpVerilogAndOdb(const std::string& name) const
{
  // Write verilog
  std::string vlog_file = name + ".v";
  sta::writeVerilog(vlog_file.c_str(), false, {}, sta_->network());

  // Dump ODB content
  std::ofstream orig_odb_file(name + "_odb.txt");
  block_->debugPrintContent(orig_odb_file);
  orig_odb_file.close();
}

void IntegratedFixture::removeFile(const std::string& path)
{
  if (std::remove(path.c_str()) != 0) {
    logger_.warn(utl::RSZ, 0, "Could not remove '{}'.", path);
  }
}

void IntegratedFixture::writeAndCompareVerilogOutputString(
    const std::string& test_name,
    const std::string& expected_verilog,
    bool remove_file)
{
  const std::string verilog_file = test_name + "_post.v";
  sta::writeVerilog(verilog_file.c_str(), false, {}, sta_->network());

  std::ifstream file(verilog_file);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  EXPECT_EQ(content, expected_verilog);

  if (remove_file) {
    removeFile(verilog_file);
  }
}

void IntegratedFixture::writeAndCompareVerilogOutputFile(
    const std::string& test_name,
    const std::string& golden_verilog_file,
    bool remove_file)
{
  const std::string golden_file
      = getFilePath(test_root_path_ + "cpp/" + golden_verilog_file);
  const std::string verilog_file = test_name + "_out.v";
  sta::writeVerilog(verilog_file.c_str(), false, {}, sta_->network());

  // Read new verilog content
  std::ifstream if_out(verilog_file);
  std::string content((std::istreambuf_iterator<char>(if_out)),
                      std::istreambuf_iterator<char>());

  // Read golden verilog content
  std::ifstream if_golden(golden_file);
  std::string golden_content((std::istreambuf_iterator<char>(if_golden)),
                             std::istreambuf_iterator<char>());

  std::cout << "--------------------------------------------------------\n";
  std::cout << "Compare " << verilog_file << " and " << golden_file << "\n";
  std::cout << "--------------------------------------------------------\n";
  EXPECT_EQ(content, golden_content);

  if (remove_file) {
    removeFile(verilog_file);
  }
}

}  // namespace tst
