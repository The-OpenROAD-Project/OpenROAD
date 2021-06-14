#include <functional>
#include <string>

#include "boostParser.h"
#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace odb {

lefTechLayerEolKeepOutRuleParser::lefTechLayerEolKeepOutRuleParser(lefin* l)
{
  lefin_ = l;
}

void lefTechLayerEolKeepOutRuleParser::parse(std::string s,
                                             odb::dbTechLayer* layer)
{
  std::vector<std::string> rules;
  boost::split(rules, s, boost::is_any_of(";"));
  for (auto rule : rules) {
    boost::algorithm::trim(rule);
    if (rule.empty())
      continue;
    rule += " ; ";
    if (!parseSubRule(rule, layer))
      lefin_->warning(280,
                      "parse mismatch in layer propery LEF58_EOLKEEPOUT for "
                      "layer {} :\"{}\"",
                      layer->getName(),
                      rule);
  }
}
void lefTechLayerEolKeepOutRuleParser::setClass(
    std::string val,
    odb::dbTechLayerEolKeepOutRule* rule,
    odb::dbTechLayer* layer)
{
  rule->setClassName(val);
  rule->setClassValid(true);
}
void lefTechLayerEolKeepOutRuleParser::setInt(
    double val,
    odb::dbTechLayerEolKeepOutRule* rule,
    void (odb::dbTechLayerEolKeepOutRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}
bool lefTechLayerEolKeepOutRuleParser::parseSubRule(std::string s,
                                                    odb::dbTechLayer* layer)
{
  qi::rule<std::string::iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[+(char_ - ' ')];
  odb::dbTechLayerEolKeepOutRule* rule
      = odb::dbTechLayerEolKeepOutRule::create(layer);
  qi::rule<std::string::iterator, space_type> CLASS
      = (lit("CLASS")
         >> _string[boost::bind(&lefTechLayerEolKeepOutRuleParser::setClass,
                                this,
                                _1,
                                rule,
                                layer)]);
  qi::rule<std::string::iterator, space_type> EOLKEEPOUT
      = (lit("EOLKEEPOUT")
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setEolWidth)]
         >> lit("EXTENSION") >> double_[boost::bind(
             &lefTechLayerEolKeepOutRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerEolKeepOutRule::setBackwardExt)]
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setSideExt)]
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setForwardExt)]
         >> -CLASS 
         >> -lit("CORNERONLY")[boost::bind(&odb::dbTechLayerEolKeepOutRule::setCornerOnly,
                                            rule,
                                            true)]
         >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, EOLKEEPOUT, space);
  return valid && first == last;
}

}  // namespace odb
