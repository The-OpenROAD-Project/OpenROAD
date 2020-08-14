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


#include "basepacking.h"
#include "btree.h"
#include "debug.h"
#include "netlist.h"
#include "btreecompact.h"
#include "btreeanneal.h"
#include "btreeslackeval.h"
#include "mixedpacking.h"
#include "mixedpackingfromdb.h"

#include "pltobtree.h"
#include "plsptobtree.h"
#include "plcompact.h"

#include "allparquet.h"
#include "ABKCommon/paramproc.h"
#include "ABKCommon/abktimer.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <ctime>
#include <cstdlib>
#include <algorithm>
using namespace basepacking_h;
using parquetfp::DB;
using parquetfp::Command_Line;
using parquetfp::SPeval;
using parquetfp::Pl2SP;

// --------------------------------------------------------
int main(int argc, char *argv[])
{
//    ifstream infile;

//    infile.open(argv[1]);
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    HardBlockInfoType blockinfo(infile);
//    OutputHardBlockInfoType(cout, blockinfo);

//    cout.setf(ios::fixed);
//    cout.precision(0);
//    cout << "blocknum:  " << blockinfo.blocknum() << endl;
//    cout << "blockarea: " << blockinfo.blockArea() << endl;

//   BTree bt(blockinfo);
//   OutputBTree(cout, bt);

//   DebugBits2Tree(argc, argv);
//   DebugEvaluate(argc, argv);
//   DebugSwap(argc, argv);
//   DebugMove(argc, argv);

//   DebugParseBlocks(argc, argv);
//   DebugParseNets(argc, argv);
//   DebugHPWL(argc, argv);

//   DebugCopy(argc, argv);
//   DebugCompact(argc, argv);
//   DebugAnneal(argc, argv);
//   DebugWireAnneal(argc, argv);
//   DebugSSTreeToBTree(argc, argv);
//   DebugParquetBTree(argc, argv);
//   DebugBTreeSlack(argc, argv);
//   DebugMixedPacking(argc, argv);
//   DebugSoftPacking(argc, argv);
//   DebugPltoSP(argc, argv);
//   DebugPltoBTree(argc, argv);
//   DebugPlSPtoBTree(argc, argv);
//   DebugShiftBlock(argc, argv);
//   DebugShiftLegalizer(argc, argv);
//   DebugMixedBlockInfoTypeFromDB(argc, argv);
//   DebugBTreeAnnealerFromDB(argc, argv);
   DebugDBCorner(argc, argv);
   return 0;
}
// --------------------------------------------------------
void DebugDBCorner(int argc, char *argv[])
{
   
}
// --------------------------------------------------------
void DebugBTreeAnnealerFromDB(int argc, char *argv[])
{
   BoolParam help1 ("h", argc, argv);
   BoolParam help2 ("help", argc, argv);
   NoParams  noParams(argc,argv);  // this acts as a flag
   Command_Line* params = new Command_Line(argc, (const char **)argv);
   params->printAnnealerParams();

   if (noParams.found() || help1.found() || help2.found())
   {
      params->printHelp(argc, const_cast<const char**>(argv));
      exit (0);
   }	

   Timer T;
   T.stop();
   float totalTime=0;
   float successTime = 0;
   float successAvgWL = 0;
   float successAvgArea = 0;

   float minArea=1e100;
   float minWS=1e100;
   float minWL=1e100;
   float aveArea=0;
   float aveWS=0;
   float aveWL=0;
   float currArea;
   float lastArea;
   float currWS;
   float currWL;
   float currXSize;
   float currYSize;
   float successAR=0;
		
   for (int i=0; i<params->iterations; i++)
   {
      DB* db = new DB(params->inFileName);
      float blocksArea = db->getNodesArea();
      float reqdArea = (1+(params->maxWS/100))*blocksArea;
      float reqdWidth = sqrt(reqdArea*params->reqdAR);
      float reqdHeight = reqdWidth/params->reqdAR;
      bool gotBetterSol = false;

      string blocksfilename(params->inFileName);
      blocksfilename += ".blocks";
      MixedBlockInfoType blockinfo(blocksfilename, "blocks");     

      T.start(0.0);
      BTreeAreaWireAnnealer *annealer_ptr = NULL;

      if (!strcmp(argv[argc-1], "--db"))
         annealer_ptr = new BTreeAreaWireAnnealer(params, db);
      else          
         annealer_ptr = new BTreeAreaWireAnnealer(blockinfo, params, db);      
      BTreeAreaWireAnnealer& annealer = *annealer_ptr;
      
      annealer.go();
      annealer.currSolution().save_bbb("dummy_out");
      T.stop();

      totalTime += T.getUserTime();
      currXSize = annealer.currSolution().totalWidth();  // db->getXSize();
      currYSize = annealer.currSolution().totalHeight(); // db->getYSize();
      currArea = currXSize*currYSize;
      currWS = 100*(currArea - blocksArea)/blocksArea; // currArea;
      currWL = db->evalHPWL();
      aveArea += currArea;
      aveWS += currWS;
      aveWL += currWL;
      if(minArea > currArea)
      {
         minArea = currArea;
         gotBetterSol = true;
      }
      if(minWS > currWS)
         minWS = currWS;
      if(minWL > currWL)
         minWL = currWL;

      if(params->reqdAR != -9999 && ((currArea<=reqdArea && 
                                      currXSize<=reqdWidth && currYSize<=reqdHeight) || db->successAR))
      {
         ++successAR;
         successTime += T.getUserTime();
         successAvgWL += currWL;
         successAvgArea += currArea;
      }
      //plot and save the best solution

      if(gotBetterSol)
      {
         if(params->plot)
         {
            float currAR = currXSize/currYSize;
            bool plotSlacks = !params->plotNoSlacks;
            bool plotNets = !params->plotNoNets;
            bool plotNames = !params->plotNoNames;
            db->plot("out.plt", currArea, currWS, currAR, T.getUserTime(), 
                     currWL, plotSlacks, plotNets, plotNames);
         }
         if(params->savePl)
            db->getNodes()->savePl(params->outPlFile);

         if(params->saveCapoPl)
            db->getNodes()->saveCapoPl(params->capoPlFile);

         if(params->saveCapo)
            db->saveCapo(params->capoBaseFile, params->reqdAR);

         if(params->save)
            db->save(params->baseFile);
	      
         //if(db->successAR)
         //db->saveBestCopyPl("best.pl");
      }
      cout<<endl;
      delete annealer_ptr;
   }
   aveArea /= params->iterations;
   aveWS /= params->iterations;
   aveWL /= params->iterations;
   totalTime /= params->iterations;
   successTime /= successAR;
   successAvgWL /= successAR;
   successAvgArea /= successAR;
   successAR /= params->iterations;
	
   cout<<endl<<"Average Area: "<<aveArea<<" Minimum Area: "<<minArea<<endl
       <<"Average HPWL: "<<aveWL<<" Minimum HPWL: "<<minWL<<endl
       <<"Average WhiteSpace: "<<aveWS<<"%"<<" Minimum WhiteSpace: "
       <<minWS<<"%"<<endl
       <<"Average Time: "<<totalTime<<endl;
   if(params->reqdAR != -9999)
   {
      cout<<endl<<"Success Rate of satisfying fixed outline: "
          <<100*successAR<<" %"<<endl;
      cout<<"Average Time for successfull AR runs: "<<successTime<<endl;
      cout<<"Average Area for successfull AR runs: "<<successAvgArea<<endl;
      cout<<"Average WL for successfull AR runs: "<<successAvgWL<<endl;
   }
}
// --------------------------------------------------------
void DebugMixedBlockInfoTypeFromDB(int argc, char *argv[])
{
   ifstream infile;
   string blocksfilename(argv[1]);
   blocksfilename += ".blocks";
   infile.open(blocksfilename.c_str());
   if (!infile.good())
   {
      cout << "ERROR: Cannot open file: " << blocksfilename << endl;
      exit(1);
   }

   MixedBlockInfoType blockinfoOne(blocksfilename, "blocks");

   DB db(argv[1]);
   MixedBlockInfoTypeFromDB blockinfoTwo(db);

   ofstream outfile1("dummy_file");
   ofstream outfile2("dummy_db");

   outfile1.setf(ios::fixed);
   outfile1.precision(1);
   OutputMixedBlockInfoType(outfile1, blockinfoOne);

   outfile2.setf(ios::fixed);
   outfile2.precision(1);
   OutputMixedBlockInfoType(outfile2, blockinfoTwo);
}
// --------------------------------------------------------
// void DebugShiftLegalizer(int argc, char *argv[])
// {
//    ifstream infile;
//    infile.open(argv[1]);

//    OrientedPacking packing;
//    Read_bbb(infile, packing);
//    Save_bbb(cout, packing);
   
//    float left_bound, right_bound, bottom_bound, top_bound;
//    cout << "Enter boundary <xStart><xEnd><yStart><yEnd> ->";
//    cin >> left_bound >> right_bound >> bottom_bound >> top_bound;

//    ShiftLegalizer legalizer(packing.xloc, packing.yloc,
//                             packing.width, packing.height,
//                             left_bound, right_bound,
//                             top_bound, bottom_bound);

   
//    int blocknum = packing.xloc.size();

//    vector<int> checkBlks;
//    vector<int> badBlks;

// //    for (int i = 0; i < blocknum; i++)
// //       checkBlks.push_back((i) % blocknum);

//    checkBlks.push_back(0);
//    checkBlks.push_back(1);
//    checkBlks.push_back(2);

