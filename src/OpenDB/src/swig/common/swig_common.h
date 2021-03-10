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

#include <boost/polygon/polygon.hpp>

#include "opendb/db.h"

bool db_diff(odb::dbDatabase* db1, odb::dbDatabase* db2);

bool db_def_diff(odb::dbDatabase* db1, const char* def_filename);

int write_lef(odb::dbLib* lib, const char* path);

int write_tech_lef(odb::dbTech* tech, const char* path);

odb::dbDatabase* read_db(odb::dbDatabase* db, const char* db_path);

int write_db(odb::dbDatabase* db, const char* db_path);

// This is a very basic polygon API for scripting.  In C++ you should
// directly use the Boost Polygon classes.

using Rectangle = boost::polygon::rectangle_data<int>;
using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
using Polygon90Set = boost::polygon::polygon_90_set_data<int>;

Polygon90Set* newSetFromRect(int xLo, int yLo, int xHi, int yHi);

Polygon90Set* bloatSet(const Polygon90Set* set, int bloating);
Polygon90Set* bloatSet(const Polygon90Set* set, int bloatX, int bloatY);

Polygon90Set* shrinkSet(const Polygon90Set* set, int shrinking);
Polygon90Set* shrinkSet(const Polygon90Set* set, int shrinkX, int shrinkY);

Polygon90Set* andSet(const Polygon90Set* set1, const Polygon90Set* set2);

Polygon90Set* orSet(const Polygon90Set* set1, const Polygon90Set* set2);
Polygon90Set* orSets(const std::vector<Polygon90Set>& sets);

Polygon90Set* subtractSet(const Polygon90Set* set1, const Polygon90Set* set2);

std::vector<Polygon90> getPolygons(const Polygon90Set* set);

std::vector<odb::Rect> getRectangles(const Polygon90Set* set);

std::vector<odb::Point> getPoints(const Polygon90* polygon);
