/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "../../dr/FlexDR.h"

void fr::drNet::incNRipupAvoids()
{
  nRipupAvoids_++;
}
bool fr::drNet::hasNDR() const
{
  return getFrNet()->getNondefaultRule() != nullptr;
}

bool fr::drNet::isClockNet() const { 
    return fNet_->isClock(); 
}
