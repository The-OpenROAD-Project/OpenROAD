// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Python side of options structs (utl/Validate.h).
//
//   %options_struct(gpl::PlaceOptions, PlaceOptions)
//
// after the %include of the struct's header gives the proxy class:
//   - a constructor taking a dict and/or keyword arguments, nested dicts
//     for groups: PlaceOptions(density=0.6, timing=dict(enabled=True))
//   - update(dict, **kwargs), to_dict(only_changed=False), __repr__
//   - an AttributeError for a misspelled option
//   - nested struct attributes that keep their parent alive.
// The Python values cross into C++ as JSON text, so that the schema is the
// only converter, and the Python wrapper does not need the full Python API.

%{
#include <string>

#include <boost/json.hpp>

#include "utl/Logger.h"
#include "utl/Validate.h"

#ifndef BAZEL
#include "ord/OpenRoad.hh"
#endif

static utl::Logger* optionsLogger()
{
#ifdef BAZEL
  return utl::Logger::defaultLogger();
#else
  ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
  return openroad != nullptr ? openroad->getLogger()
                             : utl::Logger::defaultLogger();
#endif
}
%}

%include <std_string.i>

%pythoncode %{
import json as _options_json


def _options_encode(obj):
    if hasattr(obj, "to_dict"):
        return obj.to_dict()
    raise TypeError("%r is not an options value" % (obj,))


def _options_group_property(prop):
    def fget(self):
        child = prop.fget(self)
        # The child proxy points into the parent; keep the parent alive.
        child._options_parent = self
        return child

    def fset(self, value):
        if isinstance(value, dict):
            prop.fget(self).update(value)
        else:
            prop.fset(self, value)

    return property(fget, fset)


def _options_install(cls):
    swig_init = cls.__init__

    def __init__(self, *args, **kwargs):
        swig_init(self)
        if args or kwargs:
            self.update(*args, **kwargs)

    def update(self, *args, **kwargs):
        values = dict(*args, **kwargs)
        self._update_json(_options_json.dumps(values, default=_options_encode))
        return self

    def to_dict(self, only_changed=False):
        return _options_json.loads(self._to_json(only_changed))

    def __repr__(self):
        return "%s(%s)" % (cls.__name__, _options_json.dumps(self.to_dict(True)))

    def __setattr__(self, name, value):
        # "this" is SWIG's pointer to the C++ object.
        if name != "this" and not name.startswith("_") and not hasattr(cls, name):
            raise AttributeError(
                "%s has no option %r; valid options: %s"
                % (cls.__name__, name, " ".join(cls._option_names().split())))
        object.__setattr__(self, name, value)

    for name in cls._group_names().split():
        setattr(cls, name, _options_group_property(getattr(cls, name)))
    cls.__init__ = __init__
    cls.update = update
    cls.to_dict = to_dict
    cls.__repr__ = __repr__
    cls.__setattr__ = __setattr__
%}

%define %options_struct(TYPE, PYNAME)
%extend TYPE {
  std::string _to_json(bool only_changed) const
  {
    return boost::json::serialize(TYPE::schema().toJson(*$self, only_changed));
  }
  void _update_json(const std::string& text)
  {
    TYPE::schema().apply(*$self, boost::json::parse(text), optionsLogger());
  }
  static std::string _group_names()
  {
    std::string names;
    for (const utl::FieldMeta* field : TYPE::schema().fields()) {
      if (field->kind == utl::FieldMeta::Kind::kGroup) {
        names += field->name + " ";
      }
    }
    return names;
  }
  static std::string _option_names()
  {
    std::string names;
    for (const utl::FieldMeta* field : TYPE::schema().fields()) {
      names += field->name + " ";
    }
    return names;
  }
  static std::string describe()
  {
    return utl::describeText(TYPE::schema());
  }
}
%pythoncode {
_options_install(PYNAME)
}
%enddef
