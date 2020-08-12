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


// 040607 hhchan added btree annealer stuff
// 040608 hhchan wrapped everything to class Parquet
// 040608 hhchan put all pre/post-processing out of the annealers

#include "Parquet.h"
#include "ABKCommon/abkcommon.h"
#include "ABKCommon/abkmessagebuf.h"
#include "mixedpacking.h"
#include "baseannealer.h"
#include "btreeanneal.h"
#ifdef USEFLUTE
#include "Flute/flute.h"
#endif

#include <iostream>
#include <iomanip>
#include <cfloat>
#include <algorithm>

using namespace parquetfp;
using std::min;
using std::max;
using std::string;
using std::cout;
using std::setw;
using std::endl;

// --------------------------------------------------------
int main(int argc, const char **argv)
{
   cout<<getABKMessageBuf()<<endl;
   Parquet engine(argc, argv);
   return engine.go();
}
// --------------------------------------------------------
Parquet::Parquet(int argc, const char **argv)
   : params(argc, argv)
{
   BoolParam help1 ("h", argc, argv);
   BoolParam help2 ("help", argc, argv);
   NoParams  noParams(argc,argv);  // this acts as a flag
   params.printAnnealerParams();
   
   if (noParams.found() || help1.found() || help2.found())
   {
      params.printHelp(argc, argv);
      exit (0);
   }	
}
// --------------------------------------------------------
int Parquet::go()
{
#ifdef USEFLUTE
   bool printSteiner = params.printSteiner || params.useSteiner;
   if(printSteiner)
   {
     readLUT();
   }
#endif
   Timer T;
   T.stop();
   
   float totalTime = 0;
   float successTime = 0;
   float successAvgWL = 0;
   float successAvgWLnoWts = 0;
   float successAvgArea = 0;
   float successMinWL = FLT_MAX;
   float successMinWLnoWts = FLT_MAX;
   float successMinArea = FLT_MAX;
   float successMaxWL = 0;
   float successMaxWLnoWts = 0;
   float successMaxArea = 0;
   float successAR = 0;

   float minArea = FLT_MAX;
   float minWS = FLT_MAX;
   float minWL = FLT_MAX;
   float minWLnoWts = FLT_MAX;

   float aveArea = 0;
   float aveWS = 0;
   float aveWL = 0;
   float aveWLnoWts = 0;

   float maxArea = 0;
   float maxWS = 0;
   float maxWL = 0;
   float maxWLnoWts = 0;

   float currArea = FLT_MAX;
   float currWS = FLT_MAX;
   float currWL = FLT_MAX;
   float currXSize = FLT_MAX;
   float currYSize = FLT_MAX;
   float currWLnoWts = FLT_MAX;

#ifdef USEFLUTE
   float successMinSteinerWL = FLT_MAX;
   float successAveSteinerWL = 0;
   float successMaxSteinerWL = 0;
   float minSteinerWL = FLT_MAX;
   float aveSteinerWL = 0;
   float maxSteinerWL = 0;
   float currSteinerWL = FLT_MAX;
#endif

   bool bestUseBTree = false;
   unsigned bestUsedBTree = 0;
   unsigned bestBTreeSuccess = 0;
   unsigned bestUsedSP = 0;
   unsigned bestSPSuccess = 0;

   bool fixedOutline = params.reqdAR != BaseAnnealer::FREE_OUTLINE;

   string blocksname(params.inFileName);
//   blocksname += ".blocks";
//   MixedBlockInfoType blockinfo(blocksname, "blocks");
   blocksname += ".txt";
   MixedBlockInfoType blockinfo(blocksname, "txt");
   DB db(params.inFileName);
   itNode node;
   if(params.nonTrivialOutline.isValid())
   {
     params.maxWS = 100.f*params.nonTrivialOutline.getXSize()*
                    params.nonTrivialOutline.getYSize()/db.getNodesArea()-100.f;
     parquetfp::Point offset(-params.nonTrivialOutline.getMinX(),-params.nonTrivialOutline.getMinY());
     db.shiftTerminals(offset);
   }
   

//    Nodes *obsStor = new Nodes;
//    Node tmp1("obs1", 400*300, 400/300, 400/300, 0, false);
//    tmp1.putX( 200 );
//    tmp1.putY( 200 ); 
//    tmp1.putWidth( 400 );
//    tmp1.putHeight( 300 );
//  
//    obsStor->putNewNode(tmp1 );
//  
//    Node tmp2("obs2", 50*50, 50/50, 50/50, 0, false);
//    tmp2.putX( 100 );
//    tmp2.putY( 100 ); 
//    tmp2.putWidth( 50 );
//    tmp2.putHeight( 50 );
//  
//    obsStor->putNewNode(tmp2 );
//    float frame[2] = {params.nonTrivialOutline.getXSize(), params.nonTrivialOutline.getYSize()};

//   db.addObstacles(obsStor, frame);

   MaxMem maxMem;

   for (int i = 0; i < params.iterations; i++)
   {
      cout << endl << "***** START: round " << (i+1) << " / "
           << params.iterations << " *****" << endl;
      
      //string blocksname(params.inFileName);
      //blocksname += ".blocks";
      //MixedBlockInfoType blockinfo(blocksname, "blocks");
      //DB db(const_cast<char*>(params.inFileName));

      float blocksArea = db.getNodesArea();
      const float reqdArea = blocksArea * (1.f + (params.maxWS/100.f));
      const float reqdWidth = sqrt(reqdArea * params.reqdAR);
      const float reqdHeight = reqdWidth / params.reqdAR;
      bool gotBetterSol = false;

      T.start(0.0);
      if (!params.solveMulti)
      {
         BaseAnnealer *annealer = NULL;
         if (params.FPrep == "BTree")
         {
            annealer =
               new BTreeAreaWireAnnealer(blockinfo,
                                         const_cast<Command_Line*>(&params),
                                         &db);
         }
         else if (params.FPrep == "SeqPair")
         {
            annealer = new Annealer(&params, &db, &maxMem);
         }
         else if (params.FPrep == "Best")
         {
            if(params.minWL &&
               params.reqdAR != BaseAnnealer::FREE_OUTLINE)
            {
               // Fixed Outline mode while minimizing WL
               if(db.getNumNodes() < 100 && params.maxWS > 10. && !bestUseBTree)
               {
                 // Use Sequence Pair
                 bestUseBTree = false;
                 annealer = new Annealer(&params, &db, &maxMem);
                 ++bestUsedSP;
               }
               else
               {
                 // Use B*Tree
                 bestUseBTree = true;
                 annealer = new BTreeAreaWireAnnealer(blockinfo,
                                                      const_cast<Command_Line*>(&params),
                                                      &db);
                 ++bestUsedBTree;
               }
            }
            else
            {
              // Use B*Tree
              bestUseBTree = true;
              annealer = new BTreeAreaWireAnnealer(blockinfo,
                                                   const_cast<Command_Line*>(&params),
                                                   &db);
              ++bestUsedBTree;
            }
         }
         else
         {
            abkfatal(false, "Invalid floorplan representation specified");
            exit(1);
         }

         // normal flat annealing
         if (params.takePl)
         {
            cout << endl;
            cout << "----- Converting placement to initial solution -----"
                 << endl;
            annealer->takePlfromDB();
            cout << "----- done converting -----" << endl;
         } 

         if (params.initQP)
         {
            cout << endl;
            cout << "----- Computing quadratic-minimum WL solution -----"
                 << endl;
            annealer->BaseAnnealer::solveQP();
            cout << "----- done computing -----" << endl;

            cout << "----- Converting placement to initial solution -----"
                 << endl;
            annealer->takePlfromDB();
            cout << "----- done converting -----" << endl;
         }
	       
         if (params.initCompact)
         {
            // compact the curr solution
            cout << endl;
            cout << "----- Compacting initial solution -----"
                 << endl;
            bool minimizeWL = false;
            annealer->compactSoln(minimizeWL,fixedOutline,reqdHeight,reqdWidth);
            cout << "----- done compacting -----" << endl;
         }

         cout << endl;


//         BTreeAreaWireAnnealer* bAnnealer = (BTreeAreaWireAnnealer*) annealer;
//         BasePacking pack;
//         pack.xloc.push_back( 200 );
//         pack.yloc.push_back( 200 ); 
//         pack.width.push_back( 100 );
//         pack.height.push_back( 100 );
//
//         pack.xloc.push_back( 100 );
//         pack.yloc.push_back( 100 ); 
//         pack.width.push_back( 50 );
//         pack.height.push_back( 50 );

//         float frame[2] = {params.nonTrivialOutline.getXSize(), params.nonTrivialOutline.getYSize()};
//         bAnnealer->addObstracles(pack, frame);

         cout << "----- Start Annealing with ";
         if(params.FPrep == "Best")
         {
           if(bestUseBTree)
             cout << "BTree";
           else
             cout << "SeqPair";
         }
         else
         {
           cout << params.FPrep;
         }
         cout << " -----" << endl;
         annealer->go();
         cout << "----- End Annealing with ";
         if(params.FPrep == "Best")
         {
           if(bestUseBTree)
             cout << "BTree";
           else
             cout << "SeqPair";
         }
         else
         {
           cout << params.FPrep;
         }
         cout << " -----" << endl;
            
//         cout << "After Annealer!!!!" << endl;
//         BTreeAreaWireAnnealer* bAnnealer = (BTreeAreaWireAnnealer*)annealer;

//         for(int i=0; i<bAnnealer->currSolution().xloc().size(); i++) {
//           cout << i << " " << bAnnealer->currSolution().xloc(i) << " " 
//             << bAnnealer->currSolution().yloc(i) << endl;
//         }

         if(params.compact)
         {
            // compact the design
            cout << endl;
            cout << "----- Compacting the final solution -----"
                 << endl;
            annealer->compactSoln(params.minWL,fixedOutline,reqdHeight,reqdWidth);
            cout << "----- done compacting -----" << endl;
         }

         if(params.FPrep == "Best" && !bestUseBTree && params.minWL &&
            params.reqdAR != BaseAnnealer::FREE_OUTLINE)
         {
           // check for success
           currXSize = db.getXMax();
           currYSize = db.getYMax();
           currArea = currXSize * currYSize;

           if((currArea > reqdArea ||
               currXSize > reqdWidth ||
               currYSize > reqdHeight) && !db.successAR)
           {
             bestUseBTree = true;
             delete annealer;
             // switch to B*Tree and try again
             annealer = new BTreeAreaWireAnnealer(blockinfo,
                                                  const_cast<Command_Line*>(&params),
                                                  &db);
             ++bestUsedBTree;
             
             // normal flat annealing
             if (params.takePl)
             {
                cout << endl;
                cout << "----- Converting placement to initial solution -----"
                     << endl;
                annealer->takePlfromDB();
                cout << "----- done converting -----" << endl;
             }

             if (params.initQP)
             {
                cout << endl;
                cout << "----- Computing quadratic-minimum WL solution -----"
                     << endl;
                annealer->BaseAnnealer::solveQP();
                cout << "----- done computing -----" << endl;

                cout << "----- Converting placement to initial solution -----"
                     << endl;
                annealer->takePlfromDB();
                cout << "----- done converting -----" << endl;
             }

             if (params.initCompact)
             {
                // compact the curr solution
                cout << endl;
                cout << "----- Compacting initial solution -----"
                     << endl;
                bool minimizeWL = false;
                annealer->compactSoln(minimizeWL,fixedOutline,reqdHeight,reqdWidth);
                cout << "----- done compacting -----" << endl;
             }

             cout << endl;

             cout << "----- Start Annealing with ";
             if(params.FPrep == "Best")
             {
               if(bestUseBTree)
                 cout << "BTree";
               else
                 cout << "SeqPair";
             }
             else
             {
               cout << params.FPrep;
             }
             cout << " -----" << endl;
             annealer->go();
             cout << "----- End Annealing with ";
             if(params.FPrep == "Best")
             {
               if(bestUseBTree)
                 cout << "BTree";
               else
                 cout << "SeqPair";
             }
             else
             {
               cout << params.FPrep;
             }
             cout << " -----" << endl;

             

             if(params.compact)
             {
                // compact the design
                cout << endl;
                cout << "----- Compacting the final solution -----"
                     << endl;
                annealer->compactSoln(params.minWL,fixedOutline,reqdHeight,reqdWidth);
                cout << "----- done compacting -----" << endl;
             }
           }
         }

         if (params.minWL &&
             params.reqdAR != BaseAnnealer::FREE_OUTLINE)
         {
            // shift design, only in fixed-outline mode
            cout << endl;
            cout << "----- Try Shifting the design for better HPWL -----"
                 << endl;
//            annealer->postHPWLOpt();
            cout << "----- done trying -----" << endl;
         }
   
         delete annealer;
      }
      else 
      {
         // two-level annealing
         SolveMulti solveMulti(const_cast<DB*>(&db),
                               const_cast<Command_Line*>(&params), &maxMem);
         solveMulti.go();
	       
         if (params.compact)
         {
            // compact the design            
            Annealer annealer(&params, &db, &maxMem);
            annealer.takePlfromDB();
            annealer.compactSoln(params.minWL,fixedOutline,reqdHeight,reqdWidth);
         }
      }

#ifdef USEFLUTE
//      db.cornerOptimizeDesign(params.scaleTerms,params.minWL,params.useSteiner);
#else
//      db.cornerOptimizeDesign(params.scaleTerms,params.minWL);
#endif
      T.stop();

      // ----- statistics -----
      totalTime += static_cast<float>(T.getUserTime());
      currXSize = db.getXMax();
      currYSize = db.getYMax();
      
      currArea = currXSize * currYSize;
      currWS = 100*(currArea - blocksArea)/blocksArea;
      bool useWts = true;
#ifdef USEFLUTE
      bool useSteiner = true;
      currWL = db.evalHPWL(useWts,params.scaleTerms,!useSteiner);
      currWLnoWts = db.evalHPWL(!useWts,params.scaleTerms,!useSteiner);
      if(printSteiner)
      {
        currSteinerWL = db.evalHPWL(!useWts,params.scaleTerms,useSteiner);
      }
#else
      currWL = db.evalHPWL(useWts,params.scaleTerms);
      currWLnoWts = db.evalHPWL(!useWts,params.scaleTerms);
#endif

      gotBetterSol = false;
      if (params.reqdAR != BaseAnnealer::FREE_OUTLINE)
      {
         gotBetterSol = (currXSize <= reqdWidth && currYSize <= reqdHeight);

         if (params.minWL)
            gotBetterSol = gotBetterSol && (currWL < successMinWL);
         else
            gotBetterSol = gotBetterSol && (currArea < successMinArea);
      }
      else if (params.minWL)
         gotBetterSol = (currWL < minWL);
      else
         gotBetterSol = (currArea < minArea);

      aveArea += currArea;
      aveWS += currWS;
      aveWL += currWL;
      aveWLnoWts += currWLnoWts;

      minArea = min(minArea, currArea);
      minWS = min(minWS, currWS);
      minWL = min(minWL, currWL);
      minWLnoWts = min(minWLnoWts, currWLnoWts);

      maxArea = max(maxArea, currArea);
      maxWS = max(maxWS, currWS);
      maxWL = max(maxWL, currWL);
      maxWLnoWts = max(maxWLnoWts, currWLnoWts);
  
#ifdef USEFLUTE
      if(printSteiner)
      {
        aveSteinerWL += currSteinerWL;
        minSteinerWL = min(minSteinerWL, currSteinerWL);
        maxSteinerWL = max(maxSteinerWL, currSteinerWL);
      }
#endif
       
      if(params.reqdAR != BaseAnnealer::FREE_OUTLINE &&
         ((currArea <= reqdArea && 
           currXSize <= reqdWidth &&
           currYSize <= reqdHeight) || db.successAR))
      {
         ++successAR;
         successTime += static_cast<float>(T.getUserTime());
         
         successAvgWL += currWL;
         successAvgArea += currArea;
         successAvgWLnoWts += currWLnoWts;

         successMinWL = min(successMinWL, currWL);
         successMinArea = min(successMinArea, currArea);
         successMinWLnoWts = min(successMinWLnoWts, currWLnoWts);

         successMaxWL = max(successMaxWL, currWL);
         successMaxArea = max(successMaxArea, currArea);
         successMaxWLnoWts = max(successMaxWLnoWts, currWLnoWts);

#ifdef USEFLUTE
        if(printSteiner)
        {
          successAveSteinerWL += currSteinerWL;
          successMinSteinerWL = min(successMinSteinerWL, currSteinerWL);
          successMaxSteinerWL = max(successMaxSteinerWL, currSteinerWL);
        }
#endif

         if(params.FPrep == "Best")
         {
           if(bestUseBTree)
           {
             ++bestBTreeSuccess;
           }
           else
           {
             ++bestSPSuccess;
           }
         }
      }
      
      // plot and save the best solution
      if(gotBetterSol)
      {
         if(params.nonTrivialOutline.isValid())
         {
            parquetfp::Point offset(params.nonTrivialOutline.getMinX(),params.nonTrivialOutline.getMinY());
            db.shiftDesign(offset);
            db.shiftTerminals(offset);
         }

         if(params.plot)
         {
            float currAR = currXSize/currYSize;
            bool plotSlacks = !params.plotNoSlacks;
            bool plotNets = !params.plotNoNets;
            bool plotNames = !params.plotNoNames;
            if( fixedOutline ) {
              db.plot("out.plt", currArea, currWS, currAR, T.getUserTime(), 
                  currWL, plotSlacks, plotNets, plotNames,
                  fixedOutline,
                  params.nonTrivialOutline.getMinX(), 
                  params.nonTrivialOutline.getMinY(),
                  params.nonTrivialOutline.getMinX() + params.nonTrivialOutline.getXSize(),
                  params.nonTrivialOutline.getMinY() + params.nonTrivialOutline.getYSize() );
            }
            else {
              db.plot("out.plt", currArea, currWS, currAR, T.getUserTime(), 
                  currWL, plotSlacks, plotNets, plotNames);
            }
         }
         
         if(params.savePl)
            db.getNodes()->savePl(params.outPlFile.c_str());

         if(params.saveCapoPl)
            db.getNodes()->saveCapoPl(params.capoPlFile.c_str());

         if(params.saveCapo)
            db.saveCapo(params.capoBaseFile.c_str(), params.nonTrivialOutline, params.reqdAR);

         if(params.save)
            db.save(params.baseFile.c_str());
	      
         //if(db.successAR)
         //db.saveBestCopyPl("best.pl");
         if(params.nonTrivialOutline.isValid())
         {
            parquetfp::Point offset(-params.nonTrivialOutline.getMinX(),-params.nonTrivialOutline.getMinY());
            db.shiftDesign(offset);
            db.shiftTerminals(offset);
         }
      }

      BaseAnnealer::SolutionInfo curr;
      curr.area = currArea;
      curr.width = currXSize;
      curr.height = currYSize;
      curr.HPWL = currWL;

      //cout << endl << "Overall statistics: " << endl;
      //annealer->printResults(T, curr);
         
      cout << "***** DONE:  round " << (i+1) << " / "
           << params.iterations << " *****" << endl;

   } // end the for-loop
   
   aveArea /= params.iterations;
   aveWS /= params.iterations;
   aveWL /= params.iterations;
   aveWLnoWts /= params.iterations;
   totalTime /= params.iterations;
   successTime /= successAR;
   successAvgWL /= successAR;
   successAvgWLnoWts /= successAR;
   successAvgArea /= successAR;
#ifdef USEFLUTE
   aveSteinerWL /= params.iterations;
   successAveSteinerWL /= successAR;
#endif
   successAR /= params.iterations;
	
   cout << endl;
   cout << "***** SUMMARY of all rounds *****" << endl;
   cout << setw(15) << "Area: " << "Min: " << minArea << " Average: "
        << aveArea << " Max: " << maxArea << endl;
   cout << setw(15) << "HPWL: "<< "Min: " << minWL << " Average: "
        << aveWL << " Max: " << maxWL << endl;
   cout << setw(15) << "Unweighted HPWL: "<< "Min: " <<minWLnoWts<<" Average: "
        << aveWLnoWts << " Max: " << maxWLnoWts << endl;
#ifdef USEFLUTE
   if(printSteiner)
   {
     cout << setw(15) << "Steiner WL: "<< "Min: " <<minSteinerWL<<" Average: "
          << aveSteinerWL << " Max: " << maxSteinerWL << endl;
   }
#endif
   cout << setw(15) << "WhiteSpace: " << "Min: " << minWS << "% Average: "
        << aveWS << "%" << " Max: " << maxWS << "%" << endl;
   cout << "Average Time: " << totalTime << endl;
   
   if (params.reqdAR != BaseAnnealer::FREE_OUTLINE)
   {
      cout << endl;
      cout << "Success Rate of satisfying fixed outline: "
           << (100*successAR) << " %" << endl;

      if(params.FPrep == "Best" &&
         bestUsedBTree > 0 && bestUsedSP > 0)
      {
        cout << "   Success Rate of Sequence Pair: " << bestSPSuccess
             << "/" << bestUsedSP << " = " 
             << 100*static_cast<double>(bestSPSuccess)/
                    static_cast<double>(bestUsedSP) << "%" << endl;
        cout << "   Success Rate of BTree: " << bestBTreeSuccess
             << "/" << bestUsedBTree << " = "
             << 100*static_cast<double>(bestBTreeSuccess)/
                    static_cast<double>(bestUsedBTree) << "%" << endl;
      }

      if (successAR > 0)
      {
         cout << setw(15) << "Area: " << "Min: " << successMinArea 
              << " Average: "
              << successAvgArea << " Max: " << successMaxArea << endl;
         cout << setw(15) << "HPWL: "<< "Min: " << successMinWL << " Average: "
              << successAvgWL << " Max: " << successMaxWL << endl;
         cout << setw(15) << "Unweighted HPWL: "<< "Min: " << successMinWLnoWts
              << " Average: "<< successAvgWLnoWts << " Max: " 
              << successMaxWLnoWts << endl;
#ifdef USEFLUTE
         if(printSteiner)
         {
           cout << setw(15) << "Steiner WL: "<< "Min: " <<successMinSteinerWL<<" Average: "
                << successAveSteinerWL << " Max: " << successMaxSteinerWL << endl;
         }
#endif
         cout << "Average Time: " << successTime << endl;
      }
   }
   return 0;
}
