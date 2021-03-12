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

#ifndef __UTILITY_H__
#define __UTILITY_H__

namespace grt {

extern void getlen();
extern void printEdge(int netID, int edgeID);
extern void plotTree(int netID);

extern void fillVIA();
extern int threeDVIA();

extern void netpinOrderInc();

extern void writeRoute3D(char routingfile3D[]);
extern void checkRoute3D();
extern void write3D();
extern void StNetOrder();
extern Bool checkRoute3DEdgeType2(int netID, int edgeID);
extern Bool checkRoute2DTree(int netID);

extern void printTree3D(int netID);
extern void recoverEdge(int netID, int edgeID);

extern void checkUsage();
extern void netedgeOrderDec(int netID);
extern void printTree2D(int netID);
extern void finalSumCheck();
extern void ACE();

extern void newLA();
extern void iniBDE();
extern void copyBR(void);
extern void copyRS(void);
extern void freeRR(void);
extern Tree fluteToTree(stt::Tree fluteTree);
extern stt::Tree treeToFlute(Tree tree);
extern Tree pdToTree(PD::Tree pdTree);

}  // namespace grt

#endif /* __UTILITY_H__ */
