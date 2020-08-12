/**************************************************************************
 ***
 *** Copyright (c) 2000-2007 Regents of the University of Michigan,
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


#include "btreeanneal.h"
#include "btreecompact.h"
#include "basepacking.h"
#include "mixedpacking.h"
#include "mixedpackingfromdb.h"
#include "pltobtree.h"
#include "debugflags.h"

// parquet data-structures, commented out in order to compile
// #include "FPcommon.h"
// #include "DB.h"
// #include "AnalytSolve.h"
// #include "CommandLine.h"
#include "allparquet.h"

#include "skyline.h"

#include <ABKCommon/abkcommon.h>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <iterator>
#include <deque>

using parquetfp::Node;
using parquetfp::Nodes;
using parquetfp::DB;
using parquetfp::Command_Line;
using parquetfp::AnalytSolve;
using std::cout;
using std::endl;
using std::min;
using std::max;
using std::cin;
using std::vector;


static void getObstaclesFromDB(DB *const db, BasePacking &out);

// ========================================================
BTreeAreaWireAnnealer::BTreeAreaWireAnnealer(
    MixedBlockInfoType& nBlockinfo)
: BaseAnnealer(),
  _blockinfo_cleaner(NULL),
  _blockinfo(nBlockinfo),
  blockinfo(nBlockinfo),
  in_curr_solution(_blockinfo.currDimensions),
  in_next_solution(_blockinfo.currDimensions),
  in_best_solution(_blockinfo.currDimensions),
  _slackEval(NULL)
{}
// --------------------------------------------------------
BTreeAreaWireAnnealer::BTreeAreaWireAnnealer(
    MixedBlockInfoType& nBlockinfo,
    const Command_Line *const params,
    DB *const db)
: BaseAnnealer(params, db),
  _blockinfo_cleaner(NULL),
  _blockinfo(nBlockinfo),
  blockinfo(nBlockinfo),
  in_curr_solution(_blockinfo.currDimensions),
  in_next_solution(_blockinfo.currDimensions),
  in_best_solution(_blockinfo.currDimensions)
{
  getObstaclesFromDB(db, _obstacleinfo);
  _obstacleFrame[0] = db->getObstacleFrame()[0];
  _obstacleFrame[1] = db->getObstacleFrame()[1];
  in_curr_solution.addObstacles(_obstacleinfo, _obstacleFrame);
  in_next_solution.addObstacles(_obstacleinfo, _obstacleFrame);
  in_best_solution.addObstacles(_obstacleinfo, _obstacleFrame);
  // NOTE: must construct slackEval after trees have been filled with
  // obstacles, or btreeslackeval won't get obstacles
  _slackEval = new BTreeSlackEval(in_curr_solution); 
  constructor_core();
}
// --------------------------------------------------------
BTreeAreaWireAnnealer::BTreeAreaWireAnnealer(
    const Command_Line *const params,
    DB *const db)
  : BaseAnnealer(params, db),
  _blockinfo_cleaner(static_cast<MixedBlockInfoType*>
      (new MixedBlockInfoTypeFromDB(*db))),
  _blockinfo(*_blockinfo_cleaner),
  blockinfo(_blockinfo),
  in_curr_solution(_blockinfo.currDimensions),
  in_next_solution(_blockinfo.currDimensions),
  in_best_solution(_blockinfo.currDimensions)
{
  getObstaclesFromDB(db, _obstacleinfo);
  _obstacleFrame[0] = db->getObstacleFrame()[0];
  _obstacleFrame[1] = db->getObstacleFrame()[1];
  in_curr_solution.addObstacles(_obstacleinfo, _obstacleFrame);
  in_next_solution.addObstacles(_obstacleinfo, _obstacleFrame);
  in_best_solution.addObstacles(_obstacleinfo, _obstacleFrame);
  // NOTE: must construct slackEval after trees have been filled with
  // obstacles or btreeslackeval won't get obstacles
  _slackEval = new BTreeSlackEval(in_curr_solution);
  constructor_core();
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::constructor_core()
{
  // mgwoo: -noRotation bug fixed here
  Nodes* theNodes = _db->getNodes();
  unsigned numNodes = _db->getNumNodes();
  for(unsigned i = 0; i < numNodes; ++i)
  {
    Node &node = theNodes->getNode(i);
    if(_params->noRotation ||
       (!node.isOrientFixed() && node.isHard() &&
        equalFloat(node.getminAR(), 1.f) && node.allPinsAtCenter))
    {
      node.putIsOrientFixed(true);
    }
  }
  // initialize the orientation map
  _physicalOrient.resize(in_curr_solution.NUM_BLOCKS);
  for (int i = 0; i < in_curr_solution.NUM_BLOCKS; i++)
  {      
    _physicalOrient[i].resize(blockinfo.Orient_Num);
    for (int theta = 0; theta < blockinfo.Orient_Num; theta++)
    {
      parquetfp::Node& currBlock = _db->getNodes()->getNode(i);
      if (currBlock.isOrientFixed()) {
        _physicalOrient[i][theta] = parquetfp::N;
      }
      else {
        _physicalOrient[i][theta] = parquetfp::ORIENT(theta);
        //            _physicalOrient[i][theta] = parquetfp::N;
        //            cout << "currentOrient: " << _physicalOrient[i][theta] << endl;
      }
    }
  }

  // initialize the dimensions of soft blocks
  for (int i = 0; i < in_curr_solution.NUM_BLOCKS; i++)
    if (blockinfo.blockARinfo[i].isSoft)
    {
      float blockAR =
        max(blockinfo.blockARinfo[i].minAR[0],
            min(blockinfo.blockARinfo[i].maxAR[0], float(1.0))); // minAR <= AR <= maxAR
      float blockWidth =
        sqrt(blockinfo.blockARinfo[i].area * blockAR);
      float blockHeight =
        sqrt(blockinfo.blockARinfo[i].area / blockAR);

      _blockinfo.setBlockDimensions(i, blockWidth, blockHeight, 0);
    }

  // generate an initial solution
  GenerateRandomSoln(in_curr_solution, blockinfo.currDimensions.blocknum());
  in_best_solution = in_curr_solution;
  in_next_solution = in_curr_solution;

  // initialize the orientations of nodes in *_db if necessary
  for (int i = 0; i < in_curr_solution.NUM_BLOCKS; i++)
  {
    int initOrient = int(in_curr_solution.tree[i].orient);
    initOrient = _physicalOrient[i][initOrient];

    float initWidth = in_curr_solution.width(i);
    float initHeight = in_curr_solution.height(i);

    _db->getNodes()->changeOrient(i, parquetfp::ORIENT(initOrient),
        *(_db->getNets()));
    _db->getNodes()->putNodeWidth(i, initWidth);
    _db->getNodes()->putNodeHeight(i, initHeight);
  }


  // -----debug messages-----
  for (int i = 0; i < in_curr_solution.NUM_BLOCKS; i++)
  {
    for (int thetaIt = 0; thetaIt < blockinfo.Orient_Num; thetaIt++)
      if ((_db->getNodes()->getNode(i)).isOrientFixed() &&
          int(in_curr_solution.tree[i].orient) != 0)
      {
        cout << "ctor: orient of block[" << i << "] should be \"n\"" << endl;
        printf("in_curr_solution:orient %d\n",
            int(in_curr_solution.tree[i].orient));
        cin.get();
      }

    if (_params->minWL && (int(in_curr_solution.tree[i].orient) !=
          int(_db->getNodes()->getNode(i).getOrient())))
    {
      cout << "ctor: orient of block[" << i << "] is not consistent" << endl;
      printf("in_curr_solution:orient %d vs. _db->orient: %d\n",
          int(in_curr_solution.tree[i].orient),
          int(_db->getNodes()->getNode(i).getOrient()));
      cin.get();
    }

    int theta = in_curr_solution.tree[i].orient;
    if (std::abs(in_curr_solution.width(i) - blockinfo.currDimensions[i].width[theta]) > 1e-6)
    {
      printf("ctor: width of block[%d] is not consistent.  in_curr_soln: %.2f vs. blockinfo: %.2f\n",
          i, in_curr_solution.width(i), blockinfo.currDimensions[i].width[theta]);
      cin.get();
    }

    if (std::abs(in_curr_solution.height(i) - blockinfo.currDimensions[i].height[theta]) > 1e-6)
    {
      printf("ctor: height of block[%d] is not consistent.  in_curr_soln: %.2f vs. blockinfo: %.2f\n",
          i, in_curr_solution.height(i), blockinfo.currDimensions[i].height[theta]);
      cin.get();
    }

    if (_params->minWL && std::abs(in_curr_solution.width(i) - _db->getNodes()->getNodeWidth(i)) > 1e-6)
    {
      printf("ctor: width of block[%d] is not consistent.  in_curr_solution: %.2f vs._db: %.2f\n",
          i, in_curr_solution.width(i), _db->getNodes()->getNodeWidth(i));
      cin.get();
    }
  }
}
// --------------------------------------------------------
bool BTreeAreaWireAnnealer::go()
{      
  Timer T;

  DBfromSoln(in_curr_solution);

  //turn off rotation for hard square macros
  //with pins in the center
  Nodes* theNodes = _db->getNodes();
  unsigned numNodes = _db->getNumNodes();
  for(unsigned i = 0; i < numNodes; ++i)
  {
    Node &node = theNodes->getNode(i);
    if(_params->noRotation ||
       (!node.isOrientFixed() && node.isHard() &&
        equalFloat(node.getminAR(), 1.f) && node.allPinsAtCenter))
    {
      node.putIsOrientFixed(true);
    }
  }
  // DEBUG::
  //   for(int i=0; i<numNodes; i++) {
  //      Node &node = theNodes->getNode(i);
  //      cout << "i: " << node.isOrientFixed() << endl;
  //      cout << node.getName() << " " << node.getWidth() << " " << node.getHeight () << endl;
  //   }

  bool success = false;
  if (in_curr_solution.NUM_BLOCKS > 1)
    success = anneal();
  else
    success = packOneBlock();
  T.stop();

  annealTime += T.getUserTime();

//  cout << "before DB from soln" << endl;
  // update *_db for locs, dimensions and slacks
  DBfromSoln(in_curr_solution);
//  cout << "after DB from soln" << endl;

//  for(int i=0; i<in_curr_solution.xloc().size(); i++) {
//    cout << i << " " << in_curr_solution.xloc()[i] << " " << in_curr_solution.yloc()[i] << endl;
//  }

  in_best_solution = in_curr_solution;

  // print solutions
  SolutionInfo currSoln;
  currSoln.area = in_curr_solution.totalArea();
  currSoln.width = in_curr_solution.totalWidth();
  currSoln.height = in_curr_solution.totalHeight();
  bool useWts = true;
#ifdef USEFLUTE
  currSoln.HPWL = _db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
  currSoln.HPWL = _db->evalHPWL(useWts,_params->scaleTerms);
#endif
  printResults(T, currSoln);

  return success;
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::DBfromSoln(const BTree& soln)
{
  _db->updatePlacement(const_cast<vector<float>&>(soln.xloc()),
      const_cast<vector<float>&>(soln.yloc()));
  for (int i = 0; i < soln.NUM_BLOCKS; i++)
  {
    parquetfp::ORIENT newOrient =
      parquetfp::ORIENT(soln.tree[i].orient);
    _db->getNodes()->changeOrient(i, newOrient, *(_db->getNets()));
    _db->getNodes()->putNodeWidth(i, soln.width(i));
    _db->getNodes()->putNodeHeight(i, soln.height(i));
  }

//  cout << "Before evaluate Slacks: " << endl;
//  for(int i=0; i<soln.xloc().size(); i++) {
//    cout << i << " " << soln.xloc()[i] << " " << soln.yloc()[i] << endl;
//  }

  _slackEval->evaluateSlacks(soln);

//  cout << "After evaluate Slacks: " << endl;
//  for(int i=0; i<soln.xloc().size(); i++) {
//    cout << i << " " << soln.xloc()[i] << " " << soln.yloc()[i] << endl;
//  }


  int blocknum = soln.NUM_BLOCKS;
  vector<float> xSlacks(blocknum); // % x-slacks
  vector<float> ySlacks(blocknum); // % y-slacks
  float totalWidth = soln.totalWidth();
  float totalHeight = soln.totalHeight();
  for (int i = 0; i < blocknum; i++)
  {
    xSlacks[i] = (_slackEval->xSlack()[i] / totalWidth) * 100;
    ySlacks[i] = (_slackEval->ySlack()[i] / totalHeight) * 100;
  }      
  _db->updateSlacks(xSlacks, ySlacks);

#ifdef PARQUET_DEBUG_HAYWARD_ASSERT_BTREEANNEAL
  for (int i = 0; i < blocknum; i++)
  {
    int theta = soln.tree[i].orient;
    if (theta != _physicalOrient[i][theta])
    {
      printf("block: %d theta: %d physicalOrient: %d\n",
          i, theta, _physicalOrient[i][theta]);
      cin.get();
    }
  }   
#endif
}
// --------------------------------------------------------
bool BTreeAreaWireAnnealer::packOneBlock()
{
  const float blocksArea = in_curr_solution.blockArea();
  const float reqdAR = _params->reqdAR;
  const float reqdArea = blocksArea * (1+(_params->maxWS/100.0));
  const float reqdWidth = sqrt(reqdArea * reqdAR);
  const float reqdHeight = reqdWidth / reqdAR;

  const vector<float> defaultXloc(1, 0); // 1 copy of "0"
  const vector<float> defaultYloc(1, 0);
  const int defaultOrient = 0;

  if(_params->verb.getForActions() > 0)
    cout << "Only one block is detected, deterministic algo used." << endl;

  if (_params->reqdAR != FREE_OUTLINE && _params->verb.getForMajStats() > 0)
    cout << "outline width: " << reqdWidth
      << " outline height: " << reqdHeight << endl;

  int bestOrient = defaultOrient; 

  float bestHPWL = basepacking_h::Dimension::Infty;
  bool success = false;
  for (int theta = 0;
      theta < basepacking_h::Dimension::Orient_Num; theta++) 
  {
    if(_db->getNodes()->getNode(0).isOrientFixed() && theta > 0) break;

    in_curr_solution.rotate(0, theta);

    float blockWidth = in_curr_solution.width(0);
    float blockHeight = in_curr_solution.height(0);
    bool fitsInside = ((_params->reqdAR == FREE_OUTLINE) ||
        ((blockWidth <= reqdWidth) &&
         (blockHeight <= reqdHeight)));

    if(_params->verb.getForMajStats() > 0)
      cout << "orient: " << theta
        << " width: " << blockWidth
        << " height: " << blockHeight
        << " inside: " << ((fitsInside)? "T" : "F");

    success = success || fitsInside;
    if (fitsInside && _params->minWL)
    {
      DBfromSoln(in_curr_solution);
      bool useWts = true;
#ifdef USEFLUTE
      float currHPWL = _db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
      float currHPWL = _db->evalHPWL(useWts,_params->scaleTerms);
#endif

      if(_params->verb.getForMajStats() > 0)
        cout << " HPWL: " << currHPWL;
      if (currHPWL < bestHPWL)
      {
        bestOrient = theta;
        bestHPWL = currHPWL;
      }
    }
    else if (fitsInside)
      bestOrient = theta;

    if(_params->verb.getForMajStats() > 0)
      cout << endl;
  }

  in_curr_solution.rotate(0, bestOrient);
  DBfromSoln(in_curr_solution);

  return success;
}
// ---------------------------------------------------------

bool BTreeAreaWireAnnealer::anneal()
{
  // options
  const bool budgetTime = _params->budgetTime;
  float seconds = _params->seconds;  
  const bool minWL = _params->minWL;

  // input params
  const float wireWeight = _params->wireWeight;
  const float areaWeight = _params->areaWeight;
  const float ARWeight = max(1 - areaWeight - wireWeight, float(0.0));

  // input params
  const float blocksArea = _db->getNodesArea();
  const float size = in_curr_solution.NUM_BLOCKS;

  float currTime = _params->startTime;

  // if any constraint is imposed
  const float reqdAR = _params->reqdAR;
  //   const float reqdArea = blocksArea * (1+((_params->maxWS-1)/100.0));
  //   const float reqdWidth = sqrt(reqdArea*reqdAR);
  //   const float reqdHeight = reqdWidth/reqdAR;

  // const float real_reqdAR = _params->reqdAR;
  const float real_reqdArea = blocksArea * (1+(_params->maxWS/100.0));
  const float real_reqdWidth = sqrt(real_reqdArea*reqdAR);
  const float real_reqdHeight = real_reqdWidth / reqdAR;
//  const float maxDist = sqrt(real_reqdWidth * real_reqdWidth * 
//      real_reqdHeight * real_reqdHeight);
  

  // save attributes of the best solution
  // float bestArea = FLT_MAX;
  // float bestHPWL = FLT_MAX;

  // global counters and book-keepers
  int move = UNINITIALIZED;
  int count = 0;
  int prev_move = UNINITIALIZED;

  unsigned int timeChangeCtr = 0;
  unsigned int moveSelect = UNSIGNED_UNINITIALIZED;
  unsigned int iter = UNSIGNED_UNINITIALIZED;
  unsigned int masterMoveSel = 0;

  bool moveAccepted = false; 
  bool brokeFromLoop = false;

  float unit=0, total=seconds, percent=1;
  unsigned int moves = 1000;

  //Added by DAP to support terminate on 
  //acceptence ratio less than 0.5%
  //unsigned accept_ct = 0;
  bool saved_best=false;


  Timer looptm;
  looptm.stop();

  _db->updatePlacement(const_cast<vector<float>&>(in_curr_solution.xloc()),
      const_cast<vector<float>&>(in_curr_solution.yloc()));
  bool useWts = true;
#ifdef USEFLUTE
  float currHPWL = _db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
  float currHPWL = _db->evalHPWL(useWts,_params->scaleTerms);
#endif

//  float currDist = in_curr_solution.getDistance(real_reqdWidth, real_reqdHeight);

  SkylineContour mapY(in_curr_solution, true);
  float currWastedArea = 
    in_curr_solution.totalContourArea()
    + mapY.GetContourArea() 
    - 2.0 * in_curr_solution.blockArea();



  // calculate wastedArea 
//  SkylineContour mapX(in_curr_solution), mapY(in_curr_solution, true);
//  float currWastedContArea 
//        = mapX.GetContourArea() 
//        + mapY.GetContourArea() 
//        - 2.0 * in_curr_solution.blockArea();


  float bestHPWL = currHPWL;
  float bestArea = std::numeric_limits<float>::max();
//  float bestDist = std::numeric_limits<float>::min();
  float bestWastedArea = std::numeric_limits<float>::max();

  while(currTime > _params->timeCool || budgetTime)
  {
    brokeFromLoop = false;
    iter = 0;

    // ----------------------------------------
    // an iteration under the same termperature
    // ----------------------------------------
    do
    {
      // ------------------------------------
      // special treatment when time is fixed
      // ------------------------------------
      if (budgetTime)
      {
        if (count==0)
        {
          looptm.start(0.0);
        }
        else if (count==1000)
        {  
          looptm.stop();               
          unit = looptm.getUserTime() / 1000;
          if (unit == 0)
          {
            unit = 10e-6f;
          }
          seconds -= looptm.getUserTime();
          if(_params->verb.getForMajStats() > 0)
            cout << int(seconds/unit) << " moves left with "
              << (unit*1000000) <<" micro seconds per move." << endl;
          moves = unsigned(seconds/unit/125);// moves every .08 degree
        }
        else if (count > 1000)
        {
          seconds -= unit;
          if (seconds <= 0)
          {
            if(_params->verb.getForMajStats() > 0)
              cout << "TimeOut" << endl;
            return false;
          }
        }
      }
      else
      {
        if (count==0)
        {
          looptm.start(0.0);
        }
        else if (count==1000)
        {  
          looptm.stop();               
          unit = looptm.getUserTime() / 1000;
          if (unit == 0)
          {
            unit = 10e-6f;
          }
          if(_params->verb.getForMajStats() > 0)
            cout << (unit * 1e6) <<" micro seconds per move." << endl;
        }
      }         
      // finish treating time if necessary 

      // -----------------------
      // select and apply a move
      // -----------------------

      // current solution, "currHPWL" updated only when necessary
      // "currWastedArea" also updated only when necessary
      float currArea = in_curr_solution.totalArea();
      float currHeight = in_curr_solution.totalHeight();
      float currWidth = in_curr_solution.totalWidth();
      float currAR = currWidth / currHeight;

      ++count; 
      ++iter;
      prev_move = move;

      // -----select the types of moves here-----
      if (_params->softBlocks && currTime < 50)
        masterMoveSel = rand() % 1000;
      moveSelect = rand() % 1000;

      // -----take action-----
      int indexOrient = UNSIGNED_UNINITIALIZED;
      parquetfp::ORIENT newOrient = parquetfp::N;
      parquetfp::ORIENT oldOrient = parquetfp::N;

      int index = UNINITIALIZED;
      float newWidth = UNINITIALIZED;
      float newHeight = UNINITIALIZED;

      //         float deadspacePercent = (currArea / blocksArea - 1) * 100;
      if(_params->softBlocks &&
          //            deadspacePercent < _params->startSoftMovePercent &&
          masterMoveSel == 1)
        move = packSoftBlocks(2); // needs its return-value (-1)!!!

      else if(_params->softBlocks &&
          //                 deadspacePercent < _params->startSoftMovePercent &&
          masterMoveSel > 950)
        move = makeSoftBlMove(index, newWidth, newHeight);

      else if (((reqdAR-currAR)/reqdAR > 0.00005 || 
            (currAR-reqdAR)/reqdAR > 0.00005) && 
          (timeChangeCtr % 4) == 0 && reqdAR != FREE_OUTLINE)
      {            
        //             move = makeMove(indexOrient, newOrient, oldOrient);
        move = makeARMove();
      }

      else if (/*false && !minWL &&*/
          currTime < 30 && (timeChangeCtr % 5) == 0) {
//        move = compactBlocks();  
      }

      else if (moveSelect < 150)
      {
        if (reqdAR != FREE_OUTLINE)
          move = makeARMove();
        else 
          move = makeMove(indexOrient, newOrient, oldOrient);
      }

      else if (moveSelect < 300 && minWL)
      {
        if (reqdAR != FREE_OUTLINE)
          move = makeARWLMove();
        else
          move = makeHPWLMove();
      }

      else
        move = makeMove(indexOrient, newOrient, oldOrient);

      // -----additional book-keeping for special moves-----
      // for orientation moves
      if (move == REP_SPEC_ORIENT || move == ORIENT)
      {
        // temp values
        if (minWL)
        {
          _db->getNodes()->changeOrient(indexOrient, newOrient,
              *(_db->getNets()));
        }
      }

      // for soft-block moves
      if (move == SOFT_BL)
      {
        int indexTheta = in_curr_solution.tree[index].orient;
        _blockinfo.setBlockDimensions(index,
            newWidth, newHeight, indexTheta);
//        cout << "Next Solution Evaluate" << endl;
        in_next_solution.evaluate(in_curr_solution.tree);

        if (minWL)
        {
          _db->getNodes()->putNodeWidth(index, newWidth);
          _db->getNodes()->putNodeHeight(index, newHeight);
        }
      }            

      // attributes of "in_next_solution"
      float tempHeight = in_next_solution.totalHeight(); 
      float tempWidth = in_next_solution.totalWidth();  
      float tempArea = in_next_solution.totalArea();
      float tempAR = tempWidth / tempHeight;
      float tempHPWL = UNINITIALIZED;
