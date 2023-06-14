// Copyright 2023 Google LLC

// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <unistd.h>

#include <memory>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/lefin.h"
#include "sky130_test_fixture.h"
#include "utl/Logger.h"

namespace odb {

TEST_F(OdbSky130TestFixture, CanParseTieOffPins)
{
  // Arrange
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  lef_reader.updateLib(lib_.get(), "data/sky130hd/sky130_pin_use.lef");

  // Act & Assert
  odb::dbMaster* standard_cell
      = lib_->findMaster("tieoff_scan_reset_standard_cell");
  odb::dbMTerm* tieoff_pin = standard_cell->findMTerm("TIEOFFPIN");
  EXPECT_EQ(tieoff_pin->getSigType(), odb::dbSigType::TIEOFF);
}

TEST_F(OdbSky130TestFixture, CanParseScanPins)
{
  // Arrange
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  lef_reader.updateLib(lib_.get(), "data/sky130hd/sky130_pin_use.lef");

  // Act & Assert
  odb::dbMaster* standard_cell
      = lib_->findMaster("tieoff_scan_reset_standard_cell");
  odb::dbMTerm* tieoff_pin = standard_cell->findMTerm("SCANPIN");
  EXPECT_EQ(tieoff_pin->getSigType(), odb::dbSigType::SCAN);
}

TEST_F(OdbSky130TestFixture, CanParseResetPins)
{
  // Arrange
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  lef_reader.updateLib(lib_.get(), "data/sky130hd/sky130_pin_use.lef");

  // Act & Assert
  odb::dbMaster* standard_cell
      = lib_->findMaster("tieoff_scan_reset_standard_cell");
  odb::dbMTerm* tieoff_pin = standard_cell->findMTerm("RESETPIN");
  EXPECT_EQ(tieoff_pin->getSigType(), odb::dbSigType::RESET);
}
}  // namespace odb