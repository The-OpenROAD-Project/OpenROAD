// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Declarative options structs, shared by every OpenROAD API level.
//
// OpenROAD exposes three API levels: C++, Python (a SWIG wrap of the C++ API)
// and Tcl (commands that bind to the C++ API).  C++ is the common point, so
// an options struct declares its fields once, in C++, and that declaration
// drives everything the other two levels need:
//
//   - the constraints on each value and the message id reported when a value
//     is rejected (the same message from C++, Python and Tcl),
//   - conversion of a nested dictionary (a Tcl dict, a Python dict or kwargs,
//     JSON) onto the struct, with unknown keys rejected,
//   - the reverse: the struct as a dictionary, all of it or only the values
//     that differ from the defaults,
//   - help text and the README options table.
//
// A struct publishes its schema with a static schema():
//
//   struct TimingOptions
//   {
//     bool enabled = false;
//     float nets_percentage = 10;
//     rsz::RepairDesignOptions repair_design;   // a nested options struct
//     static const utl::Schema<TimingOptions>& schema();
//   };
//
//   const utl::Schema<TimingOptions>& TimingOptions::schema()
//   {
//     static const utl::Schema<TimingOptions> s
//         = utl::Schema<TimingOptions>(GPL, kBadOptionId)
//               .field("enabled", &TimingOptions::enabled,
//                      "Reweight the nets with the worst slack.")
//               .field("nets_percentage", &TimingOptions::nets_percentage, 424,
//                      "Share of the nets that is reweighted.", utl::Percent{})
//               .group("repair_design", &TimingOptions::repair_design,
//                      "repair_design settings used to find the slacks.");
//     return s;
//   }
//
// The struct's default member initializers are the defaults.  A field with
// constraints carries the message id its violations report with; a field
// without constraints and the generic errors (unknown key, value of the wrong
// type) use the id given to the Schema constructor (one id per struct).
//
// The C++ API entry point that takes the struct calls utl::validate(options,
// logger), so every caller is checked.  Supported field types: bool, integral
// and floating point types, std::string, std::optional<T>, std::vector<T>,
// std::array<T, N> and any struct with its own schema().
//
// A rule is the stand-alone form for a plain API argument:
//
//   const utl::Rule<std::optional<int>> kCompression{
//       "compression", ORD, 77, "gzip compression level.", utl::Range{0, 9}};
//   kCompression.enforce(level, logger_);

#pragma once

#include <algorithm>
#include <array>
#include <boost/json.hpp>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "utl/Logger.h"

