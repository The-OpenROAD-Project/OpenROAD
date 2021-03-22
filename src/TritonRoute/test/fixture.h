/* Author: Matt Liberty */
/*
 * Copyright (c) 2020, The Regents of the University of California
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

#include "frDesign.h"

// General Fixture for tests using db objects.
class Fixture
{
 public:
  Fixture();
  virtual ~Fixture() = default;

  void addLayer(fr::frTechObject* tech,
                const char* name,
                fr::frLayerTypeEnum type,
                fr::frPrefRoutingDirEnum dir = fr::frcNonePrefRoutingDir);

  void setupTech(fr::frTechObject* tech);

  void makeDesign();

  void makeCornerConstraint(fr::frLayerNum layer_num,
                            fr::frCoord eolWidth = -1,
                            fr::frCornerTypeEnum type
                            = fr::frCornerTypeEnum::CONVEX);

  void makeSpacingConstraint(fr::frLayerNum layer_num);

  void makeMinStepConstraint(fr::frLayerNum layer_num);

  void makeMinStep58Constraint(fr::frLayerNum layer_num);

  void makeRectOnlyConstraint(fr::frLayerNum layer_num);

  void makeMinEnclosedAreaConstraint(fr::frLayerNum layer_num);

  void makeSpacingEndOfLineConstraint(fr::frLayerNum layer_num,
                                      fr::frCoord par_space = -1,
                                      fr::frCoord par_within = -1,
                                      bool two_edges = false);
  void makeLef58SpacingEndOfLineConstraint(fr::frLayerNum layer_num,
                                           fr::frCoord par_space = -1,
                                           fr::frCoord par_within = -1,
                                           bool two_edges = false,
                                           fr::frCoord min_max_length = -1,
                                           bool max = true,
                                           bool two_sides = true);

  fr::frNet* makeNet(const char* name);

  void makePathseg(fr::frNet* net,
                   fr::frLayerNum layer_num,
                   const fr::frPoint& begin,
                   const fr::frPoint& end,
                   fr::frUInt4 width = 100,
                   fr::frEndStyleEnum begin_style = fr::frcTruncateEndStyle,
                   fr::frEndStyleEnum end_style = fr::frcTruncateEndStyle);

  void makePathsegExt(fr::frNet* net,
                      fr::frLayerNum layer_num,
                      const fr::frPoint& begin,
                      const fr::frPoint& end,
                      fr::frUInt4 width = 100)
  {
    makePathseg(net,
                layer_num,
                begin,
                end,
                width,
                fr::frcExtendEndStyle,
                fr::frcExtendEndStyle);
  }

  void initRegionQuery();

  // Public data members are accessible from inside the test function
  std::unique_ptr<fr::Logger> logger;
  std::unique_ptr<fr::frDesign> design;
};

// BOOST_TEST wants an operator<< for any type it compares.  We
// don't have those for enums and they are tedious to write.
// Just compare them as integers to avoid this requirement.
#define TEST_ENUM_EQUAL(L, R) \
  BOOST_TEST(static_cast<int>(L) == static_cast<int>(R))
