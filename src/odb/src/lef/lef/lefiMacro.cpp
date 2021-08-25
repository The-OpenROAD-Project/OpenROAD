// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2017, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include <string.h>
#include <stdlib.h>
#include "lex.h"
#include "lefiMacro.hpp"
#include "lefiMisc.hpp"
#include "lefiDebug.hpp"
#include "lefiUtil.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// *****************************************************************************
// lefiObstruction
// *****************************************************************************

lefiObstruction::lefiObstruction()
: geometries_(NULL)
{
    Init();
}

void
lefiObstruction::Init()
{
    geometries_ = 0;
}


LEF_COPY_CONSTRUCTOR_C( lefiObstruction ) 
: geometries_(NULL) {
    LEF_MALLOC_FUNC( geometries_, lefiGeometries, sizeof(lefiGeometries)* 1 );
}

LEF_ASSIGN_OPERATOR_C ( lefiObstruction ) {
    CHECK_SELF_ASSIGN
    LEF_MALLOC_FUNC( geometries_, lefiGeometries, sizeof(lefiGeometries)* 1 );
    return *this;
}


lefiObstruction::~lefiObstruction()
{
    Destroy();
}

void
lefiObstruction::Destroy()
{
    clear();
}

void
lefiObstruction::clear()
{
//    if (geometries_) {
//        geometries_->Destroy();
//        lefFree((char*) (geometries_));
//    }
    geometries_ = 0;
}

void
lefiObstruction::setGeometries(lefiGeometries *g)
{
    clear();
    geometries_ = g;
}

lefiGeometries *
lefiObstruction::geometries() const
{
    return geometries_;
}

void
lefiObstruction::print(FILE *f) const
{
    lefiGeometries *g;

    fprintf(f, "  Obstruction\n");

    g = geometries_;
    g->print(f);

}


// *****************************************************************************
// lefiPinAntennaModel
// *****************************************************************************

lefiPinAntennaModel::lefiPinAntennaModel()
: oxide_(NULL),
  hasReturn_(0),
  numAntennaGateArea_(0),
  antennaGateAreaAllocated_(0),
  antennaGateArea_(NULL),
  antennaGateAreaLayer_(NULL),
  numAntennaMaxAreaCar_(0),
  antennaMaxAreaCarAllocated_(0),
  antennaMaxAreaCar_(NULL),
  antennaMaxAreaCarLayer_(NULL),
  numAntennaMaxSideAreaCar_(0),
  antennaMaxSideAreaCarAllocated_(0),
  antennaMaxSideAreaCar_(NULL),
  antennaMaxSideAreaCarLayer_(NULL),
  numAntennaMaxCutCar_(0),
  antennaMaxCutCarAllocated_(0),
  antennaMaxCutCar_(NULL),
  antennaMaxCutCarLayer_(NULL)
{
    Init();
}

void
lefiPinAntennaModel::Init()
{
    numAntennaGateArea_ = 0;
    antennaGateAreaAllocated_ = 1;
    antennaGateArea_ = (double*) lefMalloc(sizeof(double));
    antennaGateAreaLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaMaxAreaCar_ = 0;
    antennaMaxAreaCarAllocated_ = 1;
    antennaMaxAreaCar_ = (double*) lefMalloc(sizeof(double));
    antennaMaxAreaCarLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaMaxSideAreaCar_ = 0;
    antennaMaxSideAreaCarAllocated_ = 1;
    antennaMaxSideAreaCar_ = (double*) lefMalloc(sizeof(double));
    antennaMaxSideAreaCarLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaMaxCutCar_ = 0;
    antennaMaxCutCarAllocated_ = 1;
    antennaMaxCutCar_ = (double*) lefMalloc(sizeof(double));
    antennaMaxCutCarLayer_ = (char**) lefMalloc(sizeof(char*));

    oxide_ = 0;
    hasReturn_ = 0;

}

LEF_COPY_CONSTRUCTOR_C( lefiPinAntennaModel )
: oxide_(NULL),
  hasReturn_(0),
  numAntennaGateArea_(0),
  antennaGateAreaAllocated_(0),
  antennaGateArea_(NULL),
  antennaGateAreaLayer_(NULL),
  numAntennaMaxAreaCar_(0),
  antennaMaxAreaCarAllocated_(0),
  antennaMaxAreaCar_(NULL),
  antennaMaxAreaCarLayer_(NULL),
  numAntennaMaxSideAreaCar_(0),
  antennaMaxSideAreaCarAllocated_(0),
  antennaMaxSideAreaCar_(NULL),
  antennaMaxSideAreaCarLayer_(NULL),
  numAntennaMaxCutCar_(0),
  antennaMaxCutCarAllocated_(0),
  antennaMaxCutCar_(NULL),
  antennaMaxCutCarLayer_(NULL) 
{
    LEF_MALLOC_FUNC( oxide_, char, sizeof(char)*(strlen(prev.oxide_)+1) );
    LEF_COPY_FUNC( hasReturn_ );
    LEF_COPY_FUNC( numAntennaGateArea_ );
    LEF_COPY_FUNC( antennaGateAreaAllocated_ );
    LEF_MALLOC_FUNC( antennaGateArea_, double, sizeof(double) * numAntennaGateArea_);

    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaGateAreaLayer_, antennaGateAreaAllocated_  );


    LEF_COPY_FUNC( numAntennaMaxAreaCar_ );
    LEF_COPY_FUNC( antennaMaxAreaCarAllocated_ );
    LEF_MALLOC_FUNC( antennaMaxAreaCar_, double, sizeof(double) * numAntennaMaxAreaCar_);
    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaMaxAreaCarLayer_, numAntennaMaxAreaCar_ );

    LEF_COPY_FUNC( numAntennaMaxSideAreaCar_ );
    LEF_COPY_FUNC( antennaMaxSideAreaCarAllocated_ );
    LEF_MALLOC_FUNC( antennaMaxSideAreaCar_, double, sizeof(double) * numAntennaMaxSideAreaCar_);
    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaMaxSideAreaCarLayer_, numAntennaMaxSideAreaCar_);

    LEF_COPY_FUNC( numAntennaMaxCutCar_ );
    LEF_COPY_FUNC( antennaMaxCutCarAllocated_ );
    LEF_MALLOC_FUNC( antennaMaxCutCar_, double, sizeof(double) * numAntennaMaxCutCar_);
    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaMaxCutCarLayer_, numAntennaMaxCutCar_ );
}

LEF_ASSIGN_OPERATOR_C( lefiPinAntennaModel ) {
    CHECK_SELF_ASSIGN
    oxide_ = 0;
    antennaGateArea_ = 0;
    antennaGateAreaLayer_ = 0;
    antennaMaxAreaCar_ = 0;
    antennaMaxAreaCarLayer_ = 0;
    antennaMaxSideAreaCar_ = 0;
    antennaMaxSideAreaCarLayer_ = 0;
    antennaMaxCutCar_ = 0;
    antennaMaxCutCarLayer_ = 0;


    LEF_MALLOC_FUNC( oxide_, char, sizeof(char)*(strlen(prev.oxide_)+1) );
    LEF_COPY_FUNC( hasReturn_ );
    LEF_COPY_FUNC( numAntennaGateArea_ );
    LEF_COPY_FUNC( antennaGateAreaAllocated_ );
    LEF_MALLOC_FUNC( antennaGateArea_, double, sizeof(double) * numAntennaGateArea_);

    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaGateAreaLayer_, numAntennaGateArea_ );

    LEF_COPY_FUNC( numAntennaMaxAreaCar_ );
    LEF_COPY_FUNC( antennaMaxAreaCarAllocated_ );
    LEF_MALLOC_FUNC( antennaMaxAreaCar_, double, sizeof(double) * numAntennaMaxAreaCar_);
    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaMaxAreaCarLayer_, numAntennaMaxAreaCar_ );

    LEF_COPY_FUNC( numAntennaMaxSideAreaCar_ );
    LEF_COPY_FUNC( antennaMaxSideAreaCarAllocated_ );
    LEF_MALLOC_FUNC( antennaMaxSideAreaCar_, double, sizeof(double) * numAntennaMaxSideAreaCar_);
    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaMaxSideAreaCarLayer_, numAntennaMaxSideAreaCar_);

    LEF_COPY_FUNC( numAntennaMaxCutCar_ );
    LEF_COPY_FUNC( antennaMaxCutCarAllocated_ );
    LEF_MALLOC_FUNC( antennaMaxCutCar_, double, sizeof(double) * numAntennaMaxCutCar_);
    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaMaxCutCarLayer_, numAntennaMaxCutCar_ );
    return *this;
}

lefiPinAntennaModel::~lefiPinAntennaModel()
{
    Destroy();
}

void
lefiPinAntennaModel::Destroy()
{
    clear();
}

void
lefiPinAntennaModel::clear()
{
    int i;

    if (oxide_)
        lefFree((char*) (oxide_));
    else       // did not declare
        return;
    oxide_ = 0;
    hasReturn_ = 0;

    for (i = 0; i < numAntennaGateArea_; i++) {
        if (antennaGateAreaLayer_[i])
            lefFree(antennaGateAreaLayer_[i]);
    }
    numAntennaGateArea_ = 0;

    for (i = 0; i < numAntennaMaxAreaCar_; i++) {
        if (antennaMaxAreaCarLayer_[i])
            lefFree(antennaMaxAreaCarLayer_[i]);
    }
    numAntennaMaxAreaCar_ = 0;

    for (i = 0; i < numAntennaMaxSideAreaCar_; i++) {
        if (antennaMaxSideAreaCarLayer_[i])
            lefFree(antennaMaxSideAreaCarLayer_[i]);
    }
    numAntennaMaxSideAreaCar_ = 0;

    for (i = 0; i < numAntennaMaxCutCar_; i++) {
        if (antennaMaxCutCarLayer_[i])
            lefFree(antennaMaxCutCarLayer_[i]);
    }
    numAntennaMaxCutCar_ = 0;
    lefFree((char*) (antennaGateArea_));
    lefFree((char*) (antennaGateAreaLayer_));
    lefFree((char*) (antennaMaxAreaCar_));
    lefFree((char*) (antennaMaxAreaCarLayer_));
    lefFree((char*) (antennaMaxSideAreaCar_));
    lefFree((char*) (antennaMaxSideAreaCarLayer_));
    lefFree((char*) (antennaMaxCutCar_));
    lefFree((char*) (antennaMaxCutCarLayer_));

    antennaGateArea_ = 0;
    antennaGateAreaLayer_ = 0;
    antennaMaxAreaCar_ = 0;
    antennaMaxAreaCarLayer_ = 0;
    antennaMaxSideAreaCar_ = 0;
    antennaMaxSideAreaCarLayer_ = 0;
    antennaMaxCutCar_ = 0;
    antennaMaxCutCarLayer_ = 0;
}

// 5.5
void
lefiPinAntennaModel::setAntennaModel(int aOxide)
{
    switch (aOxide) {
    case 1:
        oxide_ = strdup("OXIDE1");
        break;
    case 2:
        oxide_ = strdup("OXIDE2");
        break;
    case 3:
        oxide_ = strdup("OXIDE3");
        break;
    case 4:
        oxide_ = strdup("OXIDE4");
        break;
    default:
        oxide_ = NULL;
        break;
    }
}

void
lefiPinAntennaModel::addAntennaGateArea(double      val,
                                        const char  *layer)
{
    int len;
    if (numAntennaGateArea_ == antennaGateAreaAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaGateArea_;
        double  *nd;
        char    **nl;


        if (antennaGateAreaAllocated_ == 0)
            max = antennaGateAreaAllocated_ = 2;
        else
            max = antennaGateAreaAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaGateAreaLayer_[i];
            nd[i] = antennaGateArea_[i];
        }
        lefFree((char*) (antennaGateAreaLayer_));
        lefFree((char*) (antennaGateArea_));
        antennaGateAreaLayer_ = nl;
        antennaGateArea_ = nd;
    }
    antennaGateArea_[numAntennaGateArea_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaGateAreaLayer_[numAntennaGateArea_] =
            (char*) lefMalloc(len);
        strcpy(antennaGateAreaLayer_[numAntennaGateArea_],
               layer);
    } else
        antennaGateAreaLayer_[numAntennaGateArea_] = NULL;
    numAntennaGateArea_ += 1;
}

void
lefiPinAntennaModel::addAntennaMaxAreaCar(double        val,
                                          const char    *layer)
{
    int len;
    if (numAntennaMaxAreaCar_ == antennaMaxAreaCarAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaMaxAreaCar_;
        double  *nd;
        char    **nl;

        if (antennaMaxAreaCarAllocated_ == 0)
            max = antennaMaxAreaCarAllocated_ = 2;
        else
            max = antennaMaxAreaCarAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaMaxAreaCarLayer_[i];
            nd[i] = antennaMaxAreaCar_[i];
        }
        lefFree((char*) (antennaMaxAreaCarLayer_));
        lefFree((char*) (antennaMaxAreaCar_));
        antennaMaxAreaCarLayer_ = nl;
        antennaMaxAreaCar_ = nd;
    }
    antennaMaxAreaCar_[numAntennaMaxAreaCar_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaMaxAreaCarLayer_[numAntennaMaxAreaCar_] =
            (char*) lefMalloc(len);
        strcpy(antennaMaxAreaCarLayer_[numAntennaMaxAreaCar_],
               layer);
    } else
        antennaMaxAreaCarLayer_[numAntennaMaxAreaCar_] = NULL;
    numAntennaMaxAreaCar_ += 1;
}

void
lefiPinAntennaModel::addAntennaMaxSideAreaCar(double        val,
                                              const char    *layer)
{
    int len;
    if (numAntennaMaxSideAreaCar_ == antennaMaxSideAreaCarAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaMaxSideAreaCar_;
        double  *nd;
        char    **nl;

        if (antennaMaxSideAreaCarAllocated_ == 0)
            max = antennaMaxSideAreaCarAllocated_ = 2;
        else
            max = antennaMaxSideAreaCarAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaMaxSideAreaCarLayer_[i];
            nd[i] = antennaMaxSideAreaCar_[i];
        }
        lefFree((char*) (antennaMaxSideAreaCarLayer_));
        lefFree((char*) (antennaMaxSideAreaCar_));
        antennaMaxSideAreaCarLayer_ = nl;
        antennaMaxSideAreaCar_ = nd;
    }
    antennaMaxSideAreaCar_[numAntennaMaxSideAreaCar_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaMaxSideAreaCarLayer_[numAntennaMaxSideAreaCar_] =
            (char*) lefMalloc(len);
        strcpy(antennaMaxSideAreaCarLayer_[numAntennaMaxSideAreaCar_],
               layer);
    } else
        antennaMaxSideAreaCarLayer_[numAntennaMaxSideAreaCar_] = NULL;
    numAntennaMaxSideAreaCar_ += 1;
}

void
lefiPinAntennaModel::addAntennaMaxCutCar(double     val,
                                         const char *layer)
{
    int len;
    if (numAntennaMaxCutCar_ == antennaMaxCutCarAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaMaxCutCar_;
        double  *nd;
        char    **nl;

        if (antennaMaxCutCarAllocated_ == 0)
            max = antennaMaxCutCarAllocated_ = 2;
        else
            max = antennaMaxCutCarAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaMaxCutCarLayer_[i];
            nd[i] = antennaMaxCutCar_[i];
        }
        lefFree((char*) (antennaMaxCutCarLayer_));
        lefFree((char*) (antennaMaxCutCar_));
        antennaMaxCutCarLayer_ = nl;
        antennaMaxCutCar_ = nd;
    }
    antennaMaxCutCar_[numAntennaMaxCutCar_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaMaxCutCarLayer_[numAntennaMaxCutCar_] =
            (char*) lefMalloc(len);
        strcpy(antennaMaxCutCarLayer_[numAntennaMaxCutCar_],
               layer);
    } else
        antennaMaxCutCarLayer_[numAntennaMaxCutCar_] = NULL;
    numAntennaMaxCutCar_ += 1;
}

void
lefiPinAntennaModel::setAntennaReturnFlag(int flag)
{
    hasReturn_ = flag;
    return;
}

int
lefiPinAntennaModel::hasAntennaGateArea() const
{
    return numAntennaGateArea_ ? 1 : 0;
}

int
lefiPinAntennaModel::hasAntennaMaxAreaCar() const
{
    return numAntennaMaxAreaCar_ ? 1 : 0;
}

int
lefiPinAntennaModel::hasAntennaMaxSideAreaCar() const
{
    return numAntennaMaxSideAreaCar_ ? 1 : 0;
}

int
lefiPinAntennaModel::hasAntennaMaxCutCar() const
{
    return numAntennaMaxCutCar_ ? 1 : 0;
}

// 5.5
char *
lefiPinAntennaModel::antennaOxide() const
{
    return oxide_;
}

const char *
lefiPinAntennaModel::antennaGateAreaLayer(int i) const
{
    return antennaGateAreaLayer_[i];
}

const char *
lefiPinAntennaModel::antennaMaxAreaCarLayer(int i) const
{
    return antennaMaxAreaCarLayer_[i];
}

