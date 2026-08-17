// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// The watermark selects nets by HMAC-SHA256, and a verifier only agrees with
// an embedder if both compute the same digest.  This module carries its own
// implementation rather than depending on OpenSSL, so it is checked against
// the published RFC 4231 test vectors.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "HmacSha256.h"
#include "gtest/gtest.h"

namespace wmk {
namespace {

std::string toHex(const std::uint8_t* data, std::size_t len)
{
  static const char* kDigits = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (std::size_t i = 0; i < len; ++i) {
    out.push_back(kDigits[data[i] >> 4]);
    out.push_back(kDigits[data[i] & 0x0f]);
  }
  return out;
}

std::string hmacHex(const std::vector<std::uint8_t>& key,
                    const std::vector<std::uint8_t>& msg)
{
  std::uint8_t mac[32];
  hmac_sha256(key.data(), key.size(), msg.data(), msg.size(), mac);
  return toHex(mac, sizeof(mac));
}

std::vector<std::uint8_t> repeated(std::uint8_t byte, std::size_t count)
{
  return std::vector<std::uint8_t>(count, byte);
}

std::vector<std::uint8_t> fromString(const std::string& s)
{
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

// RFC 4231 section 4.2
TEST(HmacSha256, Rfc4231Case1)
{
  EXPECT_EQ(hmacHex(repeated(0x0b, 20), fromString("Hi There")),
            "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
}

// RFC 4231 section 4.3 -- key shorter than the block size
TEST(HmacSha256, Rfc4231Case2)
{
  EXPECT_EQ(
      hmacHex(fromString("Jefe"), fromString("what do ya want for nothing?")),
      "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
}

// RFC 4231 section 4.4
TEST(HmacSha256, Rfc4231Case3)
{
  EXPECT_EQ(hmacHex(repeated(0xaa, 20), repeated(0xdd, 50)),
            "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe");
}

// RFC 4231 section 4.5
TEST(HmacSha256, Rfc4231Case4)
{
  std::vector<std::uint8_t> key;
  for (std::uint8_t b = 1; b <= 25; ++b) {
    key.push_back(b);
  }
  EXPECT_EQ(hmacHex(key, repeated(0xcd, 50)),
            "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b");
}

// RFC 4231 section 4.7 -- key longer than the block size, so it is hashed
// first.  This is the case a naive implementation gets wrong.
TEST(HmacSha256, Rfc4231Case6KeyLongerThanBlock)
{
  EXPECT_EQ(hmacHex(repeated(0xaa, 131),
                    fromString("Test Using Larger Than Block-Size Key - Hash "
                               "Key First")),
            "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
}

// RFC 4231 section 4.8 -- long key and a message spanning several blocks
TEST(HmacSha256, Rfc4231Case7LongKeyAndMessage)
{
  EXPECT_EQ(
      hmacHex(repeated(0xaa, 131),
              fromString("This is a test using a larger than block-size key "
                         "and a larger than block-size data. The key needs to "
                         "be hashed before being used by the HMAC algorithm.")),
      "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2");
}

// The 32-byte convenience overload must agree with the raw entry point, since
// the net selection uses it on every net.
TEST(HmacSha256, Key32OverloadMatchesRawCall)
{
  std::array<std::uint8_t, 32> key{};
  for (std::size_t i = 0; i < key.size(); ++i) {
    key[i] = static_cast<std::uint8_t>(i);
  }
  const std::string msg = std::string("net") + '\0' + "some_net_name";

  const std::array<std::uint8_t, 32> a = hmac_sha256_key32(key, msg);
  std::uint8_t b[32];
  hmac_sha256(key.data(),
              key.size(),
              reinterpret_cast<const std::uint8_t*>(msg.data()),
              msg.size(),
              b);
  EXPECT_EQ(toHex(a.data(), a.size()), toHex(b, sizeof(b)));
}

TEST(ParseHexKey32, AcceptsExactlySixtyFourHexChars)
{
  std::array<std::uint8_t, 32> key{};
  EXPECT_TRUE(parse_hex_key32(std::string(64, 'a'), key));
  EXPECT_EQ(key[0], 0xaa);
  EXPECT_EQ(key[31], 0xaa);
}

TEST(ParseHexKey32, IsCaseInsensitive)
{
  std::array<std::uint8_t, 32> lower{};
  std::array<std::uint8_t, 32> upper{};
  const std::string hex
      = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  std::string upper_hex = hex;
  for (char& c : upper_hex) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  ASSERT_TRUE(parse_hex_key32(hex, lower));
  ASSERT_TRUE(parse_hex_key32(upper_hex, upper));
  EXPECT_EQ(lower, upper);
}

TEST(ParseHexKey32, RejectsWrongLengthAndNonHex)
{
  std::array<std::uint8_t, 32> key{};
  EXPECT_FALSE(parse_hex_key32("", key));
  EXPECT_FALSE(parse_hex_key32(std::string(63, 'a'), key));
  EXPECT_FALSE(parse_hex_key32(std::string(65, 'a'), key));
  EXPECT_FALSE(parse_hex_key32(std::string(63, 'a') + "z", key));
}

// The routing statistic seeds its randomization null from SHA-256 of the
// design name, so the bare hash is used on its own and is checked here against
// the two vectors published with the algorithm.

TEST(Sha256, EmptyInput)
{
  const std::array<std::uint8_t, 32> d = sha256("");
  EXPECT_EQ(toHex(d.data(), d.size()),
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(Sha256, Abc)
{
  const std::array<std::uint8_t, 32> d = sha256("abc");
  EXPECT_EQ(toHex(d.data(), d.size()),
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

// hmac_digest length-prefixes each part before hashing.  Without that, the
// parts would simply be concatenated and two different tuples could hash the
// same -- so a cell pair could be made to collide with a differently split one
// and take its keyed bit.  This is the property that stops it.

TEST(HmacDigest, PartBoundariesChangeTheDigest)
{
  const std::array<std::uint8_t, 32> key = {};
  EXPECT_NE(hmac_digest(key, {"ab", "c"}), hmac_digest(key, {"a", "bc"}));
  EXPECT_NE(hmac_digest(key, {"a", "b", "c"}), hmac_digest(key, {"abc"}));
  EXPECT_NE(hmac_digest(key, {"", "abc"}), hmac_digest(key, {"abc", ""}));
}

TEST(HmacDigest, IsDeterministicAndKeyed)
{
  std::array<std::uint8_t, 32> key_a = {};
  std::array<std::uint8_t, 32> key_b = {};
  key_b[31] = 1;
  const std::vector<std::string> parts = {"bit", "tile", "_101_", "_102_"};
  EXPECT_EQ(hmac_digest(key_a, parts), hmac_digest(key_a, parts));
  EXPECT_NE(hmac_digest(key_a, parts), hmac_digest(key_b, parts));
}

// Embedded NUL bytes must survive: the placement watermark feeds the tile
// coordinates in as raw bytes, which are mostly zero for the first tile.

TEST(HmacDigest, HandlesEmbeddedNuls)
{
  const std::array<std::uint8_t, 32> key = {};
  const std::string zeros(8, '\0');
  std::string one_set(8, '\0');
  one_set[7] = 1;
  EXPECT_NE(hmac_digest(key, {"bit", zeros}),
            hmac_digest(key, {"bit", one_set}));
}

}  // namespace
}  // namespace wmk