//    bool success = legalizer.legalizeAll(ShiftLegalizer::NAIVE,
//                                         checkBlks, badBlks);
//    cout << ((success)? "no overlap afterwards" : "still have overlaps")
//         << endl;
// //    while (cin.good())
// //    {
// //       int currBlk = -1;
// //       cout << "Select a block (0-" << (blocknum-1) << ") ->";
// //       cin >> currBlk;
// //       if (!cin.good())
// //          break;

// //       ShiftBlock shift_block(legalizer.xloc(), legalizer.yloc(),
// //                              legalizer.widths(), legalizer.heights(),
// //                              legalizer.leftBound(), legalizer.rightBound(),
// //                              legalizer.topBound(), legalizer.bottomBound());
      
// //       vector<ShiftBlock::ShiftInfo> currShiftinfo;
// //       shift_block(currBlk, currShiftinfo);

// //       cout << "----currShiftInfo[" << currBlk << "]-----" << endl;
// //       OutputShiftInfo(cout, currShiftinfo);

// //       bool success = legalizer.legalizeBlock(currBlk);
// //       cout << ((success)? "no overlap afterwards" : "still have overlap")
// //            << endl;

// //       OrientedPacking newPK;
// //       newPK.xloc = legalizer.xloc();
// //       newPK.yloc = legalizer.yloc();
// //       newPK.width = legalizer.widths();
// //       newPK.height = legalizer.heights();

// //       Save_bbb(cout, newPK);
// //    }
//    OrientedPacking finalPK;
//    finalPK.xloc = legalizer.xloc();
//    finalPK.yloc = legalizer.yloc();
//    finalPK.width = legalizer.widths();
//    finalPK.height = legalizer.heights();

//    ofstream outfile;
//    outfile.open(argv[2]);
//    Save_bbb(outfile, finalPK);
// }     
// // --------------------------------------------------------
// void DebugShiftBlock(int argc, char *argv[])
// {   
//    ifstream infile;
//    infile.open(argv[1]);
   
//    OrientedPacking packing;
//    Read_bbb(infile, packing);

//    Save_bbb(cout, packing);

//    float left_bound, right_bound, bottom_bound, top_bound;
//    cout << "Enter boundary <xStart><xEnd><yStart><yEnd> ->";
//    cin >> left_bound >> right_bound >> bottom_bound >> top_bound;

//    ShiftBlock shift_block(packing.xloc, packing.yloc,
//                           packing.width, packing.height,
//                           left_bound, right_bound,
//                           top_bound, bottom_bound);
   
//    int blocknum = packing.xloc.size();
//    while (cin.good())
//    {
//       int currBlk = -1;
//       cout << "Select a block (0-" << (blocknum-1) << ") ->";
//       cin >> currBlk;

//       vector<ShiftBlock::ShiftInfo> currShiftinfo;
//       shift_block(currBlk, currShiftinfo);

//       cout << "----currShiftInfo[" << currBlk << "]-----" << endl;
//       cout << "NORTH: " << endl;
//       cout << "shiftRangeMin: "
//            << currShiftinfo[ShiftBlock::NORTH].shiftRangeMin << endl;
//       cout << "shiftRangeMax: "
//            << currShiftinfo[ShiftBlock::NORTH].shiftRangeMax << endl;
//       cout << "overlapMin: "
//            << currShiftinfo[ShiftBlock::NORTH].overlapMin << endl;
//       cout << "overlapMax: "
//            << currShiftinfo[ShiftBlock::NORTH].overlapMax << endl;
//       cout << endl;

//       cout << "EAST: " << endl;
//       cout << "shiftRangeMin: "
//            << currShiftinfo[ShiftBlock::EAST].shiftRangeMin << endl;
//       cout << "shiftRangeMax: "
//            << currShiftinfo[ShiftBlock::EAST].shiftRangeMax << endl;
//       cout << "overlapMin: "
//            << currShiftinfo[ShiftBlock::EAST].overlapMin << endl;
//       cout << "overlapMax: "
//            << currShiftinfo[ShiftBlock::EAST].overlapMax << endl;
//       cout << endl;
      
//       cout << "SOUTH: " << endl;
//       cout << "shiftRangeMin: "
//            << currShiftinfo[ShiftBlock::SOUTH].shiftRangeMin << endl;
//       cout << "shiftRangeMax: "
//            << currShiftinfo[ShiftBlock::SOUTH].shiftRangeMax << endl;
//       cout << "overlapMin: "
//            << currShiftinfo[ShiftBlock::SOUTH].overlapMin << endl;
//       cout << "overlapMax: "
//            << currShiftinfo[ShiftBlock::SOUTH].overlapMax << endl;
//       cout << endl;

//       cout << "WEST: " << endl;
//       cout << "shiftRangeMin: "
//            << currShiftinfo[ShiftBlock::WEST].shiftRangeMin << endl;
//       cout << "shiftRangeMax: "
//            << currShiftinfo[ShiftBlock::WEST].shiftRangeMax << endl;
//       cout << "overlapMin: "
//            << currShiftinfo[ShiftBlock::WEST].overlapMin << endl;
//       cout << "overlapMax: "
//            << currShiftinfo[ShiftBlock::WEST].overlapMax << endl;
//       cout << endl;
//       cout << "--------" << endl;
//    }
// }     
// // --------------------------------------------------------
// void DebugPlSPtoBTree(int argc, char *argv[])
// {
//    srand(time(NULL));
//    srand48(time(NULL));
   
//    Timer tm1;
//    tm1.stop();
//    ifstream infile;
//    infile.open(argv[1]);
   
//    OrientedPacking packing;
//    Read_bbb(infile, packing);

//    //    cout << "-----before pl2sp-----" << endl;
// //    tm1.start();
// //    Pl2SP pl2sp(packing.xloc, packing.yloc,
// //                packing.width, packing.height,
// //                parquetfp::TCG_ALGO);
// //    tm1.stop();
// //    cout << "-----pls2sp(TCG_ALGO) takes: " << tm1.getUserTime() << endl;

// //    if (packing.xloc.size() < 400)
// //    {
// //       cout << "XX: ";
// //       for (unsigned int i = 0; i < packing.xloc.size(); i++)
// //          cout << pl2sp.getXSP()[i] << " ";
// //       cout << endl;
// //       cout << "YY: ";
// //       for (unsigned int i = 0; i < packing.xloc.size(); i++)
// //          cout << pl2sp.getYSP()[i] << " ";
// //       cout << endl;
// //    }

// //    tm1.start();
// //    PlSP2BTree plsp2btree(packing.xloc, packing.yloc,
// //                          packing.width, packing.height,
// //                          pl2sp.getXSP(), pl2sp.getYSP());
// //    tm1.stop();
// //    cout << "-----plsp2btree takes: " << tm1.getUserTime() << endl;

//    infile.clear();
//    infile.close();
//    infile.open(argv[1]);

//    float width, height;
//    infile >> width >> height;
//    HardBlockInfoType blockinfo(infile, "txt");
// // //   OutputHardBlockInfoType(cout, blockinfo);
   
// //    BTree recon_btree(blockinfo);
// //    recon_btree.evaluate(plsp2btree.btree());   
// //    tm1.start();
// //    Pl2BTree pl2btree(packing.xloc, packing.yloc,
// //                      packing.width, packing.height,
// //                      Pl2BTree::TCG);
// //    tm1.stop();
// //    cout << "-----TCG algo takes: " << tm1.getUserTime() << endl;
  
//    tm1.start();
//    Pl2BTree pl2btree2(packing.xloc, packing.yloc,
//                       packing.width, packing.height,
//                       Pl2BTree::HEURISTIC);
//    tm1.stop();
//    cout << "-----heuristic algo takes: " << tm1.getUserTime() << endl;

// //    BTree recon_btree_tcg(blockinfo, 1e-10);
// //    recon_btree_tcg.evaluate(pl2btree.btree());
   
//    BTree recon_btree_heur(blockinfo, 1e-6);
//    recon_btree_heur.evaluate(pl2btree2.btree());
//    BTreeOrientedPacking bopacking(recon_btree_heur);
   
//    ofstream outfile;
//    outfile.open("dummy_out");
//    outfile.precision(0);
//    Save_bbb(outfile, bopacking);

//    printf("orig width: %.2lf height: %.2lf\n", width, height);
//    printf("heur width: %.2lf height: %.2lf\n",
//           recon_btree_heur.totalWidth(), recon_btree_heur.totalHeight());

// //    bool same = true;
// //    for (unsigned int i = 0; i < plsp2btree.btree().size(); i++)
// //    {
// //       const BTree::BTreeNode& node1 = pl2btree.btree()[i];
// //       const BTree::BTreeNode& node2 = pl2btree2.btree()[i];
// //       same = (same &&
// //               node1.parent == node2.parent &&
// //               node1.left == node2.left &&
// //               node1.right == node2.right &&
// //               node1.block_index == node2.block_index &&
// //               node1.orient == node2.orient);
// //    }
   
// //    cout << ((same)? "TCG and heur btree same" : "btree not the same") << endl;
// //    printf("orig width: %.2lf height: %.2lf \n", width, height);
// //    printf("TCG  width: %.2lf height: %.2lf \nHeuristic width: %.2lf height: %.2lf\n",
// //           recon_btree_tcg.totalWidth(), recon_btree_tcg.totalHeight(),
// //           recon_btree_heur.totalWidth(), recon_btree_heur.totalHeight());

