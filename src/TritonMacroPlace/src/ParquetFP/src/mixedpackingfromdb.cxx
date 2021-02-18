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


#include "mixedpackingfromdb.h"
#include "basepacking.h"
#include "mixedpacking.h"
#include "DB.h"
#include "debugflags.h"

using parquetfp::Node;
using parquetfp::Nodes;
using parquetfp::DB;
using std::vector;

// --------------------------------------------------------
MixedBlockInfoTypeFromDB::MixedBlockInfoTypeFromDB(const DB& db)
   : MixedBlockInfoType(db.getNumNodes())
{
   // extract information from DB
   vector<float> widths(db.getNodeWidths());
   vector<float> heights(db.getNodeHeights());

   Nodes& nodes = *(const_cast<DB&>(db).getNodes());
   int blocknum = db.getNumNodes();
   for (int i = 0; i < blocknum; i++)
   {
      Node& currBlk = nodes.getNode(i);
      int theta = (currBlk.getOrient());
      setBlockDimensions(i, widths[i], heights[i], theta);
      _currDimensions.in_block_names[i] = currBlk.getName();

#ifdef PARQUET_DEBUG_HAYWARD_DISPLAY_MIXEDPACKINGFROMDB
      printf("[%d]: theta: %d, width: %lf, height %lf\n",
             i, theta, widths[i], heights[i]);
#endif
      
      float currMaxAR = currBlk.getmaxAR();
      float currMinAR = currBlk.getminAR();
      float currArea = currBlk.getArea();

      _blockARinfo[i].area = currArea;
      set_blockARinfo_AR(i, currMinAR, currMaxAR);
      _blockARinfo[i].isSoft = (currMaxAR > currMinAR);

#ifdef PARQUET_DEBUG_HAYWARD_DISPLAY_MIXEDPACKINGFROMDB
      printf("[%d]: theta: %d, maxAR: %lf, minAR: %lf, area: %lf\n",
             i, theta, currMaxAR, currMinAR, currArea);
#endif
   }

   static const float Infty = basepacking_h::Dimension::Infty;   
   _currDimensions.set_dimensions(blocknum, 0, Infty);
   _currDimensions.in_block_names[blocknum] = "LEFT";
   _blockARinfo[blocknum].area = 0;
   _blockARinfo[blocknum].minAR.resize(Orient_Num, 0);
   _blockARinfo[blocknum].maxAR.resize(Orient_Num, 0);
   _blockARinfo[blocknum].isSoft = false;

   _currDimensions.set_dimensions(blocknum+1, Infty, 0);
   _currDimensions.in_block_names[blocknum+1] = "BOTTOM";
   _blockARinfo[blocknum+1].area = 0;
   _blockARinfo[blocknum+1].minAR.resize(Orient_Num, Infty);
   _blockARinfo[blocknum+1].maxAR.resize(Orient_Num, Infty);
   _blockARinfo[blocknum+1].isSoft = false;
}
// --------------------------------------------------------