//      float tempDist = in_next_solution.getDistance( real_reqdWidth, real_reqdHeight);

      SkylineContour mapY(in_next_solution, true);

      float tempWastedArea = 
        in_next_solution.totalContourArea()
        + mapY.GetContourArea()
        - 2.0 * in_next_solution.blockArea();
      

      // -----------------------------------------------------
      // evaulate the temporary solution and calculate "delta"
      // -----------------------------------------------------

      float deltaArea = 0;
      float deltaHPWL = 0;  
      float deltaAR = UNINITIALIZED;
      float delta = UNINITIALIZED;
//      float deltaDist = UNINITIALIZED;
      float deltaWastedArea= UNINITIALIZED;

      /* area objective */
      if (currTime > 30)
        deltaArea = ((tempArea-currArea)*1.2*_params->timeInit) / blocksArea;
      else
        deltaArea = ((tempArea-currArea)*1.5*_params->timeInit) / blocksArea;


      /* area objective */
      if (currTime > 30)
        deltaWastedArea= ((tempWastedArea-currWastedArea)*1.2*_params->timeInit) / blocksArea;
      else
        deltaWastedArea= ((tempWastedArea-currWastedArea)*1.5*_params->timeInit) / blocksArea;

      
//      deltaDist = -1 * ((tempDist-currDist) * _params->timeInit) / maxDist;

      /* HPWL objective if applicable */ 
      if (minWL)
      {
        _db->updatePlacement(const_cast<vector<float>&>(in_next_solution.xloc()),
            const_cast<vector<float>&>(in_next_solution.yloc()));
//        cout << "placement Info Updated! curr" << endl;
//        for(int i=0; i<in_curr_solution.xloc().size(); i++) {
//          cout << i << " " << in_curr_solution.xloc(i) << " " << in_curr_solution.yloc(i) << endl;
//        }
//        cout << "placement Info Updated! next" << endl;
//        for(int i=0; i<in_next_solution.xloc().size(); i++) {
//          cout << i << " " << in_next_solution.xloc(i) << " " << in_next_solution.yloc(i) << endl;
//        }

#ifdef USEFLUTE
        tempHPWL = _db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
        tempHPWL = _db->evalHPWL(useWts,_params->scaleTerms);
#endif
        if(currHPWL == 0)
          deltaHPWL = 0;
        else
        {
          if(currTime>30)
            deltaHPWL = ((tempHPWL-currHPWL)*1.2*_params->timeInit)/currHPWL; // 1.2
          else
            deltaHPWL = ((tempHPWL-currHPWL)*1.5*_params->timeInit)/currHPWL; // 1.5
        }
      }

      //          if (_isFixedOutline)
      //          {
      //             /* max(x-viol, y-viol) objective */
      //             if((tempHeight-currHeight) > (tempWidth-currWidth))
      //                deltaArea=((tempHeight-currHeight)*1.3*_params->timeInit)/(real_reqdHeight);
      //             else
      //                deltaArea=((tempWidth-currWidth)*1.3*_params->timeInit)/(real_reqdWidth);

      // //          /* x-viol + y-viol objective */
      // //          deltaArea=0.5*1.3*_params->timeInit*
      // //             (abs(tempHeight-currHeight) / (reqdHeight) +
      // //              abs(tempWidth-currWidth) / (reqdWidth));         
      //          }

      //if (_isFixedOutline && getNumObstacles())
      //{ // XXX <aaronnn> compute cost using outline violations, rather than AR
      //  // when obstacles are present (because AR is meaningless when there are obstacles)
      //  // max(x-viol, y-viol) objective

      //    float deltaHeight = ((tempHeight-currHeight)*1.3*_params->timeInit)/(real_reqdHeight);
      //    float deltaWidth = ((tempWidth-currWidth)*1.3*_params->timeInit)/(real_reqdWidth);

      //    if (deltaHeight > 0 && deltaWidth > 0) 
      //   	 deltaArea = deltaHeight + deltaWidth;
      //    else if (deltaHeight > 0)
      //   	 deltaArea = deltaHeight;
      //    else if (deltaWidth > 0)
      //   	 deltaArea = deltaWidth;
      //    else 
      //   	 deltaArea = deltaHeight + deltaWidth;
      //}

      /* AR and overall objective */
      delta = deltaArea;
      if(reqdAR != FREE_OUTLINE)
      {            
        deltaAR = ((tempAR - reqdAR)*(tempAR - reqdAR) -
            (currAR - reqdAR)*(currAR - reqdAR)) * 20 * _params->timeInit; // 10 // 1.2

        // This Function !!!
        if(minWL) {
//          delta = (areaWeight * deltaArea +
//              wireWeight * deltaHPWL +
//              ARWeight * deltaAR);
//          delta = 0.2 * deltaHPWL +
//                  0.6 * deltaWastedContArea;        
//          delta = 0.2 * deltaHPWL + 0.4 * deltaAR + 0.4 * deltaDist;
          delta = 0.2 * deltaHPWL + 0.4 * deltaAR + 0.4 * deltaWastedArea;
//          delta = 0.4 * deltaAR + 0.8 * deltaWastedArea;
//          cout << "c: " << count << " dHpwl: " << deltaHPWL 
//            << " dAR:" << deltaAR << " dWArea:" << deltaWastedArea<< endl;
  
        }
        else
          delta = ((areaWeight + wireWeight/2.0) * deltaArea + 
              (ARWeight + wireWeight/2.0) * deltaAR);

      }
      else if(minWL)
      {
        delta = ((areaWeight + ARWeight/2.0)*deltaArea + 
            (wireWeight + ARWeight/2.0)*deltaHPWL);
      }
      else
        delta = deltaArea;
      // finish calculating "delta"        

      // --------------------------------------------------
      // decide whether a move is accepted based on "delta"
      // --------------------------------------------------

