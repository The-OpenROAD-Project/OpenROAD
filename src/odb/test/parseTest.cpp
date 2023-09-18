///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#define BOOST_TEST_MODULE parse

#ifdef HAS_BOOST_UNIT_TEST_LIBRARY
// Shared library version
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#else
// Header only version
#include <boost/test/included/unit_test.hpp>
#endif

#include "odb/parse.h"
#include "utl/CFileUtils.h"
#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"

namespace odb {

// Note: this is an undefined symbol when we depend on the odb library alone,
// so we populate it with a dummy implementation.
int notice(int code, const char* msg, ...)
{
  abort();
}

BOOST_AUTO_TEST_CASE(parser_init_and_parse_line_with_integers)
{
  utl::Logger logger;
  Ath__parser parser(&logger);

  BOOST_TEST(parser.getLineNum() == 0);

  utl::ScopedTemporaryFile scoped_temp_file(&logger);
  const std::string kContents = "1 2 3 4";
  boost::span<const uint8_t> contents(
      reinterpret_cast<const uint8_t*>(kContents.data()), kContents.size());
  utl::WriteAll(scoped_temp_file.file(), contents, &logger);
  fseek(scoped_temp_file.file(), SEEK_SET, 0);

  parser.setInputFP(scoped_temp_file.file());
  BOOST_TEST(parser.readLineAndBreak() == 4);

  BOOST_TEST(parser.get(0) == "1");
  BOOST_TEST(parser.get(1) == "2");
  BOOST_TEST(parser.get(2) == "3");
  BOOST_TEST(parser.get(3) == "4");

  BOOST_TEST(parser.getInt(0) == 1);
  BOOST_TEST(parser.getInt(1) == 2);
  BOOST_TEST(parser.getInt(2) == 3);
  BOOST_TEST(parser.getInt(3) == 4);

  // If we don't reset this we get an error as it thinks it owns the `FILE*`
  // and can fclose it.
  parser.setInputFP(nullptr);
}

}  // namespace odb
