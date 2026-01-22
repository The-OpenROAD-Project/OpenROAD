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
  \file verilog_regex.hpp
  \brief Regular expressions used by the Verilog parser.

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <regex>

namespace lorina
{

namespace verilog_regex
{
static std::regex immediate_assign( R"(^(~)?\(?([[:alnum:]\[\]_']+)\)?$)" );
static std::regex binary_expression( R"(^(~)?([[:alnum:]\[\]_']+)([&|^])(~)?([[:alnum:]\[\]_']+)$)" );
static std::regex ternary_expression( R"(^(~)?([[:alnum:]\[\]_']+)([&|^?])(~)?([[:alnum:]\[\]_']+)([&|^:])(~)?([[:alnum:]\[\]_']+)$)" );
static std::regex maj3_expression( R"(^\((~)?([[:alnum:]\[\]_']+)&(~)?([[:alnum:]\[\]_']+)\)\|\((~)?([[:alnum:]\[\]_']+)&(~)?([[:alnum:]\[\]_']+)\)\|\((~)?([[:alnum:]\[\]_']+)&(~)?([[:alnum:]\[\]_']+)\)$)" );
static std::regex negated_binary_expression( R"(^~\((~)?([[:alnum:]\[\]_']+)([&|^])(~)?([[:alnum:]\[\]_']+)\)$)" );
static std::regex const_size_range( R"(^(\d+):(\d+)$)" );
} // namespace verilog_regex

} // namespace lorina