//      cout << "Iter: " << iter << " Delta: " << delta << endl;

      if (delta < 0 || move == MISC)
        moveAccepted = true;         
      else if (currTime > _params->timeCool) 
        // become greedy below time > timeCool
      {
        float ran = rand() % 10000;
        float r = float(ran) / 9999;
        if (r < exp(-1*delta/currTime))
          moveAccepted = true;
        else
          moveAccepted = false;
      }
      else
        moveAccepted = false;


      // -----update current solution if accept-----
      if (moveAccepted && move != MISC)
      {
//        cout << "Move Accepted!!" << endl;
        in_curr_solution = in_next_solution;
        currHPWL = tempHPWL;
//        currDist = tempDist;
        currWastedArea = tempWastedArea;
      }

      // -----additional book-keeping for special moves-----
      if (move == REP_SPEC_ORIENT || move == ORIENT)
      {
        // if move not accepted, then put back "oldOrient"
        parquetfp::ORIENT actualOrient =
          parquetfp::ORIENT(in_curr_solution.tree[indexOrient].orient);

        if (minWL) {
          _db->getNodes()->changeOrient(indexOrient, actualOrient,
              *(_db->getNets()));
//          cout << "After Changing Orient" << endl;
//          cout << ""
        }
      }

      if (move == SOFT_BL)
      {
        // if move not accepted, then put back "oldWidth/Height"
        float actualWidth = in_curr_solution.width(index);
        float actualHeight = in_curr_solution.height(index);
        int actualTheta = in_curr_solution.tree[index].orient;
        _blockinfo.setBlockDimensions(index,
            actualWidth, actualHeight,
            actualTheta);

        if (minWL)
        {
          _db->getNodes()->putNodeWidth(index, actualWidth);
          _db->getNodes()->putNodeHeight(index, actualHeight);
        }
      }

      // check the best updates
      bool updateBest = false;
      if (_params->reqdAR != FREE_OUTLINE)
      {
        if (minWL)
        {
          updateBest = (currWidth <= real_reqdWidth &&
              currHeight <= real_reqdHeight &&
              currHPWL < bestHPWL &&
              currWastedArea < bestWastedArea );
//              currDist > bestDist);
//          cout << "HPWL: " << currHPWL << " " << bestHPWL << endl;
        }
        else
        {
          updateBest = (currWidth <= real_reqdWidth &&
              currHeight <= real_reqdHeight &&
              currArea < bestArea);
        }
      }
      else if (minWL)
      {
        float currCost =
          areaWeight * currArea + wireWeight * currHPWL;
        float bestCost =
          areaWeight * bestArea + wireWeight * bestHPWL;

        updateBest = (currCost < bestCost);
      }
      else
        updateBest = (currArea < bestArea);

      if (updateBest) 
      {
        saved_best=true;
        bestHPWL = currHPWL;
        bestArea = currArea;
//        bestDist = currDist;
        bestWastedArea = currWastedArea;
        in_best_solution = in_curr_solution;
      }
      //}

      // ------------------------------
      // special terminating conditions
      // ------------------------------

      // Final Termination!!
      if (minWL/* && _params->startTime > 100*/)//for lowT anneal don't have this condition
      {
        // hhchan TODO:  clean-up this code mess 
        if(currArea <= real_reqdArea && currHeight <= real_reqdHeight && 
            currWidth <= real_reqdWidth && reqdAR != FREE_OUTLINE && currTime < 5) {
//          cout << "case1 !!!" << endl;
          return true;
        }

        if (reqdAR != FREE_OUTLINE && currTime < 5 && 
            _params->dontClusterMacros && _params->solveTop)
        {
          float widthWMacroOnly = _db->getXSizeWMacroOnly();
          float heightWMacroOnly = _db->getYSizeWMacroOnly();

          if (widthWMacroOnly <= real_reqdWidth &&
              heightWMacroOnly <= real_reqdHeight) {
//            cout << "case2 !!!" << endl;
            return true;
          }
        }
      }
      else
      {
        if(currArea <= real_reqdArea && currHeight <= real_reqdHeight && 
            currWidth <= real_reqdWidth && reqdAR != FREE_OUTLINE) 
        {
//          cout << "case3 !!!" << endl;
          return true;
        }
      }

      // Hard limit for moves
      if (iter >= moves)
        break;

