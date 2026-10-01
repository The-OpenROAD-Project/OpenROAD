// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors
#pragma once

#include <string>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "tst/loaded_design.h"
#include "utl/ServiceRegistry.h"

namespace tst {

class IntegratedFixture : public tst::Fixture
{
 public:
  // Kept as a member name so existing callers spelling
  // IntegratedFixture::Technology::kNangate45 still compile; the enum itself
  // now lives at namespace scope so LoadedDesign shares it.
  using Technology = tst::Technology;

  IntegratedFixture(Technology tech, const std::string& test_root_path);
  ~IntegratedFixture() override = default;

 protected:
  // `hierarchy` defaults to true, which is what this fixture has always done.
  void readVerilogAndSetup(const std::string& verilog_file,
                           bool init_default_sdc = true,
                           bool hierarchy = true);
  void initStaDefaultSdc();
  void dumpVerilogAndOdb(const std::string& name) const;
  void removeFile(const std::string& path);

  // Compare with verilog string input
  // 'remove_file=false' will keep the output verilog for debug
  void writeAndCompareVerilogOutputString(
      const std::string& test_name,
      const std::string& expected_verilog_content,
      bool remove_file = true);

  // Compare with golden verilog file
  void writeAndCompareVerilogOutputFile(const std::string& test_name,
                                        const std::string& golden_verilog_file,
                                        bool remove_file = true);

 protected:
  odb::dbLib* lib_{nullptr};
  odb::dbBlock* block_{nullptr};
  sta::dbNetwork* db_network_{nullptr};

  stt::SteinerTreeBuilder stt_;
  utl::ServiceRegistry service_registry_;
  dpl::Opendp dp_;
  ant::AntennaChecker ant_;
  grt::GlobalRouter grt_;
  est::EstimateParasitics ep_;
  rsz::Resizer resizer_;

  const std::string test_root_path_;
};

}  // namespace tst