// //    // ----compare sequence-pairs-----
// //    bool sp_same = true;
// //    for (unsigned int i = 0; i < pl2sp.getXSP().size(); i++)
// //    {
// //       sp_same = (sp_same &&
// //                  int(pl2sp.getXSP()[i]) == pl2btree.getXX()[i] &&
// //                  int(pl2sp.getYSP()[i]) == pl2btree.getYY()[i]);
// //    }
// //    cout << ((sp_same)? "SP same" : "SP not same") << endl;

// //    // ----verify sequence-pairs-----
// //    SPeval sp_eval(packing.height, packing.width, false);
// //    sp_eval.evaluate(const_cast< vector<unsigned>& >(pl2sp.getXSP()),
// //                     const_cast< vector<unsigned>& >(pl2sp.getYSP()));
// //    cout << "-----sequence-pairs-----" << endl;
// //    printf("Pl2SP(TCG_ALOG) width: %.2lf height: %.2lf\n",
// //           sp_eval.xSize, sp_eval.ySize);

// //    vector<unsigned> XXus(pl2btree.getXX().size());
// //    vector<unsigned> YYus(pl2btree.getYY().size());
// //    for (unsigned int i = 0; i < pl2btree.getXX().size(); i++)
// //    {
// //       XXus[i] = pl2btree.getXX()[i];
// //       YYus[i] = pl2btree.getYY()[i];
// //    }   
// //    sp_eval.evaluate(XXus, YYus);
// //    printf("pl2btree (TCG) width: %.2lf height: %.2lf\n",
// //           sp_eval.xSize, sp_eval.ySize);
// }
// // --------------------------------------------------------
// void DebugPltoBTree(int argc, char *argv[])
// {
//    srand(time(NULL));
//    srand48(time(NULL));
   
//    ifstream infile;
//    infile.open(argv[1]);

//    OrientedPacking packing;
//    Read_bbb(infile, packing);

//    vector<unsigned int> XXrand(packing.xloc.size());
//    vector<unsigned int> YYrand(packing.yloc.size());

//    int trial_num = 1000;
//    int numSuccess = 0;
//    for (int i = 0; i < trial_num; i++)
//    {
      
//    for (unsigned int i = 0; i < packing.xloc.size(); i++)
//    {
//       XXrand[i] = i;
//       YYrand[i] = i;
//    }
//    random_shuffle(XXrand.begin(), XXrand.end());
//    random_shuffle(YYrand.begin(), YYrand.end());

// //    cout << "XX: ";
// //    for (unsigned int i = 0; i < packing.xloc.size(); i++)
// //       cout << XXrand[i] << " ";
// //    cout << endl;
// //    cout << "YY: ";
// //    for (unsigned int i = 0; i < packing.yloc.size(); i++)
// //       cout << YYrand[i] << " ";
// //    cout << endl;

//    SPeval sp_eval(packing.height, packing.width, false);
//    sp_eval.evaluate(XXrand, YYrand);
   
// //    cout << "-----before pl2sp-----" << endl;
//    Pl2SP pl2sp(sp_eval.xloc, sp_eval.yloc,
//                packing.width, packing.height,
//                parquetfp::TCG_ALGO);
// //    cout << "-----after pl2sp-----" << endl;


// //    cout << "-----before pl2btree-----" << endl;
//    Pl2BTree pl2btree(sp_eval.xloc,
//                      sp_eval.yloc,
//                      packing.width,
//                      packing.height,
//                      Pl2BTree::TCG);
// //    cout << "-----after pl2btree-----" << endl;

//    vector<unsigned int> XXnew(pl2btree.getXX().size());
//    vector<unsigned int> YYnew(pl2btree.getYY().size());
//    for (unsigned int i = 0; i < XXnew.size(); i++)
//    {
//       XXnew[i] = pl2btree.getXX()[i];
//       YYnew[i] = pl2btree.getYY()[i];
//    }

//    if (pl2sp.getXSP() == XXnew && pl2sp.getYSP() == YYnew)
//       numSuccess++;
// //    if (pl2sp.getXSP() == XXnew)
// // //       cout << "XX ok" << endl;
// // //    else
// // //       cout << "XX's differ" << endl;

// //    if (pl2sp.getYSP() == YYnew)
// // //       cout << "YY ok" << endl;
// // //    else
// // //       cout << "YY's differ" << endl;
//    }

//    cout << "numSuccess / trial_num: " << numSuccess << " / " << trial_num << endl;
   
// //   OutputHardBlockInfoType(cout, blockinfo);

// //    BTree recon_btree(blockinfo);
// //    recon_btree.evaluate(pl2btree.btree());
// // //   OutputBTree(cout, recon_btree);
   
// //    BTreeOrientedPacking bopacking(recon_btree);
   
// //    ofstream outfile;
// //    outfile.open("dummy_out");
// //    outfile.precision(0);
// //    Save_bbb(outfile, bopacking);
// }
// // --------------------------------------------------------
// void DebugPltoSP(int argc, char *argv[])
// {
//    ifstream infile;
//    infile.open(argv[1]);
//    HardBlockInfoType blockinfo(infile, "txt");

//    int blocknum = blockinfo.blocknum();
//    vector<unsigned int> XX(blocknum);
//    vector<unsigned int> YY(blocknum);
//    for (int i = 0; i < blocknum; i++)
//    {
//       XX[i] = unsigned(i);
//       YY[i] = unsigned(i);
//    }
//    random_shuffle(XX.begin(), XX.end());
//    random_shuffle(YY.begin(), YY.end());

//    vector<float> widths(blocknum);
//    vector<float> heights(blocknum);
//    for (int i = 0; i < blocknum; i++)
//    {
//       widths[i] = blockinfo[i].width[0];
//       heights[i] = blockinfo[i].height[0];
//    }

//    SPeval sp_eval(heights, widths, false);
//    sp_eval.evaluate(XX, YY);
//    cout << "done evaluate" << endl;

//    Pl2SP pl2sp(sp_eval.xloc, sp_eval.yloc,
//                widths, heights, parquetfp::TCG_ALGO);
//    cout << "done pl2sp" << endl;   
// }
// // --------------------------------------------------------
// void DebugSoftPacking(int argc, char *argv[])
// {
//    BoolParam help1 ("h", argc, argv);
//    BoolParam help2 ("help", argc, argv);
//    NoParams  noParams(argc,argv);  // this acts as a flag
//    Command_Line* params = new Command_Line(argc, (const char **)argv);
//    params->printAnnealerParams();

//    if (noParams.found() || help1.found() || help2.found())
//    {
//       params->printHelp ();
//       exit (0);
//    }	

//    DB* db = new DB(params->inFileName);

//    string blocksfilename(params->inFileName);
//    blocksfilename += ".blocks";
//    MixedBlockInfoType blockinfo(blocksfilename, "blocks");
   
//    BTreeAreaWireAnnealer annealer(blockinfo, params, db);
   
//    cout << "---before packSoftBlocks()-----" << endl;   
//    cout << endl;
   
//    const BTree& initSolution = annealer.currSolution();
//    initSolution.save_bbb("dummy_init");

//    cout << "right before packSoftBlocks()" << endl;
   
//    float oldArea = annealer.currSolution().totalArea();
//    float newArea = oldArea;
//    int iter = 0;
//    while (cin.good())
//    {
//       annealer.packSoftBlocks(2);
//       newArea = annealer.currSolution().totalArea();
//       printf("[%d] area %.2lf -> %.2lf\n",
//              iter, oldArea, newArea);
//       iter++;
//       oldArea = newArea;
//       cin.get();
//    }
//    cout << "right after packSoftBlocks()" << endl;
   
//    const BTree& finalSolution = annealer.currSolution();
//    finalSolution.save_bbb("dummy_final");

//    cout << "---after packSoftBlocks()-----" << endl;
//    cout << endl;
// }  
// // --------------------------------------------------------
// void DebugMixedPacking(int argc, char *argv[])
// {
//    MixedBlockInfoType blockinfo(argv[1], "blocks");
//    OutputMixedBlockInfoType(cout, blockinfo);
// }
// // --------------------------------------------------------
// void DebugBTreeSlack(int argc, char *argv[])
// {
// //   srand(time(NULL)); // 2
   
//    ifstream infile;
//    infile.open(argv[1]);
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    HardBlockInfoType blockinfo(infile, "txt");
//    BTree btree(blockinfo);

//    BTreeAreaWireAnnealer::GenerateRandomSoln(btree, btree.NUM_BLOCKS);
//    OutputBTree(cout, btree.tree);

//    vector<BTree::BTreeNode> rev_tree(btree.tree.size());
//    BTreeSlackEval::reverse_tree(btree.tree, rev_tree);
//    OutputBTree(cout, rev_tree);

//    cout << "bit-vectors" << endl;
//    int Undefined = BTree::Undefined;
//    int tree_prev = btree.NUM_BLOCKS;
//    int tree_curr = btree.tree[btree.NUM_BLOCKS].left;

//    const vector<BTree::BTreeNode>& tree = btree.tree;
//    vector<int> tree_bits;
//    while (tree_curr != btree.NUM_BLOCKS)
//    {
//       if (tree_prev == tree[tree_curr].parent)
//       {
//          if (tree_curr == tree[tree_prev].left)
//          {
//             cout << "0";
//             tree_bits.push_back(0);
//          }
//          else
//          {
//             cout << "10";
//             tree_bits.push_back(1);
//             tree_bits.push_back(0);
//          }

