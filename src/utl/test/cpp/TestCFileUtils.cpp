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

#include <filesystem>
#include <numeric>

#include "gtest/gtest.h"
#include "utl/CFileUtils.h"
#include "utl/ScopedTemporaryFile.h"

namespace utl {

TEST(Utl, read_all_of_empty_file)
{
  Logger logger;
  ScopedTemporaryFile stf(&logger);
  std::string contents = GetContents(stf.file(), &logger);
  EXPECT_TRUE(contents.empty());
}

// Writes then reads 4B of data.
TEST(Utl, read_all_of_written_file_seek_required)
{
  Logger logger;
  ScopedTemporaryFile stf(&logger);

  const std::vector<uint8_t> kTestData = {0x01, 0x02, 0x03, 0x04};
  WriteAll(stf.file(), kTestData, &logger);

  std::string contents = GetContents(stf.file(), &logger);
  EXPECT_EQ(contents.size(), kTestData.size());
  for (size_t i = 0; i < contents.size(); ++i) {
    EXPECT_EQ(static_cast<uint8_t>(contents.at(i)), kTestData.at(i));
  }
}

// Writes then reads 1024B of data.
TEST(Utl, read_all_of_file_exactly_1024B)
{
  Logger logger;
  ScopedTemporaryFile stf(&logger);

  std::vector<uint8_t> test_data(1024);
  std::iota(test_data.begin(), test_data.end(), 0);

  WriteAll(stf.file(), test_data, &logger);

  std::string contents = GetContents(stf.file(), &logger);
  EXPECT_EQ(contents.size(), test_data.size());
  for (size_t i = 0; i < contents.size(); ++i) {
    EXPECT_EQ(static_cast<uint8_t>(contents.at(i)), test_data.at(i));
  }
}

// Writes then reads 1025B of data (whitebox test, we know internally the read
// buffer size is 1024B so this causes two chunks of read).
TEST(Utl, read_all_of_file_exactly_1025B)
{
  Logger logger;
  ScopedTemporaryFile stf(&logger);

  std::vector<uint8_t> test_data(1025);
  std::iota(test_data.begin(), test_data.end(), 0);

  WriteAll(stf.file(), test_data, &logger);

  std::string contents = GetContents(stf.file(), &logger);
  EXPECT_EQ(contents.size(), test_data.size());
  for (size_t i = 0; i < contents.size(); ++i) {
    EXPECT_EQ(static_cast<uint8_t>(contents.at(i)), test_data.at(i));
  }
}

// Add new tests for StreamHandler
TEST(Utl, stream_handler_write_and_read)
{
  const char* filename = "test_write_and_read.txt";
  const std::string kTestData = "\x1\x2\x3\x4";

  {
    StreamHandler sh(filename);
    std::ofstream& os = sh.getStream();
    os.write(kTestData.c_str(), kTestData.size());
  }

  std::ifstream is(filename, std::ios_base::binary);
  std::string contents((std::istreambuf_iterator<char>(is)),
                       std::istreambuf_iterator<char>());
  EXPECT_EQ(contents, kTestData);
  std::filesystem::remove(filename);
}

TEST(Utl, stream_handler_temp_file_handling)
{
  const char* filename = "test_temp_file_handling.txt";
  std::string tmp_filename = std::string(filename) + ".1";

  // Check that the temp file is created
  {
    StreamHandler sh(filename);
    EXPECT_TRUE(std::filesystem::exists(tmp_filename));
  }

  // Check that the temp file is renamed to the original filename
  EXPECT_TRUE(!std::filesystem::exists(tmp_filename));
  EXPECT_TRUE(std::filesystem::exists(filename));
  std::filesystem::remove(filename);
}

TEST(Utl, stream_handler_exception_handling)
{
  const char* filename = "test_exception_handling.txt";

  // Ensure the temporary file is handled correctly if an exception occurs
  try {
    StreamHandler sh(filename);
    throw std::runtime_error("Simulated exception");
  } catch (...) {
    std::string tmp_filename = std::string(filename) + ".1";
    // Ensure temporary file is cleaned up
    EXPECT_TRUE(!std::filesystem::exists(tmp_filename));
    // Original file should exist
    EXPECT_TRUE(std::filesystem::exists(filename));
    std::filesystem::remove(filename);
  }
}

// Add new tests for FileHandler
TEST(Utl, file_handler_write_and_read)
{
  const char* filename = "test_write_and_read_file.txt";
  const std::string kTestData = "\x1\x2\x3\x4";
  {
    FileHandler fh(filename, true);  // binary mode
    FILE* file = fh.getFile();
    fwrite(kTestData.c_str(), sizeof(uint8_t), kTestData.size(), file);
  }

  std::ifstream is(filename, std::ios_base::binary);
  std::string contents((std::istreambuf_iterator<char>(is)),
                       std::istreambuf_iterator<char>());
  EXPECT_EQ(contents, kTestData);
  std::filesystem::remove(filename);
}

TEST(Utl, file_handler_temp_file_handling)
{
  const char* filename = "test_temp_file_handling_file.txt";
  std::string tmp_filename = std::string(filename) + ".1";

  // Check that the temp file is created
  {
    FileHandler fh(filename, true);  // binary mode
    EXPECT_TRUE(std::filesystem::exists(tmp_filename));
  }

  // Check that the temp file is renamed to the original filename
  EXPECT_TRUE(!std::filesystem::exists(tmp_filename));
  EXPECT_TRUE(std::filesystem::exists(filename));
  std::filesystem::remove(filename);
}

TEST(Utl, file_handler_exception_handling)
{
  const char* filename = "test_exception_handling_file.txt";

  // Ensure the temporary file is handled correctly if an exception occurs
  try {
    FileHandler fh(filename, true);  // binary mode
    throw std::runtime_error("Simulated exception");
  } catch (...) {
    std::string tmp_filename = std::string(filename) + ".1";
    // Ensure temporary file is cleaned up
    EXPECT_TRUE(!std::filesystem::exists(tmp_filename));
    // Original file should exist
    EXPECT_TRUE(std::filesystem::exists(filename));
    std::filesystem::remove(filename);
  }
}

}  // namespace utl
