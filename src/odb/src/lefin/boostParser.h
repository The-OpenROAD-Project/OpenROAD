// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/bind/bind.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <string>

namespace odb {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using namespace boost::placeholders;

using ascii::blank;
using ascii::char_;
using boost::fusion::at_c;
using boost::spirit::ascii::alpha;
using boost::spirit::ascii::space_type;
using boost::spirit::ascii::string;
using boost::spirit::qi::lit;

using qi::double_;
using qi::int_;
using qi::lexeme;

using ascii::space;
using phoenix::ref;

static const qi::rule<std::string::iterator, std::string(), ascii::space_type>
    _string = lexeme[(alpha >> *(char_ - blank - '\n'))];

}  // namespace odb
