// *****************************************************************************
// *****************************************************************************
// ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!
// *****************************************************************************
// *****************************************************************************
// Copyright 2012, Cadence Design Systems
//
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8.
//
// Licensed under the Apache License, Version 2.0 (the \"License\");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an \"AS IS\" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
//
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
//
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefiNonDefault.h"
#include "lefiNonDefault.hpp"

// Wrappers definitions.
const char* lefiNonDefault_name(const ::lefiNonDefault* obj)
{
  return ((const LefParser::lefiNonDefault*) obj)->name();
}

int lefiNonDefault_hasHardspacing(const ::lefiNonDefault* obj)
{
  return ((LefParser::lefiNonDefault*) obj)->hasHardspacing();
}

int lefiNonDefault_numProps(const ::lefiNonDefault* obj)
{
  return ((LefParser::lefiNonDefault*) obj)->numProps();
}

const char* lefiNonDefault_propName(const ::lefiNonDefault* obj, int index)
{
  return ((const LefParser::lefiNonDefault*) obj)->propName(index);
}

const char* lefiNonDefault_propValue(const ::lefiNonDefault* obj, int index)
{
  return ((const LefParser::lefiNonDefault*) obj)->propValue(index);
}

double lefiNonDefault_propNumber(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->propNumber(index);
}

const char lefiNonDefault_propType(const ::lefiNonDefault* obj, int index)
{
  return ((const LefParser::lefiNonDefault*) obj)->propType(index);
}

int lefiNonDefault_propIsNumber(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->propIsNumber(index);
}

int lefiNonDefault_propIsString(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->propIsString(index);
}

int lefiNonDefault_numLayers(const ::lefiNonDefault* obj)
{
  return ((LefParser::lefiNonDefault*) obj)->numLayers();
}

const char* lefiNonDefault_layerName(const ::lefiNonDefault* obj, int index)
{
  return ((const LefParser::lefiNonDefault*) obj)->layerName(index);
}

int lefiNonDefault_hasLayerWidth(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->hasLayerWidth(index);
}

double lefiNonDefault_layerWidth(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->layerWidth(index);
}

int lefiNonDefault_hasLayerSpacing(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->hasLayerSpacing(index);
}

double lefiNonDefault_layerSpacing(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->layerSpacing(index);
}

int lefiNonDefault_hasLayerWireExtension(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->hasLayerWireExtension(index);
}

double lefiNonDefault_layerWireExtension(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->layerWireExtension(index);
}

int lefiNonDefault_hasLayerResistance(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->hasLayerResistance(index);
}

double lefiNonDefault_layerResistance(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->layerResistance(index);
}

int lefiNonDefault_hasLayerCapacitance(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->hasLayerCapacitance(index);
}

double lefiNonDefault_layerCapacitance(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->layerCapacitance(index);
}

int lefiNonDefault_hasLayerEdgeCap(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->hasLayerEdgeCap(index);
}

double lefiNonDefault_layerEdgeCap(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->layerEdgeCap(index);
}

int lefiNonDefault_hasLayerDiagWidth(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->hasLayerDiagWidth(index);
}

double lefiNonDefault_layerDiagWidth(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->layerDiagWidth(index);
}

int lefiNonDefault_numVias(const ::lefiNonDefault* obj)
{
  return ((LefParser::lefiNonDefault*) obj)->numVias();
}

const ::lefiVia* lefiNonDefault_viaRule(const ::lefiNonDefault* obj, int index)
{
  return (const ::lefiVia*) ((LefParser::lefiNonDefault*) obj)->viaRule(index);
}

int lefiNonDefault_numSpacingRules(const ::lefiNonDefault* obj)
{
  return ((LefParser::lefiNonDefault*) obj)->numSpacingRules();
}

const ::lefiSpacing* lefiNonDefault_spacingRule(const ::lefiNonDefault* obj,
                                                int index)
{
  return (const ::lefiSpacing*) ((LefParser::lefiNonDefault*) obj)
      ->spacingRule(index);
}

int lefiNonDefault_numUseVia(const ::lefiNonDefault* obj)
{
  return ((LefParser::lefiNonDefault*) obj)->numUseVia();
}

const char* lefiNonDefault_viaName(const ::lefiNonDefault* obj, int index)
{
  return ((const LefParser::lefiNonDefault*) obj)->viaName(index);
}

int lefiNonDefault_numUseViaRule(const ::lefiNonDefault* obj)
{
  return ((LefParser::lefiNonDefault*) obj)->numUseViaRule();
}

const char* lefiNonDefault_viaRuleName(const ::lefiNonDefault* obj, int index)
{
  return ((const LefParser::lefiNonDefault*) obj)->viaRuleName(index);
}

int lefiNonDefault_numMinCuts(const ::lefiNonDefault* obj)
{
  return ((LefParser::lefiNonDefault*) obj)->numMinCuts();
}

const char* lefiNonDefault_cutLayerName(const ::lefiNonDefault* obj, int index)
{
  return ((const LefParser::lefiNonDefault*) obj)->cutLayerName(index);
}

int lefiNonDefault_numCuts(const ::lefiNonDefault* obj, int index)
{
  return ((LefParser::lefiNonDefault*) obj)->numCuts(index);
}