const char *
lefiPinAntennaModel::antennaMaxSideAreaCarLayer(int i) const
{
    return antennaMaxSideAreaCarLayer_[i];
}

const char *
lefiPinAntennaModel::antennaMaxCutCarLayer(int i) const
{
    return antennaMaxCutCarLayer_[i];
}

int
lefiPinAntennaModel::numAntennaGateArea() const
{
    return numAntennaGateArea_;
}

int
lefiPinAntennaModel::numAntennaMaxAreaCar() const
{
    return numAntennaMaxAreaCar_;
}

int
lefiPinAntennaModel::numAntennaMaxSideAreaCar() const
{
    return numAntennaMaxSideAreaCar_;
}

int
lefiPinAntennaModel::numAntennaMaxCutCar() const
{
    return numAntennaMaxCutCar_;
}

double
lefiPinAntennaModel::antennaGateArea(int i) const
{
    return antennaGateArea_[i];
}

double
lefiPinAntennaModel::antennaMaxAreaCar(int i) const
{
    return antennaMaxAreaCar_[i];
}

double
lefiPinAntennaModel::antennaMaxSideAreaCar(int i) const
{
    return antennaMaxSideAreaCar_[i];
}

double
lefiPinAntennaModel::antennaMaxCutCar(int i) const
{
    return antennaMaxCutCar_[i];
}

int
lefiPinAntennaModel::hasReturn() const
{
    return hasReturn_;
}

// *****************************************************************************
// lefiPin
// *****************************************************************************

lefiPin::lefiPin()
: nameSize_(0),
  name_(NULL),
  hasLEQ_(0),
  hasDirection_(0),
  hasUse_(0),
  hasShape_(0),
  hasMustjoin_(0),
  hasOutMargin_(0),
  hasOutResistance_(0),
  hasInMargin_(0),
  hasPower_(0),
  hasLeakage_(0),
  hasMaxload_(0),
  hasMaxdelay_(0),
  hasCapacitance_(0),
  hasResistance_(0),
  hasPulldownres_(0),
  hasTieoffr_(0),
  hasVHI_(0),
  hasVLO_(0),
  hasRiseVoltage_(0),
  hasFallVoltage_(0),
  hasRiseThresh_(0),
  hasFallThresh_(0),
  hasRiseSatcur_(0),
  hasFallSatcur_(0),
  hasCurrentSource_(0),
  hasTables_(0),
  hasAntennasize_(0),
  hasRiseSlewLimit_(0),
  hasFallSlewLimit_(0),
  numForeigns_(0),
  foreignAllocated_(0),
  hasForeignOrient_(NULL),
  hasForeignPoint_(NULL),
  foreignOrient_(NULL),
  foreignX_(NULL),
  foreignY_(NULL),
  foreign_(NULL),
  LEQSize_(0),
  LEQ_(NULL),
  mustjoinSize_(0),
  mustjoin_(NULL),
  outMarginH_(0.0),
  outMarginL_(0.0),
  outResistanceH_(0.0),
  outResistanceL_(0.0),
  inMarginH_(0.0),
  inMarginL_(0.0),
  power_(0.0),
  leakage_(0.0),
  maxload_(0.0),
  maxdelay_(0.0),
  capacitance_(0.0),
  resistance_(0.0),
  pulldownres_(0.0),
  tieoffr_(0.0),
  VHI_(0.0),
  VLO_(0.0),
  riseVoltage_(0.0),
  fallVoltage_(0.0),
  riseThresh_(0.0),
  fallThresh_(0.0),
  riseSatcur_(0.0),
  fallSatcur_(0.0),
  lowTableSize_(0),
  lowTable_(NULL),
  highTableSize_(0),
  highTable_(NULL),
  riseSlewLimit_(0.0),
  fallSlewLimit_(0.0),
  numAntennaModel_(0),
  antennaModelAllocated_(0),
  curAntennaModelIndex_(0),
  antennaModel_(NULL),
  numAntennaSize_(0),
  antennaSizeAllocated_(0),
  antennaSize_(NULL),
  antennaSizeLayer_(NULL),
  numAntennaMetalArea_(0),
  antennaMetalAreaAllocated_(0),
  antennaMetalArea_(NULL),
  antennaMetalAreaLayer_(NULL),
  numAntennaMetalLength_(0),
  antennaMetalLengthAllocated_(0),
  antennaMetalLength_(NULL),
  antennaMetalLengthLayer_(NULL),
  numAntennaPartialMetalArea_(0),
  antennaPartialMetalAreaAllocated_(0),
  antennaPartialMetalArea_(NULL),
  antennaPartialMetalAreaLayer_(NULL),
  numAntennaPartialMetalSideArea_(0),
  antennaPartialMetalSideAreaAllocated_(0),
  antennaPartialMetalSideArea_(NULL),
  antennaPartialMetalSideAreaLayer_(NULL),
  numAntennaPartialCutArea_(0),
  antennaPartialCutAreaAllocated_(0),
  antennaPartialCutArea_(NULL),
  antennaPartialCutAreaLayer_(NULL),
  numAntennaDiffArea_(0),
  antennaDiffAreaAllocated_(0),
  antennaDiffArea_(NULL),
  antennaDiffAreaLayer_(NULL),
  taperRule_(NULL),
  netEpxr_(NULL),
  ssPinName_(NULL),
  gsPinName_(NULL),
  numProperties_(0),
  propertiesAllocated_(0),
  propNames_(NULL),
  propValues_(NULL),
  propNums_(NULL),
  propTypes_(NULL),
  numPorts_(0),
  portsAllocated_(0),
  ports_(NULL)
{
    Init();
}

void
lefiPin::Init()
{
    nameSize_ = 16;
    name_ = (char*) lefMalloc(16);
    portsAllocated_ = 2;
    ports_ = (lefiGeometries**) lefMalloc(sizeof(lefiGeometries*) * 2);
    numPorts_ = 0;
    numProperties_ = 0;
    propertiesAllocated_ = 0;
    propNames_ = 0;
    propValues_ = 0;
    propTypes_ = 0;
    foreign_ = 0;
    LEQ_ = 0;
    mustjoin_ = 0;
    lowTable_ = 0;
    highTable_ = 0;
    taperRule_ = 0;
    antennaModel_ = 0;
    numAntennaModel_ = 0;
    netEpxr_ = 0;
    ssPinName_ = 0;
    gsPinName_ = 0;

    bump(&(LEQ_), 16, &(LEQSize_));
    bump(&(mustjoin_), 16, &(mustjoinSize_));
    bump(&(lowTable_), 16, &(lowTableSize_));
    bump(&(highTable_), 16, &(highTableSize_));

    numAntennaSize_ = 0;
    antennaSizeAllocated_ = 1;
    antennaSize_ = (double*) lefMalloc(sizeof(double));
    antennaSizeLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaMetalArea_ = 0;
    antennaMetalAreaAllocated_ = 1;
    antennaMetalArea_ = (double*) lefMalloc(sizeof(double));
    antennaMetalAreaLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaMetalLength_ = 0;
    antennaMetalLengthAllocated_ = 1;
    antennaMetalLength_ = (double*) lefMalloc(sizeof(double));
    antennaMetalLengthLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaPartialMetalArea_ = 0;
    antennaPartialMetalAreaAllocated_ = 1;
    antennaPartialMetalArea_ = (double*) lefMalloc(sizeof(double));
    antennaPartialMetalAreaLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaPartialMetalSideArea_ = 0;
    antennaPartialMetalSideAreaAllocated_ = 1;
    antennaPartialMetalSideArea_ = (double*) lefMalloc(sizeof(double));
    antennaPartialMetalSideAreaLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaPartialCutArea_ = 0;
    antennaPartialCutAreaAllocated_ = 1;
    antennaPartialCutArea_ = (double*) lefMalloc(sizeof(double));
    antennaPartialCutAreaLayer_ = (char**) lefMalloc(sizeof(char*));

    numAntennaDiffArea_ = 0;
    antennaDiffAreaAllocated_ = 1;
    antennaDiffArea_ = (double*) lefMalloc(sizeof(double));
    antennaDiffAreaLayer_ = (char**) lefMalloc(sizeof(char*));
}

LEF_COPY_CONSTRUCTOR_C(lefiPin)
: nameSize_(0),
  name_(NULL),
  hasLEQ_(0),
  hasDirection_(0),
  hasUse_(0),
  hasShape_(0),
  hasMustjoin_(0),
  hasOutMargin_(0),
  hasOutResistance_(0),
  hasInMargin_(0),
  hasPower_(0),
  hasLeakage_(0),
  hasMaxload_(0),
  hasMaxdelay_(0),
  hasCapacitance_(0),
  hasResistance_(0),
  hasPulldownres_(0),
  hasTieoffr_(0),
  hasVHI_(0),
  hasVLO_(0),
  hasRiseVoltage_(0),
  hasFallVoltage_(0),
  hasRiseThresh_(0),
  hasFallThresh_(0),
  hasRiseSatcur_(0),
  hasFallSatcur_(0),
  hasCurrentSource_(0),
  hasTables_(0),
  hasAntennasize_(0),
  hasRiseSlewLimit_(0),
  hasFallSlewLimit_(0),
  numForeigns_(0),
  foreignAllocated_(0),
  hasForeignOrient_(NULL),
  hasForeignPoint_(NULL),
  foreignOrient_(NULL),
  foreignX_(NULL),
  foreignY_(NULL),
  foreign_(NULL),
  LEQSize_(0),
  LEQ_(NULL),
  mustjoinSize_(0),
  mustjoin_(NULL),
  outMarginH_(0.0),
  outMarginL_(0.0),
  outResistanceH_(0.0),
  outResistanceL_(0.0),
  inMarginH_(0.0),
  inMarginL_(0.0),
  power_(0.0),
  leakage_(0.0),
  maxload_(0.0),
  maxdelay_(0.0),
  capacitance_(0.0),
  resistance_(0.0),
  pulldownres_(0.0),
  tieoffr_(0.0),
  VHI_(0.0),
  VLO_(0.0),
  riseVoltage_(0.0),
  fallVoltage_(0.0),
  riseThresh_(0.0),
  fallThresh_(0.0),
  riseSatcur_(0.0),
  fallSatcur_(0.0),
  lowTableSize_(0),
  lowTable_(NULL),
  highTableSize_(0),
  highTable_(NULL),
  riseSlewLimit_(0.0),
  fallSlewLimit_(0.0),
  numAntennaModel_(0),
  antennaModelAllocated_(0),
  curAntennaModelIndex_(0),
  antennaModel_(NULL),
  numAntennaSize_(0),
  antennaSizeAllocated_(0),
  antennaSize_(NULL),
  antennaSizeLayer_(NULL),
  numAntennaMetalArea_(0),
  antennaMetalAreaAllocated_(0),
  antennaMetalArea_(NULL),
  antennaMetalAreaLayer_(NULL),
  numAntennaMetalLength_(0),
  antennaMetalLengthAllocated_(0),
  antennaMetalLength_(NULL),
  antennaMetalLengthLayer_(NULL),
  numAntennaPartialMetalArea_(0),
  antennaPartialMetalAreaAllocated_(0),
  antennaPartialMetalArea_(NULL),
  antennaPartialMetalAreaLayer_(NULL),
  numAntennaPartialMetalSideArea_(0),
  antennaPartialMetalSideAreaAllocated_(0),
  antennaPartialMetalSideArea_(NULL),
  antennaPartialMetalSideAreaLayer_(NULL),
  numAntennaPartialCutArea_(0),
  antennaPartialCutAreaAllocated_(0),
  antennaPartialCutArea_(NULL),
  antennaPartialCutAreaLayer_(NULL),
  numAntennaDiffArea_(0),
  antennaDiffAreaAllocated_(0),
  antennaDiffArea_(NULL),
  antennaDiffAreaLayer_(NULL),
  taperRule_(NULL),
  netEpxr_(NULL),
  ssPinName_(NULL),
  gsPinName_(NULL),
  numProperties_(0),
  propertiesAllocated_(0),
  propNames_(NULL),
  propValues_(NULL),
  propNums_(NULL),
  propTypes_(NULL),
  numPorts_(0),
  portsAllocated_(0),
  ports_(NULL)
{
//    printf("COPY CONSTRUCTOR STARTED\n");
//    fflush(stdout);
    this->Init();
//    printf("COPY CONSTRUCTOR INIT\n");
//    fflush(stdout);
    LEF_COPY_FUNC( nameSize_ );
    LEF_MALLOC_FUNC( name_, char, sizeof(char)*nameSize_ );
    LEF_COPY_FUNC( hasLEQ_ );
    LEF_COPY_FUNC( hasDirection_ );
    LEF_COPY_FUNC( hasUse_ );
    LEF_COPY_FUNC( hasShape_ );
    LEF_COPY_FUNC( hasMustjoin_ );
    LEF_COPY_FUNC( hasOutMargin_ );
    LEF_COPY_FUNC( hasOutResistance_ );
    LEF_COPY_FUNC( hasInMargin_ );
    LEF_COPY_FUNC( hasPower_ );
    LEF_COPY_FUNC( hasLeakage_ );
    LEF_COPY_FUNC( hasMaxload_ );
    LEF_COPY_FUNC( hasMaxdelay_ );
    LEF_COPY_FUNC( hasCapacitance_ );
    LEF_COPY_FUNC( hasResistance_ );
    LEF_COPY_FUNC( hasPulldownres_ );
    LEF_COPY_FUNC( hasTieoffr_ );
    LEF_COPY_FUNC( hasVHI_ );
    LEF_COPY_FUNC( hasVLO_ );
    LEF_COPY_FUNC( hasRiseVoltage_ );
    LEF_COPY_FUNC( hasFallVoltage_ );
    LEF_COPY_FUNC( hasRiseThresh_ );
    LEF_COPY_FUNC( hasFallThresh_ );
    LEF_COPY_FUNC( hasRiseSatcur_ );
    LEF_COPY_FUNC( hasFallSatcur_ );
    LEF_COPY_FUNC( hasCurrentSource_ );
    LEF_COPY_FUNC( hasTables_ );
    LEF_COPY_FUNC( hasAntennasize_ );
    LEF_COPY_FUNC( hasRiseSlewLimit_ );
    LEF_COPY_FUNC( hasFallSlewLimit_ );
    LEF_COPY_FUNC( numForeigns_ );
    LEF_COPY_FUNC( foreignAllocated_ );
    LEF_MALLOC_FUNC( hasForeignOrient_, int, sizeof(int)* foreignAllocated_ );
    LEF_MALLOC_FUNC( hasForeignPoint_, int, sizeof(int)* foreignAllocated_ );
    LEF_MALLOC_FUNC( foreignOrient_, int, sizeof(int)* foreignAllocated_ );
    LEF_MALLOC_FUNC( foreignX_, double, sizeof(double)* foreignAllocated_ );
    LEF_MALLOC_FUNC( foreignY_, double, sizeof(double)* foreignAllocated_ );

    LEF_MALLOC_FUNC_FOR_2D_STR( foreign_, numForeigns_);
    LEF_COPY_FUNC( LEQSize_ );
    LEF_MALLOC_FUNC( LEQ_, char, sizeof(char)* LEQSize_ );
    LEF_COPY_FUNC( mustjoinSize_ );
    LEF_MALLOC_FUNC( mustjoin_, char, sizeof(char)*mustjoinSize_ );
    LEF_COPY_FUNC( outMarginH_ );
    LEF_COPY_FUNC( outMarginL_ );
    LEF_COPY_FUNC( outResistanceH_ );
    LEF_COPY_FUNC( outResistanceL_ );
    LEF_COPY_FUNC( inMarginH_ );
    LEF_COPY_FUNC( inMarginL_ );
    LEF_COPY_FUNC( power_ );
    LEF_COPY_FUNC( leakage_ );
    LEF_COPY_FUNC( maxload_ );
    LEF_COPY_FUNC( maxdelay_ );
    LEF_COPY_FUNC( capacitance_ );
    LEF_COPY_FUNC( resistance_ );
    LEF_COPY_FUNC( pulldownres_ );
    LEF_COPY_FUNC( tieoffr_ );
    LEF_COPY_FUNC( VHI_ );
    LEF_COPY_FUNC( VLO_ );
    LEF_COPY_FUNC( riseVoltage_ );
    LEF_COPY_FUNC( fallVoltage_ );
    LEF_COPY_FUNC( riseThresh_ );
    LEF_COPY_FUNC( fallThresh_ );
    LEF_COPY_FUNC( riseSatcur_ );
    LEF_COPY_FUNC( fallSatcur_ );
    LEF_COPY_FUNC( lowTableSize_ );
    LEF_MALLOC_FUNC( lowTable_, char, sizeof(char)*lowTableSize_ );
    LEF_COPY_FUNC( highTableSize_ );
    LEF_MALLOC_FUNC( highTable_, char, sizeof(char)*highTableSize_ );
    LEF_COPY_FUNC( riseSlewLimit_ );
    LEF_COPY_FUNC( fallSlewLimit_ );
    LEF_COPY_FUNC( numAntennaModel_ );
    LEF_COPY_FUNC( antennaModelAllocated_ );
    LEF_COPY_FUNC( curAntennaModelIndex_ );


    // printf("antennaModel_ %x\n", antennaModel_);
    // printf("prev.antennaModel_ %x\n", prev.antennaModel_);
    // printf("numAntennaModel_ %d\n", numAntennaModel_);
    // fflush(stdout);

    // if( prev.antennaModel_ ) {
    //     for(int i=0; i<numAntennaModel_; i++) {
    //         printf("%d %x\n", i, prev.antennaModel_[i]);
    //         fflush(stdout);
    //     }
    // }

    LEF_MALLOC_FUNC_FOR_2D( antennaModel_, lefiPinAntennaModel, numAntennaModel_, 1); 


    LEF_COPY_FUNC( numAntennaSize_ );
    LEF_COPY_FUNC( antennaSizeAllocated_ );
    LEF_MALLOC_FUNC( antennaSize_, double, sizeof(double)*antennaSizeAllocated_ );
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaSizeLayer_, numAntennaSize_);

    LEF_COPY_FUNC( numAntennaMetalArea_ );
    LEF_COPY_FUNC( antennaMetalAreaAllocated_ );
    LEF_MALLOC_FUNC( antennaMetalArea_, double, sizeof(double)*antennaMetalAreaAllocated_ );
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaMetalAreaLayer_, numAntennaMetalArea_);

    LEF_COPY_FUNC( numAntennaMetalLength_ );
    LEF_COPY_FUNC( antennaMetalLengthAllocated_ );
    LEF_MALLOC_FUNC( antennaMetalLength_, double, sizeof(double)*antennaMetalLengthAllocated_ );
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaMetalLengthLayer_, numAntennaMetalLength_);

    LEF_COPY_FUNC( numAntennaPartialMetalArea_ );
    LEF_COPY_FUNC( antennaPartialMetalAreaAllocated_ );
    LEF_MALLOC_FUNC( antennaPartialMetalArea_, double, sizeof(double)*antennaPartialMetalAreaAllocated_ );
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaPartialMetalAreaLayer_, numAntennaPartialMetalArea_);

    LEF_COPY_FUNC( numAntennaPartialMetalSideArea_ );
    LEF_COPY_FUNC( antennaPartialMetalSideAreaAllocated_ );
    LEF_MALLOC_FUNC( antennaPartialMetalSideArea_, double, sizeof(double)*antennaPartialMetalSideAreaAllocated_ );
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaPartialMetalSideAreaLayer_, numAntennaPartialMetalSideArea_);

    LEF_COPY_FUNC( numAntennaPartialCutArea_ );
    LEF_COPY_FUNC( antennaPartialCutAreaAllocated_ );
    LEF_MALLOC_FUNC( antennaPartialCutArea_, double, sizeof(double)*antennaPartialCutAreaAllocated_ );
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaPartialCutAreaLayer_, numAntennaPartialCutArea_ );

    LEF_COPY_FUNC( numAntennaDiffArea_ );
    LEF_COPY_FUNC( antennaDiffAreaAllocated_ );
    LEF_MALLOC_FUNC( antennaDiffArea_, double, sizeof(double)*antennaDiffAreaAllocated_ );
    LEF_MALLOC_FUNC_FOR_2D_STR( antennaDiffAreaLayer_, numAntennaDiffArea_);

    LEF_MALLOC_FUNC( taperRule_, char, sizeof(char) *   (strlen(prev.taperRule_) + 1));
    LEF_MALLOC_FUNC( netEpxr_, char, sizeof(char)   *   (strlen(prev.netEpxr_) + 1));
    LEF_MALLOC_FUNC( ssPinName_, char, sizeof(char) *   (strlen(prev.ssPinName_) + 1));
    LEF_MALLOC_FUNC( gsPinName_, char, sizeof(char) *   (strlen(prev.gsPinName_) + 1));

    memcpy(direction_, prev.direction_, sizeof(char)*32);
    memcpy(use_, prev.use_, sizeof(char)*12);
    memcpy(shape_, prev.shape_, sizeof(char)*12);
    memcpy(currentSource_, prev.currentSource_, sizeof(char)*12);

    LEF_COPY_FUNC( numProperties_ );
    LEF_COPY_FUNC( propertiesAllocated_ );

    LEF_MALLOC_FUNC_FOR_2D_STR( propNames_, numProperties_);
    LEF_MALLOC_FUNC_FOR_2D_STR( propValues_, numProperties_);
    LEF_MALLOC_FUNC( propNums_, double, sizeof(double) * propertiesAllocated_);
    LEF_MALLOC_FUNC( propTypes_, char, sizeof(char) * propertiesAllocated_ );

    LEF_COPY_FUNC( numPorts_ );
    LEF_COPY_FUNC( portsAllocated_ );