namespace utl {

////////////////////////////////////////////////////////////////
// Type traits for the supported value types.

template <typename T>
struct IsOptional : std::false_type
{
};
template <typename T>
struct IsOptional<std::optional<T>> : std::true_type
{
};

template <typename T>
struct IsVector : std::false_type
{
};
template <typename T>
struct IsVector<std::vector<T>> : std::true_type
{
};

template <typename T>
struct IsArray : std::false_type
{
};
template <typename T, std::size_t N>
struct IsArray<std::array<T, N>> : std::true_type
{
};

class SchemaBase;
template <typename S>
class Schema;

// True when S publishes its own schema as `static const Schema<S>& schema()`.
template <typename S, typename = void>
struct HasSchema : std::false_type
{
};
template <typename S>
struct HasSchema<
    S,
    std::enable_if_t<std::is_same_v<decltype(S::schema()), const Schema<S>&>>>
    : std::true_type
{
};

// The physical unit of a value.  The C++ API always takes SI units (meters,
// seconds, farads, ohms); a Tcl command converts from the user's units.
enum class Unit
{
  kNone,
  kDistance,
  kTime,
  kCapacitance,
  kResistance
};

namespace val {

// Formats a double the way it is shown in messages and help.  Constraint
// bounds are declared as doubles; use format() for a value of its own type.
std::string formatDouble(double value);

template <typename T>
std::string format(const T& value)
{
  if constexpr (IsOptional<T>::value) {
    return value ? format(*value) : std::string("none");
  } else if constexpr (IsVector<T>::value || IsArray<T>::value) {
    std::string result = "{";
    for (const auto& elem : value) {
      if (result.size() > 1) {
        result += ' ';
      }
      result += format(elem);
    }
    return result + "}";
  } else if constexpr (std::is_same_v<T, bool>) {
    return value ? "true" : "false";
  } else if constexpr (std::is_floating_point_v<T>) {
    // Formatted as the type it is: a float widened to double would print
    // the digits of its binary representation (0.7f as 0.699999988079071).
    return fmt::format("{}", value);
  } else if constexpr (std::is_integral_v<T>) {
    return std::to_string(value);
  } else {
    return std::string(value);
  }
}

// Formats a JSON value in the style of format(): lists and dicts in braces.
std::string formatJson(const boost::json::value& value);

// Applies fn to each scalar inside value (recursing through optional, vector
// and array) and returns the first non-empty result.
template <typename T, typename F>
std::string forEachScalar(const T& value, const F& fn)
{
  if constexpr (IsOptional<T>::value) {
    return value ? forEachScalar(*value, fn) : std::string();
  } else if constexpr (IsVector<T>::value || IsArray<T>::value) {
    for (const auto& elem : value) {
      if (std::string msg = forEachScalar(elem, fn); !msg.empty()) {
        return msg;
      }
    }
    return {};
  } else {
    return fn(value);
  }
}

// True for an arithmetic type, or an optional, vector or array of one.
template <typename T>
struct IsNumeric : std::bool_constant<std::is_arithmetic_v<T>>
{
};
template <typename T>
struct IsNumeric<std::optional<T>> : IsNumeric<T>
{
};
template <typename T>
struct IsNumeric<std::vector<T>> : IsNumeric<T>
{
};
template <typename T, std::size_t N>
struct IsNumeric<std::array<T, N>> : IsNumeric<T>
{
};

// The name of a type as shown in help and in "not a <type>" messages.
template <typename T>
std::string typeName()
{
  if constexpr (IsOptional<T>::value) {
    return typeName<typename T::value_type>();
  } else if constexpr (IsVector<T>::value) {
    return "list of " + typeName<typename T::value_type>();
  } else if constexpr (IsArray<T>::value) {
    return "list of " + std::to_string(std::tuple_size_v<T>) + " "
           + typeName<typename T::value_type>();
  } else if constexpr (std::is_same_v<T, bool>) {
    return "bool";
  } else if constexpr (std::is_integral_v<T>) {
    return "int";
  } else if constexpr (std::is_floating_point_v<T>) {
    return "float";
  } else if constexpr (std::is_same_v<T, std::string>) {
    return "string";
  } else if constexpr (HasSchema<T>::value) {
    return "group";
  } else {
    static_assert(sizeof(T) == 0, "unsupported option type");
  }
}

////////////////////////////////////////////////////////////////
// JSON conversions.  A value may arrive typed (from Python or a JSON file)
// or as a string (from Tcl); both are accepted.

// Parses a bool from its common spellings.
bool parseBool(std::string_view text, bool& out);
// Strict parses: the whole string must be consumed.
bool parseInt(std::string_view text, int64_t& out);
bool parseDouble(std::string_view text, double& out);
// Splits a string on whitespace (the string form of a list).
std::vector<std::string> splitList(std::string_view text);

// Converts a JSON value to T.  Returns an empty string on success, or the
// phrase that reads after "not" in "<name> <value> not <reason>".
template <typename T>
std::string fromJson(const boost::json::value& value, T& out)
{
  namespace json = boost::json;
  if constexpr (IsOptional<T>::value) {
    // Unset: null from JSON, or the empty string a Tcl dict shows it as.
    if (value.is_null() || (value.is_string() && value.get_string().empty())) {
      out.reset();
      return {};
    }
    typename T::value_type inner{};
    if (std::string reason = fromJson(value, inner); !reason.empty()) {
      return reason;
    }
    out = std::move(inner);
    return {};
  } else if constexpr (IsVector<T>::value || IsArray<T>::value) {
    using Elem = typename T::value_type;
    std::vector<json::value> elems;
    if (const json::array* arr = value.if_array()) {
      elems.assign(arr->begin(), arr->end());
    } else if (const json::string* str = value.if_string()) {
      for (const std::string& item : splitList(*str)) {
        elems.emplace_back(item);
      }
    } else {
      elems.push_back(value);
    }
    if constexpr (IsArray<T>::value) {
      if (elems.size() != std::tuple_size_v<T>) {
        return "a list of " + std::to_string(std::tuple_size_v<T>) + " values";
      }
    } else {
      out.clear();
      out.resize(elems.size());
    }
    for (std::size_t i = 0; i < elems.size(); ++i) {
      Elem elem{};
      if (std::string reason = fromJson(elems[i], elem); !reason.empty()) {
        return "a " + typeName<T>();
      }
      out[i] = std::move(elem);
    }
    return {};
  } else if constexpr (std::is_same_v<T, bool>) {
    if (const bool* b = value.if_bool()) {
      out = *b;
      return {};
    }
    if (value.is_number()) {
      out = value.to_number<double>() != 0.0;
      return {};
    }
    if (const json::string* str = value.if_string()) {
      if (parseBool(*str, out)) {
        return {};
      }
    }
    return "a bool";
  } else if constexpr (std::is_integral_v<T>) {
    int64_t parsed = 0;
    if (value.is_int64() || value.is_uint64()) {
      parsed = value.to_number<int64_t>();
    } else if (const double* d = value.if_double()) {
      if (*d != static_cast<double>(static_cast<int64_t>(*d))) {
        return "an int";
      }
      parsed = static_cast<int64_t>(*d);
    } else if (const json::string* str = value.if_string()) {
      if (!parseInt(*str, parsed)) {
        return "an int";
      }
    } else {
      return "an int";
    }
    if (parsed < std::numeric_limits<T>::lowest()
        || parsed > std::numeric_limits<T>::max()) {
      return "an int in range";
    }
    out = static_cast<T>(parsed);
    return {};
  } else if constexpr (std::is_floating_point_v<T>) {
    double parsed = 0;
    if (value.is_number()) {
      parsed = value.to_number<double>();
    } else if (const json::string* str = value.if_string()) {
      if (!parseDouble(*str, parsed)) {
        return "a float";
      }
    } else {
      return "a float";
    }
    out = static_cast<T>(parsed);
    return {};
  } else if constexpr (std::is_same_v<T, std::string>) {
    if (const json::string* str = value.if_string()) {
      out = std::string(*str);
    } else {
      out = formatJson(value);
    }
    return {};
  } else {
    static_assert(sizeof(T) == 0, "unsupported option type");
  }
}

// Converts T to a JSON value.
template <typename T>
boost::json::value toJson(const T& value)
{
  namespace json = boost::json;
  if constexpr (IsOptional<T>::value) {
    return value ? toJson(*value) : json::value(nullptr);
  } else if constexpr (IsVector<T>::value || IsArray<T>::value) {
    json::array result;
    for (const auto& elem : value) {
      result.push_back(toJson(elem));
    }
    return result;
  } else if constexpr (std::is_same_v<T, bool>) {
    return json::value(value);
  } else if constexpr (std::is_integral_v<T>) {
    return json::value(static_cast<int64_t>(value));
  } else if constexpr (std::is_floating_point_v<T>) {
    // Through the shortest round-trip text, so that 0.7f is 0.7 and not
    // 0.699999988079071 once widened to double.
    double d = 0;
    parseDouble(format(value), d);
    return json::value(d);
  } else if constexpr (std::is_same_v<T, std::string>) {
    return json::value(value);
  } else {
    static_assert(sizeof(T) == 0, "unsupported option type");
  }
}

}  // namespace val

////////////////////////////////////////////////////////////////
// Constraints.
//
// A constraint is any object with
//   template <typename T> std::string check(const T& value) const;
// returning an empty string when the value is acceptable, and
//   std::string describe() const;
// a phrase that reads after "not", so that a violation reports as
// "<name> <value> not <describe()>", the wording utl::Validator uses.
//
// Constraints on a scalar apply to every scalar inside an optional, vector or
// array value.

// Numeric interval; the ends are closed unless flagged open.
struct Range
{
  double min;
  double max;
  bool min_open = false;
  bool max_open = false;

