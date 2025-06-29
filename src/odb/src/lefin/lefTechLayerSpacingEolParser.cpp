// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <iostream>
#include <string>
#include <vector>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "parserUtils.h"

namespace odb::lefTechLayerSpacingEol {

void parallelEdgeParser(
    const boost::fusion::vector<
        std::string,
        boost::optional<std::string>,
        double,
        double,
        boost::optional<boost::fusion::vector2<std::string, double>>,
        boost::optional<boost::fusion::vector2<std::string, double>>,
        boost::optional<std::string>,
        boost::optional<std::string>,
        boost::optional<std::string>,
        boost::optional<std::string>>& params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  // Decoding struct
  const boost::optional<std::string>& isSubtractEolWidth = at_c<1>(params);
  const double& parSpace = at_c<2>(params);
  const double& parWithin = at_c<3>(params);
  const boost::optional<boost::fusion::vector2<std::string, double>>& prl
      = at_c<4>(params);
  const boost::optional<boost::fusion::vector2<std::string, double>>& minLength
      = at_c<5>(params);
  const boost::optional<std::string>& isTwoEdges = at_c<6>(params);
  const boost::optional<std::string>& isSameMetal = at_c<7>(params);
  const boost::optional<std::string>& isNonEolCornerOnly = at_c<8>(params);
  const boost::optional<std::string>& isParallelSameMask = at_c<9>(params);

  // Populating Object
  sc->setParallelEdgeValid(true);
  if (isSubtractEolWidth.is_initialized()) {
    sc->setSubtractEolWidthValid(true);
  }
  sc->setParSpace(l->dbdist(parSpace));
  sc->setParWithin(l->dbdist(parWithin));
  if (prl.is_initialized()) {
    sc->setParPrlValid(true);
  }
  if (prl.is_initialized()) {
    sc->setParPrl(l->dbdist(at_c<1>(prl.value())));
  }
  if (minLength.is_initialized()) {
    sc->setParMinLengthValid(true);
  }
  if (minLength.is_initialized()) {
    sc->setParMinLength(l->dbdist(at_c<1>(minLength.value())));
  }
  if (isTwoEdges.is_initialized()) {
    sc->setTwoEdgesValid(true);
  }
  if (isSameMetal.is_initialized()) {
    sc->setSameMetalValid(true);
  }
  if (isNonEolCornerOnly.is_initialized()) {
    sc->setNonEolCornerOnlyValid(true);
  }
  if (isParallelSameMask.is_initialized()) {
    sc->setParallelSameMaskValid(true);
  }
}

void exceptExactParser(
    const boost::fusion::vector<std::string, double, double>& params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  const double& exactWidth = at_c<1>(params);
  const double& otherWidth = at_c<2>(params);

  sc->setExceptExactWidthValid(true);
  sc->setExactWidth(l->dbdist(exactWidth));
  sc->setOtherWidth(l->dbdist(otherWidth));
}

void fillConcaveParser(const boost::fusion::vector<std::string, double>& params,
                       odb::dbTechLayerSpacingEolRule* sc,
                       odb::lefinReader* l)
{
  const double& fillTriangle = at_c<1>(params);

  sc->setFillConcaveCornerValid(true);
  sc->setFillTriangle(l->dbdist(fillTriangle));
}

void withcutParser(
    const boost::fusion::vector<
        std::string,
        boost::optional<boost::fusion::vector2<std::string, double>>,
        boost::optional<std::string>,
        double,
        boost::optional<boost::fusion::vector3<
            std::string,
            double,
            boost::optional<boost::fusion::vector2<std::string, double>>>>>&
        params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  const boost::optional<boost::fusion::vector2<std::string, double>>& cutclass
      = at_c<1>(params);
  const boost::optional<std::string>& above = at_c<2>(params);
  const double& cutSpace = at_c<3>(params);
  const boost::optional<boost::fusion::vector3<
      std::string,
      double,
      boost::optional<boost::fusion::vector2<std::string, double>>>>&
      enclosureEnd
      = at_c<4>(params);

  sc->setWithcutValid(true);
  if (cutclass.is_initialized()) {
    sc->setCutClassValid(true);
  }
  if (cutclass.is_initialized()) {
    sc->setCutClass(l->dbdist(at_c<1>(cutclass.value())));
  }
  if (above.is_initialized()) {
    sc->setWithCutAboveValid(true);
  }
  sc->setWithCutSpace(l->dbdist(cutSpace));
  if (enclosureEnd.is_initialized()) {
    sc->setEnclosureEndValid(true);
  }
  if (enclosureEnd.is_initialized()) {
    sc->setEnclosureEndWidth(l->dbdist(at_c<1>(enclosureEnd.value())));
  }
  if (enclosureEnd.is_initialized()
      && (at_c<2>(enclosureEnd.value())).is_initialized()) {
    sc->setEnclosureEndWithinValid(true);
  }
  if (enclosureEnd.is_initialized()
      && (at_c<2>(enclosureEnd.value())).is_initialized()) {
    const double& enclosureEndWithin
        = at_c<1>((at_c<2>(enclosureEnd.value())).value());
    sc->setEnclosureEndWithin(l->dbdist(enclosureEndWithin));
  }
}

void endprlspacingParser(
    const boost::fusion::vector<std::string, double, std::string, double>&
        params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  sc->setEndPrlSpacingValid(true);
  sc->setEndPrlSpace(l->dbdist(at_c<1>(params)));
  sc->setPrlValid(true);
  sc->setEndPrl(l->dbdist(at_c<3>(params)));
}

void endtoendspacingParser(
    const boost::fusion::vector<
        std::string,
        double,
        boost::optional<boost::fusion::vector2<double, double>>,
        boost::optional<boost::fusion::vector3<std::string,
                                               double,
                                               boost::optional<double>>>,
        boost::optional<boost::fusion::vector2<std::string, double>>>& params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  const boost::optional<boost::fusion::vector2<double, double>>& twoCutSpaces
      = at_c<2>(params);
  const boost::optional<
      boost::fusion::vector3<std::string, double, boost::optional<double>>>&
      extension
      = at_c<3>(params);
  const boost::optional<boost::fusion::vector2<std::string, double>>
      otherendWidth = at_c<4>(params);

  sc->setEndToEndValid(true);
  sc->setEndToEndSpace(l->dbdist(at_c<1>(params)));
  if (twoCutSpaces.is_initialized()) {
    sc->setOneCutSpace(l->dbdist(at_c<0>(twoCutSpaces.value())));
  }
  if (twoCutSpaces.is_initialized()) {
    sc->setTwoCutSpace(l->dbdist(at_c<1>(twoCutSpaces.value())));
  }

  if (extension.is_initialized()) {
    sc->setExtensionValid(true);
  }
  if (extension.is_initialized()) {
    sc->setExtension(l->dbdist(at_c<1>(extension.value())));
  }
  if (extension.is_initialized()
      && (at_c<2>(extension.value())).is_initialized()) {
    sc->setWrongDirExtensionValid(true);
    sc->setWrongDirExtension(l->dbdist((at_c<2>(extension.value())).value()));
  }

  if (otherendWidth.is_initialized()) {
    sc->setOtherEndWidthValid(true);
  }
  if (otherendWidth.is_initialized()) {
    sc->setOtherEndWidth(l->dbdist(at_c<1>(otherendWidth.value())));
  }
}

void maxminlengthParser(
    const boost::variant<
        boost::fusion::vector<std::string, double>,
        boost::fusion::
            vector<std::string, double, boost::optional<std::string>>>& params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  if (boost::get<boost::fusion::vector<std::string, double>>(&params)) {
    boost::fusion::vector<std::string, double> mx
        = boost::get<boost::fusion::vector<std::string, double>>(params);
    sc->setMaxLengthValid(true);
    sc->setMaxLength(l->dbdist(at_c<1>(mx)));
  } else {
    boost::fusion::vector<std::string, double, boost::optional<std::string>> mn
        = boost::get<boost::fusion::vector<std::string,
                                           double,
                                           boost::optional<std::string>>>(
            params);
    sc->setMinLengthValid(true);
    sc->setMinLength(l->dbdist(at_c<1>(mn)));
    if ((at_c<2>(mn)).is_initialized()) {
      sc->setTwoSidesValid(true);
    }
  }
}

void enclosecutParser(
    const boost::fusion::vector<
        std::string,
        boost::optional<boost::variant<std::string, std::string>>,
        double,
        std::string,
        double,
        boost::optional<std::string>>& params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  sc->setEncloseCutValid(true);
  if ((at_c<1>(params)).is_initialized()
      && boost::get<std::string>((at_c<1>(params)).value()) == "ABOVE") {
    sc->setAboveValid(true);
  } else if ((at_c<1>(params)).is_initialized()
             && boost::get<std::string>((at_c<1>(params)).value()) == "BELOW") {
    sc->setBelowValid(true);
  }
  sc->setEncloseDist(l->dbdist(at_c<2>(params)));
  sc->setCutSpacingValid(true);
  sc->setCutToMetalSpace(l->dbdist(at_c<4>(params)));
  if ((at_c<5>(params)).is_initialized()) {
    sc->setAllCutsValid(true);
  }
}

void oppositeWidthParser(
    const boost::fusion::vector<std::string, double>& params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  const double& oppositeWidth = at_c<1>(params);

  sc->setOppositeWidthValid(true);
  sc->setOppositeWidth(l->dbdist(oppositeWidth));
}

void concaveCornerParser(
    const boost::fusion::vector<
        std::string,
        boost::optional<boost::fusion::vector<std::string, double>>,
        boost::optional<boost::fusion::vector2<
            std::string,
            boost::variant<boost::fusion::vector<double, double>, double>>>>&
        params,
    odb::dbTechLayerSpacingEolRule* sc,
    odb::lefinReader* l)
{
  const boost::optional<boost::fusion::vector<std::string, double>>& minlength
      = at_c<1>(params);
  const boost::optional<boost::fusion::vector2<
      std::string,
      boost::variant<boost::fusion::vector<double, double>, double>>>&
      minAdjlength
      = at_c<2>(params);

  sc->setToConcaveCornerValid(true);
  if (minlength.is_initialized()) {
    sc->setMinLengthValid(true);
  }
  if (minlength.is_initialized()) {
    sc->setMinLength(l->dbdist(at_c<1>(minlength.value())));
  }

  if (minAdjlength.is_initialized()) {
    sc->setMinAdjacentLengthValid(true);
  }
  if (minAdjlength.is_initialized()
      && boost::get<double>(&at_c<1>(minAdjlength.value()))) {
    sc->setMinAdjLength(
        l->dbdist(boost::get<double>(at_c<1>(minAdjlength.value()))));
  } else if (minAdjlength.is_initialized()) {
    const boost::fusion::vector<double, double>& twoAdjLengths
        = boost::get<boost::fusion::vector<double, double>>(
            at_c<1>(minAdjlength.value()));
    sc->setMinAdjLength1(l->dbdist(at_c<0>(twoAdjLengths)));
    sc->setMinAdjLength2(l->dbdist(at_c<1>(twoAdjLengths)));
  }
}

void eolWithinParser(double value,
                     odb::dbTechLayerSpacingEolRule* sc,
                     odb::lefinReader* l)
{
  sc->setEolWithin(l->dbdist(value));
}

void wrongDirWithinParser(double value,
                          odb::dbTechLayerSpacingEolRule* sc,
                          odb::lefinReader* l)
{
  sc->setWrongDirWithin(l->dbdist(value));
}

void notchLengthParser(double value,
                       odb::dbTechLayerSpacingEolRule* sc,
                       odb::lefinReader* l)
{
  sc->setNotchLength(l->dbdist(value));
}

void eolSpaceParser(double value,
                    odb::dbTechLayerSpacingEolRule* sc,
                    odb::lefinReader* l)
{
  sc->setEolSpace(l->dbdist(value));
}

void eolwidthParser(double value,
                    odb::dbTechLayerSpacingEolRule* sc,
                    odb::lefinReader* l)
{
  sc->setEolWidth(l->dbdist(value));
}

void wrongDirSpaceParser(double value,
                         odb::dbTechLayerSpacingEolRule* sc,
                         odb::lefinReader* l)
{
  sc->setWrongDirSpace(l->dbdist(value));
}

bool parse(const std::string& s, odb::dbTechLayer* layer, odb::lefinReader* l)
{
  odb::dbTechLayerSpacingEolRule* sc
      = odb::dbTechLayerSpacingEolRule::create(layer);
  qi::rule<std::string::const_iterator, space_type> prlEdgeRule
      = (string("PARALLELEDGE") >> -(string("SUBTRACTEOLWIDTH")) >> double_
         >> lit("WITHIN") >> double_ >> -(string("PRL") >> double_)
         >> -(string("MINLENGTH") >> double_) >> -(string("TWOEDGES"))
         >> -(string("SAMEMETAL")) >> -(string("NONEOLCORNERONLY")) >> -(string(
             "PARALLELSAMEMASK")))[boost::bind(&parallelEdgeParser, _1, sc, l)];

  qi::rule<std::string::const_iterator, space_type> exceptexactRule
      = (string("EXCEPTEXACTWIDTH") >> double_
         >> double_)[boost::bind(&exceptExactParser, _1, sc, l)];

  qi::rule<std::string::const_iterator, space_type> fillConcaveCornerRule
      = (string("FILLCONCAVECORNER")
         >> double_)[boost::bind(&fillConcaveParser, _1, sc, l)];

  qi::rule<std::string::const_iterator, space_type> withCutRule
      = (string("WITHCUT") >> -(string("CUTCLASS") >> double_)
         >> -(string("ABOVE")) >> double_
         >> -(string("ENCLOSUREEND") >> double_
              >> -(string("WITHIN")
                   >> double_)))[boost::bind(&withcutParser, _1, sc, l)];

  qi::rule<std::string::const_iterator, space_type> endprlspacingrule
      = (string("ENDPRLSPACING") >> double_ >> string("PRL")
         >> double_)[boost::bind(&endprlspacingParser, _1, sc, l)];

  qi::rule<std::string::const_iterator, space_type> endtoendspacingrule
      = (string("ENDTOEND") >> double_ >> -(double_ >> double_)
         >> -(string("EXTENSION") >> double_ >> -(double_))
         >> -(string("OTHERENDWIDTH")
              >> double_))[boost::bind(&endtoendspacingParser, _1, sc, l)];

  qi::rule<std::string::const_iterator, space_type> maxminlengthrule
      = ((string("MAXLENGTH") >> double_)
         | (string("MINLENGTH") >> double_ >> -(string(
                "TWOSIDES"))))[boost::bind(&maxminlengthParser, _1, sc, l)];

  qi::rule<std::string::const_iterator, space_type> enclosecutrule
      = (string("ENCLOSECUT") >> -(string("ABOVE") | string("BELOW")) >> double_
         >> string("CUTSPACING") >> double_
         >> -(string("ALLCUTS")))[boost::bind(&enclosecutParser, _1, sc, l)];

  qi::rule<std::string::const_iterator, space_type> withinRule
      = (-(string("OPPOSITEWIDTH")
           >> double_)[boost::bind(&oppositeWidthParser, _1, sc, l)]
         >> lit("WITHIN")[boost::bind(
             &odb::dbTechLayerSpacingEolRule::setWithinValid, sc, true)]
         >> double_[boost::bind(&eolWithinParser, _1, sc, l)]
         >> -(double_[boost::bind(&wrongDirWithinParser, _1, sc, l)])
         >> -(lit("SAMEMASK")[boost::bind(
             &odb::dbTechLayerSpacingEolRule::setSameMaskValid, sc, true)])
         >> -exceptexactRule >> -fillConcaveCornerRule >> -withCutRule
         >> -endprlspacingrule >> -endtoendspacingrule >> -maxminlengthrule
         >> -(lit("EQUALRECTWIDTH")[boost::bind(
             &odb::dbTechLayerSpacingEolRule::setEqualRectWidthValid,
             sc,
             true)])
         >> -prlEdgeRule >> -enclosecutrule);

  qi::rule<std::string::const_iterator, space_type> toconcavecornerrule
      = (string("TOCONCAVECORNER") >> -(string("MINLENGTH") >> double_)
         >> -(string("MINADJACENTLENGTH")
              >> ((double_ >> double_)
                  | double_)))[boost::bind(&concaveCornerParser, _1, sc, l)];
  ;

  qi::rule<std::string::const_iterator, space_type> tonotchlengthrule
      = (lit("TONOTCHLENGTH")[boost::bind(
             &odb::dbTechLayerSpacingEolRule::setToNotchLengthValid, sc, true)]
         >> double_[boost::bind(&notchLengthParser, _1, sc, l)]);

  qi::rule<std::string::const_iterator, space_type> spacingRule
      = (lit("SPACING") >> (double_[boost::bind(&eolSpaceParser, _1, sc, l)])
         >> lit("ENDOFLINE") >> double_[boost::bind(&eolwidthParser, _1, sc, l)]
         >> -(lit("EXACTWIDTH")[boost::bind(
             &odb::dbTechLayerSpacingEolRule::setExactWidthValid, sc, true)])
         >> -(lit("WRONGDIRSPACING")[boost::bind(
                  &odb::dbTechLayerSpacingEolRule::setWrongDirSpacingValid,
                  sc,
                  true)]
              >> double_[boost::bind(&wrongDirSpaceParser, _1, sc, l)])
         >> (withinRule | toconcavecornerrule | tonotchlengthrule)
         >> -lit(";"));

  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, spacingRule, space) && first == last;

  if (!valid) {
    odb::dbTechLayerSpacingEolRule::destroy(sc);
  }
  return valid;
}
}  // namespace odb::lefTechLayerSpacingEol

namespace odb {

void lefTechLayerSpacingEolParser::parse(const std::string& s,
                                         dbTechLayer* layer,
                                         odb::lefinReader* l)
{
  processRules(s, [layer, l](const std::string& rule) {
    if (rule.find("ENDOFLINE") == std::string::npos) {
      l->warning(254,
                 "unsupported LEF58_SPACING property for layer {} :\"{}\"",
                 layer->getName(),
                 rule);
      return;
    }
    if (!lefTechLayerSpacingEol::parse(rule, layer, l)) {
      l->warning(255,
                 "parse mismatch in layer property LEF58_SPACING ENDOFLINE for "
                 "layer {} :\"{}\"",
                 layer->getName(),
                 rule);
    }
  });
}

}  // namespace odb