//    printf("PORTS BEFORE CONSTRUCTOR\n");
//    fflush(stdout);
    LEF_MALLOC_FUNC_FOR_2D( ports_, lefiGeometries, numPorts_, 1);
//    printf("PORTS AFTER CONSTRUCTOR\n");
//    printf("prev_ports_Adder : %x\n", prev.ports_);
//    printf("ports_Adder : %x\n", ports_);
//    printf("prev num_items: %d\n", prev.ports_[0]->lefiGeometries::numItems());
//    printf("curr num_items: %d\n", ports_[0]->lefiGeometries::numItems());
//    printf("PREV LAYER %d\n", prev.ports_[0]->lefiGeometries::getLayer(0));
//    printf("CURRENT LAYER %d\n", ports_[0]->lefiGeometries::getLayer(0));
    fflush(stdout);
}

lefiPin::~lefiPin()
{
    Destroy();
}

void
lefiPin::Destroy()
{

    clear();
    lefFree(name_);
    lefFree((char*) (ports_));
    lefFree(LEQ_);
    lefFree(mustjoin_);
    lefFree(lowTable_);
    lefFree(highTable_);
    if (propNames_)
        lefFree((char*) (propNames_));
    propNames_ = 0;
    if (propValues_)
        lefFree((char*) (propValues_));
    propValues_ = 0;
    if (propNums_)
        lefFree((char*) (propNums_));
    propNums_ = 0;
    if (propTypes_)
        lefFree((char*) (propTypes_));
    propTypes_ = 0;
    lefFree((char*) (antennaSize_));
    lefFree((char*) (antennaSizeLayer_));
    lefFree((char*) (antennaMetalArea_));
    lefFree((char*) (antennaMetalAreaLayer_));
    lefFree((char*) (antennaMetalLength_));
    lefFree((char*) (antennaMetalLengthLayer_));
    lefFree((char*) (antennaPartialMetalArea_));
    lefFree((char*) (antennaPartialMetalAreaLayer_));
    lefFree((char*) (antennaPartialMetalSideArea_));
    lefFree((char*) (antennaPartialMetalSideAreaLayer_));
    lefFree((char*) (antennaPartialCutArea_));
    lefFree((char*) (antennaPartialCutAreaLayer_));
    lefFree((char*) (antennaDiffArea_));
    lefFree((char*) (antennaDiffAreaLayer_));
    if (foreignAllocated_) {
        lefFree((char*) (hasForeignOrient_));
        lefFree((char*) (hasForeignPoint_));
        lefFree((char*) (foreignOrient_));
        lefFree((char*) (foreignX_));
        lefFree((char*) (foreignY_));
        lefFree((char*) (foreign_));
        foreignAllocated_ = 0;
    }
}

void
lefiPin::clear()
{
    int                 i;
    lefiPinAntennaModel *aModel;

//    This destructiong is automatically done in Vector's destructor
//    for (i = 0; i < numPorts_; i++) {
//        g = ports_[i];
//        g->Destroy();
//        lefFree((char*) g);
//    }
    numPorts_ = 0;
    portsAllocated_ = 0;

    hasLEQ_ = 0;
    hasDirection_ = 0;
    hasUse_ = 0;
    hasShape_ = 0;
    hasMustjoin_ = 0;
    hasOutMargin_ = 0;
    hasOutResistance_ = 0;
    hasInMargin_ = 0;
    hasPower_ = 0;
    hasLeakage_ = 0;
    hasMaxload_ = 0;
    hasMaxdelay_ = 0;
    hasCapacitance_ = 0;
    hasResistance_ = 0;
    hasPulldownres_ = 0;
    hasTieoffr_ = 0;
    hasVHI_ = 0;
    hasVLO_ = 0;
    hasRiseVoltage_ = 0;
    hasFallVoltage_ = 0;
    hasRiseThresh_ = 0;
    hasFallThresh_ = 0;
    hasRiseSatcur_ = 0;
    hasFallSatcur_ = 0;
    hasCurrentSource_ = 0;
    hasRiseSlewLimit_ = 0;
    hasFallSlewLimit_ = 0;
    hasTables_ = 0;
    strcpy(use_, "SIGNAL");

    for (i = 0; i < numForeigns_; i++) {
        hasForeignOrient_[i] = 0;
        hasForeignPoint_[i] = 0;
        foreignOrient_[i] = -1;
        lefFree((char*) (foreign_[i]));
    }
    numForeigns_ = 0;

    for (i = 0; i < numAntennaSize_; i++) {
        if (antennaSizeLayer_[i])
            lefFree(antennaSizeLayer_[i]);
    }
    numAntennaSize_ = 0;

    for (i = 0; i < numAntennaMetalLength_; i++) {
        if (antennaMetalLengthLayer_[i])
            lefFree(antennaMetalLengthLayer_[i]);
    }
    numAntennaMetalLength_ = 0;

    for (i = 0; i < numAntennaMetalArea_; i++) {
        if (antennaMetalAreaLayer_[i])
            lefFree(antennaMetalAreaLayer_[i]);
    }
    numAntennaMetalArea_ = 0;

    for (i = 0; i < numAntennaPartialMetalArea_; i++) {
        if (antennaPartialMetalAreaLayer_[i])
            lefFree(antennaPartialMetalAreaLayer_[i]);
    }
    numAntennaPartialMetalArea_ = 0;

    for (i = 0; i < numAntennaPartialMetalSideArea_; i++) {
        if (antennaPartialMetalSideAreaLayer_[i])
            lefFree(antennaPartialMetalSideAreaLayer_[i]);
    }
    numAntennaPartialMetalSideArea_ = 0;

    for (i = 0; i < numAntennaPartialCutArea_; i++) {
        if (antennaPartialCutAreaLayer_[i])
            lefFree(antennaPartialCutAreaLayer_[i]);
    }
    numAntennaPartialCutArea_ = 0;

    for (i = 0; i < numAntennaDiffArea_; i++) {
        if (antennaDiffAreaLayer_[i])
            lefFree(antennaDiffAreaLayer_[i]);
    }
    numAntennaDiffArea_ = 0;

    if (numAntennaModel_ > 0) {
        for (i = 0; i < numAntennaModel_; i++) {        // 5.5
            aModel = antennaModel_[i];
            aModel->Destroy();
        }
    }
    for (i = 0; i < numAntennaModel_; i++) {  // 5.5
        lefFree((char*) antennaModel_[i]);
    }
    if (antennaModel_)                              // 5.5
        lefFree((char*) antennaModel_);
    antennaModel_ = 0;             // 5.5
    numAntennaModel_ = 0;          // 5.5
    curAntennaModelIndex_ = 0;     // 5.5
    antennaModelAllocated_ = 0;    // 5.5

    for (i = 0; i < numProperties_; i++) {
        lefFree(propNames_[i]);
        lefFree(propValues_[i]);
    }
    numProperties_ = 0;
    propertiesAllocated_ = 0;
    if (taperRule_) {
        lefFree(taperRule_);
        taperRule_ = 0;
    }
    if (netEpxr_) {
        lefFree(netEpxr_);
        netEpxr_ = 0;
    }
    if (ssPinName_) {
        lefFree(ssPinName_);
        ssPinName_ = 0;
    }
    if (gsPinName_) {
        lefFree(gsPinName_);
        gsPinName_ = 0;
    }
}

void
lefiPin::bump(char  **array,
              int   len,
              int   *size)
{
    if (*array)
        lefFree(*array);
    if (len > 0)
        *array = (char*) lefMalloc(len);
    else
        *array = 0;
    *size = len;
}


void
lefiPin::setName(const char *name)
{
    int len = strlen(name) + 1;
    clear();
    if (len > nameSize_) {
        lefFree(name_);
        name_ = (char*) lefMalloc(len);
        nameSize_ = len;
    }
    strcpy(name_, CASE(name));
}

void
lefiPin::addPort(lefiGeometries *g)
{
    if (numPorts_ == portsAllocated_) {
        int             i;
        lefiGeometries  **ng;
        if (portsAllocated_ == 0)
            portsAllocated_ = 2;
        else
            portsAllocated_ *= 2;
        ng = (lefiGeometries**) lefMalloc(sizeof(lefiGeometries*) * portsAllocated_);
        for (i = 0; i < numPorts_; i++)
            ng[i] = ports_[i];
        lefFree((char*) (ports_));
        ports_ = ng;
    }
    ports_[numPorts_++] = g;
}

void
lefiPin::addForeign(const char  *name,
                    int         hasPnt,
                    double      x,
                    double      y,
                    int         orient)
{
    int     i;
    int     *hfo;
    int     *hfp;
    int     *fo;
    double  *fx;
    double  *fy;
    char    **f;

    if (foreignAllocated_ == numForeigns_) {
        if (foreignAllocated_ == 0)
            foreignAllocated_ = 16; // since it involves char*, it will
            // costly in the number is too small
        else
            foreignAllocated_ *= 2;
        hfo = (int*) lefMalloc(sizeof(int) * foreignAllocated_);
        hfp = (int*) lefMalloc(sizeof(int) * foreignAllocated_);
        fo = (int*) lefMalloc(sizeof(int) * foreignAllocated_);
        fx = (double*) lefMalloc(sizeof(double) * foreignAllocated_);
        fy = (double*) lefMalloc(sizeof(double) * foreignAllocated_);
        f = (char**) lefMalloc(sizeof(char*) * foreignAllocated_);
        if (numForeigns_ != 0) {
            for (i = 0; i < numForeigns_; i++) {
                hfo[i] = hasForeignOrient_[i];
                hfp[i] = hasForeignPoint_[i];
                fo[i] = foreignOrient_[i];
                fx[i] = foreignX_[i];
                fy[i] = foreignY_[i];
                f[i] = foreign_[i];
            }
            lefFree((char*) (hasForeignOrient_));
            lefFree((char*) (hasForeignPoint_));
            lefFree((char*) (foreignOrient_));
            lefFree((char*) (foreignX_));
            lefFree((char*) (foreignY_));
            lefFree((char*) (foreign_));
        }
        hasForeignOrient_ = hfo;
        hasForeignPoint_ = hfp;
        foreignOrient_ = fo;
        foreignX_ = fx;
        foreignY_ = fy;
        foreign_ = f;
    }


    // orient=-1 means no orient was specified.
    if (orient != -1)
        hasForeignOrient_[numForeigns_] = 1;
    else
        hasForeignOrient_[numForeigns_] = -1;

    hasForeignPoint_[numForeigns_] = hasPnt;
    foreignOrient_[numForeigns_] = orient;
    foreignX_[numForeigns_] = x;
    foreignY_[numForeigns_] = y;
    foreign_[numForeigns_] = (char*) lefMalloc(strlen(name) + 1);
    strcpy(foreign_[numForeigns_], CASE(name));
    numForeigns_ += 1;
}

void
lefiPin::setLEQ(const char *name)
{
    int len = strlen(name) + 1;
    if (len > LEQSize_)
        bump(&(LEQ_), len, &(LEQSize_));
    strcpy(LEQ_, CASE(name));
    hasLEQ_ = 1;
}

void
lefiPin::setDirection(const char *name)
{
    strcpy(direction_, CASE(name));
    hasDirection_ = 1;
}

void
lefiPin::setUse(const char *name)
{
    strcpy(use_, CASE(name));
    hasUse_ = 1;
}

void
lefiPin::setShape(const char *name)
{
    strcpy(shape_, CASE(name));
    hasShape_ = 1;
}

void
lefiPin::setMustjoin(const char *name)
{
    int len = strlen(name) + 1;
    if (len > mustjoinSize_)
        bump(&(mustjoin_), len, &(mustjoinSize_));
    strcpy(mustjoin_, CASE(name));
    hasMustjoin_ = 1;
}

void
lefiPin::setOutMargin(double    high,
                      double    low)
{
    outMarginH_ = high;
    outMarginL_ = low;
    hasOutMargin_ = 1;
}

void
lefiPin::setOutResistance(double    high,
                          double    low)
{
    outResistanceH_ = high;
    outResistanceL_ = low;
    hasOutResistance_ = 1;
}

void
lefiPin::setInMargin(double high,
                     double low)
{
    inMarginH_ = high;
    inMarginL_ = low;
    hasInMargin_ = 1;
}

void
lefiPin::setPower(double power)
{
    power_ = power;
    hasPower_ = 1;
}

void
lefiPin::setLeakage(double current)
{
    leakage_ = current;
    hasLeakage_ = 1;
}

void
lefiPin::setMaxload(double capacitance)
{
    maxload_ = capacitance;
    hasMaxload_ = 1;
}

