// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "utl/Logger.h"
#include "utl/Validate.h"

namespace utl {
namespace {

// A nested options struct, the way a sub-tool's options are embedded.
struct Inner
{
  int iterations = 50;
  bool verbose = false;
  std::array<int, 2> origin{0, 0};

  static const Schema<Inner>& schema();
};

const Schema<Inner>& Inner::schema()
{
  static const Schema<Inner> schema
      = Schema<Inner>(UTL, 898)
            .field("iterations",
                   &Inner::iterations,
                   910,
                   "Iteration count.",
                   NonNegative{})
            .field("verbose", &Inner::verbose, "Report progress.")
            .field("origin", &Inner::origin, "Grid origin.");
  return schema;
}

// An options struct that declares the constraints on its own fields, the way
// a C++ API's options struct does.
struct Options
{
  float density = 0.7;
  int iterations = 20;
  int pad = 0;
  std::vector<int> overflows{64, 20};
  std::array<float, 2> coefficients{1.0, 0.0};
  std::string version = "5.8";
  std::optional<int> seed;
  bool verbose = false;
  double length = 0;
  // A sub-tool's options with this tool's own defaults for it.
  Inner inner{.iterations = 0, .verbose = true};

  static const Schema<Options>& schema();
  void validate(Logger* logger) const { utl::validate(*this, logger); }
};

const Schema<Options>& Options::schema()
{
  static const Schema<Options> schema
      = Schema<Options>(UTL, 899)
            .field("density",
                   &Options::density,
                   900,
                   "Target density.",
                   Range{0.0, 1.0})
            .field("iterations",
                   &Options::iterations,
                   901,
                   "Iteration count.",
                   Positive{})
            .field("pad", &Options::pad, 902, "Padding.", NonNegative{})
            .field("overflows",
                   &Options::overflows,
                   903,
                   "Overflow thresholds.",
                   Positive{})
            .field("coefficients",
                   &Options::coefficients,
                   904,
                   "Metric coefficients.",
                   NonNegative{})
            .field("version",
                   &Options::version,
                   905,
                   "File version.",
                   OneOf{"5.8", "5.7"})
            .field("seed", &Options::seed, 906, "Random seed.", NonNegative{})
            .field("verbose", &Options::verbose, "Report progress.")
            .field("length",
                   &Options::length,
                   907,
                   "Wire length.",
                   NonNegative{},
                   Distance{})
            .group("inner",
                   &Options::inner,
                   "The nested options, with a help text that is long enough "
                   "to live on the heap and be copied around.")
            .invariant(
                908, "verbose requires inner.verbose", [](const Options& o) {
                  return !o.verbose || o.inner.verbose;
                });
  return schema;
}

class ValidateTest : public ::testing::Test
{
 protected:
  // The message reported for a violation, without the logger's prefix.
  std::string report(const std::function<void()>& call)
  {
    logger_.redirectStringBegin();
    try {
      call();
    } catch (const std::runtime_error&) {
    }
    return logger_.redirectStringEnd();
  }

  boost::json::value parse(const std::string& text)
  {
    return boost::json::parse(text);
  }

