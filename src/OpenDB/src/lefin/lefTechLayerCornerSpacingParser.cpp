#include "lefLayerPropParser.h"
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <iostream>
#include <string>

#include "lefin.h"
#include "db.h"



namespace lefTechLayerCornerSpacing {
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    namespace phoenix = boost::phoenix;
   
    using boost::spirit::qi::lit;
    using boost::spirit::ascii::string;
    using boost::spirit::ascii::space_type;
    using boost::fusion::at_c;
    
    using qi::double_;
    using qi::int_;
    // using qi::_1;
    using ascii::space;
    using phoenix::ref;
    
    void setWithin(double value, odb::dbTechLayerCornerSpacingRule* rule, odb::lefin* lefin)
    {
        rule->setCornerOnly(true);
        rule->setWithin(lefin->dbdist(value));
    }
    void setEolWidth(double value, odb::dbTechLayerCornerSpacingRule* rule, odb::lefin* lefin)
    {
        rule->setExceptEol(true);
        rule->setEolWidth(lefin->dbdist(value));
    }
    void setJogLength(double value, odb::dbTechLayerCornerSpacingRule* rule, odb::lefin* lefin)
    {
        rule->setExceptJogLength(true);
        rule->setJogLength(lefin->dbdist(value));
    }
    void setMinLength(double value, odb::dbTechLayerCornerSpacingRule* rule, odb::lefin* lefin)
    {
        rule->setMinLengthValid(true);
        rule->setMinLength(lefin->dbdist(value));
    }
    void addSpacing(boost::fusion::vector<double,double>& params, odb::dbTechLayerCornerSpacingRule* rule, odb::lefin* lefin)
    {
        auto width = lefin->dbdist(at_c<0>(params));
        auto spacing = lefin->dbdist(at_c<1>(params));
        rule->addSpacing(width,spacing);
    }
    template <typename Iterator>
    bool parse(Iterator first, Iterator last, odb::dbTechLayerCornerSpacingRule* rule, odb::lefin* lefin)
    {

        qi::rule<std::string::iterator, space_type> convexCornerRule = (
                                                                        lit("CONVEXCORNER")[boost::bind(&odb::dbTechLayerCornerSpacingRule::setType, rule, odb::dbTechLayerCornerSpacingRule::CONVEXCORNER)]
                                                                        >> -(lit("SAMEMASK")[boost::bind(&odb::dbTechLayerCornerSpacingRule::setSameMask, rule, true)])
                                                                        >> -(lit("CORNERONLY") >> double_ [boost::bind(&setWithin, _1, rule, lefin)])
                                                                        >> -(
                                                                            lit("EXCEPTEOL") >> double_ [boost::bind(&setEolWidth, _1, rule, lefin)]
                                                                            >> -(
                                                                                lit("EXCEPTJOGLENGTH") >> double_ [boost::bind(&setJogLength, _1, rule, lefin)]
                                                                                >> -(
                                                                                    lit("INCLUDELSHAPE")[boost::bind(&odb::dbTechLayerCornerSpacingRule::setIncludeShape, rule, true)]
                                                                                )
                                                                            )
                                                                        )
                                                                      );
        qi::rule<std::string::iterator, space_type> concaveCornerRule = (
            lit("CONCAVECORNER")[boost::bind(&odb::dbTechLayerCornerSpacingRule::setType, rule, odb::dbTechLayerCornerSpacingRule::CONCAVECORNER)]
            >> -(
                lit("MINLENGTH") >> double_ [boost::bind(&setMinLength, _1, rule, lefin)]
                >> -(
                    lit("EXCEPTNOTCH") [boost::bind(&odb::dbTechLayerCornerSpacingRule::setExceptNotch, rule, true)]
                )
            )
        );
        qi::rule<std::string::iterator, space_type> exceptSameRule = (
            lit("EXCEPTSAMENET")[boost::bind(&odb::dbTechLayerCornerSpacingRule::setExceptSameNet, rule, true)]
            |
            lit("EXCEPTSAMEMETAL")[boost::bind(&odb::dbTechLayerCornerSpacingRule::setExceptSameMetal, rule, true)]
        );

        qi::rule<std::string::iterator, space_type> spacingRule = (
            lit("WIDTH") >> double_ >> lit("SPACING") >> double_
        )[boost::bind(&addSpacing, _1, rule, lefin)];
        
        qi::rule<std::string::iterator, space_type> cornerSpacingRule = (
                                                                        lit("CORNERSPACING") 
                                                                        >> ( convexCornerRule | concaveCornerRule )
                                                                        >> -(exceptSameRule)
                                                                        >> +(spacingRule)
                                                                        >> lit(";")   
                                                                  );

        bool r = qi::phrase_parse(first, last, cornerSpacingRule, space);
        

        if (first != last) // fail if we did not get a full match
            return false;
        return r;
    }
}


namespace odb{


dbTechLayerCornerSpacingRule* lefTechLayerCornerSpacingParser::parse(std::string s, dbTechLayer* layer, odb::lefin* l)
{
    dbTechLayerCornerSpacingRule* rule = dbTechLayerCornerSpacingRule::create(layer);
    if(lefTechLayerCornerSpacing::parse(s.begin(), s.end(), rule, l) )
        return rule;
    else 
    {
        odb::dbTechLayerCornerSpacingRule::destroy(rule);
        return nullptr;
    }
} 


}

