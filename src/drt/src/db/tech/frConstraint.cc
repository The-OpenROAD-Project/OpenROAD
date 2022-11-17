#include "frConstraint.h"

#include "frLayer.h"
using namespace fr;

frCoord frSpacingTableTwConstraint::find(frCoord width1,
                                         frCoord width2,
                                         frCoord prl) const
{
  auto dbLayer = layer_->getDbLayer();
  return dbLayer->findTwSpacing(width1, width2, prl);
}