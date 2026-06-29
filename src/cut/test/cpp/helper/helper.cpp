// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "helper.h"

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include "base/abc/abc.h"
#include "base/main/abcapis.h"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/lefin.h"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "sta/VerilogReader.hh"

namespace cut {

std::once_flag CutFixture::init_abc_flag{};
const std::string CutFixture::kPrefix = "_main/src/cut/test/";

void CutFixture::SetUp()
{
  std::call_once(init_abc_flag, []() { abc::Abc_Start(); });
  InitLibrary();

  sta::Units* units = library_->units();
  power_unit_ = units->powerUnit();
}

void CutFixture::InitLibrary()
{
  library_ = readLiberty(kPrefix + "Nangate45/Nangate45_typ.lib");

  odb::dbTech* tech
      = loadTechLef("nangate45", kPrefix + "Nangate45/Nangate45_tech.lef");
  loadLibaryLef(tech, "nangate45", kPrefix + "Nangate45/Nangate45_stdcell.lef");
}

void CutFixture::LoadVerilog(const std::string& file_name,
                             const std::string& top)
{
  // Assumes module name is "top" and clock name is "clk"
  sta::dbNetwork* network = sta_->getDbNetwork();
  ord::dbVerilogNetwork verilog_network(sta_.get());

  sta::VerilogReader verilog_reader(&verilog_network);
  verilog_reader.read(getFilePath(file_name).c_str());

  ord::dbLinkDesign(top.c_str(),
                    &verilog_network,
                    db_.get(),
                    &logger_,
                    /*hierarchy = */ false);

  sta_->postReadDb(db_.get());

  sta::Cell* top_cell = network->cell(network->topInstance());
  sta::Port* clk_port = network->findPort(top_cell, "clk");
  sta::Pin* clk_pin = network->findPin(network->topInstance(), clk_port);

  sta::PinSet pinset(network);
  pinset.insert(clk_pin);

  // 0.5ns
  double period = sta_->units()->timeUnit()->userToSta(0.5);
  sta::FloatSeq waveform;
  waveform.push_back(0);
  waveform.push_back(period / 2.0);

  sta_->makeClock("core_clock",
                  pinset,
                  /*add_to_pins=*/false,
                  /*period=*/period,
                  waveform,
                  /*comment=*/"",
                  /*mode=*/sta_->cmdMode());

  sta_->ensureGraph();
  sta_->ensureLevelized();
}

}  // namespace cut
