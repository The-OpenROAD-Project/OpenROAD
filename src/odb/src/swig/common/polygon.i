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
#include "swig_common.h"
%}

%template(Points) std::vector<odb::Point>;
%template(Rects) std::vector<odb::Rect>;
%template(Polygon90Set) std::vector<Polygon90>;
%template(Polygon90Sets) std::vector<Polygon90Set>;

// Simple constructor
%newobject newSetFromRect;
Polygon90Set* newSetFromRect(int xLo, int yLo, int xHi, int yHi);

// Query methods - return vectors for easy swig'ing
std::vector<odb::Point> getPoints(const Polygon90* polygon);
std::vector<Polygon90> getPolygons(const Polygon90Set* set);
std::vector<odb::Rect> getRectangles(const Polygon90Set* set);


%newobject bloatSet;
Polygon90Set* bloatSet(Polygon90Set* set, int bloating);

%newobject bloatSet;
Polygon90Set* bloatSet(Polygon90Set* set, int bloatX, int bloatY);

%newobject shrinkSet;
Polygon90Set* shrinkSet(Polygon90Set* set, int shrinking);

%newobject shrinkSet;
Polygon90Set* shrinkSet(Polygon90Set* set, int shrinkX, int shrinkY);

%newobject andSet;
Polygon90Set* andSet(Polygon90Set* set1, const Polygon90Set* set2);

%newobject orSet;
Polygon90Set* orSet(Polygon90Set* set1, const Polygon90Set* set2);

// It makese no sense to me that we have a vector by value and not
// pointer but swig seems to automatically add the pointer in the
// generated wrapper.
%newobject orSets;
Polygon90Set* orSets(const std::vector<Polygon90Set>& sets);

%newobject subtractSet;
Polygon90Set* subtractSet(Polygon90Set* set1, const Polygon90Set* set2);
