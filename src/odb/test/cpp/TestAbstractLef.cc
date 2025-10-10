// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <unistd.h>

#include <sstream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/lefout.h"
#include "tst/sky130_fixture.h"
#include "utl/Logger.h"

namespace odb {

using ::testing::HasSubstr;

using tst::Sky130Fixture;

TEST_F(Sky130Fixture, AbstractLefWriterMapsTieOffToSignal)
{
  // Arrange
  std::ostringstream os;
  odb::lefout lefout(&logger_, os);
  odb::dbNet* tieoff_net = odb::dbNet::create(block_, "tieoff_net");
  tieoff_net->setSigType(odb::dbSigType::TIEOFF);
  odb::dbBTerm::create(tieoff_net, "tieoff_pin");

  // Act
  lefout.writeAbstractLef(block_);
  std::string result = os.str();

  // Assert
  //  Not Found
  EXPECT_THAT(result, Not(HasSubstr("TIEOFF")));
  // Found
  EXPECT_THAT(result, HasSubstr("SIGNAL"));
}

TEST_F(Sky130Fixture, AbstractLefWriterMapsScanToSignal)
{
  // Arrange
  std::ostringstream os;
  odb::lefout lefout(&logger_, os);
  odb::dbNet* tieoff_net = odb::dbNet::create(block_, "scan_net");
  tieoff_net->setSigType(odb::dbSigType::SCAN);
  odb::dbBTerm::create(tieoff_net, "scan_pin");

  // Act
  lefout.writeAbstractLef(block_);
  std::string result = os.str();

  // Assert
  //  Not Found
  EXPECT_THAT(result, Not(HasSubstr("SCAN")));
  // Found
  EXPECT_THAT(result, HasSubstr("SIGNAL"));
}

TEST_F(Sky130Fixture, AbstractLefWriterMapsResetToSignal)
{
  // Arrange
  std::ostringstream os;
  odb::lefout lefout(&logger_, os);
  odb::dbNet* tieoff_net = odb::dbNet::create(block_, "reset_net");
  tieoff_net->setSigType(odb::dbSigType::RESET);
  odb::dbBTerm::create(tieoff_net, "reset_pin");

  // Act
  lefout.writeAbstractLef(block_);
  std::string result = os.str();

  // Assert
  //  Not Found
  EXPECT_THAT(result, Not(HasSubstr("RESET")));
  // Found
  EXPECT_THAT(result, HasSubstr("SIGNAL"));
}

}  // namespace odb
