/**************************************************************************
***    
*** Copyright (c) 2000-2006 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining 
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation 
***  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
***  and/or sell copies of the Software, and to permit persons to whom the 
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/


#ifndef DEBUG_H
#define DEBUG_H

#include "btree.h"
#include "netlist.h"
#include "mixedpacking.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>

const int FIELD_WIDTH = 10;

void OutputHardBlockInfoType(std::ostream& outs,
                             const HardBlockInfoType& blockinfo);
void OutputBTree(std::ostream& outs,
                 const BTree& bt);
void OutputBTree(std::ostream& outs,
                 const std::vector<BTree::BTreeNode>& tree);
void OutputPacking(std::ostream& outs,
                   const OrientedPacking& pk);

void OutputMixedBlockInfoType(std::ostream& outs,
                              const MixedBlockInfoType& blockinfo);
   
void OutputDouble(std::ostream& outs, float d);
void OutputIndex(std::ostream& outs, int i);

void DebugBits2Tree(int argc, char *argv[]);
void DebugEvaluate(int argc, char *argv[]);
void DebugSwap(int argc, char *argv[]);
void DebugMove(int argc, char *argv[]);

void DebugParseBlocks(int argc, char *argv[]);
void DebugParseNets(int argc, char *argv[]);
void DebugHPWL(int argc, char *argv[]);

void DebugCopy(int argc, char *argv[]);
void DebugCompact(int argc, char *argv[]);

void DebugAnneal(int argc, char *argv[]);
void DebugWireAnneal(int argc, char *argv[]);

void DebugSSTreeToBTree(int argc, char *argv[]);

void DebugParquetBTree(int argc, char *argv[]);
void DebugBTreeSlack(int argc, char *argv[]);
void DebugMixedPacking(int argc, char *argv[]);
void DebugSoftPacking(int argc, char *argv[]);
void DebugPltoSP(int argc, char *argv[]);
void DebugPltoBTree(int argc, char *argv[]);
void DebugPlSPtoBTree(int argc, char *argv[]);
void DebugShiftBlock(int argc, char *argv[]);
void DebugShiftLegalizer(int argc, char *argv[]);
void DebugMixedBlockInfoTypeFromDB(int argc, char *argv[]);
void DebugBTreeAnnealerFromDB(int argc, char *argv[]);

#endif

