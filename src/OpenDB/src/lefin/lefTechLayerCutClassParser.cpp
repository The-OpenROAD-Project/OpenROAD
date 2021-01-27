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



namespace lefTechLayerCutClass {
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    namespace phoenix = boost::phoenix;
    using qi::lexeme;
    using ascii::char_;
    using boost::spirit::qi::lit;
    using boost::spirit::ascii::string;
    using boost::spirit::ascii::space_type;
    using boost::fusion::at_c;
    
    using qi::double_;
    using qi::int_;
    // using qi::_1;
    using ascii::space;
    using phoenix::ref;
    void addCutClassSubRule(boost::fusion::vector<std::string, double, boost::optional<double>, boost::optional<int>>& params,
                            odb::dbTechLayerCutClassRule* rule, 
                            odb::lefin* lefin) 
    {
        std::string name = at_c<0>(params);
        auto subrule = odb::dbTechLayerCutClassSubRule::create(rule, name.c_str());
        subrule->setWidth(lefin->dbdist(at_c<1>(params)));
        auto length = at_c<2>(params);
        auto cnt = at_c<3>(params);
        if(length.is_initialized())
        {
            subrule->setLengthValid(true);
            subrule->setLength(lefin->dbdist(length.value()));
        }
        if(cnt.is_initialized())
        {
            subrule->setCutsValid(true);
            subrule->setNumCuts(cnt.value());
        }
    }
    void print(std::string name)
    {
        std::cout<<name<<std::endl;
    }
    template <typename Iterator>
    bool parse(Iterator first, Iterator last, odb::dbTechLayerCutClassRule* rule, odb::lefin* lefin)
    {   

        qi::rule<Iterator, std::string(), ascii::space_type> _string;
        _string %= lexeme[+(char_-' ')];
        qi::rule<std::string::iterator, space_type> cutClassRule = (
            +(
               lit("CUTCLASS")
               >> _string
               >> lit("WIDTH")
               >> double_
               >> -(lit("LENGTH") >> double_) 
               >> -(lit("CUTS") >> int_)
               >> lit(";")
            )[boost::bind(&addCutClassSubRule,_1,rule,lefin)]
        );

        bool r = qi::phrase_parse(first, last, cutClassRule, space);
        

        if (first != last) // fail if we did not get a full match
            return false;
        return r;
    }
}


namespace odb{


dbTechLayerCutClassRule* lefTechLayerCutClassParser::parse(std::string s, dbTechLayer* layer, odb::lefin* l)
{
    dbTechLayerCutClassRule* rule = dbTechLayerCutClassRule::create(layer);
    if(lefTechLayerCutClass::parse(s.begin(), s.end(), rule, l) )
        return rule;
    else 
    {
        odb::dbTechLayerCutClassRule::destroy(rule);
        return nullptr;
    }
} 


}