  template <typename T>
  std::string check(const T& value) const
  {
    return val::forEachScalar(value, [this](const auto& v) -> std::string {
      static_assert(std::is_arithmetic_v<std::decay_t<decltype(v)>>,
                    "Range requires a numeric value");
      const double d = static_cast<double>(v);
      if (d < min || (min_open && d == min) || d > max
          || (max_open && d == max)) {
        return describe();
      }
      return {};
    });
  }
  std::string describe() const
  {
    return std::string(min_open ? "in (" : "in [") + val::formatDouble(min)
           + ", " + val::formatDouble(max) + (max_open ? ")" : "]");
  }
};

// value >= min, or > min when open.
struct Min
{
  double min;
  bool open = false;

  template <typename T>
  std::string check(const T& value) const
  {
    return val::forEachScalar(value, [this](const auto& v) -> std::string {
      static_assert(std::is_arithmetic_v<std::decay_t<decltype(v)>>,
                    "Min requires a numeric value");
      const double d = static_cast<double>(v);
      if (d < min || (open && d == min)) {
        return describe();
      }
      return {};
    });
  }
  std::string describe() const
  {
    return std::string(open ? "> " : ">= ") + val::formatDouble(min);
  }
};

// value <= max, or < max when open.
struct Max
{
  double max;
  bool open = false;

