// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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
