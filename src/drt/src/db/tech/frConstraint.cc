#include "frConstraint.h"

#include "frLayer.h"
namespace drt {

std::string frConstraint::getViolName() const
{
  switch (typeId()) {
    case frConstraintTypeEnum::frcShortConstraint:
      return "Short";

    case frConstraintTypeEnum::frcMinWidthConstraint:
      return "Min Width";

    case frConstraintTypeEnum::frcSpacingConstraint:
      return "Metal Spacing";

    case frConstraintTypeEnum::frcSpacingEndOfLineConstraint:
      return "EOL Spacing";

    case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
      return "Metal Spacing";

    case frConstraintTypeEnum::frcCutSpacingConstraint:
      return "Cut Spacing";

    case frConstraintTypeEnum::frcMinStepConstraint:
      return "Min Step";

    case frConstraintTypeEnum::frcNonSufficientMetalConstraint:
      return "NS Metal";

    case frConstraintTypeEnum::frcSpacingSamenetConstraint:
      return "Metal Spacing";

    case frConstraintTypeEnum::frcOffGridConstraint:
      return "Off Grid";

    case frConstraintTypeEnum::frcMinEnclosedAreaConstraint:
      return "Min Hole";

    case frConstraintTypeEnum::frcAreaConstraint:
      return "Min Area";

    case frConstraintTypeEnum::frcLef58CornerSpacingConstraint:
      return "Corner Spacing";

    case frConstraintTypeEnum::frcLef58CutSpacingConstraint:
      return "Cut Spacing";

    case frConstraintTypeEnum::frcLef58RectOnlyConstraint:
      return "Rect Only";

    case frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint:
      return "RightWayOnGridOnly";

    case frConstraintTypeEnum::frcLef58MinStepConstraint:
      return "Min Step";

    case frConstraintTypeEnum::frcSpacingTableInfluenceConstraint:
      return "MetSpacingInf";

    case frConstraintTypeEnum::frcSpacingEndOfLineParallelEdgeConstraint:
      return "SpacingEOLParallelEdge";

    case frConstraintTypeEnum::frcSpacingTableConstraint:
      return "SpacingTable";

    case frConstraintTypeEnum::frcSpacingTableTwConstraint:
      return "SpacingTableTw";

    case frConstraintTypeEnum::frcLef58SpacingTableConstraint:
      return "Lef58SpacingTable";

    case frConstraintTypeEnum::frcLef58CutSpacingTableConstraint:
      return "Lef58CutSpacingTable";

    case frConstraintTypeEnum::frcLef58CutSpacingTablePrlConstraint:
      return "Lef58CutSpacingTablePrl";

    case frConstraintTypeEnum::frcLef58CutSpacingTableLayerConstraint:
      return "Lef58CutSpacingTableLayer";

    case frConstraintTypeEnum::frcLef58CutSpacingParallelWithinConstraint:
      return "Lef58CutSpacingParallelWithin";

    case frConstraintTypeEnum::frcLef58CutSpacingAdjacentCutsConstraint:
      return "Lef58CutSpacingAdjacentCuts";

    case frConstraintTypeEnum::frcLef58CutSpacingLayerConstraint:
      return "Lef58CutSpacingLayer";

    case frConstraintTypeEnum::frcMinimumcutConstraint:
    case frConstraintTypeEnum::frcLef58MinimumCutConstraint:
      return "Minimum Cut";

    case frConstraintTypeEnum::frcLef58CornerSpacingConcaveCornerConstraint:
      return "Lef58CornerSpacingConcaveCorner";

    case frConstraintTypeEnum::frcLef58CornerSpacingConvexCornerConstraint:
      return "Lef58CornerSpacingConvexCorner";

    case frConstraintTypeEnum::frcLef58CornerSpacingSpacingConstraint:
      return "Lef58CornerSpacingSpacing";

    case frConstraintTypeEnum::frcLef58CornerSpacingSpacing1DConstraint:
      return "Lef58CornerSpacingSpacing1D";

    case frConstraintTypeEnum::frcLef58CornerSpacingSpacing2DConstraint:
      return "Lef58CornerSpacingSpacing2D";

    case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint:
      return "Lef58SpacingEndOfLine";

    case frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinConstraint:
      return "Lef58SpacingEndOfLineWithin";

    case frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinEndToEndConstraint:
      return "Lef58SpacingEndOfLineWithinEndToEnd";

    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinEncloseCutConstraint:
      return "Lef58SpacingEndOfLineWithinEncloseCut";

    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinParallelEdgeConstraint:
      return "Lef58SpacingEndOfLineWithinParallelEdge";

    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint:
      return "Lef58SpacingEndOfLineWithinMaxMinLength";

    case frConstraintTypeEnum::frcLef58SpacingWrongDirConstraint:
      return "Lef58SpacingWrongDir";

    case frConstraintTypeEnum::frcLef58CutClassConstraint:
      return "Lef58CutClass";

    case frConstraintTypeEnum::frcRecheckConstraint:
      return "Recheck";

    case frConstraintTypeEnum::frcLef58EolExtensionConstraint:
      return "Lef58EolExtension";

    case frConstraintTypeEnum::frcLef58EolKeepOutConstraint:
      return "Lef58EolKeepOut";

    case frConstraintTypeEnum::frcMetalWidthViaConstraint:
      return "MetalWidthViaMap";

    case frConstraintTypeEnum::frcLef58AreaConstraint:
      return "Lef58Area";
    case frConstraintTypeEnum::frcLef58KeepOutZoneConstraint:
      return "KeepOutZone";
    case frConstraintTypeEnum::frcSpacingRangeConstraint:
      return "SpacingRange";
    case frConstraintTypeEnum::frcLef58TwoWiresForbiddenSpcConstraint:
      return "TwoWiresForbiddenSpc";
    case frConstraintTypeEnum::frcLef58ForbiddenSpcConstraint:
      return "ForbiddenSpc";
    case frConstraintTypeEnum::frcLef58EnclosureConstraint:
      return "Lef58Enclosure";
    case frConstraintTypeEnum::frcLef58MaxSpacingConstraint:
      return "Lef58MaxSpacing";
    case frConstraintTypeEnum::frcSpacingTableOrth:
      return "SpacingTableOrth";
    case frConstraintTypeEnum::frcLef58WidthTableOrth:
      return "WidthTableOrth";
  }
  return "";
}

void frLef58MinStepConstraint::report(utl::Logger* logger) const
{
  logger->report(
      "MINSTEP minStepLength {} insideCorner {} outsideCorner {} step {} "
      "maxLength {} maxEdges {} minAdjLength {} convexCorner {} exceptWithin "
      "{} concaveCorner {} threeConcaveCorners {} width {} minAdjLength2 {} "
      "minBetweenLength {} exceptSameCorners {} eolWidth {} concaveCorners "
      "{} ",
      minStepLength_,
      insideCorner_,
      outsideCorner_,
      step_,
      maxLength_,
      maxEdges_,
      minAdjLength_,
      convexCorner_,
      exceptWithin_,
      concaveCorner_,
      threeConcaveCorners_,
      width_,
      minAdjLength2_,
      minBetweenLength_,
      exceptSameCorners_,
      eolWidth_,
      concaveCorners_);
}

frCoord frSpacingTableTwConstraint::find(frCoord width1,
                                         frCoord width2,
                                         frCoord prl) const
{
  auto dbLayer = layer_->getDbLayer();
  return dbLayer->findTwSpacing(width1, width2, prl);
}

}  // namespace drt
