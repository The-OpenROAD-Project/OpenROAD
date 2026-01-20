#include <cstdint>
#include <sstream>

#include "gtest/gtest.h"
#include "odb/dbObject.h"
#include "odb/dbStream.h"
#include "tst/fixture.h"

namespace odb {
namespace {

class ObjectTypeFixture : public tst::Fixture
{
};

TEST_F(ObjectTypeFixture, EnumNames)
{
  EXPECT_STREQ(dbObject::getTypeName(dbGdsLibObj), "dbGDSLib");
}

TEST_F(ObjectTypeFixture, HashRedirection)
{
  std::stringstream ss;
  // 0x2 is the primary hash for dbGdsLibObj
  constexpr uint32_t primary_hash = 0x2;
  ss.write(reinterpret_cast<const char*>(&primary_hash), sizeof(primary_hash));

  // 0x53 is the old duplicate hash for Index 91, which should now map to Index
  // 0
  constexpr uint32_t old_hash = 0x53;
  ss.write(reinterpret_cast<const char*>(&old_hash), sizeof(old_hash));

  // Cast dbDatabase* to _dbDatabase* for the internal stream constructor
  dbIStream in((_dbDatabase*) db_.get(), ss);
  dbObjectType type1, type2;

  in >> type1;
  in >> type2;

  EXPECT_EQ(type1, dbGdsLibObj);
  EXPECT_EQ(type2, dbGdsLibObj);
}

TEST_F(ObjectTypeFixture, WriteHash)
{
  std::stringstream ss;
  // Cast dbDatabase* to _dbDatabase* for the internal stream constructor
  dbOStream out((_dbDatabase*) db_.get(), ss);

  out << dbGdsLibObj;

  // Should write the primary hash 0x2
  uint32_t hash;
  ss.read(reinterpret_cast<char*>(&hash), sizeof(hash));
  constexpr uint32_t primary_hash = 0x2;
  EXPECT_EQ(hash, primary_hash);
}

TEST_F(ObjectTypeFixture, EnumShift)
{
  EXPECT_STREQ(dbObject::getTypeName(dbLibObj), "dbLib");
  EXPECT_STREQ(dbObject::getTypeName(dbSiteObj), "dbSite");
}

}  // namespace
}  // namespace odb
