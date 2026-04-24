// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <mutex>
#include <string>

#include "tst/fixture.h"

namespace sta {
class Unit;
class LibertyLibrary;
};  // namespace sta

namespace cut {

class CutFixture : public tst::Fixture
{
  static std::once_flag init_abc_flag;

 protected:
  static const std::string kPrefix;
  void SetUp() override;
  void LoadVerilog(const std::string& file_name,
                   const std::string& top = "top");
  virtual void InitLibrary();

  sta::Unit* power_unit_;
  sta::LibertyLibrary* library_;
};

}  // namespace cut