  Logger logger_;
};

TEST_F(ValidateTest, ConstraintsDescribeThemselves)
{
  EXPECT_EQ(Range({0.0, 1.0}).describe(), "in [0, 1]");
  EXPECT_EQ(Range({0.0, 1.0, true, true}).describe(), "in (0, 1)");
  EXPECT_EQ(Min({5.0}).describe(), ">= 5");
  EXPECT_EQ(Max({5.0, true}).describe(), "< 5");
  EXPECT_EQ(Positive().describe(), "> 0");
  EXPECT_EQ(NonNegative().describe(), ">= 0");
  EXPECT_EQ(Percent().describe(), "in [0, 100]");
  EXPECT_EQ(OneOf({"a", "b"}).describe(), "one of a b");
  EXPECT_EQ(Count({4}).describe(), "a list of 4 values");
}

TEST_F(ValidateTest, ConstraintsCheckScalarsAndContainers)
{
  EXPECT_TRUE(Range({0.0, 1.0}).check(0.5f).empty());
  EXPECT_FALSE(Range({0.0, 1.0}).check(1.5f).empty());
  EXPECT_TRUE(Range({0.0, 1.0}).check(1.0f).empty());
  EXPECT_FALSE(Range({0.0, 1.0, false, true}).check(1.0f).empty());

  EXPECT_TRUE(Positive().check(std::vector<int>{1, 2}).empty());
  EXPECT_FALSE(Positive().check(std::vector<int>{1, -2}).empty());
  EXPECT_TRUE(NonNegative().check(std::array<float, 2>{0.0, 1.0}).empty());
  EXPECT_FALSE(NonNegative().check(std::array<float, 2>{0.0, -1.0}).empty());

  EXPECT_TRUE(NonNegative().check(std::optional<int>()).empty());
  EXPECT_FALSE(NonNegative().check(std::optional<int>(-1)).empty());

  EXPECT_TRUE(OneOf({"a", "b"}).check(std::string("b")).empty());
  EXPECT_FALSE(OneOf({"a", "b"}).check(std::string("c")).empty());
  EXPECT_TRUE(Count({2}).check(std::vector<int>{1, 2}).empty());
  EXPECT_FALSE(Count({2}).check(std::vector<int>{1}).empty());
}

TEST_F(ValidateTest, UnitTagsAreCheckedWhenDeclared)
{
  // One unit, on a numeric value or a list of them, is recorded.
  EXPECT_EQ(Rule<double>("l", UTL, 1, "", Distance{}).unit(), Unit::kDistance);
  EXPECT_EQ(Rule<std::optional<float>>("t", UTL, 1, "", Time{}).unit(),
            Unit::kTime);
  EXPECT_EQ(Rule<std::vector<int>>("c", UTL, 1, "", Capacitance{}).unit(),
            Unit::kCapacitance);
  EXPECT_EQ(Rule<double>("x", UTL, 1, "", NonNegative{}).unit(), Unit::kNone);
  EXPECT_TRUE((val::IsNumeric<std::array<float, 2>>::value));
  EXPECT_FALSE(val::IsNumeric<std::string>::value);
  EXPECT_FALSE(val::IsNumeric<std::vector<std::string>>::value);
  // These do not compile, by the static_asserts in Rule:
  //   Rule<double>("l", UTL, 1, "", Distance{}, Time{});
  //   Rule<std::string>("s", UTL, 1, "", Distance{});
}

TEST_F(ValidateTest, RuleCarriesItsMessageIdentity)
{
  const Rule<std::optional<int>> rule(
      "compression", ORD, 77, "gzip compression level.", Range{0, 9});

  EXPECT_EQ(rule.name(), "compression");
  EXPECT_EQ(rule.constraints(), (std::vector<std::string>{"in [0, 9]"}));
  EXPECT_EQ(rule.id(), 77);
  EXPECT_FALSE(rule.check(std::optional<int>()).has_value());

  const std::optional<Violation> violation = rule.check(std::optional<int>(12));
  ASSERT_TRUE(violation.has_value());
  EXPECT_EQ(violation->message, "compression 12 not in [0, 9]");
  EXPECT_EQ(report([&] { rule.enforce(std::optional<int>(12), &logger_); }),
            "[ERROR ORD-0077] compression 12 not in [0, 9]\n");
}

TEST_F(ValidateTest, SchemaValidatesEveryField)
{
  Options options;
  options.validate(&logger_);  // the defaults satisfy their own schema

  options.density = 1.5;
  EXPECT_EQ(report([&] { options.validate(&logger_); }),
            "[ERROR UTL-0900] density 1.5 not in [0, 1]\n");

  auto message = [this](const std::function<void(Options&)>& set) {
    Options o;
    set(o);
    return report([&] { o.validate(&logger_); });
  };
  EXPECT_EQ(message([](Options& o) { o.iterations = 0; }),
            "[ERROR UTL-0901] iterations 0 not > 0\n");
  EXPECT_EQ(message([](Options& o) { o.overflows = {10, -20}; }),
            "[ERROR UTL-0903] overflows {10 -20} not > 0\n");
  EXPECT_EQ(message([](Options& o) { o.version = "5.6"; }),
            "[ERROR UTL-0905] version 5.6 not one of 5.8 5.7\n");
  // A nested struct is checked by its own schema, reported with its path.
  EXPECT_EQ(message([](Options& o) { o.inner.iterations = -1; }),
            "[ERROR UTL-0910] inner.iterations -1 not >= 0\n");
  // An invariant spans fields.
  EXPECT_EQ(message([](Options& o) {
              o.verbose = true;
              o.inner.verbose = false;
            }),
            "[ERROR UTL-0908] verbose requires inner.verbose\n");
}

TEST_F(ValidateTest, SchemaDescribesItsFields)
{
  const SchemaBase& schema = Options::schema();
  const FieldMeta* density = schema.find("density");
  ASSERT_NE(density, nullptr);
  EXPECT_EQ(density->type_name, "float");
  EXPECT_EQ(density->kind, FieldMeta::Kind::kScalar);
  EXPECT_EQ(schema.find("overflows")->type_name, "list of int");
  EXPECT_EQ(schema.find("overflows")->kind, FieldMeta::Kind::kList);
  EXPECT_EQ(schema.find("coefficients")->type_name, "list of 2 float");
  EXPECT_EQ(schema.find("length")->unit, Unit::kDistance);
  EXPECT_EQ(schema.find("inner")->kind, FieldMeta::Kind::kGroup);
  EXPECT_EQ(schema.findPath("inner.origin")->type_name, "list of 2 int");
  EXPECT_EQ(schema.findPath("inner.nope"), nullptr);
  EXPECT_EQ(schema.findPath("density.x"), nullptr);
  EXPECT_EQ(schema.find("nope"), nullptr);

  // Defaults come from the struct, including this struct's own defaults
  // for its nested group.
  const std::vector<FieldInfo> info = schema.describe();
  ASSERT_EQ(info.size(), 13);
  EXPECT_EQ(info[0].path, "density");
  EXPECT_EQ(info[0].default_text, "0.7");
  EXPECT_EQ(info[3].default_text, "{64 20}");
  EXPECT_EQ(info[10].path, "inner.iterations");
  EXPECT_EQ(info[10].default_text, "0");
  EXPECT_EQ(info[10].depth, 1);
  EXPECT_EQ(info[11].default_text, "true");

  const std::string text = describeText(schema);
  EXPECT_NE(text.find("float = 0.7 (in [0, 1])  Target density."),
            std::string::npos);
  EXPECT_NE(text.find("float, distance = 0 (>= 0)  Wire length."),
            std::string::npos);
  EXPECT_NE(
      text.find("(group)  The nested options, with a help text that is "
                "long enough to live on the heap and be copied around.\n"),
      std::string::npos);
  const std::string markdown = describeMarkdown(schema);
  EXPECT_NE(markdown.find(
                "| `inner.iterations` | int (>= 0) | `0` | Iteration count. |"),
            std::string::npos);
}

TEST_F(ValidateTest, ApplyAcceptsTypedAndStringValues)
{
  Options options;
  // Typed values, as Python or a JSON file gives them.
  Options::schema().apply(
      options,
      parse(R"({"density": 0.5, "iterations": 3, "overflows": [30, 10],
              "seed": 7, "verbose": true, "inner": {"verbose": true,
              "origin": [1, 2]}})"),
      &logger_);
  EXPECT_FLOAT_EQ(options.density, 0.5);
  EXPECT_EQ(options.iterations, 3);
  EXPECT_EQ(options.overflows, (std::vector<int>{30, 10}));
  EXPECT_EQ(options.seed, 7);
  EXPECT_TRUE(options.verbose);
  EXPECT_EQ(options.inner.origin, (std::array<int, 2>{1, 2}));
  EXPECT_EQ(options.inner.iterations, 0);  // untouched

