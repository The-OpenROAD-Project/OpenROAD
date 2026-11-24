// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <sstream>
#include <string>

#include "db/obj/frBPin.h"
#include "db/obj/frBTerm.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frMTerm.h"
#include "db/obj/frShape.h"
#include "drt-global.h"
#include "gtest/gtest.h"

namespace drt {
namespace {

// The connectivity checker prints a pin it could not reach through the
// frBlockObject stream operator, so a block or master term has to print as
// itself there rather than send the operator back to itself.
std::string print(const frBlockObject& obj)
{
  std::ostringstream os;
  os << obj;
  return os.str();
}

TEST(TermPrintTest, BlockTermPrintsItsName)
{
  const frBTerm term("in_a");
  EXPECT_NE(print(term).find("in_a"), std::string::npos);
}

TEST(TermPrintTest, BlockTermPrintsItsPins)
{
  frBTerm term("in_a");
  auto pin = std::make_unique<frBPin>();
  pin->addPinFig(std::make_unique<frRect>(0, 0, 10, 10, 1, nullptr));
  term.addPin(std::move(pin));
  const std::string out = print(term);
  EXPECT_NE(out.find("PIN (NAME) in_a"), std::string::npos);
  EXPECT_EQ(out.find("UNKNOWN"), std::string::npos);
}

TEST(TermPrintTest, MasterTermPrintsItsName)
{
  const frMTerm term("A1");
  EXPECT_NE(print(term).find("A1"), std::string::npos);
}

}  // namespace
}  // namespace drt
