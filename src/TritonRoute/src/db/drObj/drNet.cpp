/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "../../dr/FlexDR.h"

void fr::drNet::incNdrRipupThresh(){
    if (isNDR()){
       ndrRipupThresh_++; 
    }
}
bool fr::drNet::isNDR(){
    return getFrNet()->getNondefaultRule() != nullptr;
}