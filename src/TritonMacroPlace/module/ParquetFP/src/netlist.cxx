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
#include "netlist.h"
#include "basepacking.h"
#include "parsers.h"

#include <iostream>
#include <fstream>
#include <algorithm>

using namespace parse_utils;
using namespace basepacking_h;
using std::min;
using std::max;
using std::ifstream;
using std::ofstream;
using std::string;
using std::cout;
using std::endl;
using std::vector;

// --------------------------------------------------------
float NetType::getHPWL(const OrientedPacking& pk) const
{
   float minX = Dimension::Infty;
   float maxX = -1 * Dimension::Infty;
   float minY = Dimension::Infty;
   float maxY = -1 * Dimension::Infty;

   for (unsigned int i = 0; i < in_pads.size(); i++)
   {
      float xloc = in_pads[i].xloc;
      float yloc = in_pads[i].yloc;

      minX = min(minX, xloc);
      maxX = max(maxX, xloc);
      minY = min(minY, yloc);
      maxY = max(maxY, yloc);
//      printf("PAD [%d]: %lf, %lf\n", i, xloc, yloc);
   }

   for (unsigned int i = 0; i < in_pins.size(); i++)
   {
      float orig_x_offset = in_pins[i].x_offset;
      float orig_y_offset = in_pins[i].y_offset;

      int blk_index = in_pins[i].block;
      OrientedPacking::ORIENT blk_orient = pk.orient[blk_index];

      float x_length = (blk_orient % 2 == 0)?
         pk.width[blk_index] : pk.height[blk_index];
      float y_length = (blk_orient % 2 == 0)?
         pk.height[blk_index] : pk.width[blk_index];

      float x_offset = -1;
      float y_offset = -1;
      getOffsets(orig_x_offset, orig_y_offset, blk_orient,
                 x_offset, y_offset);

      float xloc = pk.xloc[blk_index] + (x_length / 2) + x_offset;
      float yloc = pk.yloc[blk_index] + (y_length / 2) + y_offset;

      minX = min(minX, xloc);
      maxX = max(maxX, xloc);
      minY = min(minY, yloc);
      maxY = max(maxY, yloc);
//      printf("PIN [%d]: %lf (%lf-%lf), %lf(%lf-%lf)\n",
//             i, xloc, pk.xloc[blk_index], pk.xloc[blk_index]+x_length,
//             yloc, pk.yloc[blk_index], pk.yloc[blk_index]+y_length);
   }
//   printf("x-span: %lf - %lf\n", minX, maxX);
//   printf("y-span: %lf - %lf\n", minY, maxY);
//   printf("HPWL: %lf ", (maxX-minX) + (maxY-minY));
   return ((maxX - minX) + (maxY - minY));
}
// --------------------------------------------------------
void NetListType::ParsePl(ifstream& infile,
                          const HardBlockInfoType& blockinfo)
{
   in_all_pads.clear();
   
   ofstream outfile;
   outfile.open("dummy_pl");
   
   string line;
   int objCount = 0;
   for (int lineCount = 0;
        getline(infile, line); lineCount++)
   {
      char name[100];
      float xloc, yloc;

      outfile << line << endl;
      if (sscanf(line.c_str(), "%s %f %f",
                 name, &xloc, &yloc) == 3)
      {
         if (objCount >= blockinfo.blocknum())
         {
            PadInfoType temp_pad;
            temp_pad.pad_name = name;
            temp_pad.xloc = xloc;
            temp_pad.yloc = yloc;
            
            in_all_pads.push_back(temp_pad);
         }
         objCount++;
      }
   }
}   
// --------------------------------------------------------
void NetListType::ParseNets(ifstream& infile,
                            const HardBlockInfoType& blockinfo)
{
   string line;
   int numNets = -1;
   int numPins = -1;
   if(!infile.good())
   {
      cout << "ERROR: .nets file could not be opened successfully" << endl;
      exit(1);
   }

   // get NumNets
   getline(infile, line);
   while(getline(infile, line))
   {
      char first_word[100];
      char ch;
                 
      if ((sscanf(line.c_str(), "%s %c %d",
                  first_word, &ch, &numNets) == 3) &&
          !strcmp(first_word, "NumNets"))
         break;
      
   }

   if (!infile.good())
   {
      cout << "ERROR in reading .net file (# nets)." << endl;
      exit(1);
   }
   
   // get NumPins
   while(getline(infile, line))
   {
      char first_word[100];
      char ch;

      if ((sscanf(line.c_str(), "%s %c %d",
                  first_word, &ch, &numPins) == 3) &&
          !strcmp(first_word, "NumPins"))
         break;
   }   

   if (!infile.good())
   {
      cout << "ERROR in reading .net file (# pins)." << endl;
      exit(1);
   }


   // parse the contents of the nets
   int netDegree = -1;               // degree of the current net
   int degreeCount = 0;
   int pinCount = 0;
   vector<NetType::PadType> netPads; // vector of pads for the current net;
   vector<NetType::PinType> netPins; // vector of pins for the current net;
   for (int lineCount = 0; infile.good(); lineCount++)
   {
      char dummy_word[1000];
      char block_name[1000];
      char dummy_chars[100];
      int number;
      float x_percent, y_percent;

      getline(infile, line);
      if (sscanf(line.c_str(), "%s", dummy_word) < 1)
      {
         // blank line
         if (infile.eof())
         {
            // wrap up the last net.
            int degreeCount = netPins.size() + netPads.size();
            if (degreeCount == netDegree)
            {
               NetType temp(netPins, netPads);
               in_nets.push_back(temp);

               degreeCount = 0;
               netPins.clear();
               netPads.clear();
            }
            else
            {
               printf("ERROR: netDegree in net %lu does not tally\n",
                      in_nets.size());
               exit(1);
            }
         }
      }
      else if ((sscanf(line.c_str(), "%c", dummy_chars+0) == 1) &&
               dummy_chars[0] == '#')
      {  /* comments (starts with '#') */ }
      else if (sscanf(line.c_str(), "%s %c %c %c%f %c%f",
                      block_name, dummy_chars+0, dummy_chars+1,
                      dummy_chars+2, &x_percent,
                      dummy_chars+3, &y_percent) == 7)
      {
         // consider a pin, on the block "block_name".
         int index = get_index(string(block_name), blockinfo);
         if (index == -1)
         {
            printf("ERROR: Unrecognized block \"%s\"\n", block_name);
            exit(1);
         }
         else
         {
            x_percent /= 100;
            y_percent /= 100;
            
            NetType::PinType temp;
            temp.block = index;
            temp.x_offset = (blockinfo[index].width[0] * x_percent) / 2;
            temp.y_offset = (blockinfo[index].height[0] * y_percent) / 2;
//            printf("block %d: x_offset: %.2lf y_offset: %.2lf width: %.2lf height: %2.lf\n",
//                   index, temp.x_offset, temp.y_offset,
//                   blockinfo[index].width[0], blockinfo[index].height[0]);
            netPins.push_back(temp);
            degreeCount++;
            pinCount++;
         }
      }
      else if (sscanf(line.c_str(), "%s %c %d",
                      dummy_word, dummy_chars+0, &number) == 3)
      {
         if (!strcmp(dummy_word, "NetDegree"))
         {
            // see "NetDegree : x, wrap up previous net
            if (netDegree == -1)
               netDegree = number;
            else if (degreeCount == netDegree)
            {
               NetType temp(netPins, netPads);
               in_nets.push_back(NetType(netPins, netPads));

               netDegree = number;
               degreeCount = 0;
               netPins.clear();
               netPads.clear();
            }
            else
            {
               printf("ERROR: netDegree in net %lu does not tally (%d vs. %d).\n",
                      in_nets.size(), degreeCount, netDegree);
               exit(1);
            }            
         }
         else
         {
            printf("ERROR in parsing .net file (line %d after NumPins).\n",
                   lineCount+1);
            printf("line: %s\n", line.c_str());
            exit(1);
         }
      }
      else if (sscanf(line.c_str(), "%s %c",
                      block_name, dummy_chars+0) == 2)
      {
         // consider a pad, with name "block_name".
         vector<int> indices;
         get_index(string(block_name), indices);
         if (indices.empty())
         {
            printf("ERROR: unrecognized pad \"%s\"\n", block_name);
            exit(1);
         }
         else
         {
            for (unsigned int i = 0; i < indices.size(); i++)
            {
               NetType::PadType temp;
               temp.xloc = in_all_pads[indices[i]].xloc;
               temp.yloc = in_all_pads[indices[i]].yloc;
               netPads.push_back(temp);
               pinCount++;
            }
            degreeCount++;
         }               
         
      }
      else
      {
         printf("ERROR in parsing .net file (line %d after NumPins).\n",
                lineCount+1);
         printf("line: %s\n", line.c_str());
         exit(1); 
      }
   }

   if (pinCount != numPins)
   {
      printf("ERROR: # pins does not tally (%d vs. %d).",
             pinCount, numPins);
      exit(1);
   }
   else if (int(in_nets.size()) != numNets)
   {
      printf("ERROR: # nets does not tally (%lu vz. %d).",
             in_nets.size(), numNets);
      exit(1);
   }
   infile.close();
}
// --------------------------------------------------------
         
   

