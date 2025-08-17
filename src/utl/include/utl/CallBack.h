// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

namespace utl {
class CallBackHandler;

/**
 * @brief Base class for callbacks to enable inter-module communication.
 *
 * This class provides a standardized interface for modules to communicate
 * with each other through callback mechanisms. Derived classes should
 * implement specific callback methods as needed.
 */
class CallBack
{
 public:
  virtual ~CallBack();

  /**
   * @brief Called when pin access needs to be updated.
   *
   * This callback is triggered when modules detect that access points need to
   * be recalculated or updated.
   */
  virtual void onPinAccessUpdateRequired() {}

  /**
   * @brief Called when parasitic estimation is required.
   *
   * This callback is triggered when modules need to perform parasitic
   * estimation for timing.
   */
  virtual void onEstimateParasiticsRequired() {}

  void setOwner(CallBackHandler* owner);
  CallBackHandler* getOwner() const { return owner_; }
  void removeOwner();

 private:
  CallBackHandler* owner_{nullptr};
};

}  // namespace utl
