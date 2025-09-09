// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <unistd.h>

#include <memory>
#include <sstream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/lefout.h"
#include "sky130_test_fixture.h"
#include "utl/Logger.h"

namespace odb {

using ::testing::HasSubstr;

TEST_F(Sky130TestFixutre, AbstractLefWriterMapsTieOffToSignal)
{
  // Arrange
  std::ostringstream os;
  odb::lefout lefout(&logger_, os);
  odb::dbNet* tieoff_net = odb::dbNet::create(block_.get(), "tieoff_net");
  tieoff_net->setSigType(odb::dbSigType::TIEOFF);
  odb::dbBTerm::create(tieoff_net, "tieoff_pin");

  // Act
  lefout.writeAbstractLef(block_.get());
  std::string result = os.str();

  // Assert
  //  Not Found
  EXPECT_THAT(result, Not(HasSubstr("TIEOFF")));
  // Found
  EXPECT_THAT(result, HasSubstr("SIGNAL"));
}

TEST_F(Sky130TestFixutre, AbstractLefWriterMapsScanToSignal)
{
  // Arrange
  std::ostringstream os;
  odb::lefout lefout(&logger_, os);
  odb::dbNet* tieoff_net = odb::dbNet::create(block_.get(), "scan_net");
  tieoff_net->setSigType(odb::dbSigType::SCAN);
  odb::dbBTerm::create(tieoff_net, "scan_pin");

  // Act
  lefout.writeAbstractLef(block_.get());
  std::string result = os.str();

  // Assert
  //  Not Found
  EXPECT_THAT(result, Not(HasSubstr("SCAN")));
  // Found
  EXPECT_THAT(result, HasSubstr("SIGNAL"));
}

TEST_F(Sky130TestFixutre, AbstractLefWriterMapsResetToSignal)
{
  // Arrange
  std::ostringstream os;
  odb::lefout lefout(&logger_, os);
  odb::dbNet* tieoff_net = odb::dbNet::create(block_.get(), "reset_net");
  tieoff_net->setSigType(odb::dbSigType::RESET);
  odb::dbBTerm::create(tieoff_net, "reset_pin");

  // Act
  lefout.writeAbstractLef(block_.get());
  std::string result = os.str();

  // Assert
  //  Not Found
  EXPECT_THAT(result, Not(HasSubstr("RESET")));
  // Found
  EXPECT_THAT(result, HasSubstr("SIGNAL"));
}

}  // namespace odb