//          tree_prev = tree_curr;
//          if (tree[tree_curr].left != Undefined)
//             tree_curr = tree[tree_curr].left;
//          else if (tree[tree_curr].right != Undefined)
//             tree_curr = tree[tree_curr].right;
//          else
//             tree_curr = tree[tree_curr].parent;
//       }
//       else if (tree_prev == tree[tree_curr].left)
//       {
//          cout << "1";
//          tree_bits.push_back(1);

//          tree_prev = tree_curr;
//          if (tree[tree_curr].right != Undefined)
//             tree_curr = tree[tree_curr].right;
//          else
//             tree_curr = tree[tree_curr].parent;
//       }
//       else
//       {
//          tree_prev = tree_curr;
//          tree_curr = tree[tree_curr].parent;
//       }
//    }
//    cout << "1" << endl;
//    tree_bits.push_back(1);
//    tree_prev = btree.NUM_BLOCKS;
//    tree_curr = rev_tree[btree.NUM_BLOCKS].left;

//    vector<int> rev_tree_bits;
//    while (tree_curr != btree.NUM_BLOCKS)
//    {
//       if (tree_prev == rev_tree[tree_curr].parent)
//       {
//          if (tree_curr == rev_tree[tree_prev].left)
//          {
//             cout << "0";
//             rev_tree_bits.push_back(0);
//          }
//          else
//          {
//             cout << "10";
//             rev_tree_bits.push_back(1);
//             rev_tree_bits.push_back(0);
//          }

//          tree_prev = tree_curr;
//          if (rev_tree[tree_curr].left != Undefined)
//             tree_curr = rev_tree[tree_curr].left;
//          else if (rev_tree[tree_curr].right != Undefined)
//             tree_curr = rev_tree[tree_curr].right;
//          else
//             tree_curr = rev_tree[tree_curr].parent;
//       }
//       else if (tree_prev == rev_tree[tree_curr].left)
//       {
//          cout << "1";
//          rev_tree_bits.push_back(1);

//          tree_prev = tree_curr;
//          if (rev_tree[tree_curr].right != Undefined)
//             tree_curr = rev_tree[tree_curr].right;
//          else
//             tree_curr = rev_tree[tree_curr].parent;
//       }
//       else
//       {
//          tree_prev = tree_curr;
//          tree_curr = rev_tree[tree_curr].parent;
//       }
//    }
//    cout << "1" << endl;
//    rev_tree_bits.push_back(1);

//    for (unsigned int i = 0; i < rev_tree_bits.size(); i++)
//       rev_tree_bits[i] = (rev_tree_bits[i] == 1)? 0 : 1;

//    cout << endl << "compare" << endl;
//    copy(tree_bits.begin(), tree_bits.end(), ostream_iterator<int>(cout, ""));
//    cout << endl;
   
//    reverse(rev_tree_bits.begin(), rev_tree_bits.end());
//    copy(rev_tree_bits.begin(), rev_tree_bits.end(), ostream_iterator<int>(cout, ""));
//    cout << endl;

//    btree.evaluate(btree.tree);
//    btree.save_bbb("dummy");

//    btree.evaluate(rev_tree);
//    btree.save_bbb("dummy_rev");

//    BTreeSlackEval slackEval(btree);

//    slackEval.evaluateSlacks(btree);
//    const vector<float> xSlack = slackEval.xSlack();
//    const vector<float> ySlack = slackEval.ySlack();
//    for (int i = 0; i < btree.NUM_BLOCKS; i++)
//       cout << i << ": " << xSlack[i] << ", " << ySlack[i] << endl;
// }
// // --------------------------------------------------------
// void DebugParquetBTree(int argc, char *argv[])
// {
//    BoolParam help1 ("h", argc, argv);
//    BoolParam help2 ("help", argc, argv);
//    NoParams  noParams(argc,argv);  // this acts as a flag
//    Command_Line* params = new Command_Line(argc, (const char **)argv);
//    params->printAnnealerParams();

//    if (noParams.found() || help1.found() || help2.found())
//    {
//       params->printHelp ();
//       exit (0);
//    }	

//    Timer T;
//    T.stop();
//    float totalTime=0;
//    float successTime = 0;
//    float successAvgWL = 0;
//    float successAvgArea = 0;

//    float minArea=1e100;
//    float minWS=1e100;
//    float minWL=1e100;
//    float aveArea=0;
//    float aveWS=0;
//    float aveWL=0;
//    float currArea;
//    float lastArea;
//    float currWS;
//    float currWL;
//    float currXSize;
//    float currYSize;
//    float successAR=0;
		
//    for (int i=0; i<params->iterations; i++)
//    {
//       DB* db = new DB(params->inFileName);
//       float blocksArea = db->getNodesArea();
//       float reqdArea = (1+(params->maxWS/100))*blocksArea;
//       float reqdWidth = sqrt(reqdArea*params->reqdAR);
//       float reqdHeight = reqdWidth/params->reqdAR;
//       bool gotBetterSol = false;

//       string blocksfilename(params->inFileName);
//       blocksfilename += ".blocks";
//       MixedBlockInfoType blockinfo(blocksfilename, "blocks");     

//       T.start(0.0);
//       BTreeAreaWireAnnealer annealer(blockinfo, params, db);
//       annealer.go();
//       annealer.currSolution().save_bbb("dummy_out");
//       T.stop();

//       totalTime += T.getUserTime();
//       currXSize = annealer.currSolution().totalWidth();  // db->getXSize();
//       currYSize = annealer.currSolution().totalHeight(); // db->getYSize();
//       currArea = currXSize*currYSize;
//       currWS = 100*(currArea - blocksArea)/blocksArea; // currArea;
//       currWL = db->evalHPWL();
//       aveArea += currArea;
//       aveWS += currWS;
//       aveWL += currWL;
//       if(minArea > currArea)
//       {
//          minArea = currArea;
//          gotBetterSol = true;
//       }
//       if(minWS > currWS)
//          minWS = currWS;
//       if(minWL > currWL)
//          minWL = currWL;

//       if(params->reqdAR != -9999 && ((currArea<=reqdArea && 
//                                       currXSize<=reqdWidth && currYSize<=reqdHeight) || db->successAR))
//       {
//          ++successAR;
//          successTime += T.getUserTime();
//          successAvgWL += currWL;
//          successAvgArea += currArea;
//       }
//       //plot and save the best solution

//       if(gotBetterSol)
//       {
//          if(params->plot)
//          {
//             float currAR = currXSize/currYSize;
//             bool plotSlacks = !params->plotNoSlacks;
//             bool plotNets = !params->plotNoNets;
//             bool plotNames = !params->plotNoNames;
//             db->plot("out.plt", currArea, currWS, currAR, T.getUserTime(), 
//                      currWL, plotSlacks, plotNets, plotNames);
//          }
//          if(params->savePl)
//             db->getNodes()->savePl(params->outPlFile);

//          if(params->saveCapoPl)
//             db->getNodes()->saveCapoPl(params->capoPlFile);

//          if(params->saveCapo)
//             db->saveCapo(params->capoBaseFile, params->reqdAR);

//          if(params->save)
//             db->save(params->baseFile);
	      
//          //if(db->successAR)
//          //db->saveBestCopyPl("best.pl");
//       }
//       cout<<endl;
//    }
//    aveArea /= params->iterations;
//    aveWS /= params->iterations;
//    aveWL /= params->iterations;
//    totalTime /= params->iterations;
//    successTime /= successAR;
//    successAvgWL /= successAR;
//    successAvgArea /= successAR;
//    successAR /= params->iterations;
	
//    cout<<endl<<"Average Area: "<<aveArea<<" Minimum Area: "<<minArea<<endl
//        <<"Average HPWL: "<<aveWL<<" Minimum HPWL: "<<minWL<<endl
//        <<"Average WhiteSpace: "<<aveWS<<"%"<<" Minimum WhiteSpace: "
//        <<minWS<<"%"<<endl
//        <<"Average Time: "<<totalTime<<endl;
//    if(params->reqdAR != -9999)
//    {
//       cout<<endl<<"Success Rate of satisfying fixed outline: "
//           <<100*successAR<<" %"<<endl;
//       cout<<"Average Time for successfull AR runs: "<<successTime<<endl;
//       cout<<"Average Area for successfull AR runs: "<<successAvgArea<<endl;
//       cout<<"Average WL for successfull AR runs: "<<successAvgWL<<endl;
//    }
// }
// // --------------------------------------------------------
// void DebugSSTreeToBTree(int argc, char *argv[])
// {
//    srand(time(NULL));
   
// //    ifstream infile;
// //    infile.open(argv[1]);
// //    if (!infile.good())
// //    {
// //       cout << "ERROR: cannot read file " << argv[1] << endl;
// //       exit(1);
// //    }
// //    HardBlockInfoType blockinfo(infile, "txt"); // didn't sort

//    ifstream infile2;
//    infile2.open(argv[1]);
//    if (!infile2.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    BlockInfoType softblockinfo(BlockInfoType::TXT, infile2); // sorted   
//    SoftSTree sst(softblockinfo);

//    cout << "DONE WITH I/O (blocknum: " << softblockinfo.BLOCK_NUM() << ")" << endl;
//    int blocknum = softblockinfo.BLOCK_NUM();
//    vector<int> perm(blocknum);
//    for (int i = 0; i < blocknum; i++)
//       perm[i] = i;
//    random_shuffle(perm.begin(), perm.end());
//    cout << "after random_shuffle (perm.size: " << perm.size() << ")" << endl;