#ifdef PARQUET_DEBUG_HAYWARD_ASSERT_BTREEANNEAL
      // -----debugging messages-----
      for (int i = 0; i < in_curr_solution.NUM_BLOCKS; i++)
      {
        if (minWL && (int(in_curr_solution.tree[i].orient) !=
              int(_db->getNodes()->getNode(i).getOrient())))
        {
          cout << "round [" << count << "]: orient of block[" << i << "] is not consistent" << endl;
          printf("in_curr_solution:orient %d vs. _db->orient: %d, move: %d",
              int(in_curr_solution.tree[i].orient),
              int(_db->getNodes()->getNode(i).getOrient()), move);
          cin.get();
        }

        int theta = in_curr_solution.tree[i].orient;
        if (theta != int(_physicalOrient[i][theta]))
        {
          printf("round[%d]: orient of block[%d] is not consistent, in_curr_soln: %d vs phyOrient: %d move: %d\n",
              count, i, theta, _physicalOrient[i][theta], move);
          cin.get();
        }

        if (std::abs(in_curr_solution.width(i) - blockinfo.currDimensions[i].width[theta]) > 1e-6)
        {
          printf("round[%d]: width of block[%d] is not consistent.  in_curr_soln: %.2f vs. blockinfo: %.2f move: %d prevMove: %d\n",
              count, i, in_curr_solution.width(i), blockinfo.currDimensions[i].width[theta], move, prev_move);
          printf("round[%d]: oldWidth: %.2f oldHeight: %.2f newWidth: %.2f newHeight: %.2f moveAccepted: %s\n",
              count, -1.0, -1.0, newWidth, newHeight, (moveAccepted)? "T" : "F");
          cin.get();
        }

        if (std::abs(in_curr_solution.height(i) - blockinfo.currDimensions[i].height[theta]) > 1e-6)
        {
          printf("round[%d]: height of block[%d] is not consistent.  in_curr_soln: %.2f vs. blockinfo: %.2f move: %d prevMove: %d\n",
              count, i, in_curr_solution.height(i), blockinfo.currDimensions[i].height[theta], move, prev_move);
          printf("round[%d]: oldWidth: %.2f oldHeight: %.2f newWidth: %.2f newHeight: %.2f moveAccepted: %s\n",
              count, -1.0, -1.0, newWidth, newHeight, (moveAccepted)? "T" : "F");
          cin.get();
        }

        if (minWL && std::abs(in_curr_solution.width(i) - _db->getNodes()->getNodeWidth(i)) > 1e-6)
        {
          printf("round[%d]: width of block[%d] is not consistent.  in_curr_solution: %.2f vs._db: %.2f move: %d\n",
              count, i, in_curr_solution.width(i), _db->getNodes()->getNodeWidth(i), move);
          cin.get();
        }

        if (in_curr_solution.width(i) < 1e-10 ||
            in_curr_solution.height(i) < 1e-10)
        {
          printf("round[%d]: width of block[%d]: %f height: %f move: %d\n",
              count, i, in_curr_solution.width(i), in_curr_solution.height(i), move);
          cin.get();
        }
      }
      // -----end of debugging messages-----
