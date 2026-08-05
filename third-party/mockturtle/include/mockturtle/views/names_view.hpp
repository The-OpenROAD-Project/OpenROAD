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
  \file names_view.hpp
  \brief Implements methods to declare names for network signals

  \author Heinz Riener
  \author Marcel Walter
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"

#include <map>
#include <string>

namespace mockturtle
{

template<class Ntk>
class names_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  template<typename StrType = const char*>
  names_view( Ntk const& ntk = Ntk(), StrType name = "" )
      : Ntk( ntk ), _network_name{ name }
  {
  }

  names_view( names_view<Ntk> const& named_ntk )
      : Ntk( named_ntk ), _network_name( named_ntk._network_name ), _signal_names( named_ntk._signal_names ), _output_names( named_ntk._output_names )
  {
  }

  names_view<Ntk>& operator=( names_view<Ntk> const& named_ntk )
  {
    if ( this != &named_ntk ) // Check for self-assignment
    {
      Ntk::operator=( named_ntk );
      _signal_names = named_ntk._signal_names;
      _network_name = named_ntk._network_name;
      _output_names = named_ntk._output_names;
    }
    return *this;
  }

  /*! \brief Creates a primary input and set its name.
   *
   * \param name Name of the created primary input
   */
  signal create_pi( std::string const& name = {} )
  {
    const auto s = Ntk::create_pi();
    if ( !name.empty() )
    {
      set_name( s, name );
    }
    return s;
  }

  /*! \brief Creates a primary output and set its name.
   *
   * \param s Signal that drives the created primary output
   * \param name Name of the created primary output
   */
  void create_po( signal const& s, std::string const& name = {} )
  {
    const auto index = Ntk::num_pos();
    Ntk::create_po( s );
    if ( !name.empty() )
    {
      set_output_name( index, name );
    }
  }

  /*! \brief Sets network name.
   *
   * \param name Name of the network
   */
  template<typename StrType = const char*>
  void set_network_name( StrType name ) noexcept
  {
    _network_name = name;
  }

  /*! \brief Gets network name.
   *
   * \return Network name
   */
  std::string get_network_name() const noexcept
  {
    return _network_name;
  }

  /*! \brief Checks if a signal has a name.
   *
   * Note that complemented signals may have different names.
   *
   * \param s Signal to be checked
   * \return Whether the signal has a name in record
   */
  bool has_name( signal const& s ) const
  {
    return ( _signal_names.find( s ) != _signal_names.end() );
  }

  /*! \brief Sets the name for a signal.
   *
   * Note that names are set separately for complemented signals.
   *
   * \param s Signal to be set a name
   * \param name Name of the signal
   */
  void set_name( signal const& s, std::string const& name )
  {
    _signal_names[s] = name;
  }

  /*! \brief Gets signal name.
   *
   * Note that complemented signals may have different names.
   *
   * \param s Signal to be queried
   * \return Name of the signal
   */
  std::string get_name( signal const& s ) const
  {
    return _signal_names.at( s );
  }

  /*! \brief Checks if a primary output has a name.
   *
   * \param index Index of the primary output to be checked
   * \return Whether the primary output has a name in record
   */
  bool has_output_name( uint32_t index ) const
  {
    return ( _output_names.find( index ) != _output_names.end() );
  }

  /*! \brief Sets the name for a primary output.
   *
   * Note that even if two primary outputs are driven by
   * the same signal, they may have different names.
   *
   * \param index Index of the primary output to set a name
   * \param name Name of the primary output
   */
  void set_output_name( uint32_t index, std::string const& name )
  {
    _output_names[index] = name;
  }

  /*! \brief Gets the name of a primary output.
   *
   * Note that even if two primary outputs are driven by
   * the same signal, they may have different names.
   *
   * \param index Index of the primary output to be queried
   * \return Name of the primary output
   */
  std::string get_output_name( uint32_t index ) const
  {
    return _output_names.at( index );
  }

private:
  std::string _network_name;
  std::map<signal, std::string> _signal_names;
  std::map<uint32_t, std::string> _output_names;
}; /* names_view */

template<class T>
names_view( T const& ) -> names_view<T>;

template<class T>
names_view( T const&, typename T::signal const& ) -> names_view<T>;

} // namespace mockturtle