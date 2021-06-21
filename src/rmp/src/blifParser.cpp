#include "rmp/blifParser.h"

#include <boost/bind.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace blif_parser {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using boost::fusion::at_c;
using boost::spirit::ascii::space_type;
using boost::spirit::ascii::string;
using boost::spirit::qi::lit;
using qi::lexeme;

using ascii::char_;
using ascii::space;

using phoenix::ref;
using qi::double_;
using qi::int_;

void setNewInput(std::string input, rmp::BlifParser* parser)
{
  if (input != "\\")
    parser->addInput(input);
}

void setNewOutput(std::string output, rmp::BlifParser* parser)
{
  if (output != "\\")
    parser->addOutput(output);
}

void setNewClock(std::string clock, rmp::BlifParser* parser)
{
  if (clock != "\\")
    parser->addClock(clock);
}

void setNewInstanceType(std::string type, rmp::BlifParser* parser)
{
  parser->addNewInstanceType(type);
}

void setNewGate(std::string gate, rmp::BlifParser* parser)
{
  parser->addNewGate(gate);
}

void setGateNets(std::string net, rmp::BlifParser* parser)
{
  parser->addConnection(net);
}

void endParser(std::string end, rmp::BlifParser* parser)
{
  parser->endParser();
}

template <typename Iterator>
bool parse(Iterator first, Iterator last, rmp::BlifParser* parser)
{
  qi::rule<Iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[+(char_ - (' ' | qi::eol))];

  qi::rule<std::string::iterator, space_type> rule
      = (lit(".model") >> _string >> lit(".inputs")
         >> +(!(&lit(".outputs"))
              >> _string[boost::bind(&setNewInput, _1, parser)])
         >> lit(".outputs")
         >> +(!(&lit(".gate")) >> !(&lit(".clock"))
              >> _string[boost::bind(&setNewOutput, _1, parser)])
         >> -(lit(".clock")
              >> +(!(&lit(".gate"))
                   >> _string[boost::bind(&setNewClock, _1, parser)]))
         >> +((lit(".gate")[boost::bind(&setNewInstanceType, "gate", parser)]
               | lit(".mlatch")[boost::bind(
                   &setNewInstanceType, "mlatch", parser)])
              >> _string[boost::bind(&setNewGate, _1, parser)]
              >> +(!(&lit(".gate")) >> !(&lit(".mlatch")) >> !(&lit(".end"))
                   >> _string[boost::bind(&setGateNets, _1, parser)]))
         >> string(".end")[boost::bind(&endParser, _1, parser)]);

  bool valid = qi::phrase_parse(first, last, rule, space);
  return valid;
}

}  // namespace blif_parser

namespace rmp {

BlifParser::BlifParser()
{
  combCount = 0;
  flopCount = 0;
  currentInstanceType = "";
  currentGate = "";
}
void BlifParser::addInput(std::string& input)
{
  inputs.push_back(input);
}
void BlifParser::addOutput(std::string& output)
{
  outputs.push_back(output);
}
void BlifParser::addClock(std::string& clock)
{
  clocks.push_back(clock);
}
void BlifParser::addNewInstanceType(std::string& type)
{
  if (currentInstanceType != "") {
    gates.push_back(
        std::make_tuple(currentInstanceType, currentGate, currentConnections));
  }
  currentInstanceType = type;
  if (currentInstanceType == "mlatch")
    flopCount++;
  else if (currentInstanceType == "gate")
    combCount++;
  currentConnections.clear();
}
void BlifParser::addNewGate(std::string& cell_name)
{
  currentGate = cell_name;
}
void BlifParser::addConnection(std::string& connection)
{
  currentConnections.push_back(connection);
}
void BlifParser::endParser()
{
  if (currentInstanceType != "") {
    gates.push_back(
        std::make_tuple(currentInstanceType, currentGate, currentConnections));
  }
}

std::vector<std::string>& BlifParser::getInputs()
{
  return inputs;
}
std::vector<std::string>& BlifParser::getOutputs()
{
  return outputs;
}
std::vector<std::string>& BlifParser::getClocks()
{
  return clocks;
}
std::vector<gate_>& BlifParser::getGates()
{
  return gates;
}
int BlifParser::getCombGateCount()
{
  return combCount;
}
int BlifParser::getFlopCount()
{
  return flopCount;
}

bool BlifParser::parse(std::string& file_contents)
{
  return blif_parser::parse(file_contents.begin(), file_contents.end(), this);
}
}  // namespace rmp