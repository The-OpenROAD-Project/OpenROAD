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


#include "btreecompactsstree.h"
#include "basepacking.h"
#include "btreecompact.h"
#include "btreefromsstree.h"

#include <fstream>
#include <string>
#include <iomanip>

// --------------------------------------------------------
float BTreeCompactSlice(const SoftPacking& spk,
                         const string& outfilename)
{
   SoftPackingHardBlockInfoType hardblockinfo(spk);
   BTreeFromSoftPacking btree(hardblockinfo, spk,
                              getTolerance(hardblockinfo));
   BTreeCompactor compactor(btree);

   int numChanged = 0;
   int i = 0;

   float orig_totalArea = compactor.totalArea();
   float orig_blockArea = compactor.blockArea();
   float spk_totalArea = spk.totalWidth * spk.totalHeight;
   printf("converted to B*-Tree: %.2lf (%.2lf%%) -> %.2lf (%.2lf%%)\n",
          spk_totalArea, (spk.deadspace / spk.blockArea) * 100,
          orig_totalArea, (orig_totalArea / orig_blockArea - 1)* 100);
   do
   {
      numChanged = compactor.compact();
   
      printf("round %d: %6d blks changed: %.2lf (%.2lf%%) -> %.2lf (%.2lf%%)\n",
             i, numChanged, orig_totalArea,
             (orig_totalArea / orig_blockArea - 1) * 100,
             compactor.totalArea(),
             (compactor.totalArea() / compactor.blockArea() - 1) * 100);
      i++;
      orig_totalArea = compactor.totalArea();
      orig_blockArea = compactor.blockArea();
   } while (numChanged != 0);

   float final_blkArea = compactor.blockArea();
   float final_deadspace = compactor.totalArea() - final_blkArea;
   cout << endl;
   cout << "After compaction, " << endl;
   cout << "blkArea: " << setw(11) << final_blkArea
        << " deadspace: " << setw(11) << final_deadspace
        << " (" << ((final_deadspace / final_blkArea) * 100) << "%)" << endl;

   cout << endl;
   PrintDimensions(compactor.totalWidth(), compactor.totalHeight());
   PrintAreas(final_deadspace, final_blkArea);
   cout << endl;
   PrintUtilization(final_deadspace, final_blkArea);
   cout << endl;
   
   ofstream outfile;
   outfile.open(outfilename.c_str());
   Save_bbb(outfile, BTreeOrientedPacking(compactor));
   if (outfile.good())
      cout << "Output successfully written to " << outfilename << endl;
   else
      cout << "Something wrong with the file " << outfilename << endl;
   outfile.close();

   return (compactor.totalArea() - compactor.blockArea());
}
// --------------------------------------------------------
