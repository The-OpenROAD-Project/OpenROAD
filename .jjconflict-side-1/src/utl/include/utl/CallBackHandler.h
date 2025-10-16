// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <set>

namespace utl {
class Logger;
class CallBack;

/**
 * @brief Handler class for managing callback registration and triggering.
 *
 * This class provides a centralized mechanism for modules to register
 * callbacks and trigger them when specific events occur.
 */
class CallBackHandler
{
 public:
  CallBackHandler(utl::Logger* logger);
  ~CallBackHandler() = default;

  /**
   * @brief Register a callback for event notifications.
   *
   * @param callback Pointer to the callback object to register
   */
  void addCallBack(CallBack* callback);

  /**
   * @brief Unregister a callback from event notifications.
   *
   * @param callback Pointer to the callback object to unregister
   */
  void removeCallBack(CallBack* callback);

  /**
   * @brief Trigger the onPinAccessUpdateRequired callback for all registered
   * callbacks.
   */
  void triggerOnPinAccessUpdateRequired();

  /**
   * @brief Trigger the onEstimateParasiticsRequired callback for all registered
   * callbacks.
   */
  void triggerOnEstimateParasiticsRequired();

 private:
  utl::Logger* logger_;
  std::set<CallBack*> callbacks_;
};

}  // namespace utl