  template <typename T>
  std::string check(const T& value) const
  {
    return val::forEachScalar(value, [this](const auto& v) -> std::string {
      static_assert(std::is_arithmetic_v<std::decay_t<decltype(v)>>,
                    "Max requires a numeric value");
      const double d = static_cast<double>(v);
      if (d > max || (open && d == max)) {
        return describe();
      }
      return {};
    });
  }
  std::string describe() const
  {
    return std::string(open ? "< " : "<= ") + val::formatDouble(max);
  }
};

struct Positive : Min
{
  Positive() : Min{0.0, true} {}
};

struct NonNegative : Min
{
  NonNegative() : Min{0.0, false} {}
};

struct Percent : Range
{
  Percent() : Range{0.0, 100.0} {}
};

// A string that must be one of a fixed set of values.
struct OneOf
{
  std::vector<std::string> values;

  OneOf(std::initializer_list<const char*> list)
      : values(list.begin(), list.end())
  {
  }
  explicit OneOf(std::vector<std::string> list) : values(std::move(list)) {}

  template <typename T>
  std::string check(const T& value) const
  {
    return val::forEachScalar(value, [this](const std::string& v) {
      if (std::find(values.begin(), values.end(), v) == values.end()) {
        return describe();
      }
      return std::string();
    });
  }
  std::string describe() const
  {
    std::string result = "one of";
    for (const std::string& v : values) {
      result += ' ' + v;
    }
    return result;
  }
};

// The list must have exactly `count` elements.
struct Count
{
  std::size_t count;

  template <typename T>
  std::string check(const T& value) const
  {
    if constexpr (IsOptional<T>::value) {
      return value ? check(*value) : std::string();
    } else {
      static_assert(IsVector<T>::value, "Count requires a list value");
      return value.size() == count ? std::string() : describe();
    }
  }
  std::string describe() const
  {
    return "a list of " + std::to_string(count) + " values";
  }
};

// Arbitrary predicate on the scalar value.  The function returns an empty
// string when the value is acceptable, or the reason it is not.
template <typename F>
struct Check
{
  F fn;
  std::string description;