void
lefiPin::setMaxdelay(double dtime)
{
    maxdelay_ = dtime;
    hasMaxdelay_ = 1;
}

void
lefiPin::setCapacitance(double capacitance)
{
    capacitance_ = capacitance;
    hasCapacitance_ = 1;
}

void
lefiPin::setResistance(double resistance)
{
    resistance_ = resistance;
    hasResistance_ = 1;
}

void
lefiPin::setPulldownres(double resistance)
{
    pulldownres_ = resistance;
    hasPulldownres_ = 1;
}

void
lefiPin::setTieoffr(double resistance)
{
    tieoffr_ = resistance;
    hasTieoffr_ = 1;
}

void
lefiPin::setVHI(double voltage)
{
    VHI_ = voltage;
    hasVHI_ = 1;
}

void
lefiPin::setVLO(double voltage)
{
    VLO_ = voltage;
    hasVLO_ = 1;
}

void
lefiPin::setRiseVoltage(double voltage)
{
    riseVoltage_ = voltage;
    hasRiseVoltage_ = 1;
}

void
lefiPin::setFallVoltage(double voltage)
{
    fallVoltage_ = voltage;
    hasFallVoltage_ = 1;
}

void
lefiPin::setFallSlewLimit(double num)
{
    fallSlewLimit_ = num;
    hasFallSlewLimit_ = 1;
}

void
lefiPin::setRiseSlewLimit(double num)
{
    riseSlewLimit_ = num;
    hasRiseSlewLimit_ = 1;
}

void
lefiPin::setRiseThresh(double capacitance)
{
    riseThresh_ = capacitance;
    hasRiseThresh_ = 1;
}

void
lefiPin::setTaperRule(const char *name)
{
    int len = strlen(name) + 1;
    taperRule_ = (char*) lefMalloc(len);
    strcpy(taperRule_, name);
}

void
lefiPin::setNetExpr(const char *name)
{
    netEpxr_ = strdup(name);
}

void
lefiPin::setSupplySensitivity(const char *pinName)
{
    ssPinName_ = strdup(pinName);
}

void
lefiPin::setGroundSensitivity(const char *pinName)
{
    gsPinName_ = strdup(pinName);
}

void
lefiPin::setFallThresh(double capacitance)
{
    fallThresh_ = capacitance;
    hasFallThresh_ = 1;
}

void
lefiPin::setRiseSatcur(double current)
{
    riseSatcur_ = current;
    hasRiseSatcur_ = 1;
}

void
lefiPin::setFallSatcur(double current)
{
    fallSatcur_ = current;
    hasFallSatcur_ = 1;
}

void
lefiPin::setCurrentSource(const char *name)
{
    strcpy(currentSource_, CASE(name));
    hasCurrentSource_ = 1;
}

void
lefiPin::setTables(const char   *highName,
                   const char   *lowName)
{
    int len = strlen(highName) + 1;
    if (len > highTableSize_)
        bump(&(highTable_), len, &(highTableSize_));
    strcpy(highTable_, CASE(highName));
    len = strlen(lowName) + 1;
    if (len > lowTableSize_)
        bump(&(lowTable_), len, &(lowTableSize_));
    strcpy(lowTable_, CASE(lowName));
    hasTables_ = 1;
}

void
lefiPin::setProperty(const char *name,
                     const char *value,
                     const char type)
{
    int len;
    if (numProperties_ == propertiesAllocated_)
        bumpProps();
    len = strlen(name) + 1;
    propNames_[numProperties_] = (char*) lefMalloc(len);
    strcpy(propNames_[numProperties_], CASE(name));
    len = strlen(value) + 1;
    propValues_[numProperties_] = (char*) lefMalloc(len);
    strcpy(propValues_[numProperties_], CASE(value));
    propNums_[numProperties_] = 0;
    propTypes_[numProperties_] = type;
    numProperties_ += 1;
}

void
lefiPin::setNumProperty(const char  *name,
                        double      d,
                        const char  *value,
                        const char  type)
{
    int len;

    if (numProperties_ == propertiesAllocated_)
        bumpProps();
    len = strlen(name) + 1;
    propNames_[numProperties_] = (char*) lefMalloc(len);
    strcpy(propNames_[numProperties_], CASE(name));
    len = strlen(value) + 1;
    propValues_[numProperties_] = (char*) lefMalloc(len);
    strcpy(propValues_[numProperties_], CASE(value));
    propNums_[numProperties_] = d;
    propTypes_[numProperties_] = type;
    numProperties_ += 1;
}

void
lefiPin::bumpProps()
{
    int     lim = propertiesAllocated_;
    int     news;
    char    **newpn;
    char    **newpv;
    double  *newd;
    char    *newt;

    news = lim ? lim + lim : 2;

    newpn = (char**) lefMalloc(sizeof(char*) * news);
    newpv = (char**) lefMalloc(sizeof(char*) * news);
    newd = (double*) lefMalloc(sizeof(double) * news);
    newt = (char*) lefMalloc(sizeof(char) * news);

    lim = propertiesAllocated_ = news;

    if (lim > 2) {
        int i;
        for (i = 0; i < numProperties_; i++) {
            newpn[i] = propNames_[i];
            newpv[i] = propValues_[i];
            newd[i] = propNums_[i];
            newt[i] = propTypes_[i];
        }
    }
    if (propNames_)
        lefFree((char*) (propNames_));
    if (propValues_)
        lefFree((char*) (propValues_));
    if (propNums_)
        lefFree((char*) (propNums_));
    if (propTypes_)
        lefFree((char*) (propTypes_));
    propNames_ = newpn;
    propValues_ = newpv;
    propNums_ = newd;
    propTypes_ = newt;
}


int
lefiPin::hasForeign() const
{
    return (numForeigns_) ? 1 : 0;
}

int
lefiPin::hasForeignOrient(int index) const
{
    return (hasForeignOrient_[index] == -1) ? 0 : 1;
}

int
lefiPin::hasForeignPoint(int index) const
{
    return hasForeignPoint_[index];
}

int
lefiPin::hasLEQ() const
{
    return hasLEQ_;
}

int
lefiPin::hasDirection() const
{
    return hasDirection_;
}

int
lefiPin::hasUse() const
{
    return hasUse_;
}

int
lefiPin::hasShape() const
{
    return hasShape_;
}

int
lefiPin::hasMustjoin() const
{
    return hasMustjoin_;
}

int
lefiPin::hasOutMargin() const
{
    return hasOutMargin_;
}

int
lefiPin::hasOutResistance() const
{
    return hasOutResistance_;
}

int
lefiPin::hasInMargin() const
{
    return hasInMargin_;
}

int
lefiPin::hasPower() const
{
    return hasPower_;
}

int
lefiPin::hasLeakage() const
{
    return hasLeakage_;
}

int
lefiPin::hasMaxload() const
{
    return hasMaxload_;
}

int
lefiPin::hasMaxdelay() const
{
    return hasMaxdelay_;
}

int
lefiPin::hasCapacitance() const
{
    return hasCapacitance_;
}

int
lefiPin::hasResistance() const
{
    return hasResistance_;
}

int
lefiPin::hasPulldownres() const
{
    return hasPulldownres_;
}

int
lefiPin::hasTieoffr() const
{
    return hasTieoffr_;
}

int
lefiPin::hasVHI() const
{
    return hasVHI_;
}

int
lefiPin::hasVLO() const
{
    return hasVLO_;
}

int
lefiPin::hasFallSlewLimit() const
{
    return hasFallSlewLimit_;
}

int
lefiPin::hasRiseSlewLimit() const
{
    return hasRiseSlewLimit_;
}

int
lefiPin::hasRiseVoltage() const
{
    return hasRiseVoltage_;
}

int
lefiPin::hasFallVoltage() const
{
    return hasFallVoltage_;
}

int
lefiPin::hasRiseThresh() const
{
    return hasRiseThresh_;
}

int
lefiPin::hasFallThresh() const
{
    return hasFallThresh_;
}

int
lefiPin::hasRiseSatcur() const
{
    return hasRiseSatcur_;
}

int
lefiPin::hasFallSatcur() const
{
    return hasFallSatcur_;
}

int
lefiPin::hasCurrentSource() const
{
    return hasCurrentSource_;
}

int
lefiPin::hasTables() const
{
    return hasTables_;
}

int
lefiPin::hasAntennaSize() const
{
    return numAntennaSize_ ? 1 : 0;
}

int
lefiPin::hasAntennaMetalLength() const
{
    return numAntennaMetalLength_ ? 1 : 0;
}

int
lefiPin::hasAntennaMetalArea() const
{
    return numAntennaMetalArea_ ? 1 : 0;
}

int
lefiPin::hasAntennaPartialMetalArea() const
{
    return numAntennaPartialMetalArea_ ? 1 : 0;
}

int
lefiPin::hasAntennaPartialMetalSideArea() const
{
    return numAntennaPartialMetalSideArea_ ? 1 : 0;
}

int
lefiPin::hasAntennaPartialCutArea() const
{
    return numAntennaPartialCutArea_ ? 1 : 0;
}

int
lefiPin::hasAntennaDiffArea() const
{
    return numAntennaDiffArea_ ? 1 : 0;
}

int
lefiPin::hasAntennaModel() const
{
    return antennaModel_ ? 1 : 0;
}

int
lefiPin::hasTaperRule() const
{
    return taperRule_ ? 1 : 0;
}

int
lefiPin::hasNetExpr() const
{
    return netEpxr_ ? 1 : 0;
}

int
lefiPin::hasSupplySensitivity() const
{
    return ssPinName_ ? 1 : 0;
}

int
lefiPin::hasGroundSensitivity() const
{
    return gsPinName_ ? 1 : 0;
}

const char *
lefiPin::name() const
{
    return name_;
}

const char *
lefiPin::taperRule() const
{
    return taperRule_;
}

const char *
lefiPin::netExpr() const
{
    return netEpxr_;
}

const char *
lefiPin::supplySensitivity() const
{
    return ssPinName_;
}

const char *
lefiPin::groundSensitivity() const
{
    return gsPinName_;
}

int
lefiPin::numPorts() const
{
    return numPorts_;
}

lefiGeometries *
lefiPin::port(int index) const
{
    char msg[160];
    if (index < 0 || index > numPorts_) {
        sprintf(msg, "ERROR (LEFPARS-1350): The index number %d given for the macro PIN is invalid.\nValid index is from 0 to %d", index, numPorts_);
        lefiError(0, 1350, msg);
        return 0;
    }
    return ports_[index];
}

int
lefiPin::numForeigns() const
{
    return numForeigns_;
}

const char *
lefiPin::foreignName(int index) const
{
    return foreign_[index];
}

int
lefiPin::foreignOrient(int index) const
{
    return foreignOrient_[index];
}

const char *
lefiPin::foreignOrientStr(int index) const
{
    return (lefiOrientStr(foreignOrient_[index]));
}

double
lefiPin::foreignX(int index) const
{
    return foreignX_[index];
}

double
lefiPin::foreignY(int index) const
{
    return foreignY_[index];
}

const char *
lefiPin::LEQ() const
{
    return LEQ_;
}

const char *
lefiPin::direction() const
{
    return direction_;
}

const char *
lefiPin::use() const
{
    return use_;
}

const char *
lefiPin::shape() const
{
    return shape_;
}

const char *
lefiPin::mustjoin() const
{
    return mustjoin_;
}

double
lefiPin::outMarginHigh() const
{
    return outMarginH_;
}

double
lefiPin::outMarginLow() const
{
    return outMarginL_;
}

double
lefiPin::outResistanceHigh() const
{
    return outResistanceH_;
}

double
lefiPin::outResistanceLow() const
{
    return outResistanceL_;
}

double
lefiPin::inMarginHigh() const
{
    return inMarginH_;
}

double
lefiPin::inMarginLow() const
{
    return inMarginL_;
}

double
lefiPin::power() const
{
    return power_;
}

double
lefiPin::leakage() const
{
    return leakage_;
}

double
lefiPin::maxload() const
{
    return maxload_;
}

double
lefiPin::maxdelay() const
{
    return maxdelay_;
}

double
lefiPin::capacitance() const
{
    return capacitance_;
}

double
lefiPin::resistance() const
{
    return resistance_;
}

double
lefiPin::pulldownres() const
{
    return pulldownres_;
}

double
lefiPin::tieoffr() const
{
    return tieoffr_;
}

double
lefiPin::VHI() const
{
    return VHI_;
}

double
lefiPin::VLO() const
{
    return VLO_;
}

double
lefiPin::fallSlewLimit() const
{
    return fallSlewLimit_;
}

double
lefiPin::riseSlewLimit() const
{
    return riseSlewLimit_;
}

double
lefiPin::riseVoltage() const
{
    return riseVoltage_;
}

double
lefiPin::fallVoltage() const
{
    return fallVoltage_;
}

double
lefiPin::riseThresh() const
{
    return riseThresh_;
}

double
lefiPin::fallThresh() const
{
    return fallThresh_;
}

double
lefiPin::riseSatcur() const
{
    return riseSatcur_;
}

double
lefiPin::fallSatcur() const
{
    return fallSatcur_;
}

const char *
lefiPin::currentSource() const
{
    return currentSource_;
}

const char *
lefiPin::tableHighName() const
{
    return highTable_;
}

const char *
lefiPin::tableLowName() const
{
    return lowTable_;
}

const char *
lefiPin::antennaSizeLayer(int i) const
{
    return antennaSizeLayer_[i];
}

const char *
lefiPin::antennaMetalAreaLayer(int i) const
{
    return antennaMetalAreaLayer_[i];
}

const char *
lefiPin::antennaMetalLengthLayer(int i) const
{
    return antennaMetalLengthLayer_[i];
}

const char *
lefiPin::antennaPartialMetalAreaLayer(int i) const
{
    return antennaPartialMetalAreaLayer_[i];
}

const char *
lefiPin::antennaPartialMetalSideAreaLayer(int i) const
{
    return antennaPartialMetalSideAreaLayer_[i];
}

const char *
lefiPin::antennaPartialCutAreaLayer(int i) const
{
    return antennaPartialCutAreaLayer_[i];
}

const char *
lefiPin::antennaDiffAreaLayer(int i) const
{
    return antennaDiffAreaLayer_[i];
}

int
lefiPin::numAntennaSize() const
{
    return numAntennaSize_;
}

int
lefiPin::numAntennaMetalArea() const
{
    return numAntennaMetalArea_;
}

int
lefiPin::numAntennaMetalLength() const
{
    return numAntennaMetalLength_;
}

int
lefiPin::numAntennaPartialMetalArea() const
{
    return numAntennaPartialMetalArea_;
}

int
lefiPin::numAntennaPartialMetalSideArea() const
{
    return numAntennaPartialMetalSideArea_;
}

int
lefiPin::numAntennaPartialCutArea() const
{
    return numAntennaPartialCutArea_;
}

int
lefiPin::numAntennaDiffArea() const
{
    return numAntennaDiffArea_;
}

double
lefiPin::antennaSize(int i) const
{
    return antennaSize_[i];
}

double
lefiPin::antennaMetalArea(int i) const
{
    return antennaMetalArea_[i];
}

double
lefiPin::antennaMetalLength(int i) const
{
    return antennaMetalLength_[i];
}

double
lefiPin::antennaPartialMetalArea(int i) const
{
    return antennaPartialMetalArea_[i];
}

double
lefiPin::antennaPartialMetalSideArea(int i) const
{
    return antennaPartialMetalSideArea_[i];
}

double
lefiPin::antennaPartialCutArea(int i) const
{
    return antennaPartialCutArea_[i];
}

double
lefiPin::antennaDiffArea(int i) const
{
    return antennaDiffArea_[i];
}

void
lefiPin::addAntennaMetalLength(double       val,
                               const char   *layer)
{
    int len;
    if (numAntennaMetalLength_ == antennaMetalLengthAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaMetalLength_;
        double  *nd;
        char    **nl;

        if (antennaMetalLengthAllocated_ == 0)
            max = antennaMetalLengthAllocated_ = 2;
        else
            max = antennaMetalLengthAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaMetalLengthLayer_[i];
            nd[i] = antennaMetalLength_[i];
        }
        lefFree((char*) (antennaMetalLengthLayer_));
        lefFree((char*) (antennaMetalLength_));
        antennaMetalLengthLayer_ = nl;
        antennaMetalLength_ = nd;
    }
    antennaMetalLength_[numAntennaMetalLength_] = val;
    if (layer) {    // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaMetalLengthLayer_[numAntennaMetalLength_] =
            (char*) lefMalloc(len);
        strcpy(antennaMetalLengthLayer_[numAntennaMetalLength_],
               layer);
    } else
        antennaMetalLengthLayer_[numAntennaMetalLength_] = NULL;
    numAntennaMetalLength_ += 1;
}