//    int perm_ptr = 0;
//    for (int i = 0; i < 2*blocknum - 1; i++)
//    {
//       if (sst.balance() <= 1)
//       {
//          sst.push_operand(perm[perm_ptr]);
//          // cout << perm.back() << " ";
//          perm_ptr++;
//       }
//       else
//       {
//          float rand_num = float(rand()) / RAND_MAX;
//          if (perm_ptr < perm.size())
//          {
//             if (rand_num < 0.55)
//             {
//                sst.push_operand(perm[perm_ptr]);
//                // cout << perm.back() << " ";
//                perm_ptr++;
//             }
//             else if (rand_num < 0.775)
//             {
//                sst.push_operator(SoftSTree::STAR);
// //               cout << "* ";
//             }
//             else
//             {
//                sst.push_operator(SoftSTree::PLUS);
//                // cout << "+ ";
//             }
//          }
//          else
//          {
//             if (rand_num < 0.5)
//             {
//                sst.push_operator(SoftSTree::BOTH);
//                // cout << "- ";
//             }
//             else
//             {
//                sst.push_operator(SoftSTree::BOTH);
//                // cout << "- ";
//             }
//          }            
//       }

//       if (i % 100 == 0)
//          cout << "i: " << i << endl;
//    }

//    cout << "DONE WITH RANDOM STUFF (blocknum: " << perm.size() << ")" << endl;
//    SoftSliceRecord ssr(sst); cout << "set SoftSliceRecord" << endl;
//    SoftPacking spk(ssr, softblockinfo); cout << "set SoftPacking" << endl;

//    BTreeCompactSlice(spk, "dummy_compacted");
// //    for (vector<int>::iterator ptr = spk.expression.begin();
// //         ptr != spk.expression.end(); ptr++)
// //    {
// //       int sign = *ptr;
// //       if (SoftSTree::isOperand(sign))
// //          cout << sign << " ";
// //       else if (sign == SoftSTree::PLUS)
// //          cout << "+ ";
// //       else if (sign == SoftSTree::STAR)
// //          cout << "* ";
// //       else if (sign == SoftSTree::BOTH)
// //          cout << "- ";
// //       else
// //          cout << "? ";
// //    }
// //    cout << endl;
   
// //    ExplicitSoftPacking espk(spk); cout << "set ExplicitSoftPacking" << endl;
// // //    for (vector<int>::iterator ptr = espk.expression.begin();
// // //         ptr != espk.expression.end(); ptr++)
// // //    {
// // //       int sign = *ptr;
// // //       if (SoftSTree::isOperand(sign))
// // //          cout << sign << " ";
// // //       else if (sign == SoftSTree::PLUS)
// // //          cout << "+ ";
// // //       else if (sign == SoftSTree::STAR)
// // //          cout << "* ";
// // //       else if (sign == SoftSTree::BOTH)
// // //          cout << "- ";
// // //       else
// // //          cout << "? ";
// // //    }
// // //    cout << endl;   
// //    SoftPackingHardBlockInfoType softhardblockinfo(spk); cout << "set SoftPackingHardBlockInfoType" << endl;
// //    BTreeFromSoftPacking btree(softhardblockinfo, espk); cout << "set BTreeFromSoftPacking" << endl;

// //    ofstream outfile[2];

// //    outfile[0].open("dummy_slice");
// //    spk.output(outfile[0]);
// //    printf("slicing packing area: %.2lf (%%%.2lf)\n",
// //           spk.totalWidth * spk.totalHeight,
// //           (((spk.totalWidth * spk.totalHeight) / spk.blockArea) - 1) * 100);

// //    btree.save_bbb("dummy_btree");
// //    printf("btree packing area:   %.2lf (%%%.2lf)\n",
// //           btree.totalArea(), (btree.totalArea() / btree.blockArea() - 1) * 100);

// //    BTreeCompactor compactor(btree); cout << "set BTreeCompactor" << endl;
// //    BTree orig_tree(btree);
// //    int numChanged = 0;
// //    int i = 0;
// //    do
// //    {
// //       numChanged = compactor.compact();
   
// // //    cout << "-----compacted packing-----" << endl;
// // //    OutputPacking(cout, BTreeOrientedPacking(compactor));
// // //    cout << "area: " << compactor.totalArea() << endl << endl;

// //       printf("[%d] changed %6d: %.2lf (%%%.2lf) -> %.2lf (%%%.2lf)\n",
// //              i, numChanged, orig_tree.totalArea(),
// //              (orig_tree.totalArea() / orig_tree.blockArea() - 1) * 100,
// //              compactor.totalArea(),
// //              (compactor.totalArea() / compactor.blockArea() - 1) * 100);
// //       i++;
// //       orig_tree = compactor;
// //    } while (numChanged != 0);

// //    ofstream outfile_done;
// //    outfile_done.open("dummy_done");
// //    Save_bbb(outfile_done, BTreeOrientedPacking(compactor));
// //    outfile_done.close();
// }
// // --------------------------------------------------------
// void DebugWireAnneal(int argc, char *argv[])
// {
//    cout << "Inside debugWireAnneal" << endl;

//    ifstream infile;
//    Command_Line *params = new Command_Line(argc, (const char **)argv);
//    DB *db = new DB(params->inFileName);

//    // read "blockinfo"
//    string blocksfilename(params->inFileName);
//    blocksfilename += ".blocks";
//    infile.open(blocksfilename.c_str());
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << blocksfilename << endl;
//       exit(1);
//    }
//    else
//       cout << "Opened file " << blocksfilename << endl;
//    MixedBlockInfoType blockinfo(blocksfilename, "blocks");      
         
//    BTreeAreaWireAnnealer annealer(blockinfo, params, db);
//    cout << "annealer initialized sucessfully" << endl;

//    cout << endl << endl;
//    params->printAnnealerParams();   
//    annealer.go();

//    cout << "Exit successfully" << endl;   
// }
// // --------------------------------------------------------
// // void DebugAnneal(int argc, char *argv[])
// // {
// //    srand(time(NULL)); // 2

// //    ifstream infile;
// //    infile.open(argv[1]);
// //    if (!infile.good())
// //    {
// //       cout << "ERROR: cannot read file " << argv[1] << endl;
// //       exit(1);
// //    }
// //    HardBlockInfoType blockinfo(infile, "txt");

// //    BTreeAreaAnnealer annealer(blockinfo);
   
// //    BTree *final_ptr = annealer.go_free_outline();
// // //   float blkArea = blockinfo.blockArea();
// // //   float dspace = atof(argv[2]);
// // //   float ar = atof(argv[3]);
// // //   BTree *final_ptr = annealer.go_fixed_outline(sqrt((blkArea * (1+dspace)) * ar),
// // //                                                sqrt((blkArea * (1+dspace)) / ar));

// // //   printf("dspace: %.2lf%% ar: %.2lf\n", dspace*100, ar);
// //    printf("blockArea:   %.0lf\n", final_ptr ->blockArea());
// //    printf("totalArea:   %.0lf (%.2lf%%)\n",
// //           final_ptr ->totalArea(), 
// //           ((final_ptr ->totalArea() / final_ptr ->blockArea() - 1) * 100));
// // //    printf("width:  %.2lf (%.2lf)\n",
// // //           final_ptr ->totalWidth(), sqrt((blkArea * (1+dspace)) * ar));
// // //    printf("height: %.2lf (%.2lf)\n",
// // //           final_ptr ->totalHeight(), sqrt((blkArea * (1+dspace)) / ar));          

// //    ofstream outfile;
// //    outfile.open("dummy");
// //    Save_bbb(outfile, BTreeOrientedPacking(*final_ptr));
// //    outfile.close();

// //    BTreeCompactor compactor(*final_ptr);

// //    int changeCount = 0;
// //    float orig_area = compactor.totalArea();
// //    do
// //    {
// //       changeCount = compactor.compact();
// //       printf("changeCount: %3d %.2lf -> %.2lf\n",
// //              changeCount, orig_area, compactor.totalArea());
// //       orig_area = compactor.totalArea();
// //    } while (changeCount > 0);

// //    delete final_ptr;
// // }      
// // -------------------------------------------------------- 
// void DebugCompact(int argc, char *argv[]) 
// {
//    srand(time(NULL));

//    ifstream infile;
//    infile.open(argv[1]);
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    HardBlockInfoType blockinfo(infile, "txt");

//    vector<int> tree_bits;
//    vector<int> tree_perm;
//    vector<int> tree_orient;
//    int balance = 0;
//    for (int i = 0; i < 2*blockinfo.blocknum(); i++)
//    {
//       bool assigned = false;
//       while (!assigned)
//       {
//          float rand_num = float(rand()) / (RAND_MAX+1.0);
//          float threshold;

//          if (balance == 0)
//             threshold = 1; // push_back "0" for sure
//          else if (balance == blockinfo.blocknum()) 
//             threshold = 0; // push_back "1" for sure
//          else
//             threshold = 1 / (rand_num * (balance - rand_num));
         
//          if (rand_num >= threshold)
//          {
//             tree_bits.push_back(1);
//             balance--;
//             assigned = true;
//          }
//          else
//          {
//             tree_bits.push_back(0);
//             balance++;
//             assigned = true;
//          }
//       }
//    }

//    tree_perm.resize(blockinfo.blocknum());
//    iota(tree_perm.begin(), tree_perm.end(), 0);
//    random_shuffle(tree_perm.begin(), tree_perm.end());

//    for (int i = 0; i < blockinfo.blocknum(); i++)
//    {
//       int rand_num = int(8 * (float(rand()) / (RAND_MAX + 1.0)));
//       tree_orient.push_back(rand_num);
//    }