  template <typename T>
  std::string check(const T& value) const
  {
    return val::forEachScalar(value, fn);
  }
  std::string describe() const { return description; }
};
template <typename F>
Check(F, std::string) -> Check<F>;
template <typename F>
Check(F, const char*) -> Check<F>;

// Unit tags.  Declared among the constraints of a field, they constrain
// nothing; they tell the Tcl layer which user unit to convert from.  A
// field has at most one, on a numeric value.
struct UnitTag
{
  Unit unit;
  template <typename T>
  std::string check(const T&) const
  {
    return {};
  }
  std::string describe() const { return {}; }
};
struct Distance : UnitTag
{
  Distance() : UnitTag{Unit::kDistance} {}
};
struct Time : UnitTag
{
  Time() : UnitTag{Unit::kTime} {}
};
struct Capacitance : UnitTag
{
  Capacitance() : UnitTag{Unit::kCapacitance} {}
};
struct Resistance : UnitTag
{
  Resistance() : UnitTag{Unit::kResistance} {}
};

////////////////////////////////////////////////////////////////
// Violations

// A value that a rule rejected, with the identity of the message to report.
struct Violation
{
  ToolId tool;
  int id;
  std::string message;
};

// Reports a violation through the logger.  Logger::error throws, so the first
// violation of an API call is the one the caller sees.
[[noreturn]] void report(const Violation& violation, Logger* logger);

////////////////////////////////////////////////////////////////
// Rule: the named constraints on one value, plus its message identity.

template <typename T>
class Rule
{
 public:
  template <typename... Cs>
  Rule(std::string name,
       const ToolId tool,
       const int id,
       std::string help,
       Cs&&... constraints)
      : name_(std::move(name)), help_(std::move(help)), tool_(tool), id_(id)
  {
    constexpr int kUnits
        = (0 + ... + (std::is_base_of_v<UnitTag, std::decay_t<Cs>> ? 1 : 0));
    static_assert(kUnits <= 1, "a field has at most one unit");
    static_assert(kUnits == 0 || val::IsNumeric<T>::value,
                  "a unit belongs on a numeric field");
    (add(std::forward<Cs>(constraints)), ...);
  }

  const std::string& name() const { return name_; }
  const std::string& help() const { return help_; }
  // Human readable form of each constraint, for help text.
  const std::vector<std::string>& constraints() const { return constraints_; }
  ToolId tool() const { return tool_; }
  int id() const { return id_; }
  Unit unit() const { return unit_; }

  // The violation of the first failing constraint, if any.
  std::optional<Violation> check(const T& value) const
  {
    for (const auto& check : checks_) {
      if (std::string reason = check(value); !reason.empty()) {
        return Violation{
            tool_, id_, name_ + " " + val::format(value) + " not " + reason};
      }
    }
    return {};
  }

  // Checks the value and reports a violation through the logger.
  void enforce(const T& value, Logger* logger) const
  {
    if (const std::optional<Violation> violation = check(value)) {
      report(*violation, logger);
    }
  }

 private:
  template <typename C>
  void add(C&& constraint)
  {
    if constexpr (std::is_base_of_v<UnitTag, std::decay_t<C>>) {
      unit_ = constraint.unit;
    } else {
      constraints_.push_back(constraint.describe());
      checks_.push_back([c = std::forward<C>(constraint)](const T& value) {
        return c.check(value);
      });
    }
  }

  std::string name_;
  std::string help_;
  ToolId tool_;
  int id_;
  Unit unit_ = Unit::kNone;
  std::vector<std::string> constraints_;
  std::vector<std::function<std::string(const T&)>> checks_;
};

////////////////////////////////////////////////////////////////
// Schema: the fields of an options struct.

// The language neutral description of one field, used by the Tcl and Python
// bridges and by the help text.
struct FieldMeta
{
  enum class Kind
  {
    kScalar,
    kList,
    kGroup
  };

