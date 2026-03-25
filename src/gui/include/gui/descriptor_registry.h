// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <any>
#include <cstddef>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}

namespace gui {

class Descriptor;
class Selected;

class DescriptorRegistry
{
 public:
  static DescriptorRegistry* instance();

  void setLogger(utl::Logger* logger);

  // Register all standard descriptors (db + sta).
  // Must be called after db/sta are initialized.
  void initDescriptors(odb::dbDatabase* db, sta::dbSta* sta);

  template <class T>
  void registerDescriptor(const Descriptor* descriptor)
  {
    registerDescriptor(typeid(T), descriptor);
  }

  template <class T>
  void unregisterDescriptor()
  {
    unregisterDescriptor(typeid(T));
  }

  template <class T>
  const Descriptor* getDescriptor() const
  {
    return getDescriptor(typeid(T));
  }

  Selected makeSelected(const std::any& object);

  void registerDescriptor(const std::type_info& type,
                          const Descriptor* descriptor);
  void unregisterDescriptor(const std::type_info& type);
  const Descriptor* getDescriptor(const std::type_info& type) const;

  // Iterate over all registered descriptors (used by Gui::select)
  template <typename Func>
  void forEachDescriptor(Func&& func) const
  {
    for (auto& [type, descriptor] : descriptors_) {
      func(descriptor.get());
    }
  }

 private:
  DescriptorRegistry() = default;

  // RTTI-safe hasher/comparator for std::type_index.
  // Handles libstdc++ vs libc++ differences where the latter can
  // produce different type_index values for the same type across
  // compilation units.
  struct TypeInfoHasher
  {
    std::size_t operator()(const std::type_index& x) const;
  };
  struct TypeInfoComparator
  {
    bool operator()(const std::type_index& a, const std::type_index& b) const;
  };

  std::unordered_map<std::type_index,
                     std::unique_ptr<const Descriptor>,
                     TypeInfoHasher,
                     TypeInfoComparator>
      descriptors_;

  utl::Logger* logger_ = nullptr;
};

}  // namespace gui