// //    cout << "tree_bits: ";
// //    copy(tree_bits.begin(), tree_bits.end(), ostream_iterator<int>(cout));
// //    cout << endl;

// //    cout << "tree_perm: ";
// //    copy(tree_perm.begin(), tree_perm.end(), ostream_iterator<int>(cout, " "));
// //    cout << endl;

// //    cout << "tree_orient: ";
// //    copy(tree_orient.begin(), tree_orient.end(), ostream_iterator<int>(cout, " "));
// //    cout << endl;
   
//    BTree orig_tree(blockinfo);
//    orig_tree.evaluate(tree_bits, tree_perm, tree_orient);

// //    cout << "-----original packing-----" << endl;
// //    OutputPacking(cout, BTreeOrientedPacking(orig_tree));
// //    cout << "area: " << orig_tree.totalArea() << endl << endl;

//    ofstream outfile;
//    outfile.open("dummy");
//    Save_bbb(outfile, BTreeOrientedPacking(orig_tree));
//    outfile.close();

//    BTreeCompactor compactor(orig_tree);
//    int numChanged = 0;
//    int i = 0;
//    do
//    {
//       numChanged = compactor.compact();
   
// //    cout << "-----compacted packing-----" << endl;
// //    OutputPacking(cout, BTreeOrientedPacking(compactor));
// //    cout << "area: " << compactor.totalArea() << endl << endl;

//       printf("[%d] changed %6d: %.2lf (%%%.2lf) -> %.2lf (%%%.2lf)\n",
//              i, numChanged, orig_tree.totalArea(),
//              (orig_tree.totalArea() / orig_tree.blockArea() - 1) * 100,
//              compactor.totalArea(),
//              (compactor.totalArea() / compactor.blockArea() - 1) * 100);
//       i++;
//       orig_tree = compactor;
//    } while (numChanged != 0);

//    ofstream outfile_done;
//    outfile_done.open("dummy_done");
//    Save_bbb(outfile_done, BTreeOrientedPacking(compactor));
//    outfile_done.close();
// }
// // --------------------------------------------------------
// void DebugCopy(int argc, char *argv[])
// {
//    ifstream infile[3];
//    string base_name(argv[1]);
//    char filename[100];

//    sprintf(filename, "%s.blocks", base_name.c_str());
//    infile[0].open(filename);

//    sprintf(filename, "%s.nets", base_name.c_str());
//    infile[1].open(filename);

//    sprintf(filename, "%s.pl", base_name.c_str());
//    infile[2].open(filename);

//    HardBlockInfoType blockinfo(infile[0], "blocks");
//    NetListType netlist(infile[2], infile[1], blockinfo);

//    vector<int> tree_bits;
//    vector<int> tree_perm;
//    vector<int> tree_orient;
//    int balance = 0;
//    for (int i = 0; i < 2*blockinfo.blocknum(); i++)
//    {
//       bool assigned = false;
//       while (!assigned)
//       {
//          int rand_num = int(2 * (float(rand()) / (RAND_MAX+1.0)));
//          if (rand_num == 1)
//          {
//             if (balance > 0)
//             {
//                tree_bits.push_back(rand_num);
//                balance--;
//                assigned = true;
//             }
//          }
//          else
//          {
//             if (balance < blockinfo.blocknum())
//             {
//                tree_bits.push_back(rand_num);
//                balance++;
//                assigned = true;
//             }
//          }
//       }
//    }

//    for (int i = 0; i < blockinfo.blocknum(); i++)
//       tree_perm.push_back(i);
//    random_shuffle(tree_perm.begin(), tree_perm.end());

//    for (int i = 0; i < blockinfo.blocknum(); i++)
//    {
//       int rand_num = int(8 * (float(rand()) / (RAND_MAX + 1.0)));
//       tree_orient.push_back(rand_num);
//    }

//    BTree oldTree(blockinfo);
//    BTree newTree(oldTree);
   
//    oldTree.evaluate(tree_bits, tree_perm, tree_orient);
//    newTree.evaluate(tree_bits, tree_perm, tree_orient);

//    cout << "-----old packing-----" << endl;
//    OutputPacking(cout, BTreeOrientedPacking(oldTree));
//    cout << endl << endl;

//    cout << "-----new packing-----" << endl;
//    OutputPacking(cout, BTreeOrientedPacking(newTree));
//    cout << endl << endl;
   
//    printf("oldHPWL: %lf newHPWL: %lf\n",
//           netlist.getHPWL(BTreeOrientedPacking(oldTree)),
//           netlist.getHPWL(BTreeOrientedPacking(newTree)));
//    while (cin.good())
//    {
//       int blkA, blkB;
//       cout << "block A ->";
//       cin >> blkA;

//       do
//       {
//          cout << "block B ->";
//          cin >> blkB;
//       } while (blkA == blkB);

//       newTree.swap(blkA, blkB);
//       printf("oldHPWL: %lf newHPWL: %lf\n",
//              netlist.getHPWL(BTreeOrientedPacking(oldTree)),
//              netlist.getHPWL(BTreeOrientedPacking(newTree)));

//       oldTree = newTree;
//    }
// }  
// // --------------------------------------------------------
// void DebugHPWL(int argc, char *argv[])
// {
//    ifstream infile[3];
//    ifstream infile_bbb;

//    string base_name(argv[1]);
//    string bbb_name(argv[2]);
//    char filename[100];

//    sprintf(filename, "%s.blocks", base_name.c_str());
//    infile[0].open(filename);

//    sprintf(filename, "%s.nets", base_name.c_str());
//    infile[1].open(filename);

//    sprintf(filename, "%s.pl", base_name.c_str());
//    infile[2].open(filename);

//    infile_bbb.open(bbb_name.c_str());

//    HardBlockInfoType blockinfo(infile[0], "blocks");
//    NetListType netlist(infile[2], infile[1], blockinfo);
//    OrientedPacking packing;
//    Read_bbb(infile_bbb, packing);

//    for (int i = 0; i < 10000; i++)
//    {
//       float HPWL = netlist.getHPWL(packing);
//       cout << i << endl;
//    }
// }
// // --------------------------------------------------------
// void DebugParseNets(int argc, char *argv[])
// {
//    ifstream infile_blocks, infile_nets, infile_pl;
//    ofstream outfile;

//    infile_blocks.open(argv[1]);
//    if (!infile_blocks.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }

//    infile_nets.open(argv[2]);
//    if (!infile_nets.good())
//    {
//       cout << "ERROR: cannot read file " << argv[2] << endl;
//       exit(1);
//    }

//    infile_pl.open(argv[3]);
//    if (!infile_pl.good())
//    {
//       cout << "ERROR: cannot read file " << argv[3] << endl;
//       exit(1);
//    }

//    HardBlockInfoType blockinfo(infile_blocks, "blocks");
//    NetListType netlist(infile_pl, infile_nets, blockinfo);

//    ofstream outfile_recon;
//    outfile_recon.open("recon_nets");

//    outfile_recon << "NumNets : " << netlist.nets.size() << endl << endl;
//    int numNets = netlist.nets.size();
//    for (int i = 0; i < numNets; i++)
//    {
//       char line[100];
//       int netDegree = netlist.nets[i].pins.size() + netlist.nets[i].pads.size();
//       sprintf(line, "NetDegree : %d", netDegree);
//       outfile_recon << line << endl;

//       for (unsigned int k = 0; k < netlist.nets[i].pads.size(); k++)
//       {
//          string pad_name("default");
//          for (unsigned int p = 0; p < netlist.padinfo.size(); p++)
//             if ((netlist.nets[i].pads[k].xloc == netlist.padinfo[p].xloc) &&
//                 (netlist.nets[i].pads[k].yloc == netlist.padinfo[p].yloc))
//             {
//                pad_name = netlist.padinfo[p].pad_name;
//                break;
//             }
//          outfile_recon << pad_name << " B" << endl;
//       }

//       for (unsigned int k = 0; k < netlist.nets[i].pins.size(); k++)
//       {
//          int block = netlist.nets[i].pins[k].block;
//          float x_percent = (netlist.nets[i].pins[k].x_offset / blockinfo[block].width[0]) * 200;
//          float y_percent = (netlist.nets[i].pins[k].y_offset / blockinfo[block].height[0]) * 200;
//          sprintf(line, "%s B \t: %%%.1lf %%%.1lf",
//                  blockinfo.block_names[block].c_str(), x_percent, y_percent);
//          outfile_recon << line << endl;
//       }
//    }
// }
// // --------------------------------------------------------
// void DebugParseBlocks(int argc, char *argv[])
// {
//    ifstream infile, infile2;
//    ofstream outfile, outfile2;

//    infile.open(argv[1]);
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    outfile.open("out1");

//    HardBlockInfoType blockinfo(infile, "blocks");
//    OutputHardBlockInfoType(outfile, blockinfo);
//    outfile.close();
//    infile.close();
   
//    infile2.open(argv[2]);
//    if (!infile2.good())
//    {
//       cout << "ERROR: cannot open file " << argv[2] << endl;
//       exit(1);
//    }
//    outfile2.open("out2");
   
//    HardBlockInfoType bbb_info(infile2, "txt");
//    OutputHardBlockInfoType(outfile2, bbb_info);
//    outfile2.close();
//    infile2.close();

//    cout << "-----block_names-----" << endl;
//    for (unsigned int i = 0; i < blockinfo.block_names.size(); i++)
//       printf("[%d]: %s\n", i, blockinfo.block_names[i].c_str());