// 5.5
void
lefiPin::addAntennaModel(int oxide)
{
    // For version 5.5 only OXIDE1, OXIDE2, OXIDE3, & OXIDE4
    // are defined within a macro pin
    lefiPinAntennaModel *amo;
    int                 i;

    if (numAntennaModel_ == 0) {   // does not have antennaModel
        antennaModel_ = (lefiPinAntennaModel**)
            lefMalloc(sizeof(lefiPinAntennaModel*) * 4);
        antennaModelAllocated_ = 4;
        for (i = 0; i < 4; i++) {
            antennaModel_[i] = (lefiPinAntennaModel*)
                lefMalloc(sizeof(lefiPinAntennaModel));
            antennaModel_[i]->setAntennaModel(0);
            // just initialize it first
        }
        antennaModelAllocated_ = 4;
        amo = antennaModel_[0];
        curAntennaModelIndex_ = 0;
    } 

    // First can go any oxide, so fill pref oxides models.
    for (int idx = 0; idx < oxide - 1; idx++) {
        amo = antennaModel_[idx];
        if (!amo->antennaOxide()) {
            amo->Init();
            amo->setAntennaModel(idx + 1);   
        }
    }

        amo = antennaModel_[oxide - 1];
        curAntennaModelIndex_ = oxide - 1;
        // Oxide has not defined yet
    if (amo->antennaOxide()) {
            amo->clear();
        }

    if (oxide > numAntennaModel_) {
        numAntennaModel_ = oxide;
    }

    amo->Init();
    amo->setAntennaModel(oxide);
    return;
}

// 5.5
int
lefiPin::numAntennaModel() const
{
    return numAntennaModel_;
}

// 5.5
lefiPinAntennaModel *
lefiPin::antennaModel(int index) const
{
    int                 j = index;
    lefiPinAntennaModel *amo;

    if (index == 0) {   // reset all the return flags to 0, beginning of the loop
        int i;
        for (i = 0; i < 4; i++)
            antennaModel_[i]->setAntennaReturnFlag(0);
    }
    while (j < 4) {
        amo = antennaModel_[j];
        if (!(amo->antennaOxide()) &&
            (amo->hasReturn() == 0))
            j++;
        else
            break;
        if (j == 4) {  // something very wrong, normally this can't happen
            lefiError(0, 1351, "ERROR (LEFPARS-1351): There is an unexpected lef parser bug which cause it unable to retrieve ANTENNAMODEL data with the given index.");
            return 0;
        }
    }
    // If it arrived here, it is saved, mark the antennaModel has returned
    antennaModel_[j]->setAntennaReturnFlag(1);
    return antennaModel_[j];
}

void
lefiPin::addAntennaSize(double      val,
                        const char  *layer)
{
    int len;
    if (numAntennaSize_ == antennaSizeAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaSize_;
        double  *nd;
        char    **nl;

        if (antennaSizeAllocated_ == 0)
            max = antennaSizeAllocated_ = 2;
        else
            max = antennaSizeAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaSizeLayer_[i];
            nd[i] = antennaSize_[i];
        }
        lefFree((char*) (antennaSizeLayer_));
        lefFree((char*) (antennaSize_));
        antennaSizeLayer_ = nl;
        antennaSize_ = nd;
    }
    antennaSize_[numAntennaSize_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaSizeLayer_[numAntennaSize_] =
            (char*) lefMalloc(len);
        strcpy(antennaSizeLayer_[numAntennaSize_],
               layer);
    } else
        antennaSizeLayer_[numAntennaSize_] = NULL;
    numAntennaSize_ += 1;
}

void
lefiPin::addAntennaMetalArea(double     val,
                             const char *layer)
{
    int len;
    if (numAntennaMetalArea_ == antennaMetalAreaAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaMetalArea_;
        double  *nd;
        char    **nl;

        if (antennaMetalAreaAllocated_ == 0)
            max = antennaMetalAreaAllocated_ = 2;
        else
            max = antennaMetalAreaAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaMetalAreaLayer_[i];
            nd[i] = antennaMetalArea_[i];
        }
        lefFree((char*) (antennaMetalAreaLayer_));
        lefFree((char*) (antennaMetalArea_));
        antennaMetalAreaLayer_ = nl;
        antennaMetalArea_ = nd;
    }
    antennaMetalArea_[numAntennaMetalArea_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaMetalAreaLayer_[numAntennaMetalArea_] =
            (char*) lefMalloc(len);
        strcpy(antennaMetalAreaLayer_[numAntennaMetalArea_],
               layer);
    } else
        antennaMetalAreaLayer_[numAntennaMetalArea_] = NULL;
    numAntennaMetalArea_ += 1;
}

void
lefiPin::addAntennaPartialMetalArea(double      val,
                                    const char  *layer)
{
    int len;
    if (numAntennaPartialMetalArea_ == antennaPartialMetalAreaAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaPartialMetalArea_;
        double  *nd;
        char    **nl;

        if (antennaPartialMetalAreaAllocated_ == 0)
            max = antennaPartialMetalAreaAllocated_ = 2;
        else
            max = antennaPartialMetalAreaAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaPartialMetalAreaLayer_[i];
            nd[i] = antennaPartialMetalArea_[i];
        }
        lefFree((char*) (antennaPartialMetalAreaLayer_));
        lefFree((char*) (antennaPartialMetalArea_));
        antennaPartialMetalAreaLayer_ = nl;
        antennaPartialMetalArea_ = nd;
    }
    antennaPartialMetalArea_[numAntennaPartialMetalArea_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaPartialMetalAreaLayer_[numAntennaPartialMetalArea_] =
            (char*) lefMalloc(len);
        strcpy(antennaPartialMetalAreaLayer_[numAntennaPartialMetalArea_],
               layer);
    } else
        antennaPartialMetalAreaLayer_[numAntennaPartialMetalArea_] = NULL;
    numAntennaPartialMetalArea_ += 1;
}

void
lefiPin::addAntennaPartialMetalSideArea(double      val,
                                        const char  *layer)
{
    int len;
    if (numAntennaPartialMetalSideArea_ == antennaPartialMetalSideAreaAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaPartialMetalSideArea_;
        double  *nd;
        char    **nl;

        if (antennaPartialMetalSideAreaAllocated_ == 0)
            max = antennaPartialMetalSideAreaAllocated_ = 2;
        else
            max = antennaPartialMetalSideAreaAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaPartialMetalSideAreaLayer_[i];
            nd[i] = antennaPartialMetalSideArea_[i];
        }
        lefFree((char*) (antennaPartialMetalSideAreaLayer_));
        lefFree((char*) (antennaPartialMetalSideArea_));
        antennaPartialMetalSideAreaLayer_ = nl;
        antennaPartialMetalSideArea_ = nd;
    }
    antennaPartialMetalSideArea_[numAntennaPartialMetalSideArea_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaPartialMetalSideAreaLayer_[numAntennaPartialMetalSideArea_] =
            (char*) lefMalloc(len);
        strcpy(antennaPartialMetalSideAreaLayer_[numAntennaPartialMetalSideArea_],
               layer);
    } else
        antennaPartialMetalSideAreaLayer_[numAntennaPartialMetalSideArea_] = NULL;
    numAntennaPartialMetalSideArea_ += 1;
}

void
lefiPin::addAntennaPartialCutArea(double        val,
                                  const char    *layer)
{
    int len;
    if (numAntennaPartialCutArea_ == antennaPartialCutAreaAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaPartialCutArea_;
        double  *nd;
        char    **nl;

        if (antennaPartialCutAreaAllocated_ == 0)
            max = antennaPartialCutAreaAllocated_ = 2;
        else
            max = antennaPartialCutAreaAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaPartialCutAreaLayer_[i];
            nd[i] = antennaPartialCutArea_[i];
        }
        lefFree((char*) (antennaPartialCutAreaLayer_));
        lefFree((char*) (antennaPartialCutArea_));
        antennaPartialCutAreaLayer_ = nl;
        antennaPartialCutArea_ = nd;
    }
    antennaPartialCutArea_[numAntennaPartialCutArea_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaPartialCutAreaLayer_[numAntennaPartialCutArea_] =
            (char*) lefMalloc(len);
        strcpy(antennaPartialCutAreaLayer_[numAntennaPartialCutArea_],
               layer);
    } else
        antennaPartialCutAreaLayer_[numAntennaPartialCutArea_] = NULL;
    numAntennaPartialCutArea_ += 1;
}

void
lefiPin::addAntennaDiffArea(double      val,
                            const char  *layer)
{
    int len;
    if (numAntennaDiffArea_ == antennaDiffAreaAllocated_) {
        int     i;
        int     max;
        int     lim = numAntennaDiffArea_;
        double  *nd;
        char    **nl;

        if (antennaDiffAreaAllocated_ == 0)
            max = antennaDiffAreaAllocated_ = 2;
        else
            max = antennaDiffAreaAllocated_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * max);
        nl = (char**) lefMalloc(sizeof(double) * max);
        for (i = 0; i < lim; i++) {
            nl[i] = antennaDiffAreaLayer_[i];
            nd[i] = antennaDiffArea_[i];
        }
        lefFree((char*) (antennaDiffAreaLayer_));
        lefFree((char*) (antennaDiffArea_));
        antennaDiffAreaLayer_ = nl;
        antennaDiffArea_ = nd;
    }
    antennaDiffArea_[numAntennaDiffArea_] = val;
    if (layer) {  // layer can be null, since it is optional
        len = strlen(layer) + 1;
        antennaDiffAreaLayer_[numAntennaDiffArea_] =
            (char*) lefMalloc(len);
        strcpy(antennaDiffAreaLayer_[numAntennaDiffArea_],
               layer);
    } else
        antennaDiffAreaLayer_[numAntennaDiffArea_] = NULL;
    numAntennaDiffArea_ += 1;
}

void
lefiPin::addAntennaGateArea(double      val,
                            const char  *layer)
{
    if (numAntennaModel_ == 0)    // haven't created any antennaModel yet
        addAntennaModel(1);
    antennaModel_[curAntennaModelIndex_]->addAntennaGateArea(val, layer);
}

void
lefiPin::addAntennaMaxAreaCar(double        val,
                              const char    *layer)
{
    if (numAntennaModel_ == 0)    // haven't created any antennaModel yet
        addAntennaModel(1);
    antennaModel_[curAntennaModelIndex_]->addAntennaMaxAreaCar(val,
                                                               layer);
}

void
lefiPin::addAntennaMaxSideAreaCar(double        val,
                                  const char    *layer)
{
    if (numAntennaModel_ == 0)    // haven't created any antennaModel yet
        addAntennaModel(1);
    antennaModel_[curAntennaModelIndex_]->addAntennaMaxSideAreaCar(val,
                                                                   layer);
}

void
lefiPin::addAntennaMaxCutCar(double     val,
                             const char *layer)
{
    if (numAntennaModel_ == 0)    // haven't created any antennaModel yet
        addAntennaModel(1);
    antennaModel_[curAntennaModelIndex_]->addAntennaMaxCutCar(val,
                                                              layer);
}

int
lefiPin::numProperties() const
{
    return numProperties_;
}

const char *
lefiPin::propName(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propNames_[index];
}

const char *
lefiPin::propValue(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propValues_[index];
}

double
lefiPin::propNum(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propNums_[index];
}

char
lefiPin::propType(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propTypes_[index];
}

int
lefiPin::propIsNumber(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propNums_[index] ? 1 : 0;
}

int
lefiPin::propIsString(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propNums_[index] ? 0 : 1;
}

void
lefiPin::print(FILE *f) const
{
    int             i;
    lefiGeometries  *g;

    fprintf(f, "  Pin %s\n", name());

    for (i = 0; i < numPorts(); i++) {
        fprintf(f, "    Port %d ", i);
        g = port(i);
        g->print(f);
    }

}

// *****************************************************************************
// lefiDensity
// *****************************************************************************

lefiDensity::lefiDensity()
: numLayers_(0),
  layersAllocated_(0),
  layerName_(NULL),
  numRects_(NULL),
  rectsAllocated_(NULL),
  rects_(NULL),
  densityValue_(NULL)
{
    Init();
}

void
lefiDensity::Init()
{
    numLayers_ = 0;
    layersAllocated_ = 0;
}

LEF_COPY_CONSTRUCTOR_C( lefiDensity) 
: numLayers_(0),
  layersAllocated_(0),
  layerName_(NULL),
  numRects_(NULL),
  rectsAllocated_(NULL),
  rects_(NULL),
  densityValue_(NULL) 
{
    LEF_COPY_FUNC( numLayers_ );
    LEF_COPY_FUNC( layersAllocated_ );

    //!!
    LEF_MALLOC_FUNC_FOR_2D_STR( layerName_, layersAllocated_ );
    LEF_MALLOC_FUNC( numRects_, int, sizeof(int) * layersAllocated_ );
    LEF_MALLOC_FUNC( rectsAllocated_, int, sizeof(int) * layersAllocated_ );

    // !!
    LEF_MALLOC_FUNC_FOR_2D( rects_, lefiGeomRect, layersAllocated_, rectsAllocated_[i] );

    //!!
    LEF_MALLOC_FUNC_FOR_2D( densityValue_, double, layersAllocated_, rectsAllocated_[i] );
}


lefiDensity::~lefiDensity()
{
    Destroy();
}

void
lefiDensity::Destroy()
{
    clear();
}

void
lefiDensity::clear()
{
    for (int i = 0; i < numLayers_; i++) {
        lefFree(layerName_[i]);
        lefFree((char*) rects_[i]);
        lefFree((char*) densityValue_[i]);
        numRects_[i] = 0;
        rectsAllocated_[i] = 0;
    }
    lefFree(layerName_);
    lefFree((char*) (rects_));
    lefFree((char*) (densityValue_));
    lefFree((char*) (numRects_));
    lefFree((char*) (rectsAllocated_));
    layerName_ = 0;
    numLayers_ = 0;
    layersAllocated_ = 0;
    numRects_ = 0;
    rects_ = 0;
    densityValue_ = 0;
    rectsAllocated_ = 0;
}

void
lefiDensity::addLayer(const char *name)
{
    if (numLayers_ == layersAllocated_) {
        int             i;
        char            **ln;                   // layerName
        int             *nr;                   // number of rect within the layer
        int             *ra;                   // number of rect allocated within the layer
        lefiGeomRect    **rs;    // rect value
        double          **dv;           // density value

        layersAllocated_ = (layersAllocated_ == 0) ?
            2 : layersAllocated_ * 2;
        ln = (char**) lefMalloc(sizeof(char*) * layersAllocated_);
        nr = (int*) lefMalloc(sizeof(int) * layersAllocated_);
        ra = (int*) lefMalloc(sizeof(int) * layersAllocated_);
        rs = (lefiGeomRect**) lefMalloc(sizeof(lefiGeomRect*)
                                        * layersAllocated_);
        dv = (double**) lefMalloc(sizeof(double*) * layersAllocated_);
        for (i = 0; i < numLayers_; i++) {
            ln[i] = layerName_[i];
            nr[i] = numRects_[i];
            ra[i] = rectsAllocated_[i];
            rs[i] = rects_[i];
            dv[i] = densityValue_[i];
        }

        lefFree((char*) (layerName_));
        lefFree((char*) (rects_));
        lefFree((char*) (densityValue_));
        lefFree((char*) (numRects_));
        lefFree((char*) (rectsAllocated_));

        layerName_ = ln;
        numRects_ = nr;
        rectsAllocated_ = ra;
        rects_ = rs;
        densityValue_ = dv;
    }
    layerName_[numLayers_] = strdup(name);
    numRects_[numLayers_] = 0;
    rectsAllocated_[numLayers_] = 0;
    rects_[numLayers_] = 0;
    densityValue_[numLayers_] = 0;
    numLayers_ += 1;
}

void
lefiDensity::addRect(double x1,
                     double y1,
                     double x2,
                     double y2,
                     double value)
{
    if (numRects_[numLayers_ - 1] ==
        rectsAllocated_[numLayers_ - 1]) {

        lefiGeomRect    *rs, *ors;
        double          *dv, *odv;
        int             i;

        rectsAllocated_[numLayers_ - 1] =
            (rectsAllocated_[numLayers_ - 1] == 0) ?
            2 : rectsAllocated_[numLayers_ - 1] * 2;

        rs = (lefiGeomRect*) lefMalloc(sizeof(lefiGeomRect) *
                                       rectsAllocated_[numLayers_ - 1]);
        dv = (double*) lefMalloc(sizeof(double) *
                                 rectsAllocated_[numLayers_ - 1]);

        if (numRects_[numLayers_ - 1] > 0) {
            ors = rects_[numLayers_ - 1];
            odv = densityValue_[numLayers_ - 1];
            for (i = 0; i < numRects_[numLayers_ - 1]; i++) {
                rs[i] = ors[i];    // assign data from old rect & density value to
                dv[i] = odv[i];    // new, larger array
            }

            lefFree((char*) rects_[numLayers_ - 1]);
            lefFree((char*) densityValue_[numLayers_ - 1]);
        }

        rects_[numLayers_ - 1] = rs;
        densityValue_[numLayers_ - 1] = dv;
    }

    lefiGeomRect p;

    p.xl = x1;
    p.yl = y1;
    p.xh = x2;
    p.yh = y2;
    p.colorMask = 0;

    rects_[numLayers_ - 1][numRects_[numLayers_ - 1]] = p;
    densityValue_[numLayers_ - 1][numRects_[numLayers_ - 1]] = value;
    numRects_[numLayers_ - 1] += 1;
}

