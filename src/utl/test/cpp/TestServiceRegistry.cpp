// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <stdexcept>

#include "gtest/gtest.h"
#include "utl/Logger.h"
#include "utl/ServiceRegistry.h"

namespace utl {

namespace {

struct IFoo
{
  virtual ~IFoo() = default;
  virtual int value() const = 0;
};

struct IBar
{
  virtual ~IBar() = default;
  virtual int other() const = 0;
};

struct FooImpl : IFoo
{
  explicit FooImpl(int v) : v_(v) {}
  int value() const override { return v_; }
  int v_;
};

struct BarImpl : IBar
{
  int other() const override { return 7; }
};

}  // namespace

TEST(ServiceRegistry, FindReturnsNullWhenAbsent)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  EXPECT_EQ(registry.find<IFoo>(), nullptr);
}

TEST(ServiceRegistry, ProvideAndFind)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  FooImpl foo(42);
  registry.provide<IFoo>(&foo);
  EXPECT_EQ(registry.find<IFoo>(), &foo);
  EXPECT_EQ(registry.find<IFoo>()->value(), 42);
}

TEST(ServiceRegistry, DifferentInterfacesAreIndependent)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  FooImpl foo(1);
  BarImpl bar;
  registry.provide<IFoo>(&foo);
  registry.provide<IBar>(&bar);
  EXPECT_EQ(registry.find<IFoo>(), &foo);
  EXPECT_EQ(registry.find<IBar>(), &bar);
}

TEST(ServiceRegistry, RequireReturnsReference)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  FooImpl foo(5);
  registry.provide<IFoo>(&foo);
  EXPECT_EQ(&registry.require<IFoo>(), &foo);
}

TEST(ServiceRegistry, RequireErrorsWhenAbsent)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  EXPECT_THROW(registry.require<IFoo>(), std::runtime_error);
}

TEST(ServiceRegistry, DuplicateProvideErrors)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  FooImpl a(1), b(2);
  registry.provide<IFoo>(&a);
  EXPECT_THROW(registry.provide<IFoo>(&b), std::runtime_error);
}

TEST(ServiceRegistry, NullProvideErrors)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  EXPECT_THROW(registry.provide<IFoo>(nullptr), std::runtime_error);
}

TEST(ServiceRegistry, WithdrawRemovesOnlyMatchingProvider)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  FooImpl a(1), b(2);
  registry.provide<IFoo>(&a);

  // Withdraw with a non-matching pointer must be a no-op.
  registry.withdraw<IFoo>(&b);
  EXPECT_EQ(registry.find<IFoo>(), &a);

  registry.withdraw<IFoo>(&a);
  EXPECT_EQ(registry.find<IFoo>(), nullptr);

  // After withdraw a new provider can register.
  registry.provide<IFoo>(&b);
  EXPECT_EQ(registry.find<IFoo>(), &b);
}

TEST(ServiceRegistry, WithdrawUnknownIsNoop)
{
  Logger logger;
  ServiceRegistry registry(&logger);
  FooImpl a(1);
  registry.withdraw<IFoo>(&a);  // Nothing registered — must not throw.
  SUCCEED();
}

}  // namespace utl