//    cout << endl;
//    cout << "-----block_names (txt)-----" << endl;
//    for (unsigned int i = 0; i < bbb_info.block_names.size(); i++)
//       printf("[%d]: %s\n", i, bbb_info.block_names[i].c_str());
// }
// // --------------------------------------------------------
// void DebugMove(int argc, char *argv[])
// {
//    ifstream infile;

//    infile.open(argv[1]);
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    HardBlockInfoType blockinfo(infile, "txt");

//    vector<int> tree_bits;
//    vector<int> perm;
//    vector<int> orient;

//    string bits_string(argv[2]);
//    for (unsigned int i = 0; i < bits_string.length(); i++)
//    {
//       string dummy;
//       dummy += bits_string[i];
//       int this_bit = atoi(dummy.c_str());

//       tree_bits.push_back(this_bit);
//    }

//    string orients_string(argv[3]);
//    for (unsigned int i = 0; i < orients_string.length(); i++)
//    {
//       string dummy;
//       dummy += orients_string[i];
//       int this_orient = atoi(dummy.c_str());

//       orient.push_back(this_orient);
//    }

//    if (tree_bits.size() != 2 * orient.size())
//    {
//       cout << "ERROR: the sizes of tree_bits and orient not consistent."
//            << endl;
//       cout << "tree: " << tree_bits.size() << " vz. orient: "
//            << orient.size() << endl;
//       exit(1);
//    }

//    for (unsigned int i = 0; i < orient.size(); i++)
//    {
//       int blk = i;
//       perm.push_back(blk);
//    }
   
//    cout << "tree:   ";
//    for (unsigned int i = 0; i < tree_bits.size(); i++)
//       cout << tree_bits[i] << " ";
//    cout << endl;

//    cout << "perm:   ";
//    for (unsigned int i = 0; i < perm.size(); i++)
//       cout << perm[i] << " ";
//    cout << endl;

//    cout << "orient: ";
//    for (unsigned int i = 0; i < orient.size(); i++)
//       cout << orient[i] << " ";
//    cout << endl;

//    cout << endl;

// //    vector<BTree::BTreeNode> btree;
// //    BTree::bits2tree(tree_bits, perm, orient, btree);
// //    OutputBTree(cout, btree);

//    BTree bt(blockinfo);
//    bt.evaluate(tree_bits, perm, orient);

//    OutputBTree(cout, bt);

//    int index, target, leftChild;
//    cout << "----move-----" << endl;
//    cout << "index ->";
//    cin >> index;

//    cout << "target ->";
//    cin >> target;

//    cout << "leftChild (0 for true, 1 for false)->";
//    cin >> leftChild;

//    bt.move(index, target, (leftChild == 0));
//    OutputBTree(cout, bt);
//    bt.save_bbb(argv[argc-1]);
// }
// // --------------------------------------------------------
// void DebugSwap(int argc, char *argv[])
// {
//    ifstream infile;

//    infile.open(argv[1]);
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    HardBlockInfoType blockinfo(infile, "txt");

//    vector<int> tree_bits;
//    vector<int> perm;
//    vector<int> orient;

//    string bits_string(argv[2]);
//    for (unsigned int i = 0; i < bits_string.length(); i++)
//    {
//       string dummy;
//       dummy += bits_string[i];
//       int this_bit = atoi(dummy.c_str());

//       tree_bits.push_back(this_bit);
//    }

//    string orients_string(argv[3]);
//    for (unsigned int i = 0; i < orients_string.length(); i++)
//    {
//       string dummy;
//       dummy += orients_string[i];
//       int this_orient = atoi(dummy.c_str());

//       orient.push_back(this_orient);
//    }

//    if (tree_bits.size() != 2 * orient.size())
//    {
//       cout << "ERROR: the sizes of tree_bits and orient not consistent."
//            << endl;
//       cout << "tree: " << tree_bits.size() << " vz. orient: "
//            << orient.size() << endl;
//       exit(1);
//    }

//    for (unsigned int i = 0; i < orient.size(); i++)
//    {
//       int blk = -1;
//       cout << "Enter next block ->";
//       cin >> blk;
         
//       perm.push_back(blk);
//    }
   
//    cout << "tree:   ";
//    for (unsigned int i = 0; i < tree_bits.size(); i++)
//       cout << tree_bits[i] << " ";
//    cout << endl;

//    cout << "perm:   ";
//    for (unsigned int i = 0; i < perm.size(); i++)
//       cout << perm[i] << " ";
//    cout << endl;

//    cout << "orient: ";
//    for (unsigned int i = 0; i < orient.size(); i++)
//       cout << orient[i] << " ";
//    cout << endl;

//    cout << endl;

// //    vector<BTree::BTreeNode> btree;
// //    BTree::bits2tree(tree_bits, perm, orient, btree);
// //    OutputBTree(cout, btree);

//    BTree bt(blockinfo);
//    bt.evaluate(tree_bits, perm, orient);

//    OutputBTree(cout, bt);

//    int indexOne, indexTwo;
//    cout << "----swap-----" << endl;
//    cout << "block One ->";
//    cin >> indexOne;

//    cout << "block Two ->";
//    cin >> indexTwo;

//    bt.swap(indexOne, indexTwo);
//    OutputBTree(cout, bt);
//    bt.save_bbb(argv[argc-1]);
// }
// // --------------------------------------------------------
// void DebugEvaluate(int argc, char *argv[])
// {
//    ifstream infile;

//    infile.open(argv[1]);
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    HardBlockInfoType blockinfo(infile, "txt");

//    vector<int> tree_bits;
//    vector<int> perm;
//    vector<int> orient;

//    string bits_string(argv[2]);
//    for (unsigned int i = 0; i < bits_string.length(); i++)
//    {
//       string dummy;
//       dummy += bits_string[i];
//       int this_bit = atoi(dummy.c_str());

//       tree_bits.push_back(this_bit);
//    }

//    string orients_string(argv[3]);
//    for (unsigned int i = 0; i < orients_string.length(); i++)
//    {
//       string dummy;
//       dummy += orients_string[i];
//       int this_orient = atoi(dummy.c_str());

//       orient.push_back(this_orient);
//    }

//    if (tree_bits.size() != 2 * orient.size())
//    {
//       cout << "ERROR: the sizes of tree_bits and orient not consistent."
//            << endl;
//       cout << "tree: " << tree_bits.size() << " vz. orient: "
//            << orient.size() << endl;
//       exit(1);
//    }

//    for (unsigned int i = 0; i < orient.size(); i++)
//    {
//       int blk = -1;
//       cout << "Enter next block ->";
//       cin >> blk;
         
//       perm.push_back(blk);
//    }
   
//    cout << "tree:   ";
//    for (unsigned int i = 0; i < tree_bits.size(); i++)
//       cout << tree_bits[i] << " ";
//    cout << endl;

//    cout << "perm:   ";
//    for (unsigned int i = 0; i < perm.size(); i++)
//       cout << perm[i] << " ";
//    cout << endl;

//    cout << "orient: ";
//    for (unsigned int i = 0; i < orient.size(); i++)
//       cout << orient[i] << " ";
//    cout << endl;

//    cout << endl;

// //    vector<BTree::BTreeNode> btree;
// //    BTree::bits2tree(tree_bits, perm, orient, btree);
// //    OutputBTree(cout, btree);

//    BTree bt(blockinfo);
//    bt.evaluate(tree_bits, perm, orient);

//    OutputBTree(cout, bt);

//    cout << "-----take 2-----" << endl;
//    cout << "tree: ";
//    for (int i = 0; i < 2*bt.NUM_BLOCKS; i++)
//       cin >> tree_bits[i];

//    cout << "perm: ";
//    for (int i = 0; i < bt.NUM_BLOCKS; i++)
//       cin >> perm[i];

//    cout << "orient: ";
//    for (int i = 0; i < bt.NUM_BLOCKS; i++)
//       cin >> orient[i];

//    bt.evaluate(tree_bits, perm, orient);
//    OutputBTree(cout, bt);
//    bt.save_bbb(argv[argc-1]);
// }
// // --------------------------------------------------------
// void DebugBits2Tree(int argc, char *argv[])
// {
//    ifstream infile;
   
//    infile.open(argv[1]);
//    if (!infile.good())
//    {
//       cout << "ERROR: cannot read file " << argv[1] << endl;
//       exit(1);
//    }
//    HardBlockInfoType blockinfo(infile, "txt");

//    vector<int> tree_bits;
//    vector<int> perm;
//    vector<int> orient;

//    string bits_string(argv[2]);
//    for (unsigned int i = 0; i < bits_string.length(); i++)
//    {
//       string dummy;
//       dummy += bits_string[i];
//       int this_bit = atoi(dummy.c_str());

//       tree_bits.push_back(this_bit);
//    }

//    string orients_string(argv[3]);
//    for (unsigned int i = 0; i < orients_string.length(); i++)
//    {
//       string dummy;
//       dummy += orients_string[i];
//       int this_orient = atoi(dummy.c_str());

//       orient.push_back(this_orient);
//    }

//    if (tree_bits.size() != 2 * orient.size())
//    {
//       cout << "ERROR: the sizes of tree_bits and orient not consistent."
//            << endl;
//       cout << "tree: " << tree_bits.size() << " vz. orient: "
//            << orient.size() << endl;
//       exit(1);
//    }

//    for (unsigned int i = 0; i < orient.size(); i++)
//    {
//       perm.push_back(i);
//    }

//    for (unsigned int i = 0; i < tree_bits.size(); i++)
//       cout << tree_bits[i] << " ";
//    cout << endl;