int
lefiDensity::numLayer() const
{
    return numLayers_;
}

char *
lefiDensity::layerName(int index) const
{
    return layerName_[index];
}

int
lefiDensity::numRects(int index) const
{
    return numRects_[index];
}

lefiGeomRect
lefiDensity::getRect(int    index,
                     int    rectIndex) const
{
    lefiGeomRect *rs;

    rs = rects_[index];
    return rs[rectIndex];
}

double
lefiDensity::densityValue(int   index,
                          int   rectIndex) const
{
    double *dv;

    dv = densityValue_[index];
    return dv[rectIndex];
}

void
lefiDensity::print(FILE *f) const
{
    int i, j;

    // 11/8/2004 - Added feed back from users
    fprintf(f, "  DENSITY\n");
    for (i = 0; i < numLayers_; i++) {
        fprintf(f, "    LAYER %s\n", layerName_[i]);
        for (j = 0; j < numRects_[i]; j++) {
            fprintf(f, "      RECT %g %g %g %g ", rects_[i][j].xl,
                    rects_[i][j].yl, rects_[i][j].xh,
                    rects_[i][j].yh);
            fprintf(f, "%g\n", densityValue_[i][j]);
        }
    }
}

// *****************************************************************************
// lefiMacro
// *****************************************************************************

lefiMacro::lefiMacro()
: nameSize_(0),
  name_(NULL),
  generatorSize_(0),
  generator_(NULL),
  hasClass_(0),
  hasGenerator_(0),
  hasGenerate_(0),
  hasPower_(0),
  hasOrigin_(0),
  hasSource_(0),
  hasEEQ_(0),
  hasLEQ_(0),
  hasSymmetry_(0),
  hasSiteName_(0),
  hasSize_(0),
  hasClockType_(0),
  isBuffer_(0),
  isInverter_(0),
  EEQ_(NULL),
  EEQSize_(0),
  LEQ_(NULL),
  LEQSize_(0),
  gen1_(NULL),
  gen1Size_(0),
  gen2_(NULL),
  gen2Size_(0),
  power_(0.0),
  originX_(0.0),
  originY_(0.0),
  sizeX_(0.0),
  sizeY_(0.0),
  numSites_(0),
  sitesAllocated_(0),
  pattern_(NULL),
  numForeigns_(0),
  foreignAllocated_(0),
  hasForeignOrigin_(NULL),
  hasForeignPoint_(NULL),
  foreignOrient_(NULL),
  foreignX_(NULL),
  foreignY_(NULL),
  foreign_(NULL),
  siteNameSize_(0),
  siteName_(NULL),
  clockType_(NULL),
  clockTypeSize_(0),
  numProperties_(0),
  propertiesAllocated_(0),
  propNames_(NULL),
  propValues_(NULL),
  propNums_(NULL),
  propTypes_(NULL),
  isFixedMask_(0)
{
    Init();
}

void
lefiMacro::Init()
{
    name_ = 0;
    generator_ = 0;
    EEQ_ = 0;
    LEQ_ = 0;
    gen1_ = 0;
    gen2_ = 0;
    foreign_ = 0;
    siteName_ = 0;
    clockType_ = 0;
    propNames_ = 0;
    propValues_ = 0;
    propTypes_ = 0;

    bump(&(name_), 16, &(nameSize_));
    bump(&(generator_), 16, &(generatorSize_));
    bump(&(EEQ_), 16, &(EEQSize_));
    bump(&(LEQ_), 16, &(LEQSize_));
    bump(&(gen1_), 16, &(gen1Size_));
    bump(&(gen2_), 16, &(gen2Size_));
    bump(&(siteName_), 16, &(siteNameSize_));
    bump(&(clockType_), 16, &(clockTypeSize_));

    propertiesAllocated_ = 2;
    numProperties_ = 0;
    propNames_ = (char**) lefMalloc(sizeof(char*) * 2);
    propValues_ = (char**) lefMalloc(sizeof(char*) * 2);
    propNums_ = (double*) lefMalloc(sizeof(double) * 2);
    propTypes_ = (char*) lefMalloc(sizeof(char) * 2);

    numSites_ = 0;
    sitesAllocated_ = 0;
    pattern_ = 0;
    numForeigns_ = 0;
    foreignAllocated_ = 0;
    isFixedMask_ = 0;

    clear();
}

LEF_COPY_CONSTRUCTOR_C( lefiMacro ) 
: nameSize_(0),
  name_(NULL),
  generatorSize_(0),
  generator_(NULL),
  hasClass_(0),
  hasGenerator_(0),
  hasGenerate_(0),
  hasPower_(0),
  hasOrigin_(0),
  hasSource_(0),
  hasEEQ_(0),
  hasLEQ_(0),
  hasSymmetry_(0),
  hasSiteName_(0),
  hasSize_(0),
  hasClockType_(0),
  isBuffer_(0),
  isInverter_(0),
  EEQ_(NULL),
  EEQSize_(0),
  LEQ_(NULL),
  LEQSize_(0),
  gen1_(NULL),
  gen1Size_(0),
  gen2_(NULL),
  gen2Size_(0),
  power_(0.0),
  originX_(0.0),
  originY_(0.0),
  sizeX_(0.0),
  sizeY_(0.0),
  numSites_(0),
  sitesAllocated_(0),
  pattern_(NULL),
  numForeigns_(0),
  foreignAllocated_(0),
  hasForeignOrigin_(NULL),
  hasForeignPoint_(NULL),
  foreignOrient_(NULL),
  foreignX_(NULL),
  foreignY_(NULL),
  foreign_(NULL),
  siteNameSize_(0),
  siteName_(NULL),
  clockType_(NULL),
  clockTypeSize_(0),
  numProperties_(0),
  propertiesAllocated_(0),
  propNames_(NULL),
  propValues_(NULL),
  propNums_(NULL),
  propTypes_(NULL),
  isFixedMask_(0)
{
    this->Init();
    LEF_COPY_FUNC( nameSize_ );
    LEF_MALLOC_FUNC( name_, char, sizeof(char) * nameSize_ );

    memcpy(macroClass_, prev.macroClass_, sizeof(char)*32);
    memcpy(source_, prev.source_, sizeof(char)*12);
    LEF_COPY_FUNC( generatorSize_ );
    LEF_MALLOC_FUNC( generator_, char, sizeof(char) * generatorSize_ );
    LEF_COPY_FUNC( hasClass_ );
    LEF_COPY_FUNC( hasGenerator_ );
    LEF_COPY_FUNC( hasGenerate_ );
    LEF_COPY_FUNC( hasPower_ );
    LEF_COPY_FUNC( hasOrigin_ );
    LEF_COPY_FUNC( hasSource_ );
    LEF_COPY_FUNC( hasEEQ_ );
    LEF_COPY_FUNC( hasLEQ_ );
    LEF_COPY_FUNC( hasSymmetry_ );
    LEF_COPY_FUNC( hasSiteName_ );
    LEF_COPY_FUNC( hasSize_ );
    LEF_COPY_FUNC( hasClockType_ );
    LEF_COPY_FUNC( isBuffer_ );
    LEF_COPY_FUNC( isInverter_ );
    LEF_COPY_FUNC( EEQSize_ );
    LEF_MALLOC_FUNC( EEQ_, char, sizeof(char) * EEQSize_ );
    LEF_COPY_FUNC( LEQSize_ );
    LEF_MALLOC_FUNC( LEQ_, char, sizeof(char) * LEQSize_ );
    LEF_COPY_FUNC( gen1Size_ );
    LEF_MALLOC_FUNC( gen1_, char, sizeof(char) * gen1Size_ );
    LEF_COPY_FUNC( gen2Size_ );
    LEF_MALLOC_FUNC( gen2_, char, sizeof(char) * gen2Size_ );
    LEF_COPY_FUNC( power_ );
    LEF_COPY_FUNC( originX_ );
    LEF_COPY_FUNC( originY_ );
    LEF_COPY_FUNC( sizeX_ );
    LEF_COPY_FUNC( sizeY_ );
    LEF_COPY_FUNC( numSites_ );
    LEF_COPY_FUNC( sitesAllocated_ );

    LEF_MALLOC_FUNC_FOR_2D( pattern_, lefiSitePattern, numSites_ , 1 );

    LEF_COPY_FUNC( numForeigns_ );
    LEF_COPY_FUNC( foreignAllocated_ );
    LEF_MALLOC_FUNC( hasForeignOrigin_, int, sizeof(int) * foreignAllocated_ );
    LEF_MALLOC_FUNC( hasForeignPoint_, int, sizeof(int) * foreignAllocated_ );
    LEF_MALLOC_FUNC( foreignOrient_, int, sizeof(int) * foreignAllocated_ );
    LEF_MALLOC_FUNC( foreignX_, double, sizeof(double) * foreignAllocated_ );
    LEF_MALLOC_FUNC( foreignY_, double, sizeof(double) * foreignAllocated_ );
    LEF_MALLOC_FUNC_FOR_2D_STR( foreign_, numForeigns_);

    LEF_COPY_FUNC( siteNameSize_ );
    LEF_MALLOC_FUNC( siteName_, char, sizeof(char) * siteNameSize_ );
    LEF_COPY_FUNC( clockTypeSize_ );
    LEF_MALLOC_FUNC( clockType_, char, sizeof(char) * clockTypeSize_ );
    LEF_COPY_FUNC( numProperties_ );
    LEF_COPY_FUNC( propertiesAllocated_ );

    LEF_MALLOC_FUNC_FOR_2D_STR( propNames_, numProperties_);
    LEF_MALLOC_FUNC_FOR_2D_STR( propValues_, numProperties_);
    LEF_MALLOC_FUNC( propNums_, double, sizeof(double) * propertiesAllocated_);
    LEF_MALLOC_FUNC( propTypes_, char, sizeof(char) * propertiesAllocated_);
    LEF_COPY_FUNC( isFixedMask_ );


}
void
lefiMacro::Destroy()
{
    clear();
    lefFree(name_);
    lefFree(generator_);
    lefFree(EEQ_);
    lefFree(LEQ_);
    lefFree(gen1_);
    lefFree(gen2_);
    lefFree(siteName_);
    lefFree(clockType_);
    lefFree((char*) (propNames_));
    lefFree((char*) (propValues_));
    lefFree((char*) (propNums_));
    lefFree((char*) (propTypes_));
    if (foreignAllocated_) {
        lefFree((char*) (hasForeignOrigin_));
        lefFree((char*) (hasForeignPoint_));
        lefFree((char*) (foreignOrient_));
        lefFree((char*) (foreignX_));
        lefFree((char*) (foreignY_));
        lefFree((char*) (foreign_));
        foreignAllocated_ = 0;
    }
}

lefiMacro::~lefiMacro()
{
    Destroy();
}

void
lefiMacro::clear()
{
    int i;

    hasClass_ = 0;
    hasGenerator_ = 0;
    hasGenerate_ = 0;
    hasPower_ = 0;
    hasOrigin_ = 0;
    hasSource_ = 0;
    hasEEQ_ = 0;
    hasLEQ_ = 0;
    hasSymmetry_ = 0;
    hasSiteName_ = 0;
    hasClockType_ = 0;
    hasSize_ = 0;
    isInverter_ = 0;
    isBuffer_ = 0;

    for (i = 0; i < numForeigns_; i++) {
        hasForeignOrigin_[i] = 0;
        hasForeignPoint_[i] = 0;
        foreignOrient_[i] = -1;
        lefFree((char*) (foreign_[i]));
    }
    numForeigns_ = 0;

    if (pattern_) {
        for (i = 0; i < numSites_; i++) {
//            pattern_[i]->Destroy();
            lefFree((char*) (pattern_[i]));
        }
        numSites_ = 0;
        sitesAllocated_ = 0;
        lefFree((char*) (pattern_));
        pattern_ = 0;
    }

    for (i = 0; i < numProperties_; i++) {
        lefFree(propNames_[i]);
        lefFree(propValues_[i]);
    }

    numProperties_ = 0;
    isFixedMask_ = 0;
}

void
lefiMacro::bump(char    **array,
                int     len,
                int     *size)
{
    if (*array)
        lefFree(*array);
    if (len)
        *array = (char*) lefMalloc(len);
    else
        *array = 0;
    *size = len;
}

void
lefiMacro::setName(const char *name)
{
    int len = strlen(name) + 1;
    if (len > nameSize_)
        bump(&(name_), len, &(nameSize_));
    strcpy(name_, CASE(name));
}

void
lefiMacro::setGenerate(const char   *name,
                       const char   *n2)
{
    int len = strlen(name) + 1;
    if (len > gen1Size_)
        bump(&(gen1_), len, &(gen1Size_));
    strcpy(gen1_, CASE(name));
    len = strlen(n2) + 1;
    if (len > gen2Size_)
        bump(&(gen2_), len, &(gen2Size_));
    strcpy(gen2_, n2);
}

void
lefiMacro::setGenerator(const char *name)
{
    int len = strlen(name) + 1;
    if (len > generatorSize_)
        bump(&(generator_), len, &(generatorSize_));
    strcpy(generator_, CASE(name));
    hasGenerator_ = 1;
}

void
lefiMacro::setInverter()
{
    isInverter_ = 1;
}

void
lefiMacro::setBuffer()
{
    isBuffer_ = 1;
}

void
lefiMacro::setSource(const char *name)
{
    strcpy(source_, CASE(name));
    hasSource_ = 1;
}

void
lefiMacro::setClass(const char *name)
{
    strcpy(macroClass_, CASE(name));
    hasClass_ = 1;
}

void
lefiMacro::setOrigin(double x,
                     double y)
{
    originX_ = x;
    originY_ = y;
    hasOrigin_ = 1;
}

void
lefiMacro::setPower(double p)
{
    power_ = p;
    hasPower_ = 1;
}

void
lefiMacro::setEEQ(const char *name)
{
    int len = strlen(name) + 1;
    if (len > EEQSize_)
        bump(&(EEQ_), len, &(EEQSize_));
    strcpy(EEQ_, CASE(name));
    hasEEQ_ = 1;
}

void
lefiMacro::setLEQ(const char *name)
{
    int len = strlen(name) + 1;
    if (len > LEQSize_)
        bump(&(LEQ_), len, &(LEQSize_));
    strcpy(LEQ_, CASE(name));
    hasLEQ_ = 1;
}

void
lefiMacro::setProperty(const char   *name,
                       const char   *value,
                       const char   type)
{
    int len;
    if (numProperties_ == propertiesAllocated_)
        bumpProps();
    len = strlen(name) + 1;
    propNames_[numProperties_] = (char*) lefMalloc(len);
    strcpy(propNames_[numProperties_], CASE(name));
    len = strlen(value) + 1;
    propValues_[numProperties_] = (char*) lefMalloc(len);
    strcpy(propValues_[numProperties_], CASE(value));
    propNums_[numProperties_] = 0.0;
    propTypes_[numProperties_] = type;
    numProperties_ += 1;
}

void
lefiMacro::setNumProperty(const char    *name,
                          double        d,
                          const char    *value,
                          const char    type)
{
    int len;

    if (numProperties_ == propertiesAllocated_)
        bumpProps();
    len = strlen(name) + 1;
    propNames_[numProperties_] = (char*) lefMalloc(len);
    strcpy(propNames_[numProperties_], CASE(name));
    len = strlen(value) + 1;
    propValues_[numProperties_] = (char*) lefMalloc(len);
    strcpy(propValues_[numProperties_], CASE(value));
    propNums_[numProperties_] = d;
    propTypes_[numProperties_] = type;
    numProperties_ += 1;
}


void
lefiMacro::bumpProps()
{
    int     lim;
    int     news;
    char    **newpn;
    char    **newpv;
    double  *newd;
    char    *newt;
    int     i;

    if (propertiesAllocated_ <= 0)
        lim = 2;                 // starts with 4
    else
        lim = propertiesAllocated_;
    news = lim + lim;
    newpn = (char**) lefMalloc(sizeof(char*) * news);
    newpv = (char**) lefMalloc(sizeof(char*) * news);
    newd = (double*) lefMalloc(sizeof(double) * news);
    newt = (char*) lefMalloc(sizeof(char) * news);

    propertiesAllocated_ = news;

    for (i = 0; i < lim; i++) {
        newpn[i] = propNames_[i];
        newpv[i] = propValues_[i];
        newd[i] = propNums_[i];
        newt[i] = propTypes_[i];
    }
    lefFree((char*) (propNames_));
    lefFree((char*) (propValues_));
    lefFree((char*) (propNums_));
    lefFree((char*) (propTypes_));
    propNames_ = newpn;
    propValues_ = newpv;
    propNums_ = newd;
    propTypes_ = newt;
}

void
lefiMacro::setFixedMask(int isFixedMask)
{
    isFixedMask_ = isFixedMask;
}

int
lefiMacro::isFixedMask() const
{
    return isFixedMask_;
}

