// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

TEST_F(SimpleDbFixture, test_add_find_remove)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();

  block->addCorner("corner1");
  block->addCorner("corner2");

  EXPECT_EQ(block->findCorner("corner1")->getName(), "corner1");
  EXPECT_EQ(block->findCorner("corner2")->getName(), "corner2");
  EXPECT_EQ(block->findCorner("missing"), nullptr);

  // Removing one corner leaves the others in place.
  block->removeCorner("corner1");
  EXPECT_EQ(block->findCorner("corner1"), nullptr);
  EXPECT_NE(block->findCorner("corner2"), nullptr);

  // removeCorners clears everything that remains.
  block->removeCorners();
  EXPECT_EQ(block->findCorner("corner2"), nullptr);
}

TEST_F(SimpleDbFixture, test_add_duplicate_errors)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();

  block->addCorner("corner1");
  // Adding a second corner with the same name is a fatal error.
  EXPECT_THROW(block->addCorner("corner1"), std::runtime_error);
}

TEST_F(SimpleDbFixture, test_add_empty_name_errors)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();

  // Empty name is not allowed.
  EXPECT_THROW(block->addCorner(""), std::runtime_error);
}

TEST_F(SimpleDbFixture, test_write_read_roundtrip)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();
  block->addCorner("corner1");
  block->addCorner("corner2");

  std::filesystem::create_directory("results");
  const std::string tmp_path = "results/TestCorner.odb";

  std::ofstream write_stream;
  write_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit
                          | std::ios::eofbit);
  write_stream.open(tmp_path, std::ios::binary);
  db_->write(write_stream);
  write_stream.close();

  dbDatabase* read_db = dbDatabase::create();
  std::ifstream read_stream;
  read_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit
                         | std::ios::eofbit);
  read_stream.open(tmp_path, std::ios::binary);
  read_db->read(read_stream);

  dbBlock* read_block = read_db->getChip()->getBlock();
  EXPECT_EQ(read_block->findCorner("corner1")->getName(), "corner1");
  EXPECT_EQ(read_block->findCorner("corner2")->getName(), "corner2");

  dbDatabase::destroy(read_db);
}

}  // namespace
}  // namespace odb