  // Strings, as Tcl gives them, including the string form of a list.
  Options strings;
  Options::schema().apply(
      strings,
      parse(R"({"density": "0.25", "pad": "4", "overflows": "8 4",
              "coefficients": ["2", "3"], "inner": {"verbose": "off"}})"),
      &logger_);
  EXPECT_FLOAT_EQ(strings.density, 0.25);
  EXPECT_EQ(strings.pad, 4);
  EXPECT_EQ(strings.overflows, (std::vector<int>{8, 4}));
  EXPECT_EQ(strings.coefficients, (std::array<float, 2>{2, 3}));
  EXPECT_FALSE(strings.inner.verbose);
}

TEST_F(ValidateTest, ApplyAcceptsDottedPaths)
{
  Options options;
  Options::schema().apply(
      options,
      parse(R"({"inner.iterations": 9, "inner.verbose": false})"),
      &logger_);
  EXPECT_EQ(options.inner.iterations, 9);
  EXPECT_FALSE(options.inner.verbose);
}

TEST_F(ValidateTest, ApplyRejectsBadInput)
{
  auto message = [this](const std::string& json) {
    Options o;
    return report([&] { Options::schema().apply(o, parse(json), &logger_); });
  };
  EXPECT_EQ(message(R"({"densty": 0.5})"),
            "[ERROR UTL-0899] unknown option densty; valid options: density "
            "iterations pad overflows coefficients version seed verbose length "
            "inner\n");
  EXPECT_EQ(message(R"({"inner": {"nope": 1}})"),
            "[ERROR UTL-0898] unknown option inner.nope; valid options: "
            "iterations verbose origin\n");
  EXPECT_EQ(message(R"({"density": "abc"})"),
            "[ERROR UTL-0900] density abc not a float\n");
  EXPECT_EQ(message(R"({"pad": 1.5})"),
            "[ERROR UTL-0902] pad 1.5 not an int\n");
  EXPECT_EQ(message(R"({"verbose": "maybe"})"),
            "[ERROR UTL-0899] verbose maybe not a bool\n");
  EXPECT_EQ(message(R"({"inner": {"origin": [1]}})"),
            "[ERROR UTL-0898] inner.origin {1} not a list of 2 values\n");
  EXPECT_EQ(message(R"({"inner": 5})"),
            "[ERROR UTL-0898] inner 5 not a dictionary\n");
  EXPECT_EQ(
      message(R"({"density.x": 1})"),
      "[ERROR UTL-0899] density is not a group; density.x cannot be set\n");
  // Constraints are checked after the whole dictionary is applied.
  EXPECT_EQ(message(R"({"density": 2})"),
            "[ERROR UTL-0900] density 2 not in [0, 1]\n");
  EXPECT_EQ(message(R"({"inner": {"iterations": -3}})"),
            "[ERROR UTL-0910] inner.iterations -3 not >= 0\n");
}

