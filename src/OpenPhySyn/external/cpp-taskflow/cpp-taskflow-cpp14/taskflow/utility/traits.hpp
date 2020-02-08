#pragma once

#include <type_traits>
#include <iterator>
#include <iostream>
#include <mutex>
#include <deque>
#include <vector>
#include <algorithm>
#include <thread>
#include <future>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <list>
#include <forward_list>
#include <numeric>
#include <iomanip>
#include <cassert>
#include <cmath>

namespace tf {

//-----------------------------------------------------------------------------
// Traits
//-----------------------------------------------------------------------------

// Macro to check whether a class has a member function
#define define_has_member(member_name)                                     \
template <typename T>                                                      \
class has_member_##member_name                                             \
{                                                                          \
  typedef char yes_type;                                                   \
  typedef long no_type;                                                    \
  template <typename U> static yes_type test(decltype(&U::member_name));   \
  template <typename U> static no_type  test(...);                         \
  public:                                                                  \
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type);  \
}

#define has_member(class_, member_name)  has_member_##member_name<class_>::value

// Struct: dependent_false
template <typename... T>
struct dependent_false { 
  static constexpr bool value = false; 
};

// This cannot be compiled on GCC-4.9
//template <typename... T>
//constexpr auto dependent_false_v = dependent_false<T...>::value;

// Struct: is_iterator
template <typename T, typename = void>
struct is_iterator {
  static constexpr bool value = false;
};

template <typename T>
struct is_iterator<
  T, 
  //std::enable_if_t<!std::is_same_v<typename std::iterator_traits<T>::value_type, void>>
  std::enable_if_t<!std::is_same<typename std::iterator_traits<T>::value_type, void>::value>
> {
  static constexpr bool value = true;
};

//template <typename T>
//inline constexpr bool is_iterator_v = is_iterator<T>::value;

template< class... >
using void_t = void;

// Struct: is_iterable
template <typename T, typename = void>
struct is_iterable : std::false_type {
};

template <typename T>
struct is_iterable<T, void_t<decltype(std::declval<T>().begin()),
                             decltype(std::declval<T>().end())>>
  : std::true_type {
};

//template <typename T>
//inline constexpr bool is_iterable_v = is_iterable<T>::value;

// Struct: MoC
// Move-on-copy wrapper.
template <typename T>
struct MoC {

  MoC(T&& rhs) : object(std::move(rhs)) {}
  MoC(const MoC& other) : object(std::move(other.object)) {}

  T& get() { return object; }
  
  mutable T object; 
};

//-----------------------------------------------------------------------------
// Functors.
//-----------------------------------------------------------------------------

//// Overloadded.
//template <typename... Ts>
//struct Functors : Ts... { 
//  using Ts::operator()... ;
//};
//
//template <typename... Ts>
//Functors(Ts...) -> Functors<Ts...>;



// https://stackoverflow.com/questions/51187974/can-stdis-invocable-be-emulated-within-c11
template <typename F, typename... Args>
struct is_invocable :
    std::is_constructible<
        std::function<void(Args ...)>,
        std::reference_wrapper<typename std::remove_reference<F>::type>
    >
{};

template <typename R, typename F, typename... Args>
struct is_invocable_r :
    std::is_constructible<
        std::function<R(Args ...)>,
        std::reference_wrapper<typename std::remove_reference<F>::type>
    >
{};



}  // end of namespace tf. ---------------------------------------------------
