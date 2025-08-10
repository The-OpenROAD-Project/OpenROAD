// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <set>

#include "utl/CallBack.h"

namespace utl {

/// @brief Handler class for managing callback registration and triggering.
///
/// This class provides a centralized mechanism for modules to register
/// callbacks and trigger them when specific events occur.
class CallBackHandler
{
 public:
  CallBackHandler() = default;
  ~CallBackHandler() = default;

  /// @brief Register a callback for event notifications.
  ///
  /// @param callback Pointer to the callback object to register
  void addCallBack(CallBack* callback);

  /// @brief Unregister a callback from event notifications.
  ///
  /// @param callback Pointer to the callback object to unregister
  void removeCallBack(CallBack* callback);

  /// @brief Trigger the onPinAccessUpdateRequired callback for all registered
  /// callbacks.
  ///
  /// This method iterates through all registered callbacks and calls their
  /// onPinAccessUpdateRequired method.
  void triggerOnPinAccessUpdateRequired();

 private:
  /// @brief Set of registered callbacks.
  std::set<CallBack*> callbacks_;
};

}  // namespace utl