TEST_F(ValidateTest, ToJsonRoundTripsAndReportsChanges)
{
  Options options;
  const boost::json::value all = Options::schema().toJson(options);
  EXPECT_EQ(all.at("density"), boost::json::value(0.7));  // not 0.699999988
  EXPECT_EQ(all.at("seed"), boost::json::value(nullptr));
  EXPECT_EQ(all.at("inner").at("iterations"), boost::json::value(0));
  EXPECT_EQ(Options::schema().defaults(), all);

  // Nothing changed: an empty dictionary.
  EXPECT_TRUE(Options::schema().toJson(options, true).as_object().empty());

  options.pad = 2;
  options.inner.origin = {5, 6};
  const boost::json::value changed = Options::schema().toJson(options, true);
  EXPECT_EQ(changed, parse(R"({"pad": 2, "inner": {"origin": [5, 6]}})"));

  // Round trip through a fresh struct.
  Options copy;
  Options::schema().apply(copy, changed, &logger_);
  EXPECT_EQ(Options::schema().toJson(copy), Options::schema().toJson(options));

  // resolve() is what a command runs with: defaults plus the dictionary.
  const boost::json::value resolved
      = Options::schema().resolve(parse(R"({"pad": 2})"), &logger_);
  EXPECT_EQ(resolved.at("pad"), boost::json::value(2));
  EXPECT_EQ(resolved.at("density"), boost::json::value(0.7));
  const std::string text = reportValues(Options::schema(), resolved);
  EXPECT_NE(text.find("pad               2 *\n"), std::string::npos);
  EXPECT_NE(text.find("density           0.7\n"), std::string::npos);
}

TEST_F(ValidateTest, LegacySwitchesBecomeADictionary)
{
  const LegacyArgs table = {
      {"-density", {"density"}, "", {}},
      {"-verbose", {"verbose", "inner.verbose"}, "true", {}},
      {"-quiet", {"inner.verbose"}, "false", {}},
      {"-origin",
       {},
       "",
       [](boost::json::object& o, const std::string& v) {
         o["inner.origin"] = v + " " + v;
       }},
  };
  const std::map<std::string, std::string> keys
      = {{"-density", "0.5"}, {"-origin", "3"}, {"-options", "{}"}};
  const std::map<std::string, std::string> flags = {{"-verbose", "1"}};
  const boost::json::object json = legacyToJson(keys, flags, table);
  EXPECT_EQ(json, parse(R"({"density": "0.5", "verbose": "true",
                     "inner.verbose": "true", "inner.origin": "3 3"})"));

  Options options;
  Options::schema().apply(options, json, &logger_);
  EXPECT_FLOAT_EQ(options.density, 0.5);
  EXPECT_TRUE(options.verbose);
  EXPECT_EQ(options.inner.origin, (std::array<int, 2>{3, 3}));
}

TEST_F(ValidateTest, ValueFormatting)
{
  EXPECT_EQ(val::format(0.7f), "0.7");
  EXPECT_EQ(val::format(std::optional<int>()), "none");
  EXPECT_EQ(val::format(std::vector<int>{1, 2}), "{1 2}");
  EXPECT_EQ(val::formatJson(parse(R"({"a": [1, 2.5], "b": "x"})")),
            "{a {1 2.5} b x}");
  EXPECT_EQ(val::typeName<std::optional<float>>(), "float");
}

}  // namespace
}  // namespace utl
