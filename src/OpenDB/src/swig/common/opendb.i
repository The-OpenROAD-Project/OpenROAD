/*
 * Copyright (c) 2021, The Regents of the University of California
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

%{
#define SWIG_FILE_WITH_INIT
#include "geom.h"
#include "db.h"
#include "dbShape.h"
#include "dbViaParams.h"
#include "dbRtEdge.h"
#include "dbWireCodec.h"
#include "dbBlockCallBackObj.h"
#include "dbIterator.h"
#include "dbRtNode.h"
#include "dbTransform.h"
#include "dbWireGraph.h"
#include "dbBlockSet.h"
#include "dbNetSet.h"
#include "dbMap.h"
#include "dbRtTree.h"
#include "dbCCSegSet.h"
#include "dbSet.h"
#include "dbTypes.h"
#include "geom.h"
#include "wOrder.h"

using namespace odb;
%}

%include <stl.i>
%include <typemaps.i>
%include <std_string.i>
%include <std_vector.i>
%include <std_pair.i>

%typemap(in) (uint) = (int);
%typemap(out) (uint) = (int);
%typemap(out) (uint64) = (long);
%apply int* OUTPUT {int* x, int* y};

%ignore odb::dbTechLayerAntennaRule::pwl_pair;
%ignore odb::dbTechLayerAntennaRule::getDiffPAR() const;
%ignore odb::dbTechLayerAntennaRule::getDiffCAR() const;
%ignore odb::dbTechLayerAntennaRule::getDiffPSR() const;
%ignore odb::dbTechLayerAntennaRule::getDiffCSR() const;
%ignore odb::dbTechLayerAntennaRule::getAreaDiffReduce() const;

%include "dbenums.i"
%include "parserenums.i"
%include "dbtypes.i"
%include "dbtypes_common.i"

%include "geom.h"
%include "db.h"
%include "polygon.i"

%include "dbhelpers.i"  
%include "dbdiff.i"

%include "dbViaParams.h"
%include "dbRtEdge.h"
%include "dbWireCodec.h"
%include "dbBlockCallBackObj.h"
%include "dbIterator.h"
%include "dbRtNode.h"
%include "dbTransform.h"
%include "dbWireGraph.h"
%include "dbBlockSet.h"
%include "dbNetSet.h"
%include "dbRtTree.h"
%include "dbCCSegSet.h"
%include "wOrder.h"