#endif        
//      cout << "iter: " << iter << " size: " << 4*size << " bTime: " << budgetTime << endl;
    }
//    while (iter < 4*size || budgetTime);
    while (iter < 10*size || budgetTime);

//    cout << "whileBreak Iter: " << iter << endl;
    // finish the loop under constant temperature

    // -----------------------------
    // update temperature "currTime"
    // -----------------------------

    float alpha = UNINITIALIZED;
    ++timeChangeCtr;
    if (budgetTime)
    {
      percent = seconds/total;

      if (percent < 0.66666 && percent > 0.33333)
        alpha = 0.9f;
      else if (percent < 0.33333 && percent > 0.16666)
        alpha = 0.95f;
      else if (percent < 0.16666 && percent > 0.06666)
        alpha = 0.96f;
      else if(percent <.06666 && percent >.00333)
        alpha = 0.8f;
      else if(percent <.00333 && percent >.00003)
        alpha = 0.98f;
      else
        alpha = 0.85f;
    }
    else
    {
      if (currTime < 2000 && currTime > 1000)
        alpha = 0.9f;
      else if (currTime < 1000 && currTime > 500)
        alpha = 0.95f;
      else if (currTime < 500 && currTime > 200)
        alpha = 0.96f;
      else if (currTime < 200 && currTime > 10)
        alpha = 0.96f;
      else if (currTime < 15 && currTime > 0.1)
        alpha = 0.98f;
      else
        alpha = 0.85f;
    }
