// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/OptionsTcl.h"

#include <string>
#include <string_view>
#include <vector>

#include "utl/Logger.h"

#if TCL_MAJOR_VERSION < 9 && !defined(Tcl_Size)
using Tcl_Size = int;
#endif

namespace utl {

std::vector<std::string> argvFromTcl(Tcl_Interp* interp,
                                     Tcl_Obj* list,
                                     Logger* logger)
{
  Tcl_Size count = 0;
  Tcl_Obj** elems = nullptr;
  if (Tcl_ListObjGetElements(interp, list, &count, &elems) != TCL_OK) {
    logger->error(UTL, 111, "{} is not a list.", Tcl_GetString(list));
  }
  std::vector<std::string> result;
  result.reserve(count);
  for (Tcl_Size i = 0; i < count; ++i) {
    result.emplace_back(Tcl_GetString(elems[i]));
  }
  return result;
}

namespace {

boost::json::value convertLeaf(Tcl_Interp* interp,
                               Tcl_Obj* obj,
                               const FieldMeta* meta,
                               const std::string& path,
                               const UnitScaler& scaler,
                               Logger* logger);

boost::json::value scaleScalar(Tcl_Obj* obj,
                               const FieldMeta& meta,
                               const UnitScaler& scaler)
{
  const std::string text = Tcl_GetString(obj);
  if (meta.unit == Unit::kNone || !scaler) {
    return boost::json::value(text);
  }
  double value = 0;
  if (!val::parseDouble(text, value)) {
    // Left as a string so that the schema reports "not a float".
    return boost::json::value(text);
  }
  return boost::json::value(scaler(meta.unit, value));
}

boost::json::object convertDict(Tcl_Interp* interp,
                                Tcl_Obj* dict,
                                const SchemaBase& schema,
                                const std::string& path,
                                const UnitScaler& scaler,
                                Logger* logger)
{
  boost::json::object result;
  Tcl_DictSearch search;
  Tcl_Obj* key = nullptr;
  Tcl_Obj* value = nullptr;
  int done = 0;
  if (Tcl_DictObjFirst(interp, dict, &search, &key, &value, &done) != TCL_OK) {
    logger->error(schema.tool(),
                  schema.genericId(),
                  "{} {} not a dictionary",
                  path.empty() ? "options" : path,
                  Tcl_GetString(dict));
  }
  for (; !done; Tcl_DictObjNext(&search, &key, &value, &done)) {
    const std::string name = Tcl_GetString(key);
    const std::string field_path = path.empty() ? name : path + "." + name;
    const FieldMeta* meta = schema.findPath(name);
    result[name] = convertLeaf(interp, value, meta, field_path, scaler, logger);
  }
  Tcl_DictObjDone(&search);
  return result;
}

boost::json::value convertLeaf(Tcl_Interp* interp,
                               Tcl_Obj* obj,
                               const FieldMeta* meta,
                               const std::string& path,
                               const UnitScaler& scaler,
                               Logger* logger)
{
  if (meta == nullptr) {
    // Unknown key: the schema reports it with the valid names.
    return boost::json::value(Tcl_GetString(obj));
  }
  switch (meta->kind) {
    case FieldMeta::Kind::kGroup:
      return convertDict(interp, obj, *meta->group, path, scaler, logger);
    case FieldMeta::Kind::kList: {
      Tcl_Size count = 0;
      Tcl_Obj** elems = nullptr;
      if (Tcl_ListObjGetElements(interp, obj, &count, &elems) != TCL_OK) {
        logger->error(
            meta->tool, meta->id, "{} {} not a list", path, Tcl_GetString(obj));
      }
      boost::json::array result;
      for (Tcl_Size i = 0; i < count; ++i) {
        result.push_back(scaleScalar(elems[i], *meta, scaler));
      }
      return result;
    }
    case FieldMeta::Kind::kScalar:
      return scaleScalar(obj, *meta, scaler);
  }
  return boost::json::value(Tcl_GetString(obj));
}

}  // namespace

boost::json::object tclToJson(Tcl_Interp* interp,
                              Tcl_Obj* dict,
                              const SchemaBase& schema,
                              const UnitScaler& scaler,
                              Logger* logger)
{
  return convertDict(interp, dict, schema, "", scaler, logger);
}

DictParser tclDictParser(Tcl_Interp* interp,
                         const SchemaBase& schema,
                         const UnitScaler& scaler,
                         Logger* logger)
{
  return [interp, &schema, scaler, logger](const std::string& text) {
    Tcl_Obj* dict
        = Tcl_NewStringObj(text.data(), static_cast<Tcl_Size>(text.size()));
    Tcl_IncrRefCount(dict);
    boost::json::object result;
    try {
      result = tclToJson(interp, dict, schema, scaler, logger);
    } catch (...) {
      Tcl_DecrRefCount(dict);
      throw;
    }
    Tcl_DecrRefCount(dict);
    return result;
  };
}

Tcl_Obj* jsonToTcl(Tcl_Interp* interp, const boost::json::value& value)
{
  namespace json = boost::json;
  switch (value.kind()) {
    case json::kind::null:
      return Tcl_NewStringObj("", -1);
    case json::kind::bool_:
      return Tcl_NewBooleanObj(value.get_bool());
    case json::kind::int64:
      return Tcl_NewWideIntObj(value.get_int64());
    case json::kind::uint64:
      return Tcl_NewWideIntObj(static_cast<Tcl_WideInt>(value.get_uint64()));
    case json::kind::double_:
      return Tcl_NewDoubleObj(value.get_double());
    case json::kind::string: {
      const json::string& str = value.get_string();
      return Tcl_NewStringObj(str.data(), static_cast<Tcl_Size>(str.size()));
    }
    case json::kind::array: {
      Tcl_Obj* list = Tcl_NewListObj(0, nullptr);
      for (const json::value& elem : value.get_array()) {
        Tcl_ListObjAppendElement(interp, list, jsonToTcl(interp, elem));
      }
      return list;
    }
    case json::kind::object: {
      Tcl_Obj* dict = Tcl_NewDictObj();
      for (const auto& [key, item] : value.get_object()) {
        Tcl_DictObjPut(
            interp,
            dict,
            Tcl_NewStringObj(key.data(), static_cast<Tcl_Size>(key.size())),
            jsonToTcl(interp, item));
      }
      return dict;
    }
  }
  return Tcl_NewStringObj("", -1);
}

std::string jsonToTclString(Tcl_Interp* interp, const boost::json::value& value)
{
  Tcl_Obj* obj = jsonToTcl(interp, value);
  Tcl_IncrRefCount(obj);
  std::string result = Tcl_GetString(obj);
  Tcl_DecrRefCount(obj);
  return result;
}

}  // namespace utl
