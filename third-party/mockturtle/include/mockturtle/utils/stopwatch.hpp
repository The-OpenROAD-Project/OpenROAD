/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file stopwatch.hpp
  \brief Stopwatch

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <chrono>
#include <iostream>
#include <type_traits>

#include <fmt/format.h>

namespace mockturtle
{

/*! \brief Stopwatch interface
 *
 * This class implements a stopwatch interface to track time.  It starts
 * tracking time at construction and stops tracking time at deletion
 * automatically.  A reference to a duration object is passed to the
 * constructor.  After stopping the time the measured time interval is added
 * to the durationr reference.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration time{0};

      { // some block
        stopwatch t( time );

        // do some work
      } // stopwatch is stopped here

      std::cout << fmt::format( "{:5.2f} seconds passed\n", to_seconds( time ) );
   \endverbatim
 */
template<class Clock = std::chrono::steady_clock>
class stopwatch
{
public:
  using clock = Clock;
  using duration = typename Clock::duration;
  using time_point = typename Clock::time_point;

  /*! \brief Default constructor.
   *
   * Starts tracking time.
   */
  explicit stopwatch( duration& dur )
      : dur( dur ),
        beg( clock::now() )
  {
  }

  /*! \brief Default deconstructor.
   *
   * Stops tracking time and updates duration.
   */
  ~stopwatch()
  {
    dur += ( clock::now() - beg );
  }

private:
  duration& dur;
  time_point beg;
};

/*! \brief Calls a function and tracks time.
 *
 * The function that is passed as second parameter can be any callable object
 * that takes no parameters.  This construction can be used to avoid
 * pre-declaring the result type of a computation that should be tracked.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration time{0};

      auto result = call_with_stopwatch( time, [&]() { return function( parameters ); } );
   \endverbatim
 *
 * \param dur Duration reference (time will be added to it)
 * \param fn Callable object with no arguments
 */
template<class Fn, class Clock = std::chrono::steady_clock>
std::invoke_result_t<Fn> call_with_stopwatch( typename Clock::duration& dur, Fn&& fn )
{
  stopwatch<Clock> t( dur );
  return fn();
}

/*! \brief Constructs an object and calls time.
 *
 * This function can track the time for the construction of an object and
 * returns the constructed object.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration time{0};

      // create vector with 100000 elements initialized to 42
      auto result = make_with_stopwatch<std::vector<int>>( time, 100000, 42 );
   \endverbatim
 */
template<class T, class... Args, class Clock = std::chrono::steady_clock>
T make_with_stopwatch( typename Clock::duration& dur, Args... args )
{
  stopwatch<Clock> t( dur );
  return T{ std::forward<Args>( args )... };
}

/*! \brief Utility function to convert duration into seconds. */
template<class Duration>
inline double to_seconds( Duration const& dur )
{
  return std::chrono::duration_cast<std::chrono::duration<double>>( dur ).count();
}

template<class Clock = std::chrono::steady_clock>
class print_time
{
public:
  print_time()
      : _t( new stopwatch<Clock>( _d ) )
  {
  }

  ~print_time()
  {
    delete _t;
    std::cout << fmt::format( "[i] run-time: {:5.2f} secs\n", to_seconds( _d ) );
  }

private:
  stopwatch<Clock>* _t{ nullptr };
  typename stopwatch<Clock>::duration _d{};
};

} // namespace mockturtle