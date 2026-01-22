/* lorina: C++ parsing library
 * Copyright (C) 2018-2021  EPFL
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
  \file diagnostics.hpp
  \brief Implements diagnostics.

  \author Heinz Riener
*/

#pragma once

#include <cassert>
#include <format>
#include <iostream>
#include <map>
#include <string_view>

namespace lorina
{

class diagnostic_builder;
class diagnostic_consumer;
class diagnostic_engine;

enum class diagnostic_level
{
  ignore = 1,
  note = 2,
  remark = 3,
  warning = 4,
  error = 5,
  fatal = 6,
};

#include "diagnostics.inc"

/*! \brief A diagnostic engine. */
class diagnostic_engine
{
public:
  using desc_type = std::pair<diagnostic_level, std::string>;

public:
  /*! \brief Constructs a diagnostic engine.
   *
   * \param client Diagnostic client
   */
  explicit diagnostic_engine( diagnostic_consumer *client );

  /*! \brief Creates a diagnostic builder.
   *
   * \param id ID of diagnostic
   */
  virtual inline diagnostic_builder report( diag_id id );

  /*! \brief Emits a diagnostic message.
   *
   * \param id ID of diagnostic
   * \param args Arguments
   */
  virtual inline void emit( diag_id id, std::vector<std::string> const& args = {} ) const;

  /*! \brief Create custom diagnostic.
   *
   * \param level Severity level
   * \param message Diagnostic message
   */
  diag_id create_id( diagnostic_level level, std::string const& message );

  /*! \brief Return the number of emitted diagnostics. */
  uint64_t get_num_diagnostics() const;

private:
  /*! \brief Emit diagnostics with static ID.
   *
   * \param id ID of diagnostic
   * \param args Arguments
   */
  void emit_static_diagnostic( diag_id id, std::vector<std::string> const& args ) const;

  /*! \brief Emit diagnostics with custom ID.
   *
   * \param id ID of diagnostic
   * \param args Arguments
   */
  void emit_custom_diagnostic( diag_id id, std::vector<std::string> const& args ) const;

protected:
  diagnostic_consumer *client_ = nullptr;  /*!< Diagnostic client. */
  std::vector<desc_type> custom_diag_info; /*!< Custom diagnostics. */
  std::map<desc_type, diag_id> custom_diag_ids; /*!< Map from custom ID to diagnostic. */
  mutable uint64_t num_diagnostics{0};
}; /* diagnostic_engine */

/*! \brief A builder for diagnostics.
 *
 * An object that encapsulates a diagnostic.  The diagnostic may take
 * additional parameters and is finally issued at the end of its
 * life time.
 */
class diagnostic_builder
{
public:
  /*! \brief Constructs a diagnostic builder.
   *
   * \param engine Diagnostic engine
   * \param id ID of diagnostic
   */
  inline explicit diagnostic_builder( diagnostic_engine& engine, diag_id id );

  /*! \brief Destructs the diagnostic builder and issues the diagnostic. */
  inline ~diagnostic_builder();

  /*! \brief Add argument.
   *
   * \param s String argument
   */
  inline diagnostic_builder& add_argument( std::string const& s );

private:
  diagnostic_engine& engine_; /*!< diagnostic engine */
  diag_id id_; /*!< id of diagnostic */
  std::vector<std::string> args_; /*!< arguments for diagnostic */
}; /* diagnostic_builder */

/*! \brief A consumer for diagnostics. */
class diagnostic_consumer
{
public:
  diagnostic_consumer() = default;
  virtual ~diagnostic_consumer() = default;

  /*! \brief Handle diagnostic.
   *
   * \param level Severity level
   * \param message Diagnostic message
   */
  virtual void handle_diagnostic( diagnostic_level level, std::string const& message ) const
  {
    (void)level;
    (void)message;
  }
};

inline diagnostic_builder::diagnostic_builder( diagnostic_engine& engine, diag_id id )
  : engine_( engine ), id_( id )
{
}

inline diagnostic_builder::~diagnostic_builder()
{
  engine_.emit( id_, args_ );
}

inline diagnostic_builder& diagnostic_builder::add_argument( std::string const& s )
{
  args_.emplace_back( s );
  return *this;
}

inline diagnostic_engine::diagnostic_engine( diagnostic_consumer *client )
  : client_( client )
{
}

inline diag_id diagnostic_engine::create_id( diagnostic_level level, std::string const& message )
{
  desc_type desc{level, message};
  diag_id id{diag_id( custom_diag_info.size() )};
  custom_diag_ids.emplace( desc, id );
  custom_diag_info.emplace_back( desc );
  return diag_id( uint32_t( diag_id::NUM_STATIC_ERROR_IDS ) + uint32_t( id ) );
}

inline uint64_t diagnostic_engine::get_num_diagnostics() const
{
  return num_diagnostics;
}

inline diagnostic_builder diagnostic_engine::report( diag_id id )
{
  return diagnostic_builder( *this, id );
}

inline void diagnostic_engine::emit_static_diagnostic( diag_id id, std::vector<std::string> const& args ) const
{
  assert( id < diag_id::NUM_STATIC_ERROR_IDS );
  diagnostic_level const level = diag_info[uint32_t( id )].first;
  std::string const message = diag_info[uint32_t( id )].second;
  std::string_view const message_view = message;
  switch ( args.size() )
  {
  case 1:
    client_->handle_diagnostic( level, std::vformat( message_view, std::make_format_args(args[0]) ) );
    break;
  case 2:
    client_->handle_diagnostic( level, std::vformat( message_view, std::make_format_args(args[0], args[1]) ) );
    break;
  case 3:
    client_->handle_diagnostic( level, std::vformat( message_view, std::make_format_args(args[0], args[1], args[2]) ) );
    break;
  default:
  case 0:
    assert( args.size() == 0 );
    client_->handle_diagnostic( level, message );
  }
}

inline void diagnostic_engine::emit_custom_diagnostic( diag_id id, std::vector<std::string> const& args ) const
{
  uint32_t const custom_id = uint32_t( id ) - uint32_t( diag_id::NUM_STATIC_ERROR_IDS );
  assert( uint32_t( custom_id ) < custom_diag_info.size() );
  diagnostic_level const level = custom_diag_info[custom_id].first;
  std::string const message = custom_diag_info[custom_id].second;
  std::string_view const message_view = message;
  switch ( args.size() )
  {
  case 1:
    client_->handle_diagnostic( level, std::vformat( message_view, std::make_format_args(args[0]) ) );
    break;
  case 2:
    client_->handle_diagnostic( level, std::vformat( message_view, std::make_format_args(args[0], args[1]) ) );
    break;
  case 3:
    client_->handle_diagnostic( level, std::vformat( message_view, std::make_format_args(args[0], args[1], args[2]) ) );
    break;
  default:
  case 0:
    assert( args.size() == 0 );
    client_->handle_diagnostic( level, message );
  }
}

inline void diagnostic_engine::emit( diag_id id, std::vector<std::string> const& args ) const
{
  assert( client_ != nullptr );
  ++num_diagnostics;
  if ( id < diag_id::NUM_STATIC_ERROR_IDS )
  {
    emit_static_diagnostic( id, args );
  }
  else
  {
    emit_custom_diagnostic( id, args );
  }
}

} // namespace lorina