  std::string name;
  std::string help;
  std::string type_name;
  std::vector<std::string> constraints;
  Kind kind = Kind::kScalar;
  Unit unit = Unit::kNone;
  ToolId tool;
  int id = 0;
  // The schema of a nested options struct, for Kind::kGroup.
  const SchemaBase* group = nullptr;
};

// One field of the flattened description of a schema, for help output.
struct FieldInfo
{
  std::string path;  // dotted, from the top-level struct
  std::string default_text;
  const FieldMeta* meta;
  int depth;
};

// The part of a schema that does not depend on the struct type.
class SchemaBase
{
 public:
  virtual ~SchemaBase() = default;

  ToolId tool() const { return tool_; }
  // The id used for the generic errors: an unknown key, a value of the wrong
  // type, and the violations of fields declared without their own id.
  int genericId() const { return generic_id_; }

  virtual std::vector<const FieldMeta*> fields() const = 0;
  // The field with the name, or nullptr.
  const FieldMeta* find(std::string_view name) const;
  // Follows a dotted path through nested groups; nullptr when any segment
  // is unknown or not a group.
  const FieldMeta* findPath(std::string_view path) const;

  // The default struct as a dictionary.
  virtual boost::json::value defaults() const = 0;
  // A default struct with the dictionary applied and validated, as a
  // dictionary of every field.  What a command given `given` will run with.
  virtual boost::json::value resolve(const boost::json::value& given,
                                     Logger* logger) const
      = 0;

  // Every field, recursively, with its default as text.
  std::vector<FieldInfo> describe() const;

 protected:
  SchemaBase(const ToolId tool, const int generic_id)
      : tool_(tool), generic_id_(generic_id)
  {
  }

  ToolId tool_;
  int generic_id_;
};

// Renders a description as plain text (for help) or as the README options
// table in Markdown.
std::string describeText(const SchemaBase& schema);
std::string describeMarkdown(const SchemaBase& schema);
// Renders the dictionary of a resolved struct, one `path value` line per
// field, with a marker on the values that differ from the defaults.
std::string reportValues(const SchemaBase& schema,
                         const boost::json::value& values);

template <typename S>
class Schema : public SchemaBase
{
 public:
  // A field of S: its description, and the operations on the member it
  // binds to.
  struct Field
  {
    FieldMeta meta;
    // Checks this field of value.
    std::function<std::optional<Violation>(const S& value)> check;
    // Sets this field of value from a dictionary value.  path is the dotted
    // path of the field, for messages.
    std::function<void(S& value,
                       const boost::json::value& json,
                       const std::string& path,
                       Logger* logger)>
        set;
    // This field of value as JSON.  With only_changed, a group contains only
    // the fields that differ from base; a scalar is returned regardless.
    std::function<
        boost::json::value(const S& value, const S& base, bool only_changed)>
        get;
  };

  // generic_id is the message id for an unknown key, a value of the wrong
  // type and the violations of fields declared without an id of their own.
  Schema(const ToolId tool, const int generic_id) : SchemaBase(tool, generic_id)
  {
  }

  // Declares the constraints on one member.  The name is the key in the
  // dictionary and the name used in messages; the id is the message id
  // reported when the value is rejected.
  template <typename T, typename... Cs>
  Schema& field(std::string name,
                T S::*member,
                const int id,
                std::string help,
                Cs&&... constraints)
  {
    return field(member,
                 Rule<T>(std::move(name),
                         tool_,
                         id,
                         std::move(help),
                         std::forward<Cs>(constraints)...));
  }

  // Declares a member without constraints.
  template <typename T>
  Schema& field(std::string name, T S::*member, std::string help)
  {
    return field(member,
                 Rule<T>(std::move(name), tool_, generic_id_, std::move(help)));
  }