//    cout << "currTime: " << currTime << " -> ";
    currTime *= alpha;
//    cout << currTime << endl;
    if (brokeFromLoop)
      break;
  }
  if(saved_best)
    in_curr_solution = in_best_solution;
    //   if(_params->verb.getForActions() > 0)
//  cout << "NumMoves attempted: " << count << endl;
  if (reqdAR != FREE_OUTLINE)
    return false;
  else
    return true;
}

// --------------------------------------------------------
void BTreeAreaWireAnnealer::takePlfromDB()
{
  Pl2BTree converter(_db->getXLocs(),
      _db->getYLocs(),
      _db->getNodeWidths(),
      _db->getNodeHeights(),
      Pl2BTree::TCG);
  in_curr_solution.evaluate(converter.btree());
  in_next_solution = in_curr_solution;
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::compactSoln(bool minWL, bool fixedOutline, float reqdH, float reqdW)
{
  //TO DO: have this function check for WL and fixedOutline
  BTreeCompactor compactor(in_curr_solution);

  int round = 0;
  int numBlkChange = compactor.compact();
  float lastArea = in_curr_solution.totalArea();
  float currArea = compactor.totalArea();
  while (numBlkChange > 0)
  {
    if(_params->verb.getForActions() > 0 || _params->verb.getForMajStats() > 0)
      printf("round[%d] %d blks moved, area: %.2f -> %.2f\n",
          round, numBlkChange, lastArea, currArea);

    numBlkChange = compactor.compact();
    round++;
    lastArea = currArea;
    currArea = compactor.totalArea();
  }
  if(_params->verb.getForActions() > 0 || _params->verb.getForMajStats() > 0)
    printf("round[%d] %d blks moved, area: %.2f -> %.2f\n",
        round, numBlkChange, lastArea, currArea);

  in_curr_solution = compactor;
  DBfromSoln(in_curr_solution);
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::makeMove(int& indexOrient,
    parquetfp::ORIENT& newOrient,
    parquetfp::ORIENT& oldOrient)
{
  BTree::MoveType move = get_move();
  indexOrient = UNSIGNED_UNINITIALIZED;
  newOrient = parquetfp::N;
  oldOrient = parquetfp::N;
  switch(move)
  {
    case BTree::SWAP:
//      cout << "makeMoveSwap" << endl;
      perform_swap();
      return 1;

    case BTree::ROTATE:
//      cout << "makeMoveRotate" << endl;
      perform_rotate(indexOrient, newOrient, oldOrient);
      return int(REP_SPEC_ORIENT);

    case BTree::MOVE:
//      cout << "makeMoveSimple" << endl;
      perform_move();
      return 3;

    default:
      cout << "ERROR: invalid move specified in makeMove()" << endl;
      exit(1);
  }
  return MISC;
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::makeMoveSlacks()
{
//  cout << "makeMoveSlacks" << endl;
  int movedir = rand() % 100;
  int threshold = 50;
  bool horizontal = (movedir < threshold);

  makeMoveSlacksCore(horizontal);

  static int total = 0;
  static int numHoriz = 0;

  total++;
  numHoriz += ((horizontal)? 1 : 0);

  if (total % 1000 == 0 && _params->verb.getForMajStats() > 0)
    cout << "total: " << total << "horiz: " << numHoriz << endl;
  return SLACKS_MOVE;
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::makeARMove()
{
//  cout << "makeARMove" << endl;
  float currWidth = in_curr_solution.totalWidth();
  float currHeight = in_curr_solution.totalHeight();
  float currAR = currWidth / currHeight;

  const float reqdAR = _params->reqdAR;
  bool horizontal = currAR > reqdAR;

  makeMoveSlacksCore(horizontal);
  return AR_MOVE;
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::makeMoveSlacksCore(bool horizontal)
{
//  cout << "makeMoveSlacksCore" << endl;
  _slackEval->evaluateSlacks(in_curr_solution);
//  cout << "End SlackEval!!" << endl;

  vector<int> indices_sorted;
  const vector<float>& slacks = (horizontal)?
    _slackEval->xSlack() : _slackEval->ySlack();

  sort_slacks(slacks, indices_sorted);

  int blocknum = in_curr_solution.NUM_BLOCKS;
  int range = int(ceil(blocknum / 5.0));

  int operand_ptr = rand() % range;
  int operand = indices_sorted[operand_ptr];
  while (operand_ptr > 0 && slacks[operand] > 0)
  {
    operand_ptr--;
    operand = indices_sorted[operand_ptr];
  }

  int target_ptr = blocknum - 1 - (rand() % range);
  int target = indices_sorted[target_ptr];
  while (target_ptr < (blocknum-1) && slacks[target] <= 0)
  {
    target_ptr++;
    target = indices_sorted[target_ptr];
  }

  if (target == operand 
      || slacks[operand] < 0 || slacks[target] < 0)
    return;

  in_next_solution = in_curr_solution;
  in_next_solution.move(operand, target, horizontal);
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::makeHPWLMove()
{
//  cout << "makeHPWLMove" << endl;
  int size = in_curr_solution.NUM_BLOCKS;
  int operand = rand() % size;
  int target = UNSIGNED_UNINITIALIZED;

  vector<int> searchBlocks;
  locateSearchBlocks(operand, searchBlocks);

  if (searchBlocks.size() > 0)
  {
    int temp = rand() % searchBlocks.size();
    target = searchBlocks[temp];
  }
  else
  {
    do
      target = rand() % size;
    while(target == operand);
  }

  bool leftChild = bool(rand() % 2);
  in_next_solution = in_curr_solution;
  in_next_solution.move(operand, target, leftChild);
  return HPWL;
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::makeARWLMove()
{
//  cout << "makeARWLMove" << endl;
//  cout << "before EvalSlack xloc yloc" << endl;
//  for(int i=0; i<in_curr_solution.NUM_BLOCKS; i++) {
//    cout << i << " " << in_curr_solution.xloc(i) << "  " << in_curr_solution.yloc(i) << endl;
//  }
  _slackEval->evaluateSlacks(in_curr_solution);
  
//  cout << "End SlackEval!!" << endl;
//  for(int i=0; i<in_curr_solution.NUM_BLOCKS; i++) {
//    cout << i << " " << in_curr_solution.xloc(i) << "  " << in_curr_solution.yloc(i) << endl;
//  }

  const float reqdAR = _params->reqdAR;
  const float currAR =
    in_curr_solution.totalWidth() / in_curr_solution.totalHeight();
  const bool horizontal = currAR > reqdAR;
  const vector<float>& slacks = (horizontal)?
    _slackEval->xSlack() : _slackEval->ySlack(); 

  vector<int> indices_sorted;
  sort_slacks(slacks, indices_sorted);

  int blocknum = in_curr_solution.NUM_BLOCKS;
  int range = int(ceil(blocknum / 5.0));

  int operand_ptr = rand() % range;
  int operand = indices_sorted[operand_ptr];
  while (operand_ptr > 0 && slacks[operand] > 0)
  {
    operand_ptr--;
    operand = indices_sorted[operand_ptr];
  }

  vector<int> searchBlocks;
  locateSearchBlocks(operand, searchBlocks);

  int target = UNSIGNED_UNINITIALIZED;
  float maxSlack = -1;
  if (searchBlocks.size() == 0)
  {
    do
      target = rand() % blocknum;
    while(target == operand);
  }
  else
  {
    for (unsigned int i = 0; i < searchBlocks.size(); i++)
    {
      int thisBlk = searchBlocks[i];
      if (slacks[thisBlk] > maxSlack)
      {
        maxSlack = slacks[thisBlk];
        target = thisBlk;
      }
    }
  }

  if (target == operand || slacks[operand] < 0 || maxSlack < 0) {
    // <aaronnn> couldn't lock on a target; let's not make any risky moves
    // TODO: don't waste this move - pick something reasonable to move
    return ARWL;
  }

//  cout << "REVERT!!! xloc yloc" << endl;
//  for(int i=0; i<in_curr_solution.NUM_BLOCKS; i++) {
//    cout << i << " " << in_curr_solution.xloc(i) << "  " << in_curr_solution.yloc(i) << endl;
//  }
  in_next_solution = in_curr_solution;
//  cout << "next solution move!!!" << endl;
//  in_curr_solution.move(operand, target, horizontal);
  in_next_solution.move(operand, target, horizontal);
  return ARWL; 
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::makeSoftBlMove(int& index,
    float& newWidth,
    float& newHeight)
{
  static const int NOT_FOUND = NOT_FOUND;
  _slackEval->evaluateSlacks(in_curr_solution);

  int moveDir = rand() % 2;
  bool horizontal = (moveDir%2 == 0);
  index = getSoftBlIndex(horizontal);

  if (index == NOT_FOUND)
    index = getSoftBlIndex(!horizontal);

  if (index != NOT_FOUND)
  {
    return getSoftBlNewDimensions(index, newWidth, newHeight);
  }
  else
  {
    newWidth = NOT_FOUND;
    newHeight = NOT_FOUND;
    return MISC;
  }
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::getSoftBlIndex(bool horizontal) const
{
  static const int NOT_FOUND = NOT_FOUND;
  const int blocknum = in_curr_solution.NUM_BLOCKS;
  const vector<float>& slacks = (horizontal)?
    _slackEval->xSlack() : _slackEval->ySlack();

  const vector<float>& orth_slacks = (horizontal)?
    _slackEval->ySlack() : _slackEval->xSlack();

  vector<int> indices_sorted;
  sort_slacks(slacks, indices_sorted);

  int operand = NOT_FOUND; // "-1" stands for not found
  for (int i = 0; (i < blocknum) && (operand == NOT_FOUND); i++)
  {
    int index = indices_sorted[i];
    if (slacks[index] < 0 || orth_slacks[index] < 0) 
    {
      // <aaronnn> skip these blocks affected by obstacles
      continue;
    }

    if (blockinfo.blockARinfo[index].isSoft)
    {
      int theta = in_curr_solution.tree[index].orient;
      float minAR = blockinfo.blockARinfo[index].minAR[theta];
      float maxAR = blockinfo.blockARinfo[index].maxAR[theta];

      float currWidth = in_curr_solution.width(index);
      float currHeight = in_curr_solution.height(index);
      float currAR = currWidth / currHeight;

      bool adjustable = (horizontal)?
        (currAR > minAR) : (currAR < maxAR);

      float reqdLength = (horizontal)? _outlineHeight : _outlineWidth;
      float currLength = (horizontal)? currHeight : currWidth; 
      float slackAdjustment = ((_isFixedOutline)
          ? currLength - reqdLength : 0.0);
      float normalizedSlack = orth_slacks[index] - slackAdjustment;
      if (normalizedSlack > 0 && adjustable)
        operand = index;
    }
  }
  return operand;
}   
// --------------------------------------------------------
int BTreeAreaWireAnnealer::getSoftBlNewDimensions(int index,
    float& newWidth,
    float& newHeight) const
{
  if (_slackEval->xSlack()[index] < 0
      || _slackEval->ySlack()[index] < 0) 
  {
    // don't touch nodes around obstacles
    return NOOP;
  }

  if (blockinfo.blockARinfo[index].isSoft)
  {
    float origWidth = in_curr_solution.width(index);
    float origHeight = in_curr_solution.height(index);
    float indexArea = blockinfo.blockARinfo[index].area;

    int theta = in_curr_solution.tree[index].orient;
    float minAR = blockinfo.blockARinfo[index].minAR[theta];
    float maxAR = blockinfo.blockARinfo[index].maxAR[theta];
    if (_isFixedOutline)
    {
      bool agressiveAR = false;
      // ((in_curr_solution.totalArea() / _outlineArea - 1)
      //                              < _params->startSoftMovePercent / 100.0);
      minAR = (agressiveAR)? minAR : max(minAR, float(1.0/3.0));
      maxAR = (agressiveAR)? maxAR : min(maxAR, float(3.0));
    }            

    float maxWidth = sqrt(indexArea * maxAR);
    float minWidth = sqrt(indexArea * minAR);

    float maxHeight = sqrt(indexArea / minAR);
    float minHeight = sqrt(indexArea / maxAR);

    float indexSlackX = _slackEval->xSlack()[index];         
    float indexSlackY = _slackEval->ySlack()[index];

    // adjustment <= 0.75 * actual slack
    float slackAdjustmentX = (_isFixedOutline)
      ? (in_curr_solution.totalWidth() - _outlineWidth) : 0.0;
    //      slackAdjustmentX = max(slackAdjustmentX, 0.0);
    //      slackAdjustmentX = min(slackAdjustmentX, 0.75 * indexSlackX);

    float slackAdjustmentY = (_isFixedOutline)
      ? (in_curr_solution.totalHeight() - _outlineHeight) : 0.0;
    //      slackAdjustmentY = max(slackAdjustmentY, 0.0);
    //      slackAdjustmentY = min(slackAdjustmentY, 0.75 * indexSlackY);

    float normalizedSlackX = indexSlackX - slackAdjustmentX;
    float normalizedSlackY = indexSlackY - slackAdjustmentY;
    if (normalizedSlackX > normalizedSlackY)
    {
      newWidth = min(origWidth+normalizedSlackX, maxWidth);
      newWidth = max(newWidth, minWidth);

      newHeight = indexArea / newWidth;
    }
    else
    {
      newHeight = min(origHeight+normalizedSlackY, maxHeight);
      newHeight = max(newHeight, minHeight);

      newWidth = indexArea / newHeight;
    }
    return SOFT_BL;
  }
  else
    return NOOP;
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::packSoftBlocks(int numIter)
{
  const int NUM_BLOCKS = in_curr_solution.NUM_BLOCKS;
  for (int iter = 0; iter < numIter; iter++)
  {
    bool horizontal = (iter % 2 == 0);
    _slackEval->evaluateSlacks(in_curr_solution);

    const vector<float>& slacks = (horizontal)?
      _slackEval->xSlack() : _slackEval->ySlack();

    vector<int> indices_sorted;
    sort_slacks(slacks, indices_sorted);
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
      int index = indices_sorted[i];
      if (slacks[index] < 0) 
      {
        // <aaronnn> skip these blocks affected by obstacles
        continue;
      }

      float origWidth = in_curr_solution.width(index);
      float origHeight = in_curr_solution.height(index);

      float newWidth, newHeight;
      int softDecision = makeIndexSoftBlMove(index, newWidth, newHeight);

      if (softDecision == SOFT_BL)
      {
        // change dimensions only when needed
        int theta = in_curr_solution.tree[index].orient;
        _blockinfo.setBlockDimensions(index, newWidth, newHeight, theta);

        in_next_solution.evaluate(in_curr_solution.tree);

        float origTotalArea = in_curr_solution.totalArea();
        float newTotalArea = in_next_solution.totalArea();
        if (newTotalArea < origTotalArea)
        {
          in_curr_solution = in_next_solution;
          if (_params->minWL)
          {
            _db->getNodes()->putNodeWidth(index, newWidth);
            _db->getNodes()->putNodeHeight(index, newHeight);
          }
        }
        else
        {
          _blockinfo.setBlockDimensions(index, origWidth, origHeight, theta);
        }
      }
    }
  }
  return MISC;
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::locateSearchBlocks(int operand,
    vector<int>& searchBlocks)
{
  int size = in_curr_solution.NUM_BLOCKS;
  vector<bool> seenBlocks(size, false);
  seenBlocks[operand] = true;
  float unitRadiusSize = max(in_curr_solution.totalWidth(),
      in_curr_solution.totalHeight());
  unitRadiusSize /= sqrt(float(size));

  vector<float>& xloc = const_cast<vector<float>&>(in_curr_solution.xloc());
  vector<float>& yloc = const_cast<vector<float>&>(in_curr_solution.yloc());
  parquetfp::Point idealLoc(_analSolve->getOptLoc(operand, xloc, yloc));
  // get optimum location of "operand"

  int searchRadiusNum = int(ceil(size / 5.0));
  float searchRadius = 0;
  for(int i = 0; ((i < searchRadiusNum) &&
        (searchBlocks.size() < unsigned(searchRadiusNum))); ++i)
  {
    searchRadius += unitRadiusSize;
    for(int j = 0; ((j < size) &&
          (searchBlocks.size() < unsigned(searchRadiusNum))); ++j)
    {
      if (!seenBlocks[j])
      {
        float xDist = xloc[j] - idealLoc.x;
        float yDist = yloc[j] - idealLoc.y;
        float distance = sqrt(xDist*xDist + yDist*yDist);
        if(distance < searchRadius)
        {
          seenBlocks[j] = true; 
          searchBlocks.push_back(j);
        }
      }
    }
  }
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::GenerateRandomSoln(BTree& soln,
    int blocknum) const
{
  vector<int> tree_bits;
  int balance = 0;
  int num_zeros = 0;
  for (int i = 0; i < 2*blocknum; i++)
  {
    bool assigned = false;
    float rand_num = float(rand()) / (RAND_MAX+1.0);
    float threshold;

    if (balance == 0)
      threshold = 1; // push_back "0" for sure
    else if (num_zeros == blocknum) 
      threshold = 0; // push_back "1" for sure
    else
      threshold = 1; // (rand_num * (balance - rand_num));

    if (rand_num >= threshold)
    {
      tree_bits.push_back(1);
      balance--;
      assigned = true;
    }
    else
    {
      tree_bits.push_back(0);
      balance++;
      num_zeros++;
      assigned = true;
    }
  }

  vector<int> tree_perm;
  tree_perm.resize(blocknum);
  for (int i = 0; i < blocknum; i++)
    tree_perm[i] = i;
  random_shuffle(tree_perm.begin(), tree_perm.end());

  vector<int> tree_perm_inverse(blocknum);
  for (int i = 0; i < blocknum; i++)
    tree_perm_inverse[tree_perm[i]] = i;

  vector<int> tree_orient(blocknum);
  for (int i = 0; i < blocknum; i++)
  {
    int rand_num = int(8 * (float(rand()) / (RAND_MAX + 1.0)));
    rand_num = _physicalOrient[i][rand_num];

    tree_orient[tree_perm_inverse[i]] = rand_num;
  }

  soln.evaluate(tree_bits, tree_perm, tree_orient);
}
// --------------------------------------------------------
void getObstaclesFromDB(DB *const db, BasePacking &out)
  // <aaronnn> massage db obstacles from Node into BlockPacking
{
  out.xloc.clear();
  out.yloc.clear();
  out.width.clear();
  out.height.clear();

  Nodes *nodes = db->getObstacles();
//   Node tmp1("obs1", 400*300, 400/300, 400/300, 0, false);
//   tmp1.putX( 200 );
//   tmp1.putY( 200 ); 
//   tmp1.putWidth( 100 );
//   tmp1.putHeight( 100 );
// 
//   nodes->putNewNode(tmp1 );
// 
//   Node tmp2("obs2", 50*50, 50/50, 50/50, 0, false);
//   tmp2.putX( 100 );
//   tmp2.putY( 100 ); 
//   tmp2.putWidth( 50 );
//   tmp2.putHeight( 50 );
// 
//   nodes->putNewNode(tmp2 );

  for(unsigned i = 0; i < nodes->getNumNodes(); ++i)
  {
    Node &node = nodes->getNode(i);

    out.xloc.push_back(node.getX());
    out.yloc.push_back(node.getY());
    out.width.push_back(node.getWidth());
    out.height.push_back(node.getHeight());
  }
  //  out.xloc.push_back(0);
  //  out.yloc.push_back(0);
  //  out.width.push_back(200);
  //  out.height.push_back(50);

  //  out.xloc.push_back(100);
  //  out.yloc.push_back(50);
  //  out.width.push_back(50);
  //  out.height.push_back(50);
}
// --------------------------------------------------------
