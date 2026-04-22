// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "utl/ServiceRegistry.h"

#include <typeindex>

#include "utl/Logger.h"

namespace utl {

ServiceRegistry::ServiceRegistry(Logger* logger) : logger_(logger)
{
}

void ServiceRegistry::provideImpl(std::type_index key,
                                  void* impl,
                                  const char* name)
{
  if (impl == nullptr) {
    logger_->error(
        UTL, 201, "Registering null service for {} is not allowed", name);
  }
  auto [it, inserted] = services_.emplace(key, impl);
  if (!inserted) {
    logger_->error(UTL, 202, "Service {} already has a provider", name);
  }
}

void ServiceRegistry::withdrawImpl(std::type_index key, void* impl)
{
  auto it = services_.find(key);
  if (it != services_.end() && it->second == impl) {
    services_.erase(it);
  }
}

void* ServiceRegistry::findImpl(std::type_index key) const
{
  auto it = services_.find(key);
  return it == services_.end() ? nullptr : it->second;
}

void* ServiceRegistry::requireImpl(std::type_index key, const char* name) const
{
  auto it = services_.find(key);
  if (it == services_.end()) {
    logger_->error(UTL, 203, "Required service {} has no provider", name);
  }
  return it->second;
}

}  // namespace utl