  // Declares a member checked by an existing rule, so that a value shared
  // with a plain API argument is only declared once.
  template <typename T>
  Schema& field(T S::*member, Rule<T> rule)
  {
    static_assert(!HasSchema<T>::value,
                  "use group() for a nested options struct");
    Field entry;
    entry.meta.name = rule.name();
    entry.meta.help = rule.help();
    entry.meta.type_name = val::typeName<T>();
    entry.meta.constraints = rule.constraints();
    entry.meta.kind = (IsVector<T>::value || IsArray<T>::value)
                          ? FieldMeta::Kind::kList
                          : FieldMeta::Kind::kScalar;
    entry.meta.unit = rule.unit();
    entry.meta.tool = rule.tool();
    entry.meta.id = rule.id();
    entry.check
        = [member, rule](const S& value) { return rule.check(value.*member); };
    entry.set = [member, rule](S& value,
                               const boost::json::value& json,
                               const std::string& path,
                               Logger* logger) {
      if (std::string reason = val::fromJson(json, value.*member);
          !reason.empty()) {
        report(Violation{rule.tool(),
                         rule.id(),
                         path + " " + val::formatJson(json) + " not " + reason},
               logger);
      }
    };
    entry.get = [member](const S& value, const S&, bool) {
      return val::toJson(value.*member);
    };
    fields_.push_back(std::move(entry));
    return *this;
  }

  // Declares a nested options struct.  Its own schema describes and checks
  // its fields; in a dictionary it is a nested dictionary.
  template <typename G>
  Schema& group(std::string name, G S::*member, std::string help)
  {
    static_assert(HasSchema<G>::value,
                  "a group must publish a static schema()");
    const Schema<G>& schema = G::schema();
    Field entry;
    entry.meta.name = name;
    entry.meta.help = std::move(help);
    entry.meta.type_name = "group";
    entry.meta.kind = FieldMeta::Kind::kGroup;
    entry.meta.tool = tool_;
    entry.meta.id = generic_id_;
    entry.meta.group = &schema;
    entry.check = [member, name, &schema](const S& value) {
      std::optional<Violation> violation = schema.check(value.*member);
      if (violation) {
        violation->message = name + "." + violation->message;
      }
      return violation;
    };
    entry.set = [member, &schema](S& value,
                                  const boost::json::value& json,
                                  const std::string& path,
                                  Logger* logger) {
      schema.applyObject(value.*member, json, path, logger);
    };
    entry.get = [member, &schema](
                    const S& value, const S& base, const bool only_changed) {
      return schema.toJson(value.*member, base.*member, only_changed);
    };
    fields_.push_back(std::move(entry));
    return *this;
  }

  // Declares a check that spans more than one field, so that a combination
  // of values the API cannot honour is rejected wherever it is built.  The
  // predicate returns true when the value is acceptable.
  Schema& invariant(const int id,
                    std::string message,
                    std::function<bool(const S& value)> ok)
  {
    return invariant(
        id, [message = std::move(message), ok = std::move(ok)](const S& value) {
          return ok(value) ? std::string() : message;
        });
  }

  // The same, when the message depends on the value.  The check returns an
  // empty string when the value is acceptable.
  Schema& invariant(const int id,
                    std::function<std::string(const S& value)> check)
  {
    invariants_.push_back({tool_, id, std::move(check)});
    return *this;
  }

  std::vector<const FieldMeta*> fields() const override
  {
    std::vector<const FieldMeta*> result;
    result.reserve(fields_.size());
    for (const Field& field : fields_) {
      result.push_back(&field.meta);
    }
    return result;
  }

  // The violation of the first failing field or invariant, if any.
  std::optional<Violation> check(const S& value) const
  {
    for (const Field& field : fields_) {
      if (std::optional<Violation> violation = field.check(value)) {
        return violation;
      }
    }
    for (const Invariant& invariant : invariants_) {
      if (std::string message = invariant.check(value); !message.empty()) {
        return Violation{invariant.tool, invariant.id, std::move(message)};
      }
    }
    return {};
  }

