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
#include "parsers.h"

#include <iostream>
#include <vector>
using namespace parse_utils;
using namespace basepacking_h;
using std::vector;
using std::ifstream;
using std::string;
using std::cout;
using std::endl;

// ======================
// Contructors in the end
// ======================
const float Dimension::Infty = std::numeric_limits<float>::max();
const float Dimension::Epsilon_Accuracy = 1e10f;
const int Dimension::Undefined = -1;
const int Dimension::Orient_Num = 8;

const int HardBlockInfoType::Orient_Num = Dimension::Orient_Num;
// ========================================================
void HardBlockInfoType::set_dimensions(int i, float w, float h,
                                      float snapX, float snapY, 
                                      float haloX, float haloY, 
                                      float channelX, float channelY )
{
   in_blocks[i].width.resize(Orient_Num);
   in_blocks[i].height.resize(Orient_Num);
   in_blocks[i].snapX.resize(Orient_Num);
   in_blocks[i].snapY.resize(Orient_Num);
   in_blocks[i].haloX.resize(Orient_Num);
   in_blocks[i].haloY.resize(Orient_Num);
   in_blocks[i].channelX.resize(Orient_Num);
   in_blocks[i].channelY.resize(Orient_Num);
   for (int j = 0; j < Orient_Num; j++) {
      if (j % 2 == 0)
      {
         in_blocks[i].width[j] = w;
         in_blocks[i].height[j] = h;
         in_blocks[i].snapX[j] = snapX;
         in_blocks[i].snapY[j] = snapY;
         in_blocks[i].haloX[j] = haloX; 
         in_blocks[i].haloY[j] = haloY; 
         in_blocks[i].channelX[j] = channelX;
         in_blocks[i].channelY[j] = channelY;
      }
      else
      {
         in_blocks[i].width[j] = h;
         in_blocks[i].height[j] = w;
         in_blocks[i].snapX[j] = snapY;
         in_blocks[i].snapY[j] = snapX;
         in_blocks[i].haloX[j] = haloY; 
         in_blocks[i].haloY[j] = haloX; 
         in_blocks[i].channelX[j] = channelY;
         in_blocks[i].channelY[j] = channelX;
      }
   }
}
// --------------------------------------------------------
HardBlockInfoType::HardBlockInfoType(ifstream& ins,
                                     const string& format)
   : blocks(in_blocks),
     block_names(in_block_names)
{
   if (format == "txt")
      ParseTxt(ins);
   else if (format == "blocks")
      ParseBlocks(ins);
   else
   {
      cout << "ERROR: invalid format: " << format << endl;
      exit(1);
   }
}
// --------------------------------------------------------
void HardBlockInfoType::ParseTxt(ifstream& ins)
{
   int blocknum = -1;
   ins >> blocknum;

   if (!ins.good())
   {
      cout << "ERROR: cannot read the block count." << endl;
      exit(1);
   }

   in_blocks.resize(blocknum+2);
   in_block_names.resize(blocknum+2);
   for (int i = 0; i < blocknum; i++)
   {
      float w, h;
      ins >> w >> h;

      if (!ins.good())
      {
         cout << "ERROR: cannot read block no." << i << endl;
         exit(1);
      }

      set_dimensions(i, w, h);

      char temp[100];
      temp[0] = '\0';
      sprintf(temp, "%d", i);
      in_block_names[i] = temp;
   }
   set_dimensions(blocknum, 0, Dimension::Infty);
   in_block_names[blocknum] = "LEFT";
   
   set_dimensions(blocknum+1, Dimension::Infty, 0);
   in_block_names[blocknum+1] = "BOTTOM";
}
// --------------------------------------------------------
// taken from Nodes.cxx of Parquet using FPcommon.h/cxx
// --------------------------------------------------------
void HardBlockInfoType::ParseBlocks(ifstream& ins)
{
  char block_name[100];
  char block_type[100];
  char tempWord[100];

  int numSoftBl=0;
  int numHardBl=0;
  int numTerm=0;

  int indexBlock=0;

  if(!ins)
  {
    cout << "ERROR: .blocks file could not be opened successfully"
      << endl;
    exit(1);
  }

  skiptoeol(ins);
  while(!ins.eof())
  {
    ins >> tempWord;
    if(!(strcmp(tempWord,"NumSoftRectangularBlocks")))
      break;
  }

  if (!ins.good())
  {
    cout << "ERROR in parsing .blocks file." << endl;
    exit(1);
  }

  ins >> tempWord;
  ins >> numSoftBl;
  if (numSoftBl != 0)
  {
    cout << "ERROR: soft block packing is not supported for now." << endl;
    exit(0);
  }

  while(!ins.eof())
  {
    ins >> tempWord;
    if (!(strcmp(tempWord, "NumHardRectilinearBlocks")))
      break;
  }
  ins >> tempWord;
  ins >> numHardBl;

  while(!ins.eof())
  {
    ins >> tempWord;
    if (!(strcmp(tempWord, "NumTerminals")))
      break;
  }
  ins >> tempWord;
  ins >> numTerm;

  in_blocks.resize(numHardBl+2);
  in_block_names.resize(numHardBl+2);
  while(ins.good())
  {
    block_type[0] = '\0';
    eatblank(ins);

    if (ins.eof())
      break;
    if (ins.peek() == '#')
      eathash(ins);
    else
    {
      eatblank(ins);
      if (ins.peek() == '\n' || ins.peek() == '\r')
      {
        ins.get();
        continue;
      }

      ins >> block_name;
      ins >> block_type;

      if (!strcmp(block_type, "softrectangular"))
      {
        cout << "ERROR: soft block packing is not supported now." << endl;
        exit(1);
      }
      else if (!strcmp(block_type,"hardrectilinear"))
      {
        Point tempPoint;
        vector<Point> vertices;
        int numVertices;
        bool success;
        float width, height;

        ins >> numVertices;             
        success = true;
        if (numVertices != 4)
        {
          cout << "ERROR in parsing .blocks file. "
            << "rectilinear blocks can be only rectangles for now"
            << endl;
          exit(1);
        }

        for (int i=0; i < numVertices; ++i)
        {
          success &= needCaseChar(ins, '(');  ins.get();
          ins >> tempPoint.x;
          success &= needCaseChar(ins, ',');  ins.get();
          ins >> tempPoint.y;
          success &= needCaseChar(ins, ')');  ins.get();
          vertices.push_back(tempPoint);
        }
        if (!success)
        {
          cout << "ERROR in parsing .blocks file while processing "
            << "hardrectilinear blocks." << endl;
          exit(1);
        }

        width = vertices[2].x - vertices[0].x;
        height = vertices[2].y - vertices[0].y;

        // consider a block
        //             cout << "[" << indexBlock << "] "
        //                  << setw(10) << block_name 
        //                  << " width: " << width
        //                  << " height: " << height << endl;
        set_dimensions(indexBlock, width, height);
        if (indexBlock >= int(block_names.size()))
        {
          cout << "ERROR: too many hard block specified." << endl;
          exit(1);
        }                 
        in_block_names[indexBlock] = block_name;
        ++indexBlock;
      }
      else if (!strcmp(block_type,"terminal"))
      {  /* a pad */ }
      else if (ins.good())
      {
        cout << "ERROR: invalid block type: " << block_type << endl;
        exit(1);
      }  
    }
  } // end of while-loop
  ins.close();

  if (numSoftBl+numHardBl != indexBlock)
  {
    cout << "ERROR in parsing .blocks file. # blocks do not tally "
      << (indexBlock) << " vs. "
      << (numSoftBl+numHardBl) << endl;
    exit(1);
  }
  set_dimensions(numHardBl, 0, Dimension::Infty);
  in_block_names[numHardBl] = "LEFT";

  set_dimensions(numHardBl+1, Dimension::Infty, 0);
  in_block_names[numHardBl+1] = "BOTTOM";
}
// ========================================================
