// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace utl {

class Logger;

// A small type-indexed registry that lets modules publish abstract
// interfaces and lets consumers look them up without a shared base
// class or a hard link-time dependency on the provider.
//
// Each interface type may have at most one provider at a time; a
// duplicate provide<I>() call is a programming error and is reported
// through the logger.
class ServiceRegistry
{
 public:
  explicit ServiceRegistry(Logger* logger);

  template <class I>
  void provide(I* impl)
  {
    provideImpl(std::type_index(typeid(I)), impl, typeid(I).name());
  }

  template <class I>
  void withdraw(I* impl)
  {
    withdrawImpl(std::type_index(typeid(I)), impl);
  }

  template <class I>
  I* find() const
  {
    return static_cast<I*>(findImpl(std::type_index(typeid(I))));
  }

  template <class I>
  I& require() const
  {
    return *static_cast<I*>(
        requireImpl(std::type_index(typeid(I)), typeid(I).name()));
  }

 private:
  void provideImpl(std::type_index key, void* impl, const char* name);
  void withdrawImpl(std::type_index key, void* impl);
  void* findImpl(std::type_index key) const;
  void* requireImpl(std::type_index key, const char* name) const;

  Logger* logger_;
  std::unordered_map<std::type_index, void*> services_;
};

}  // namespace utl