  // Checks every field and reports a violation through the logger.
  void enforce(const S& value, Logger* logger) const
  {
    if (const std::optional<Violation> violation = check(value)) {
      report(*violation, logger);
    }
  }

  // Sets the fields named in the dictionary, then validates the whole
  // struct.  Keys may be nested dictionaries or dotted paths.  Fields not
  // named keep their current value.
  void apply(S& value, const boost::json::value& json, Logger* logger) const
  {
    applyObject(value, json, "", logger);
    enforce(value, logger);
  }

  // apply() without the validation, for a caller that has more to set.
  void applyObject(S& value,
                   const boost::json::value& json,
                   const std::string& path,
                   Logger* logger) const
  {
    const boost::json::object* object = json.if_object();
    if (object == nullptr) {
      report(Violation{tool_,
                       generic_id_,
                       (path.empty() ? std::string("options") : path) + " "
                           + val::formatJson(json) + " not a dictionary"},
             logger);
    }
    for (const auto& [key, item] : *object) {
      const std::string_view key_view(key);
      const std::size_t dot = key_view.find('.');
      const std::string_view head = key_view.substr(0, dot);
      const Field* field = findField(head);
      const std::string field_path
          = path.empty() ? std::string(head) : path + "." + std::string(head);
      if (field == nullptr) {
        report(Violation{tool_, generic_id_, unknownKeyMessage(field_path)},
               logger);
      }
      if (dot == std::string_view::npos) {
        field->set(value, item, field_path, logger);
      } else {
        if (field->meta.kind != FieldMeta::Kind::kGroup) {
          report(Violation{tool_,
                           generic_id_,
                           field_path + " is not a group; " + std::string(key)
                               + " cannot be set"},
                 logger);
        }
        boost::json::object rest;
        rest[key_view.substr(dot + 1)] = item;
        field->set(value, rest, field_path, logger);
      }
    }
  }

  // The struct as a dictionary.  With only_changed, only the fields whose
  // value differs from a default constructed struct.
  boost::json::value toJson(const S& value,
                            const bool only_changed = false) const
  {
    return toJson(value, S{}, only_changed);
  }

  // The struct as a dictionary, compared against base when only_changed.
  boost::json::value toJson(const S& value,
                            const S& base,
                            const bool only_changed) const
  {
    boost::json::object result;
    for (const Field& field : fields_) {
      boost::json::value item = field.get(value, base, only_changed);
      if (only_changed) {
        if (field.meta.kind == FieldMeta::Kind::kGroup) {
          if (item.as_object().empty()) {
            continue;
          }
        } else if (item == field.get(base, base, false)) {
          continue;
        }
      }
      result[field.meta.name] = std::move(item);
    }
    return result;
  }

  boost::json::value defaults() const override { return toJson(S{}, false); }

  boost::json::value resolve(const boost::json::value& given,
                             Logger* logger) const override
  {
    S value{};
    apply(value, given, logger);
    return toJson(value, false);
  }

 private:
  struct Invariant
  {
    ToolId tool;
    int id;
    std::function<std::string(const S& value)> check;
  };

  const Field* findField(std::string_view name) const
  {
    for (const Field& field : fields_) {
      if (field.meta.name == name) {
        return &field;
      }
    }
    return nullptr;
  }

  std::string unknownKeyMessage(const std::string& path) const
  {
    std::string message = "unknown option " + path + "; valid options:";
    for (const Field& field : fields_) {
      message += ' ' + field.meta.name;
    }
    return message;
  }

  std::vector<Field> fields_;
  std::vector<Invariant> invariants_;
};

// Verifies every field of an options struct against its schema.  Call this
// from the C++ API entry points that take the struct, so that C++, Python and
// Tcl callers are all verified.
template <typename S>
void validate(const S& value, Logger* logger)
{
  static_assert(HasSchema<S>::value,
                "the struct must publish a static schema()");
  S::schema().enforce(value, logger);
}

}  // namespace utl