void
lefiMacro::addForeign(const char    *name,
                      int           hasPnt,
                      double        x,
                      double        y,
                      int           orient)
{
    int     i;
    int     *hfo;
    int     *hfp;
    int     *fo;
    double  *fx;
    double  *fy;
    char    **f;

    if (foreignAllocated_ == numForeigns_) {
        if (foreignAllocated_ == 0)
            foreignAllocated_ = 16; // since it involves char*, it will
            // costly in the number is too small
        else
            foreignAllocated_ *= 2;
        hfo = (int*) lefMalloc(sizeof(int) * foreignAllocated_);
        hfp = (int*) lefMalloc(sizeof(int) * foreignAllocated_);
        fo = (int*) lefMalloc(sizeof(int) * foreignAllocated_);
        fx = (double*) lefMalloc(sizeof(double) * foreignAllocated_);
        fy = (double*) lefMalloc(sizeof(double) * foreignAllocated_);
        f = (char**) lefMalloc(sizeof(char*) * foreignAllocated_);
        if (numForeigns_ != 0) {
            for (i = 0; i < numForeigns_; i++) {
                hfo[i] = hasForeignOrigin_[i];
                hfp[i] = hasForeignPoint_[i];
                fo[i] = foreignOrient_[i];
                fx[i] = foreignX_[i];
                fy[i] = foreignY_[i];
                f[i] = foreign_[i];
            }
            lefFree((char*) (hasForeignOrigin_));
            lefFree((char*) (hasForeignPoint_));
            lefFree((char*) (foreignOrient_));
            lefFree((char*) (foreignX_));
            lefFree((char*) (foreignY_));
            lefFree((char*) (foreign_));
        }
        hasForeignOrigin_ = hfo;
        hasForeignPoint_ = hfp;
        foreignOrient_ = fo;
        foreignX_ = fx;
        foreignY_ = fy;
        foreign_ = f;
    }

    // orient=-1 means no orient was specified.
    hasForeignOrigin_[numForeigns_] = orient;
    hasForeignPoint_[numForeigns_] = hasPnt;
    foreignOrient_[numForeigns_] = orient;
    foreignX_[numForeigns_] = x;
    foreignY_[numForeigns_] = y;
    foreign_[numForeigns_] = (char*) lefMalloc(strlen(name) + 1);
    strcpy(foreign_[numForeigns_], CASE(name));
    numForeigns_ += 1;

}

    
void
lefiMacro::setXSymmetry()
{
    hasSymmetry_ |= 1;
}

void
lefiMacro::setYSymmetry()
{
    hasSymmetry_ |= 2;
}

void
lefiMacro::set90Symmetry()
{
    hasSymmetry_ |= 4;
}

void
lefiMacro::setSiteName(const char *name)
{
    int len = strlen(name) + 1;
    if (len > siteNameSize_)
        bump(&(siteName_), len, &(siteNameSize_));
    strcpy(siteName_, CASE(name));
    hasSiteName_ = 1;
}

void
lefiMacro::setClockType(const char *name)
{
    int len = strlen(name) + 1;
    if (len > clockTypeSize_)
        bump(&(clockType_), len, &(clockTypeSize_));
    strcpy(clockType_, CASE(name));
    hasClockType_ = 1;
}

void
lefiMacro::setSitePattern(lefiSitePattern *p)
{
    if (numSites_ == sitesAllocated_) {
        lefiSitePattern **np;
        int             i, lim;
        if (sitesAllocated_ == 0) {
            lim = sitesAllocated_ = 4;
            np = (lefiSitePattern**) lefMalloc(sizeof(lefiSitePattern*) * lim);
        } else {
            lim = sitesAllocated_ * 2;
            sitesAllocated_ = lim;
            np = (lefiSitePattern**) lefMalloc(sizeof(lefiSitePattern*) * lim);
            lim /= 2;
            for (i = 0; i < lim; i++)
                np[i] = pattern_[i];
            lefFree((char*) (pattern_));
        }
        pattern_ = np;
    }
    pattern_[numSites_] = p;
    numSites_ += 1;
}

void
lefiMacro::setSize(double   x,
                   double   y)
{
    hasSize_ = 1;
    sizeX_ = x;
    sizeY_ = y;
}
    

int
lefiMacro::hasClass() const
{
    return hasClass_;
}

int
lefiMacro::hasSiteName() const
{
    return hasSiteName_;
}

int
lefiMacro::hasGenerator() const
{
    return hasGenerator_;
}

int
lefiMacro::hasGenerate() const
{
    return hasGenerate_;
}

int
lefiMacro::hasPower() const
{
    return hasPower_;
}

int
lefiMacro::hasOrigin() const
{
    return hasOrigin_;
}

int
lefiMacro::hasSource() const
{
    return hasSource_;
}

int
lefiMacro::hasEEQ() const
{
    return hasEEQ_;
}

int
lefiMacro::hasLEQ() const
{
    return hasLEQ_;
}

int
lefiMacro::hasXSymmetry() const
{
    return (hasSymmetry_ & 1) ? 1 : 0;
}

int
lefiMacro::hasYSymmetry() const
{
    return (hasSymmetry_ & 2) ? 1 : 0;
}

int
lefiMacro::has90Symmetry() const
{
    return (hasSymmetry_ & 4) ? 1 : 0;
}

int
lefiMacro::hasSitePattern() const
{
    return (pattern_) ? 1 : 0;
}

int
lefiMacro::hasSize() const
{
    return hasSize_;
}

int
lefiMacro::hasForeign() const
{
    return (numForeigns_) ? 1 : 0;
}

int
lefiMacro::hasForeignOrigin(int index) const
{
    return hasForeignOrigin_[index];
}

int
lefiMacro::hasForeignOrient(int index) const
{
    return (foreignOrient_[index] == -1) ? 0 : 1;
}

int
lefiMacro::hasForeignPoint(int index) const
{
    return hasForeignPoint_[index];
}

int
lefiMacro::hasClockType() const
{
    return hasClockType_;
}

int
lefiMacro::numSitePattern() const
{
    return numSites_;
}

int
lefiMacro::numProperties() const
{
    return numProperties_;
}

const char *
lefiMacro::propName(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propNames_[index];
}

const char *
lefiMacro::propValue(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propValues_[index];
}

double
lefiMacro::propNum(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propNums_[index];
}

char
lefiMacro::propType(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propTypes_[index];
}

int
lefiMacro::propIsNumber(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propNums_[index] ? 1 : 0;
}

int
lefiMacro::propIsString(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProperties_) {
        sprintf(msg, "ERROR (LEFPARS-1352): The index number %d given for the macro property is invalid.\nValid index is from 0 to %d", index, numProperties_);
        lefiError(0, 1352, msg);
        return 0;
    }
    return propNums_[index] ? 0 : 1;
}

const char *
lefiMacro::name() const
{
    return name_;
}

const char *
lefiMacro::macroClass() const
{
    return macroClass_;
}

const char *
lefiMacro::generator() const
{
    return generator_;
}

const char *
lefiMacro::EEQ() const
{
    return EEQ_;
}

const char *
lefiMacro::LEQ() const
{
    return LEQ_;
}

const char *
lefiMacro::source() const
{
    return source_;
}

double
lefiMacro::originX() const
{
    return originX_;
}

double
lefiMacro::originY() const
{
    return originY_;
}

double
lefiMacro::power() const
{
    return power_;
}

void
lefiMacro::generate(char    **name1,
                    char    **name2) const
{
    if (name1)
        *name1 = gen1_;
    if (name2)
        *name2 = gen2_;
}

lefiSitePattern *
lefiMacro::sitePattern(int index) const
{
    return pattern_[index];
}

const char *
lefiMacro::siteName() const
{
    return siteName_;
}

double
lefiMacro::sizeX() const
{
    return sizeX_;
}

double
lefiMacro::sizeY() const
{
    return sizeY_;
}

int
lefiMacro::numForeigns() const
{
    return numForeigns_;
}

int
lefiMacro::foreignOrient(int index) const
{
    return foreignOrient_[index];
}

const char *
lefiMacro::foreignOrientStr(int index) const
{
    return (lefiOrientStr(foreignOrient_[index]));
}

double
lefiMacro::foreignX(int index) const
{
    return foreignX_[index];
}

double
lefiMacro::foreignY(int index) const
{
    return foreignY_[index];
}

const char *
lefiMacro::foreignName(int index) const
{
    return foreign_[index];
}

const char *
lefiMacro::clockType() const
{
    return clockType_;
}

int
lefiMacro::isBuffer() const
{
    return isBuffer_;
}

int
lefiMacro::isInverter() const
{
    return isInverter_;
}

void
lefiMacro::print(FILE *f) const
{
    char            *c1;
    char            *c2;
    lefiSitePattern *sp;
    int             i;

    fprintf(f, "MACRO %s\n", name());

    if (hasClass())
        fprintf(f, "  Class %s\n", macroClass());

    if (hasGenerator())
        fprintf(f, "  Generator %s\n", generator());

    if (hasGenerator()) {
        generate(&c1, &c2);
        fprintf(f, "  Generate %s %s\n", c1, c2);
    }

    if (hasPower())
        fprintf(f, "  Power %g\n", power());

    if (hasOrigin())
        fprintf(f, "  Origin %g,%g\n", originX(),
                originY());

    if (hasEEQ())
        fprintf(f, "  EEQ %s\n", EEQ());

    if (hasLEQ())
        fprintf(f, "  LEQ %s\n", LEQ());

    if (hasSource())
        fprintf(f, "  Source %s\n", source());

    if (hasXSymmetry())
        fprintf(f, "  Symmetry X\n");

    if (hasYSymmetry())
        fprintf(f, "  Symmetry Y\n");

    if (has90Symmetry())
        fprintf(f, "  Symmetry R90\n");

    if (hasSiteName())
        fprintf(f, "  Site name %s\n", siteName());

    if (hasSitePattern()) {
        for (i = 0; i < numSitePattern(); i++) {
            sp = sitePattern(i);
            fprintf(f, "  Site pattern ");
            sp->print(f);
        }
    }

    if (hasSize())
        fprintf(f, "  Size %g,%g\n", sizeX(),
                sizeY());

    if (hasForeign()) {
        for (i = 0; i < numForeigns(); i++) {
            fprintf(f, "  Foreign %s", foreignName(i));
            if (hasForeignOrigin(i))
                fprintf(f, "  %g,%g", foreignX(i),
                        foreignY(i));
            if (hasForeignOrient(i))
                fprintf(f, "  orient %s", foreignOrientStr(i));
            fprintf(f, "\n");
        }
    }

    if (hasClockType())
        fprintf(f, "  Clock type %s\n", clockType());

    fprintf(f, "END MACRO %s\n", name());
}

// *****************************************************************************
// lefiTiming
// *****************************************************************************

lefiTiming::lefiTiming()
: numFrom_(0),
  from_(NULL),
  fromAllocated_(0),
  numTo_(0),
  to_(NULL),
  toAllocated_(0),
  hasTransition_(0),
  hasDelay_(0),
  hasRiseSlew_(0),
  hasRiseSlew2_(0),
  hasFallSlew_(0),
  hasFallSlew2_(0),
  hasRiseIntrinsic_(0),
  hasFallIntrinsic_(0),
  hasRiseRS_(0),
  hasRiseCS_(0),
  hasFallRS_(0),
  hasFallCS_(0),
  hasUnateness_(0),
  hasFallAtt1_(0),
  hasRiseAtt1_(0),
  hasFallTo_(0),
  hasRiseTo_(0),
  hasStableTiming_(0),
  hasSDFonePinTrigger_(0),
  hasSDFtwoPinTrigger_(0),
  hasSDFcondStart_(0),
  hasSDFcondEnd_(0),
  hasSDFcond_(0),
  nowRise_(0),
  numOfAxisNumbers_(0),
  axisNumbers_(NULL),
  axisNumbersAllocated_(0),
  numOfTableEntries_(0),
  tableEntriesAllocated_(0),
  table_(NULL),  // three numbers per entry 
  delayRiseOrFall_(NULL),
  delayUnateness_(NULL),
  delayTableOne_(0.0),
  delayTableTwo_(0.0),
  delayTableThree_(0.0),
  transitionRiseOrFall_(NULL),
  transitionUnateness_(NULL),
  transitionTableOne_(0.0),
  transitionTableTwo_(0.0),
  transitionTableThree_(0.0),
  riseIntrinsicOne_(0.0),
  riseIntrinsicTwo_(0.0),
  riseIntrinsicThree_(0.0),
  riseIntrinsicFour_(0.0),
  fallIntrinsicOne_(0.0),
  fallIntrinsicTwo_(0.0),
  fallIntrinsicThree_(0.0),
  fallIntrinsicFour_(0.0),
  riseSlewOne_(0.0),
  riseSlewTwo_(0.0),
  riseSlewThree_(0.0),
  riseSlewFour_(0.0),
  riseSlewFive_(0.0),
  riseSlewSix_(0.0),
  riseSlewSeven_(0.0),
  fallSlewOne_(0.0),
  fallSlewTwo_(0.0),
  fallSlewThree_(0.0),
  fallSlewFour_(0.0),
  fallSlewFive_(0.0),
  fallSlewSix_(0.0),
  fallSlewSeven_(0.0),
  riseRSOne_(0.0),
  riseRSTwo_(0.0),
  riseCSOne_(0.0),
  riseCSTwo_(0.0),
  fallRSOne_(0.0),
  fallRSTwo_(0.0),
  fallCSOne_(0.0),
  fallCSTwo_(0.0),
  unateness_(NULL),
  riseAtt1One_(0.0),
  riseAtt1Two_(0.0),
  fallAtt1One_(0.0),
  fallAtt1Two_(0.0),
  fallToOne_(0.0),
  fallToTwo_(0.0),
  riseToOne_(0.0),
  riseToTwo_(0.0),
  stableSetup_(0.0),
  stableHold_(0.0),
  stableRiseFall_(NULL),
  SDFtriggerType_(NULL),
  SDFfromTrigger_(NULL),
  SDFtoTrigger_(NULL),
  SDFtriggerTableOne_(0.0),
  SDFtriggerTableTwo_(0.0),
  SDFtriggerTableThree_(0.0),
  SDFcondStart_(NULL),
  SDFcondEnd_(NULL),
  SDFcond_(NULL)
{
    Init();
}

void
lefiTiming::Init()
{
    numFrom_ = 0;
    from_ = (char**) lefMalloc(sizeof(char*));
    fromAllocated_ = 1;
    numTo_ = 0;
    to_ = (char**) lefMalloc(sizeof(char*));
    toAllocated_ = 1;

    numOfAxisNumbers_ = 0;
    axisNumbers_ = (double*) lefMalloc(sizeof(double));
    axisNumbersAllocated_ = 1;

    numOfTableEntries_ = 0;
    tableEntriesAllocated_ = 1;
    table_ = (double*) lefMalloc(sizeof(double) * 3);  // three numbers per entry 

    clear();
}

void
lefiTiming::Destroy()
{
    clear();
    lefFree((char*) (from_));
    lefFree((char*) (to_));
    lefFree((char*) (axisNumbers_));
    lefFree((char*) (table_));
}

lefiTiming::~lefiTiming()
{
    Destroy();
}

void
lefiTiming::addRiseFall(const char  *risefall,
                        double      one,
                        double      two)
{
    if (*risefall == 'r' || *risefall == 'R') {
        hasRiseIntrinsic_ = 1;
        nowRise_ = 1;
        riseIntrinsicOne_ = one;
        riseIntrinsicTwo_ = two;
    } else {
        nowRise_ = 0;
        hasFallIntrinsic_ = 1;
        fallIntrinsicOne_ = one;
        fallIntrinsicTwo_ = two;
    }
}

void
lefiTiming::addRiseFallVariable(double  one,
                                double  two)
{
    if (nowRise_ == 1) {
        riseIntrinsicThree_ = one;
        riseIntrinsicFour_ = two;
    } else {
        fallIntrinsicThree_ = one;
        fallIntrinsicFour_ = two;
    }
}

void
lefiTiming::setRiseRS(double    one,
                      double    two)
{
    hasRiseRS_ = 1;
    riseRSOne_ = one;
    riseRSTwo_ = two;
}

void
lefiTiming::setFallRS(double    one,
                      double    two)
{
    hasFallRS_ = 1;
    fallRSOne_ = one;
    fallRSTwo_ = two;
}

void
lefiTiming::setRiseCS(double    one,
                      double    two)
{
    hasRiseCS_ = 1;
    riseCSOne_ = one;
    riseCSTwo_ = two;
}

void
lefiTiming::setFallCS(double    one,
                      double    two)
{
    hasFallCS_ = 1;
    fallCSOne_ = one;
    fallCSTwo_ = two;
}

void
lefiTiming::setRiseAtt1(double  one,
                        double  two)
{
    hasRiseAtt1_ = 1;
    riseAtt1One_ = one;
    riseAtt1Two_ = two;
}

void
lefiTiming::setFallAtt1(double  one,
                        double  two)
{
    hasFallAtt1_ = 1;
    fallAtt1One_ = one;
    fallAtt1Two_ = two;
}