//    for (unsigned int i = 0; i < perm.size(); i++)
//       cout << perm[i] << " ";
//    cout << endl;

//    for (unsigned int i = 0; i < orient.size(); i++)
//       cout << orient[i] << " ";
//    cout << endl;

//    vector<BTree::BTreeNode> btree;
//    cout << "-----BEFORE-----" << endl;
//    OutputBTree(cout, btree);
//    cout << endl;
   
//    BTree::bits2tree(tree_bits, perm, orient, btree);

//    cout << "-----AFTER-----" << endl;
//    OutputBTree(cout, btree);
//    cout << endl;
// }
// // --------------------------------------------------------
void OutputHardBlockInfoType(ostream& outs,
                             const HardBlockInfoType& blockinfo)
{
   outs << "index:     ";
   for (int i = 0; i < blockinfo.blocknum()+2; i++)
      outs << setw(FIELD_WIDTH) << i;
   outs << endl << endl;

   for (int j = 0; j < HardBlockInfoType::Orient_Num; j++)
   {
      outs << "width[" << j << "]:  ";
      for (int i = 0; i < blockinfo.blocknum()+2; i++)
         if (blockinfo[i].width[j] >= Dimension::Infty)
            outs << " infty";
         else
            outs << setw(FIELD_WIDTH) << blockinfo[i].width[j];
      outs << endl;

      outs << "height[" << j << "]: ";
      for (int i = 0; i < blockinfo.blocknum()+2; i++)
         if (blockinfo[i].height[j] >= Dimension::Infty)
            outs << " infty";
         else
            outs << setw(FIELD_WIDTH) << blockinfo[i].height[j];
      outs << endl << endl;
   }
}
// --------------------------------------------------------
void OutputBTree(ostream& outs,
                 const vector<BTree::BTreeNode>& btree)
{
   int btree_size = btree.size();
   outs << "btree_size: " << btree_size << endl;
   outs << "index:      ";
   for (int i = 0; i < btree_size; i++)
      outs << setw(FIELD_WIDTH) << i;
   outs << endl;
          
   outs << "tree.par:   ";
   for (int i = 0; i < btree_size; i++)
      OutputIndex(outs, btree[i].parent);
   outs << endl;

   outs << "tree.left:  ";
   for (int i = 0; i < btree_size; i++)
      OutputIndex(outs, btree[i].left);
   outs << endl;

   outs << "tree.right: ";
   for (int i = 0; i < btree_size; i++)
      OutputIndex(outs, btree[i].right);
   outs << endl;

   outs << "tree.bkind: ";
   for (int i = 0; i < btree_size; i++)
      OutputIndex(outs, btree[i].block_index);
   outs << endl;

   outs << "tree.orien: ";
   for (int i = 0; i < btree_size; i++)
      OutputIndex(outs, btree[i].orient);
   outs << endl;

   outs << endl;

}
// --------------------------------------------------------
void OutputBTree(ostream& outs,
                 const BTree& bt)
{
   outs << "NUM_BLOCKS: " << bt.NUM_BLOCKS << endl << endl;
   outs << "index:      ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputIndex(cout, i);
   outs << endl;
          
   outs << "tree.par:   ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputIndex(cout, bt.tree[i].parent);
   outs << endl;

   outs << "tree.left:  ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputIndex(cout, bt.tree[i].left);
   outs << endl;

   outs << "tree.right: ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputIndex(cout, bt.tree[i].right);
   outs << endl;

   outs << "tree.bkind: ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputIndex(cout, bt.tree[i].block_index);
   outs << endl;

   outs << "tree.orien: ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputIndex(cout, bt.tree[i].orient);
   outs << endl;

   outs << endl;

   outs << "cont.next:  ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputIndex(cout, bt.contour[i].next);
   outs << endl;

   outs << "cont.prev:  ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputIndex(cout, bt.contour[i].prev);
   outs << endl;

   outs << "cont.begin: ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputDouble(outs, bt.contour[i].begin);
   outs << endl;

   outs << "cont.end:   ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputDouble(outs, bt.contour[i].end);
   outs << endl;//    while (cin.good())

   outs << "cont.CTL:   ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputDouble(outs, bt.contour[i].CTL);
   outs << endl;

   outs << endl;

   outs << "xloc:       ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputDouble(outs, bt.xloc(i));
   outs << endl;

   outs << "yloc:       ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputDouble(outs, bt.yloc(i));
   outs << endl;

   outs << "width:      ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputDouble(outs, bt.width(i));
   outs << endl;

   outs << "height:     ";
   for (int i = 0; i < bt.NUM_BLOCKS+2; i++)
      OutputDouble(outs, bt.height(i));
   outs << endl;

   outs << endl;

   outs << "blockArea:   " << bt.blockArea() << endl;
   outs << "totalArea:   " << bt.totalArea() << endl;
   outs << "totalWidth:  " << bt.totalWidth() << endl;
   outs << "totalHeight: " << bt.totalHeight() << endl;

   outs << endl;
   outs << "-----contour (L to R)-----" << endl;

   outs << "index: ";
   int cPtr = bt.NUM_BLOCKS;
   while (cPtr != BTree::Undefined)
   {
      outs << setw(FIELD_WIDTH) << cPtr;
      cPtr = bt.contour[cPtr].next;
   }
   outs << endl;

   outs << "begin: ";
   cPtr = bt.NUM_BLOCKS;
   while (cPtr != BTree::Undefined)
   {
      OutputDouble(cout, bt.contour[cPtr].begin);
      cPtr = bt.contour[cPtr].next;
   }
   outs << endl;

   outs << "end:   ";
   cPtr = bt.NUM_BLOCKS;
   while (cPtr != BTree::Undefined)
   {
      OutputDouble(cout, bt.contour[cPtr].end);
      cPtr = bt.contour[cPtr].next;
   }
   outs << endl;

   outs << "CTL:   ";
   cPtr = bt.NUM_BLOCKS;
   while (cPtr != BTree::Undefined)
   {
      OutputDouble(cout, bt.contour[cPtr].CTL);
      cPtr = bt.contour[cPtr].next;
   }
   outs << endl;
}
// --------------------------------------------------------
void OutputPacking(ostream& outs,
                   const OrientedPacking& pk)
{
   int blocknum = pk.xloc.size();
   outs << setw(9) << "xloc: ";
   for (int i = 0; i < blocknum; i++)
      outs << setw(5) << pk.xloc[i];
   outs << endl;

   outs << setw(9) << "yloc: ";
   for (int i = 0; i < blocknum; i++)
      outs << setw(5) << pk.yloc[i];
   outs << endl;

   outs << setw(9) << "width: ";
   for (int i = 0; i < blocknum ; i++)
      outs << setw(5) << pk.width[i];
   outs << endl;

   outs << setw(9) << "height: ";
   for (int i = 0; i < blocknum; i++)
      outs << setw(5) << pk.height[i];
   outs << endl;

   outs << setw(9) << "orient: ";
   for (int i = 0; i < blocknum; i++)
      outs << setw(5) << pk.orient[i];
   outs << endl;
}
// --------------------------------------------------------
void OutputDouble(ostream& outs, float d)
{
   outs.setf(ios::fixed);
   outs.precision(1);
   if (d >= Dimension::Infty)
      outs << setw(FIELD_WIDTH) << "infty";

   else if (d == Dimension::Undefined)
      outs << setw(FIELD_WIDTH) << "-";
   else
      outs << setw(FIELD_WIDTH) << d;
}
// --------------------------------------------------------
void OutputIndex(ostream& outs, int i)
{
   if (i == Dimension::Undefined)
      outs << setw(FIELD_WIDTH) << "-";
   else
      outs << setw(FIELD_WIDTH) << i;
}
// --------------------------------------------------------
void OutputMixedBlockInfoType(ostream& outs,
                              const MixedBlockInfoType& blockinfo)
{
   int vec_size = blockinfo.blockARinfo.size();

   outs << "_currDimensions: " << endl;
   outs << "name:     ";
   for (int i = 0; i < vec_size; i++)
   {
      char name[100];
      sprintf(name, "\"%s\"",
              blockinfo.currDimensions.block_names[i].c_str());
      outs << setw(FIELD_WIDTH) << name;
   }
   outs << endl;
   OutputHardBlockInfoType(outs, blockinfo.currDimensions);
   outs << endl;

   outs << "_blockARinfo: " << endl;
   outs.setf(ios::fixed);
   outs.precision(2);
   outs << "index:    ";
   for (int i = 0; i < vec_size; i++)
      OutputIndex(outs, i);
   outs << endl;
   for (int j = 0; j < MixedBlockInfoType::Orient_Num; j++)
   {
      outs << "minAR[" << j << "]: ";      
      for (int i = 0; i < vec_size; i++)
         OutputDouble(outs, blockinfo.blockARinfo[i].minAR[j]);
      outs << endl;
      outs << "maxAR[" << j << "]: ";
      for (int i = 0; i < vec_size; i++)
         OutputDouble(outs, blockinfo.blockARinfo[i].maxAR[j]);
      outs << endl << endl;
   }
   outs << "area:  ";
   for (int i = 0; i < vec_size; i++)
      OutputDouble(outs, blockinfo.blockARinfo[i].area);
   outs << endl;

   outs << "isSoft: ";
   for (int i = 0; i < vec_size; i++)
      outs << setw(FIELD_WIDTH)
           << ((blockinfo.blockARinfo[i].isSoft)? "SOFT" : "HARD");
   outs << endl;
}
// --------------------------------------------------------
   
   
   
