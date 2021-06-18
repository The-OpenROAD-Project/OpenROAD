/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <iostream>
#include <memory>
#include <string>

#include "db/drObj/drFig.h"
#include "db/obj/frBlock.h"
#include "frDesign.h"

extern std::string GUIDE_FILE;
extern std::string OUTGUIDE_FILE;
extern std::string DBPROCESSNODE;
extern std::string OUT_MAZE_FILE;
extern std::string DRC_RPT_FILE;
extern std::string CMAP_FILE;
// to be removed
extern int OR_SEED;
extern double OR_K;

extern int MAX_THREADS;
extern int BATCHSIZE;
extern int BATCHSIZETA;
extern int MTSAFEDIST;
extern int DRCSAFEDIST;
extern int VERBOSE;
extern std::string BOTTOM_ROUTING_LAYER_NAME;
extern std::string TOP_ROUTING_LAYER_NAME;
extern int BOTTOM_ROUTING_LAYER;
extern int TOP_ROUTING_LAYER;
extern bool ALLOW_PIN_AS_FEEDTHROUGH;
extern bool USENONPREFTRACKS;
extern bool USEMINSPACING_OBS;
extern bool ENABLE_BOUNDARY_MAR_FIX;
extern bool ENABLE_VIA_GEN;
// extern int TEST;
extern fr::frLayerNum VIAINPIN_BOTTOMLAYERNUM;
extern fr::frLayerNum VIAINPIN_TOPLAYERNUM;
extern fr::frLayerNum VIAONLY_STDCELLPIN_BOTTOMLAYERNUM;
extern fr::frLayerNum VIAONLY_STDCELLPIN_TOPLAYERNUM;

extern fr::frLayerNum VIA_ACCESS_LAYERNUM;

extern int MINNUMACCESSPOINT_MACROCELLPIN;
extern int MINNUMACCESSPOINT_STDCELLPIN;
extern int ACCESS_PATTERN_END_ITERATION_NUM;

extern int END_ITERATION;
extern int NDR_NETS_RIPUP_THRESH;
extern bool AUTO_TAPER_NDR_NETS;
extern int TAPERBOX_RADIUS;

extern fr::frUInt4 TAVIACOST;
extern fr::frUInt4 TAPINCOST;
extern fr::frUInt4 TAALIGNCOST;
extern fr::frUInt4 TADRCCOST;
extern float TASHAPEBLOATWIDTH;

extern fr::frUInt4 VIACOST;

extern fr::frUInt4 GRIDCOST;
extern fr::frUInt4 FIXEDSHAPECOST;
extern fr::frUInt4 ROUTESHAPECOST;
extern fr::frUInt4 MARKERCOST;
extern fr::frUInt4 MARKERBLOATWIDTH;
extern fr::frUInt4 BLOCKCOST;
extern fr::frUInt4 GUIDECOST;
extern float MARKERDECAY;
extern float SHAPEBLOATWIDTH;
extern int MISALIGNMENTCOST;

// GR
extern int HISTCOST;
extern int CONGCOST;

#define DIRBITSIZE 3
#define WAVEFRONTBUFFERSIZE 2
#define WAVEFRONTBITSIZE (WAVEFRONTBUFFERSIZE * DIRBITSIZE)
#define WAVEFRONTBUFFERHIGHMASK \
  (111 << ((WAVEFRONTBUFFERSIZE - 1) * DIRBITSIZE))

// GR
#define GRWAVEFRONTBUFFERSIZE 2
#define GRWAVEFRONTBITSIZE (GRWAVEFRONTBUFFERSIZE * DIRBITSIZE)
#define GRWAVEFRONTBUFFERHIGHMASK \
  (111 << ((GRWAVEFRONTBUFFERSIZE - 1) * DIRBITSIZE))

namespace fr {
frCoord getGCELLGRIDX();
frCoord getGCELLGRIDY();
frCoord getGCELLOFFSETX();
frCoord getGCELLOFFSETY();

// These need to be in the fr namespace to support argument-dependent
// lookup
std::ostream& operator<<(std::ostream& os, const fr::frViaDef& viaDefIn);
std::ostream& operator<<(std::ostream& os, const fr::frBlock& blockIn);
std::ostream& operator<<(std::ostream& os, const fr::frInst& instIn);
std::ostream& operator<<(std::ostream& os, const fr::frInstTerm& instTermIn);
std::ostream& operator<<(std::ostream& os, const fr::frTerm& termIn);
std::ostream& operator<<(std::ostream& os, const fr::frPin& pinIn);
std::ostream& operator<<(std::ostream& os, const fr::frRect& pinFig);
std::ostream& operator<<(std::ostream& os, const fr::frPolygon& pinFig);
std::ostream& operator<<(std::ostream& os, const fr::frNet& net);
std::ostream& operator<<(std::ostream& os, const fr::frPoint& pIn);
std::ostream& operator<<(std::ostream& os, const fr::frBox& box);
std::ostream& operator<<(std::ostream& os, const fr::drConnFig& fig);
std::ostream& operator<<(std::ostream& os, const frShape& fig);
std::ostream& operator<<(std::ostream& os, const frConnFig& fig);
std::ostream& operator<<(std::ostream& os, const frPathSeg& fig);
std::ostream& operator<<(std::ostream& os, const frGuide& p);
std::ostream& operator<<(std::ostream& os, const frBlockObject& fig);
// namespace fr
}  // namespace fr
#endif