void
lefiTiming::setRiseTo(double    one,
                      double    two)
{
    hasRiseTo_ = 1;
    riseToOne_ = one;
    riseToTwo_ = two;
}

void
lefiTiming::setFallTo(double    one,
                      double    two)
{
    hasFallTo_ = 1;
    fallToOne_ = one;
    fallToTwo_ = two;
}

void
lefiTiming::addUnateness(const char *typ)
{
    hasUnateness_ = 1;
    unateness_ = (char*) typ;
}

void
lefiTiming::setStable(double        one,
                      double        two,
                      const char    *typ)
{
    hasStableTiming_ = 1;
    stableSetup_ = one;
    stableHold_ = two;
    stableRiseFall_ = (char*) typ;
}

void
lefiTiming::addTableEntry(double    one,
                          double    two,
                          double    three)
{
    int     i;
    double  *n;
    if (numOfTableEntries_ >= tableEntriesAllocated_) {
        int lim;

        if (tableEntriesAllocated_ == 0)
            lim = tableEntriesAllocated_ = 2;
        else
            lim = tableEntriesAllocated_ *= 2;
        n = (double*) lefMalloc(sizeof(double) * 3 * lim);
        lim = numOfTableEntries_ * 3;
        for (i = 0; i < lim; i++) {
            n[i] = table_[i];
        }
        lefFree((char*) (table_));
        table_ = n;
    }
    i = numOfTableEntries_ * 3;
    table_[i++] = one;
    table_[i++] = two;
    table_[i] = three;
    numOfTableEntries_ += 1;
}

void
lefiTiming::addTableAxisNumber(double one)
{
    if (numOfAxisNumbers_ == axisNumbersAllocated_) {
        int     i;
        int     lim;
        double  *n;

        if (axisNumbersAllocated_ == 0)
            lim = axisNumbersAllocated_ = 2;
        else
            lim = axisNumbersAllocated_ *= 2;
        n = (double*) lefMalloc(sizeof(double) * lim);
        lim = numOfAxisNumbers_;
        for (i = 0; i < lim; i++)
            n[i] = axisNumbers_[i];
        if (axisNumbersAllocated_ > 2)
            lefFree((char*) (axisNumbers_));
        axisNumbers_ = n;
    }
    axisNumbers_[(numOfAxisNumbers_)++] = one;
}

void
lefiTiming::addRiseFallSlew(double  one,
                            double  two,
                            double  three,
                            double  four)
{
    if (nowRise_) {
        hasRiseSlew_ = 1;
        riseSlewOne_ = one;
        riseSlewTwo_ = two;
        riseSlewThree_ = three;
        riseSlewFour_ = four;
    } else {
        hasFallSlew_ = 1;
        fallSlewOne_ = one;
        fallSlewTwo_ = two;
        fallSlewThree_ = three;
        fallSlewFour_ = four;
    }
}

void
lefiTiming::addRiseFallSlew2(double one,
                             double two,
                             double three)
{
    if (nowRise_) {
        hasRiseSlew2_ = 1;
        riseSlewFive_ = one;
        riseSlewSix_ = two;
        riseSlewSeven_ = three;
    } else {
        hasFallSlew2_ = 1;
        fallSlewFive_ = one;
        fallSlewSix_ = two;
        fallSlewSeven_ = three;
    }
}

void
lefiTiming::addFromPin(const char *name)
{
    if (numFrom_ == fromAllocated_) {
        int     lim;
        int     i;
        char    **n;

        if (fromAllocated_ == 0)
            lim = fromAllocated_ = 2;
        else
            lim = fromAllocated_ *= 2;
        n = (char**) lefMalloc(sizeof(char*) * lim);
        lim = numFrom_;
        for (i = 0; i < lim; i++)
            n[i] = from_[i];
        lefFree((char*) (from_));
        from_ = n;
    }
    from_[(numFrom_)++] = (char*) name;
}

void
lefiTiming::addToPin(const char *name)
{
    if (numTo_ == toAllocated_) {
        int     lim;
        int     i;
        char    **n;

        if (toAllocated_ == 0)
            lim = toAllocated_ = 2;
        else
            lim = toAllocated_ *= 2;
        n = (char**) lefMalloc(sizeof(char*) * lim);
        lim = numTo_;
        for (i = 0; i < lim; i++)
            n[i] = to_[i];
        lefFree((char*) (to_));
        to_ = n;
    }
    to_[(numTo_)++] = (char*) name;
}

void
lefiTiming::addDelay(const char *risefall,
                     const char *unateness,
                     double     one,
                     double     two,
                     double     three)
{
    hasDelay_ = 1;
    delayRiseOrFall_ = (char*) risefall;
    delayUnateness_ = (char*) unateness;
    delayTableOne_ = one;
    delayTableTwo_ = two;
    delayTableThree_ = three;
}

void
lefiTiming::addTransition(const char    *risefall,
                          const char    *unateness,
                          double        one,
                          double        two,
                          double        three)
{
    hasTransition_ = 1;
    transitionRiseOrFall_ = (char*) risefall;
    transitionUnateness_ = (char*) unateness;
    transitionTableOne_ = one;
    transitionTableTwo_ = two;
    transitionTableThree_ = three;
}

void
lefiTiming::addSDF1Pin(const char   *trigType,
                       double       one,
                       double       two,
                       double       three)
{
    hasSDFonePinTrigger_ = 1;
    SDFtriggerType_ = (char*) trigType;
    SDFtriggerTableOne_ = one;
    SDFtriggerTableTwo_ = two;
    SDFtriggerTableThree_ = three;
}

void
lefiTiming::addSDF2Pins(const char  *trigType,
                        const char  *fromTrig,
                        const char  *toTrig,
                        double      one,
                        double      two,
                        double      three)
{
    hasSDFtwoPinTrigger_ = 1;
    SDFtriggerType_ = (char*) trigType;
    SDFfromTrigger_ = (char*) fromTrig;
    SDFtoTrigger_ = (char*) toTrig;
    SDFtriggerTableOne_ = one;
    SDFtriggerTableTwo_ = two;
    SDFtriggerTableThree_ = three;
}

void
lefiTiming::setSDFcondStart(const char *condStart)
{
    SDFcondStart_ = (char*) condStart;
}

void
lefiTiming::setSDFcondEnd(const char *condEnd)
{
    SDFcondEnd_ = (char*) condEnd;
}

void
lefiTiming::setSDFcond(const char *cond)
{
    SDFcond_ = (char*) cond;
}

int
lefiTiming::hasData()
{
    return ((numFrom_) ? 1 : 0);
}

void
lefiTiming::clear()
{
    numFrom_ = 0;
    numTo_ = 0;
    numOfAxisNumbers_ = 0;
    numOfTableEntries_ = 0;

    nowRise_ = 0;
    hasTransition_ = 0;
    hasDelay_ = 0;
    hasFallSlew_ = 0;
    hasFallSlew2_ = 0;
    hasRiseSlew_ = 0;
    hasRiseSlew2_ = 0;
    hasRiseIntrinsic_ = 0;
    hasFallIntrinsic_ = 0;
    hasRiseSlew_ = 0;
    hasFallSlew_ = 0;
    hasRiseSlew2_ = 0;
    hasFallSlew2_ = 0;
    hasRiseRS_ = 0;
    hasRiseCS_ = 0;
    hasFallRS_ = 0;
    hasFallCS_ = 0;
    hasUnateness_ = 0;
    hasFallAtt1_ = 0;
    hasRiseAtt1_ = 0;
    hasFallTo_ = 0;
    hasRiseTo_ = 0;
    hasStableTiming_ = 0;
    hasSDFonePinTrigger_ = 0;
    hasSDFtwoPinTrigger_ = 0;
    hasSDFcondStart_ = 0;
    hasSDFcondEnd_ = 0;
    hasSDFcond_ = 0;
}

int
lefiTiming::numFromPins()
{
    return numFrom_;
}

const char *
lefiTiming::fromPin(int index)
{
    return from_[index];
}

int
lefiTiming::numToPins()
{
    return numTo_;
}

const char *
lefiTiming::toPin(int index)
{
    return to_[index];
}

int
lefiTiming::hasTransition()
{
    return hasTransition_;
}

int
lefiTiming::hasDelay()
{
    return hasDelay_;
}

int
lefiTiming::hasRiseSlew()
{
    return hasRiseSlew_;
}

int
lefiTiming::hasRiseSlew2()
{
    return hasRiseSlew2_;
}

int
lefiTiming::hasFallSlew()
{
    return hasFallSlew_;
}

int
lefiTiming::hasFallSlew2()
{
    return hasFallSlew2_;
}

int
lefiTiming::hasRiseIntrinsic()
{
    return hasRiseIntrinsic_;
}

int
lefiTiming::hasFallIntrinsic()
{
    return hasFallIntrinsic_;
}

int
lefiTiming::hasSDFonePinTrigger()
{
    return hasSDFonePinTrigger_;
}

int
lefiTiming::hasSDFtwoPinTrigger()
{
    return hasSDFtwoPinTrigger_;
}

int
lefiTiming::hasSDFcondStart()
{
    return hasSDFcondStart_;
}

int
lefiTiming::hasSDFcondEnd()
{
    return hasSDFcondEnd_;
}

int
lefiTiming::hasSDFcond()
{
    return hasSDFcond_;
}

int
lefiTiming::numOfAxisNumbers()
{
    return numOfAxisNumbers_;
}

double *
lefiTiming::axisNumbers()
{
    return axisNumbers_;
}

int
lefiTiming::numOfTableEntries()
{
    return numOfTableEntries_;
}

void
lefiTiming::tableEntry(int      num,
                       double   *one,
                       double   *two,
                       double   *three)
{
    num *= 3;
    *one = table_[num];
    num++;
    *two = table_[num];
    num++;
    *three = table_[num];
}

const char *
lefiTiming::delayRiseOrFall()
{
    return delayRiseOrFall_;
}

const char *
lefiTiming::delayUnateness()
{
    return delayUnateness_;
}

double
lefiTiming::delayTableOne()
{
    return delayTableOne_;
}

double
lefiTiming::delayTableTwo()
{
    return delayTableTwo_;
}

double
lefiTiming::delayTableThree()
{
    return delayTableThree_;
}

const char *
lefiTiming::transitionRiseOrFall()
{
    return transitionRiseOrFall_;
}

const char *
lefiTiming::transitionUnateness()
{
    return transitionUnateness_;
}

double
lefiTiming::transitionTableOne()
{
    return transitionTableOne_;
}

double
lefiTiming::transitionTableTwo()
{
    return transitionTableTwo_;
}

double
lefiTiming::transitionTableThree()
{
    return transitionTableThree_;
}

double
lefiTiming::riseIntrinsicOne()
{
    return riseIntrinsicOne_;
}

double
lefiTiming::riseIntrinsicTwo()
{
    return riseIntrinsicTwo_;
}

double
lefiTiming::riseIntrinsicThree()
{
    return riseIntrinsicThree_;
}

double
lefiTiming::riseIntrinsicFour()
{
    return riseIntrinsicFour_;
}

double
lefiTiming::fallIntrinsicOne()
{
    return fallIntrinsicOne_;
}

double
lefiTiming::fallIntrinsicTwo()
{
    return fallIntrinsicTwo_;
}

double
lefiTiming::fallIntrinsicThree()
{
    return fallIntrinsicThree_;
}

double
lefiTiming::fallIntrinsicFour()
{
    return fallIntrinsicFour_;
}

double
lefiTiming::riseSlewOne()
{
    return riseSlewOne_;
}

double
lefiTiming::riseSlewTwo()
{
    return riseSlewTwo_;
}

double
lefiTiming::riseSlewThree()
{
    return riseSlewThree_;
}

double
lefiTiming::riseSlewFour()
{
    return riseSlewFour_;
}

double
lefiTiming::riseSlewFive()
{
    return riseSlewFive_;
}

double
lefiTiming::riseSlewSix()
{
    return riseSlewSix_;
}

double
lefiTiming::riseSlewSeven()
{
    return riseSlewSeven_;
}

double
lefiTiming::fallSlewOne()
{
    return fallSlewOne_;
}

double
lefiTiming::fallSlewTwo()
{
    return fallSlewTwo_;
}

double
lefiTiming::fallSlewThree()
{
    return fallSlewThree_;
}

double
lefiTiming::fallSlewFour()
{
    return fallSlewFour_;
}

double
lefiTiming::fallSlewFive()
{
    return fallSlewFive_;
}

double
lefiTiming::fallSlewSix()
{
    return fallSlewSix_;
}

double
lefiTiming::fallSlewSeven()
{
    return fallSlewSeven_;
}

int
lefiTiming::hasRiseRS()
{
    return hasRiseRS_;
}

double
lefiTiming::riseRSOne()
{
    return riseRSOne_;
}

double
lefiTiming::riseRSTwo()
{
    return riseRSTwo_;
}

int
lefiTiming::hasRiseCS()
{
    return hasRiseCS_;
}

double
lefiTiming::riseCSOne()
{
    return riseCSOne_;
}

double
lefiTiming::riseCSTwo()
{
    return riseCSTwo_;
}

int
lefiTiming::hasFallRS()
{
    return hasFallRS_;
}

double
lefiTiming::fallRSOne()
{
    return fallRSOne_;
}

double
lefiTiming::fallRSTwo()
{
    return fallRSTwo_;
}

int
lefiTiming::hasFallCS()
{
    return hasFallCS_;
}

double
lefiTiming::fallCSOne()
{
    return fallCSOne_;
}

double
lefiTiming::fallCSTwo()
{
    return fallCSTwo_;
}

int
lefiTiming::hasUnateness()
{
    return hasUnateness_;
}

const char *
lefiTiming::unateness()
{
    return unateness_;
}

int
lefiTiming::hasFallAtt1()
{
    return hasFallAtt1_;
}

double
lefiTiming::fallAtt1One()
{
    return fallAtt1One_;
}

double
lefiTiming::fallAtt1Two()
{
    return fallAtt1Two_;
}

int
lefiTiming::hasRiseAtt1()
{
    return hasRiseAtt1_;
}

double
lefiTiming::riseAtt1One()
{
    return riseAtt1One_;
}

double
lefiTiming::riseAtt1Two()
{
    return riseAtt1Two_;
}

int
lefiTiming::hasFallTo()
{
    return hasFallTo_;
}

double
lefiTiming::fallToOne()
{
    return fallToOne_;
}

double
lefiTiming::fallToTwo()
{
    return fallToTwo_;
}

int
lefiTiming::hasRiseTo()
{
    return hasRiseTo_;
}

double
lefiTiming::riseToOne()
{
    return riseToOne_;
}

double
lefiTiming::riseToTwo()
{
    return riseToTwo_;
}

int
lefiTiming::hasStableTiming()
{
    return hasStableTiming_;
}

double
lefiTiming::stableSetup()
{
    return stableSetup_;
}

double
lefiTiming::stableHold()
{
    return stableHold_;
}

const char *
lefiTiming::stableRiseFall()
{
    return stableRiseFall_;
}

const char *
lefiTiming::SDFonePinTriggerType()
{
    return SDFtriggerType_;
}

const char *
lefiTiming::SDFtwoPinTriggerType()
{
    return SDFtriggerType_;
}

const char *
lefiTiming::SDFfromTrigger()
{
    return SDFfromTrigger_;
}

const char *
lefiTiming::SDFtoTrigger()
{
    return SDFtoTrigger_;
}

double
lefiTiming::SDFtriggerOne()
{
    return SDFtriggerTableOne_;
}

double
lefiTiming::SDFtriggerTwo()
{
    return SDFtriggerTableTwo_;
}

double
lefiTiming::SDFtriggerThree()
{
    return SDFtriggerTableThree_;
}

const char *
lefiTiming::SDFcondStart()
{
    return SDFcondStart_;
}

const char *
lefiTiming::SDFcondEnd()
{
    return SDFcondEnd_;
}

const char *
lefiTiming::SDFcond()
{
    return SDFcond_;
}

lefiMacroSite::lefiMacroSite(const char            *name, 
                             const lefiSitePattern *pattern)
: siteName_(name),
  sitePattern_(pattern)
{
}

const char *
lefiMacroSite::siteName() const
{
    return siteName_;
}

const lefiSitePattern *
lefiMacroSite::sitePattern() const
{
    return sitePattern_;
}

lefiMacroForeign::lefiMacroForeign(const char *name,
                                   int        hasPts,
                                   double     x,
                                   double     y,
                                   int        hasOrient,
                                   int        orient)
: cellName_(name),
  cellHasPts_(hasPts),
  px_(x),
  py_(y),
  cellHasOrient_(hasOrient),
  cellOrient_(orient)
{
}

const char *
lefiMacroForeign::cellName() const
{
    return cellName_;
}

int 
lefiMacroForeign::cellHasPts() const
{
    return cellHasPts_;
}

double
lefiMacroForeign::px() const
{
    return px_;
}

double
lefiMacroForeign::py() const
{
    return py_;
}

int
lefiMacroForeign::cellHasOrient() const
{
    return cellHasOrient_;
}

int
lefiMacroForeign::cellOrient() const
{
    return cellOrient_;
}

END_LEFDEF_PARSER_NAMESPACE

