////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, Iowa State University All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

#ifndef __RIPUP_H__
#define __RIPUP_H__

#include "DataType.h"

namespace grt {

extern void ripupSegL(Segment* seg);
extern void ripupSegZ(Segment* seg);
extern void newRipup(TreeEdge* treeedge,
                     TreeNode* treenodes,
                     int x1,
                     int y1,
                     int x2,
                     int y2,
                     int netID);
extern bool newRipupCheck(TreeEdge* treeedge,
                          int x1,
                          int y1,
                          int x2,
                          int y2,
                          int ripup_threshold,
                          int netID,
                          int edgeID);

extern bool newRipupType2(TreeEdge* treeedge,
                          TreeNode* treenodes,
                          int x1,
                          int y1,
                          int x2,
                          int y2,
                          int deg,
                          int netID);
extern bool newRipup3DType3(int netID, int edgeID);

extern void newRipupNet(int netID);
}  // namespace grt

#endif /* __RIPUP_H__ */
