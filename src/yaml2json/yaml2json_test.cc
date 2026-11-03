// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "yaml2json.h"

#include <stdexcept>
#include <string>

#include "gtest/gtest.h"

namespace yaml2json {
namespace {

// The value of "k" in a one-entry map, as yaml2json writes it.
std::string value(const std::string& yaml)
{
  const std::string json = yamlToJson("k: " + yaml + "\n");
  const std::string head = "{\n  \"k\": ";
  const std::string tail = "\n}\n";
  EXPECT_EQ(json.rfind(head, 0), 0) << json;
  return json.substr(head.size(), json.size() - head.size() - tail.size());
}

TEST(Yaml2Json, NestedMapsAndSequencesKeepDocumentOrder)
{
  EXPECT_EQ(yamlToJson("b:\n  stages:\n    - floorplan\n    - place\na: {}\n"),
            "{\n"
            "  \"b\": {\n"
            "    \"stages\": [\n"
            "      \"floorplan\",\n"
            "      \"place\"\n"
            "    ]\n"
            "  },\n"
            "  \"a\": {}\n"
            "}\n");
}

TEST(Yaml2Json, PlainScalarsResolveAsPyYamlSafeLoad)
{
  EXPECT_EQ(value(""), "null");
  EXPECT_EQ(value("~"), "null");
  EXPECT_EQ(value("null"), "null");
  EXPECT_EQ(value("true"), "true");
  EXPECT_EQ(value("True"), "true");
  EXPECT_EQ(value("yes"), "true");
  EXPECT_EQ(value("off"), "false");
  EXPECT_EQ(value("0"), "0");
  EXPECT_EQ(value("-12"), "-12");
  EXPECT_EQ(value("+7"), "7");
  // Python's str() of these floats is what a reader of the JSON sees.
  EXPECT_EQ(value("1.0"), "1.0");
  EXPECT_EQ(value("50."), "50.0");
  EXPECT_EQ(value(".5"), "0.5");
  EXPECT_EQ(value("1.05"), "1.05");
  EXPECT_EQ(value("1.5e+3"), "1500.0");
  // YAML 1.1 wants a '.' and a signed exponent; PyYAML reads these as strings.
  EXPECT_EQ(value("1e3"), "\"1e3\"");
  EXPECT_EQ(value("."), "\".\"");
  EXPECT_EQ(value("y"), "\"y\"");
  EXPECT_EQ(value("abc def"), "\"abc def\"");
}

TEST(Yaml2Json, QuotedAndTaggedScalarsAreStrings)
{
  EXPECT_EQ(value("\"1.0\""), "\"1.0\"");
  EXPECT_EQ(value("'true'"), "\"true\"");
  EXPECT_EQ(value("!!str 3"), "\"3\"");
  EXPECT_EQ(value("!!int 3"), "3");
}

TEST(Yaml2Json, StringsAreEscaped)
{
  EXPECT_EQ(value("\"a\\\"b\\\\c\\td\""), "\"a\\\"b\\\\c\\td\"");
  EXPECT_EQ(yamlToJson("k: |\n  one\n  two\n"),
            "{\n  \"k\": \"one\\ntwo\\n\"\n}\n");
}

TEST(Yaml2Json, AmbiguousOrUnrepresentableInputFails)
{
  EXPECT_THROW(yamlToJson("a: 1\na: 2\n"), std::runtime_error);
  EXPECT_THROW(yamlToJson("? [a]\n: 1\n"), std::runtime_error);
  EXPECT_THROW(yamlToJson("b: &x {c: 1}\na:\n  <<: *x\n"), std::runtime_error);
  EXPECT_THROW(value("010"), std::runtime_error);
  EXPECT_THROW(value("0x1f"), std::runtime_error);
  EXPECT_THROW(value("1:30"), std::runtime_error);
  EXPECT_THROW(value("1_000"), std::runtime_error);
  EXPECT_THROW(value(".inf"), std::runtime_error);
  EXPECT_THROW(value(".nan"), std::runtime_error);
  EXPECT_THROW(value("!!int abc"), std::runtime_error);
  EXPECT_THROW(value("!custom x"), std::runtime_error);
}

TEST(Yaml2Json, InputMustBeOneDocument)
{
  EXPECT_THROW(yamlToJson(""), std::runtime_error);
  EXPECT_THROW(yamlToJson("a: 1\n---\nb: 2\n"), std::runtime_error);
  EXPECT_THROW(yamlToJson("a: [1\n"), YAML::Exception);
}

}  // namespace
}  // namespace yaml2json
