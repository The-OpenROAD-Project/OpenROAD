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

#include "odb/db.h"
#include "odb/dbStream.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
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

/// Tests serialization and deserialization of dbBox (both Rect and Oct) using
/// public APIs.
TEST_F(DbStreamTest, DbBoxSerializationPublic)
{
  std::stringstream ss;

  // Setup database 1 (writer)
  dbDatabase* db_out = getDb();
  dbTech* tech_out = dbTech::create(db_out, "tech");
  dbTechLayer* layer_out
      = dbTechLayer::create(tech_out, "M1", dbTechLayerType::ROUTING);
  dbChip* chip_out = dbChip::create(db_out, tech_out);
  dbBlock* block_out = dbBlock::create(chip_out, "top");

  // Create a Net and BPin to hold a Rect box
  dbNet* net_out = dbNet::create(block_out, "net");
  dbBTerm* bterm_out = dbBTerm::create(net_out, "bterm");
  dbBPin* bpin_out = dbBPin::create(bterm_out);
  dbBox* rect_box_out = dbBox::create(bpin_out, layer_out, 10, 20, 100, 200);
  rect_box_out->setLayerMask(1);
  rect_box_out->setSoft(true);
  rect_box_out->setDesignRuleWidth(50);
  rect_box_out->setMinSpacing(30);

  // Create a SWire to hold an Oct box (dbSBox)
  dbSWire* swire_out = dbSWire::create(net_out, dbWireType::ROUTED);
  dbSBox* oct_box_out = dbSBox::create(swire_out,
                                       layer_out,
                                       10,
                                       10,
                                       20,
                                       20,
                                       dbWireShapeType::NONE,
                                       dbSBox::OCTILINEAR,
                                       6);
  oct_box_out->setDesignRuleWidth(-1);
  oct_box_out->setMinSpacing(15);

  // Write database to stream
  db_out->write(ss);

  // Setup database 2 (reader)
  std::unique_ptr<dbDatabase, void (*)(dbDatabase*)> db_in(dbDatabase::create(),
                                                           dbDatabase::destroy);
  db_in->setLogger(getLogger());
  db_in->read(ss);

  // Verify database 2
  dbChip* chip_in = db_in->getChip();
  ASSERT_NE(chip_in, nullptr);
  dbBlock* block_in = chip_in->getBlock();
  ASSERT_NE(block_in, nullptr);

  dbNet* net_in = block_in->findNet("net");
  ASSERT_NE(net_in, nullptr);

  dbBTerm* bterm_in = block_in->findBTerm("bterm");
  ASSERT_NE(bterm_in, nullptr);

  dbBPin* bpin_in = nullptr;
  for (dbBPin* bpin : bterm_in->getBPins()) {
    bpin_in = bpin;
    break;
  }
  ASSERT_NE(bpin_in, nullptr);

  // Rect box verification
  dbBox* rect_box_in = nullptr;
  for (dbBox* box : bpin_in->getBoxes()) {
    rect_box_in = box;
    break;
  }
  ASSERT_NE(rect_box_in, nullptr);
  EXPECT_EQ(rect_box_in->xMin(), 10);
  EXPECT_EQ(rect_box_in->yMin(), 20);
  EXPECT_EQ(rect_box_in->xMax(), 100);
  EXPECT_EQ(rect_box_in->yMax(), 200);
  EXPECT_EQ(rect_box_in->getLayerMask(), 1);
  EXPECT_TRUE(rect_box_in->isSoft());
  EXPECT_EQ(rect_box_in->getDesignRuleWidth(), 50);
  EXPECT_EQ(rect_box_in->getMinSpacing(), 30);

  // Oct box verification (SWire)
  dbSWire* swire_in = nullptr;
  for (dbSWire* swire : net_in->getSWires()) {
    swire_in = swire;
    break;
  }
  ASSERT_NE(swire_in, nullptr);

  dbSBox* oct_box_in = nullptr;
  for (dbSBox* sbox : swire_in->getWires()) {
    oct_box_in = sbox;
    break;
  }
  ASSERT_NE(oct_box_in, nullptr);
  EXPECT_EQ(oct_box_in->getDirection(), dbSBox::OCTILINEAR);

  Oct oct = oct_box_in->getOct();
  EXPECT_EQ(oct.getCenterHigh(), Point(20, 20));
  EXPECT_EQ(oct.getCenterLow(), Point(10, 10));
  EXPECT_EQ(oct.getWidth(), 6);

  EXPECT_EQ(oct_box_in->getDesignRuleWidth(), -1);
  EXPECT_EQ(oct_box_in->getMinSpacing(), 15);
}
}  // namespace
}  // namespace odb
