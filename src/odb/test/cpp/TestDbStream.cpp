// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include "odb/dbStream.h"
#include "tst/db_fixture.h"

namespace odb {
namespace {

/// Test fixture for dbStream unit tests.
/// Inherits from tst::DbFixture to get a database instance.
class DbStreamTest : public tst::DbFixture
{
};

/// Tests serialization and deserialization of basic types.
/// This verifies the C++20 template overloads for arithmetic types,
/// the string_view overload for writing, and direct-buffer reading for strings.
TEST_F(DbStreamTest, BasicTypes)
{
  std::stringstream ss;
  dbOStream out(reinterpret_cast<_dbDatabase*>(getDb()), ss);

  int32_t i32 = -12345;
  uint32_t u32 = 12345U;
  double d = 3.14159;
  bool b1 = true;
  bool b2 = false;
  std::string s = "hello";
  std::string_view sv = "world";

  out << i32;
  out << u32;
  out << d;
  out << b1;
  out << b2;
  out << s;
  out << sv;
  out.flush();

  dbIStream in(reinterpret_cast<_dbDatabase*>(getDb()), ss);
  int32_t i32_in;
  uint32_t u32_in;
  double d_in;
  bool b1_in;
  bool b2_in;
  std::string s_in;
  std::string sv_in;

  in >> i32_in;
  in >> u32_in;
  in >> d_in;
  in >> b1_in;
  in >> b2_in;
  in >> s_in;
  in >> sv_in;

  EXPECT_EQ(i32, i32_in);
  EXPECT_EQ(u32, u32_in);
  EXPECT_DOUBLE_EQ(d, d_in);
  EXPECT_EQ(b1, b1_in);
  EXPECT_EQ(b2, b2_in);
  EXPECT_EQ(s, s_in);
  EXPECT_EQ(std::string(sv), sv_in);
}

/// Tests the buffering and flushing behavior of dbOStream.
/// Verifies that:
/// 1. Writes are buffered and not immediately flushed.
/// 2. Destructor automatically flushes the buffer.
/// 3. Explicit flush() works.
/// 4. Overflowing the 64KB buffer triggers an automatic flush.
TEST_F(DbStreamTest, BufferFlushing)
{
  // Test destructor flush
  {
    std::stringstream ss;
    {
      dbOStream out(reinterpret_cast<_dbDatabase*>(getDb()), ss);
      int32_t val = 42;
      out << val;
      // Should not have flushed yet (buffer size is 64KB, val is 4 bytes)
      EXPECT_EQ(ss.str().size(), 0);
    }
    // Destructor called, should have flushed.
    EXPECT_GT(ss.str().size(), 0);
  }

  // Test explicit flush
  {
    std::stringstream ss;
    dbOStream out(reinterpret_cast<_dbDatabase*>(getDb()), ss);
    int32_t val = 42;
    out << val;
    EXPECT_EQ(ss.str().size(), 0);
    out.flush();
    EXPECT_GT(ss.str().size(), 0);
  }

  // Test overflow with single large block (direct write)
  {
    std::stringstream ss;
    dbOStream out(reinterpret_cast<_dbDatabase*>(getDb()), ss);
    std::vector<char> large_data(70000, 'a');
    out.writeBytes(large_data);
    EXPECT_EQ(ss.str().size(), 70000);
  }

  // Test overflow with sequential small blocks (buffered copy after flush)
  {
    std::stringstream ss;
    dbOStream out(reinterpret_cast<_dbDatabase*>(getDb()), ss);
    std::vector<char> chunk1(60000, 'a');
    std::vector<char> chunk2(10000, 'b');
    out.writeBytes(chunk1);
    EXPECT_EQ(ss.str().size(), 0);  // Still buffered
    out.writeBytes(
        chunk2);  // Triggers flush of chunk1, then copies chunk2 to buffer
    EXPECT_EQ(ss.str().size(), 60000);  // chunk1 flushed
    out.flush();                        // Flush chunk2
    EXPECT_EQ(ss.str().size(), 70000);
  }

  // Test large trivially copyable struct (>64KB) with writeValueAsBytes
  {
    struct LargeBlob
    {
      char data[70000];
    };
    static_assert(std::is_trivially_copyable_v<LargeBlob>);

    std::stringstream ss;
    dbOStream out(reinterpret_cast<_dbDatabase*>(getDb()), ss);
    LargeBlob blob;
    std::memset(blob.data, 'x', sizeof(blob.data));
    out.writeValueAsBytes(blob);
    EXPECT_EQ(ss.str().size(), sizeof(LargeBlob));
  }
}

/// Tests the pos() method of dbOStream.
/// Verifies that calling pos() flushes the buffer and returns correct offset.
TEST_F(DbStreamTest, Position)
{
  std::stringstream ss;
  dbOStream out(reinterpret_cast<_dbDatabase*>(getDb()), ss);
  EXPECT_EQ(out.pos(), 0);
  int32_t val = 42;
  out << val;
  // out.pos() should flush and return 4
  EXPECT_EQ(out.pos(), 4);
  EXPECT_EQ(ss.str().size(), 4);  // pos() should have flushed
}

/// Tests serialization and deserialization of container types (vector, map,
/// tuple, variant). This ensures that the modernized basic stream operators
/// work correctly when nested inside standard container serializers.
TEST_F(DbStreamTest, ContainerTypes)
{
  std::stringstream ss;
  dbOStream out(reinterpret_cast<_dbDatabase*>(getDb()), ss);

  std::vector<int> vec = {1, 2, 3, 4, 5};
  std::map<std::string, double> map = {{"apple", 1.23}, {"banana", 4.56}};
  std::tuple<int, double, std::string> tuple = {42, 3.14, "hello"};
  std::variant<int, double, std::string> variant1 = 42;
  std::variant<int, double, std::string> variant2 = "world";

  out << vec;
  out << map;
  out << tuple;
  out << variant1;
  out << variant2;
  out.flush();

  dbIStream in(reinterpret_cast<_dbDatabase*>(getDb()), ss);
  std::vector<int> vec_in;
  std::map<std::string, double> map_in;
  std::tuple<int, double, std::string> tuple_in;
  std::variant<int, double, std::string> variant1_in;
  std::variant<int, double, std::string> variant2_in;

  in >> vec_in;
  in >> map_in;
  in >> tuple_in;
  in >> variant1_in;
  in >> variant2_in;

  EXPECT_EQ(vec, vec_in);
  EXPECT_EQ(map, map_in);
  EXPECT_EQ(tuple, tuple_in);
  EXPECT_EQ(variant1, variant1_in);
  EXPECT_EQ(variant2, variant2_in);
}

}  // namespace
}  // namespace odb
