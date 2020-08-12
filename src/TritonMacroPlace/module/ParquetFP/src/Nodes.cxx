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
#include "FPcommon.h"
#include "Nets.h"
#include "Nodes.h"
#include <map>

using namespace parquetfp;
using std::numeric_limits;
using std::map;
using std::string;
using std::cout;
using std::endl;
using std::ofstream;
using std::ifstream;
using std::vector;

//ctor

Nodes::Nodes(const string &baseName)
{
//  string fname = baseName + string(".blocks");
//  parseNodes(fname);
  
  string fname = baseName + string(".txt");
  parseTxt(fname);

  fname = baseName + string(".pl");
  parsePl(fname);
}

Nodes::Nodes(const Nodes& orig)
{
  (*this)=orig;
}

Nodes& Nodes::operator=(const Nodes& rhs)
{
    _nodes.clear();
    _nodes.insert(_nodes.end(), rhs._nodes.begin(), rhs._nodes.end());
    _terminals.clear();
    _terminals.insert(_terminals.end(), rhs._terminals.begin(), rhs._terminals.end());
    return *this;
}

// *.blocks parsing
void Nodes::parseNodes(const string &fnameBl)
{
  char block_name[1024];
  char block_type[1024];
  char tempWord1[1024];

  vector<Point> vertices;
  int numVertices;
  bool success; 
  float width, height;

  float area,minAr,maxAr;
  int numSoftBl=0;
  int numHardBl=0;
  int numTerm=0;

  int indexBlock=0;
  int indexTerm=0;

  ifstream input(fnameBl.c_str());
  if(!input)
  {
    cout<<"ERROR: .blocks file could not be opened successfully"<<endl;
    exit(0);
  }
  skiptoeol(input);
  while(!input.eof())
  {
    input>>tempWord1;
    if(!(strcmp(tempWord1,"NumSoftRectangularBlocks")))
      break;
  }
  input>>tempWord1;
  input>>numSoftBl;
  while(!input.eof())
  {
    input>>tempWord1;
    if(!(strcmp(tempWord1,"NumHardRectilinearBlocks")))
      break;
  }
  input>>tempWord1;
  input>>numHardBl;

  while(!input.eof())
  {
    input>>tempWord1;
    if(!(strcmp(tempWord1,"NumTerminals")))
      break;
  }
  input>>tempWord1;
  input>>numTerm;

  while(!input.eof())
  {
    block_type[0] = '\0';
    eatblank(input);
    if(input.eof())
      break;
    if(input.peek()=='#')
      eathash(input);
    else
    {
      eatblank(input);
      if(input.peek() == '\n' || input.peek() == '\r' 
          || input.peek() == EOF)
      {
        input.get();
        continue;
      }

      input >> block_name;
      input >>block_type ;

      if(!strcmp(block_type,"softrectangular")) 
      {
        input >> area;
        input >> minAr;
        input >> maxAr;
        Node temp(block_name,area,minAr,maxAr,indexBlock,false);
        temp.addSubBlockIndex(indexBlock);
        _nodes.push_back(temp);

        ++indexBlock;
        //cout<<block_name<<" "<<area<<endl;
      }
      else if(!strcmp(block_type,"hardrectilinear"))
      {
        input >> numVertices;
        Point tempPoint;
        success = 1;
        if(numVertices > 4)
          cout<<"ERROR in parsing .blocks file. rectilinear blocks can be only rectangles for now\n";
        for(int i=0; i<numVertices; ++i)
        {
          success &= needCaseChar(input, '(');  input.get();
          input >> tempPoint.x;
          success &= needCaseChar(input, ',');  input.get();
          input >> tempPoint.y;
          success &= needCaseChar(input, ')');  input.get();
          vertices.push_back(tempPoint);
        }
        if(!success)
          cout<<"ERROR in parsing .blocks file while processing hardrectilinear blocks"<<endl;

        width = vertices[2].x - vertices[0].x;
        height = vertices[2].y - vertices[0].y;
        cout << width << " " << height << endl;
        area = width*height;
        minAr = width/height;
        maxAr = minAr;
        Node temp(block_name,area,minAr,maxAr,indexBlock,false);
        temp.addSubBlockIndex(indexBlock);
        _nodes.push_back(temp);
        ++indexBlock;
        vertices.clear();
//        cout << block_name << " " << width << " " << height << endl;
      }
      else if(!strcmp(block_type,"terminal"))
      {
        Node temp(block_name,0,1,1,indexTerm,true);
        temp.addSubBlockIndex(indexTerm);
        _terminals.push_back(temp);
        ++indexTerm;
        //cout<<indexTerm<<"  "<<block_name<<endl;
      }
      /*
         else
         cout<<"ERROR in parsing .blocks file"<<endl;
         */
    }  
  }
  input.close();

  if(numSoftBl+numHardBl+numTerm != indexBlock+indexTerm)
    cout<<"ERROR in parsing .blocks file. No: of blocks do not tally "<<(indexBlock+indexTerm)<<" vs "<<(numSoftBl+numHardBl+numTerm)<<endl;

}

// *.blocks parsing
void Nodes::parseTxt(const string &fnameTxt)
{
  cout << "txt parsing " << endl;
  char block_name[1024];
  char block_type[1024];
  char tempWord1[1024];

  vector<Point> vertices;
  int numVertices;
  bool success; 
  float width, height;

  int numSoftBl=0;
  int numHardBl=0;
  int numTerm=0;

  int indexBlock=0;
  int indexTerm=0;

  ifstream input(fnameTxt.c_str());
  if(!input)
  {
    cout<<"ERROR: .blocks file could not be opened successfully"<<endl;
    exit(0);
  }
  
  float area,minAr,maxAr;
  float snapX, snapY, haloX, haloY, channelX, channelY;
  input>>numHardBl;
  for(int i=0; i<numHardBl; i++) {
    input >> block_name;
    input >> width >> height;
    input >> snapX >> snapY;
    input >> haloX >> haloY ;
    input >> channelX >> channelY;

    area = width*height;
    minAr = width/height;
    maxAr = minAr;
        
    Node temp(block_name,area,minAr,maxAr,indexBlock,false);
    temp.addSubBlockIndex(indexBlock);

    temp.putSnapX(snapX);
    temp.putSnapY(snapY);
    temp.putHaloX(haloX);
    temp.putHaloY(haloY);
    temp.putChannelX(channelX);
    temp.putChannelY(channelY);

    _nodes.push_back(temp);
    ++indexBlock;
  } 
  
  input >> numTerm;
  for(int i=0; i<numTerm; i++) {
    input >> block_name; 
    Node temp(block_name,0,1,1,indexTerm,true);
    temp.addSubBlockIndex(indexTerm);
    _terminals.push_back(temp);
    ++indexTerm;
  }
  input.close();

}

void Nodes::parsePl(const string &fnamePl)
{
  char block_name[1024];
  char tempWord[1024];
  char tempWord1[1024];
  float xloc;
  float yloc;
  float width;
  float height;
  bool success;
  ORIENT newOrient;
  itNode node;

  map<string, int> index;
  map<string, bool> type;
  for(node = nodesBegin(); node != nodesEnd(); node++)
  {
    index[node->getName()] = node->getIndex();
    type[node->getName()] = 0;
  }
  for(node = terminalsBegin(); node != terminalsEnd(); node++)
  {
    index[node->getName()] = node->getIndex();
    type[node->getName()] = 1;
  }

  ifstream input(fnamePl.c_str());
  if(!input)
  {
    cout<<"ERROR: .pl file could not be opened successfully"<<endl;
    return;
  }

  skiptoeol(input);
  while(!input.eof())
  {
    eatblank(input);
    if(input.eof())
      break;
    if(input.peek()=='#')
      eathash(input);
    else
    {
      eatblank(input);
      if(input.peek() == '\n' || input.peek() == '\r' 
          || input.peek() == EOF)
      {
        input.get();
        continue;
      }
      input>>block_name;
      input>>xloc;
      input>>yloc;
      int thisIndex = index[block_name];
      int thisType = type[block_name];

      updatePlacement(thisIndex, thisType, xloc, yloc);

//      cout << block_name << ", xloc: " << xloc << ", yloc: " << yloc << endl;   
      //  exit(1);

      eatblank(input);
      success = 1;
      if(input.peek() == 'D')   //block with width and height
      {
        input>>tempWord;  
        success &= needCaseChar(input,'('); input.get();
        input>>width;
        success &= needCaseChar(input,','); input.get();
        input>>height;
        success &= needCaseChar(input,')'); input.get();

        if(!success || strcmp(tempWord,"DIMS"))
          cout<<"ERROR in parsing .pl file while reading in DIMS\n";
        updateHW(thisIndex, thisType, width, height);
        getNode(thisIndex).needSyncOrient = false;
      }

      success = 1;
      eatblank(input);
      if(input.peek() == ':') //initial orientation given
      {
        input.get();
        input>>tempWord1;    //the orientation in words;
        newOrient = toOrient(tempWord1);
        updateOrient(thisIndex, thisType, newOrient);
      }
      //cout<<block_name<<" "<<xloc<<" "<<yloc<<endl;  
    }
  }
  input.close();
}
  
void Nodes::updatePlacement(int index, bool type, float xloc, float yloc)
{
  if(!type)
    {
      Node& node = getNode(index);
      node.putX(xloc);
      node.putY(yloc);
    }
  else
    {
      Node& node = getTerminal(index);
      node.putX(xloc);
      node.putY(yloc);
    }
  // cout<<"ERROR: could not find node to update placement for. Name: "<<
  //block_name<<endl;
  return;
}

void Nodes::updateOrient(int index, bool type, ORIENT newOrient)
{
  if(!type)
    {
      Node& node = getNode(index);
      node.putOrient(newOrient);
    }
  else
    {
      Node& node = getTerminal(index);
      node.putOrient(newOrient);
    }
  //cout<<"ERROR: could not find node to update Orient for. Name: "<<block_name
  //    <<endl;
}

void Nodes::updateHW(int index, bool type, float width, float height)
{
  if(!type)
    {
      Node& node = getNode(index);
      node.putWidth(width);
      node.putHeight(height);
    }
  else
    {
      Node& node = getTerminal(index);
      node.putWidth(width);
      node.putHeight(height);
    }
  //cout<<"ERROR: could not find node to update Height/Width for. Name: "<<
  //  block_name<<endl;
}

float Nodes::getNodeWidth(unsigned index)
{
  return _nodes[index].getWidth();
}

float Nodes::getNodeHeight(unsigned index)
{
  return _nodes[index].getHeight();
}

void Nodes::putNodeWidth(unsigned index, float width)
{
  _nodes[index].putWidth(width);
}

void Nodes::putNodeHeight(unsigned index, float height)
{
  _nodes[index].putHeight(height);
}

vector<float> Nodes::getNodeWidths(void)
{
  vector<float> widths;
  for(itNode iNodes=nodesBegin(); iNodes!=nodesEnd(); iNodes++)
    widths.push_back(iNodes->getWidth());
  return widths;
}

vector<float> Nodes::getNodeHeights(void)
{
  vector<float> heights;
  for(itNode iNodes=nodesBegin(); iNodes!=nodesEnd(); iNodes++)
    heights.push_back(iNodes->getHeight());
  return heights;
}


vector<float> Nodes::getXLocs(void)
{
  vector<float> xloc;
  for(itNode iNodes=nodesBegin(); iNodes!=nodesEnd(); iNodes++)
    xloc.push_back(iNodes->getX());
  return xloc;
}

vector<float> Nodes::getYLocs(void)
{
  vector<float> yloc;
  for(itNode iNodes=nodesBegin(); iNodes!=nodesEnd(); iNodes++)
    yloc.push_back(iNodes->getY());
  return yloc;
}

float Nodes::getNodesArea()
{
  itNode iNode;
  float area = 0;
  for(iNode = nodesBegin(); iNode != nodesEnd(); ++iNode)
    {
      area += iNode->getHeight()*iNode->getWidth();
    }
  return area;
}

void Nodes::changeOrient(unsigned index, ORIENT newOrient, Nets& nets)
{
  Node& node = _nodes[index];
  node.changeOrient(newOrient, nets);
}

/*following function assumes that pin info about node index is correct.*/
void Nodes::updatePinsInfo(Nets& nets)
{
  itNet net;
  itNode node;
  NodePin tempNodePin;
  unsigned pinOffset;
  
  for(node=nodesBegin(); node != nodesEnd(); ++node)
    node->clearPins();
  for(node=terminalsBegin(); node != terminalsEnd(); ++node)
    node->clearPins();

  for(net = nets.netsBegin(); net != nets.netsEnd(); net++)
  {
      pinOffset = 0;
      //for(pin = net->pinsBegin(); pin != net->pinsEnd(); pin++)
      for(pinOffset = 0; pinOffset < net->getDegree(); ++pinOffset)
      {
          pin& currPin = net->getPin(pinOffset);
          tempNodePin.netIndex = net->getIndex();
          tempNodePin.pinOffset = pinOffset;
          unsigned nodeIndex = currPin.getNodeIndex();
          if(!currPin.getType())     //if not pad
          {
              Node& node = getNode(nodeIndex);
              node.addPin(tempNodePin);
          }
          else
          {
              Node& node = getTerminal(nodeIndex);
              node.addPin(tempNodePin);
          }
          //tempNodePin.netIndex = net->getIndex();
          //tempNodePin.pinOffset = pinOffset;
          //unsigned nodeIndex = pin->getNodeIndex();
          //if(!pin->getType())     //if not pad
          //{
          //    Node& node = getNode(nodeIndex);
          //    node.addPin(tempNodePin);
          //}
          //else
          //{
          //    Node& node = getTerminal(nodeIndex);
          //    node.addPin(tempNodePin);
          //}
          //pinOffset++;
      }
  }

  //now sync the orientations of the pins with that of the nodes
  for(node = nodesBegin(); node != nodesEnd(); ++node)
    { 
      if(node->getOrient() != N)   //synchronize orientation only if not N
        node->syncOrient(nets);
    }
}


void Nodes::savePl(const char* baseFileName)
{
  char fileName[1024];
  strcpy(fileName, baseFileName);
  strcat(fileName, ".pl");
  ofstream outPl(fileName);
  if(!outPl)
    {
      cout<<"ERROR in saving .pl file"<<endl;
      return;
    }
  cout<<"Saving "<<fileName<<" as output .pl file"<<endl;
  outPl<<"UMICH blocks 1.0"<<endl<<endl<<endl<<endl;
  
  itNode node;
  
  for(node = nodesBegin(); node != nodesEnd(); node++)
    {
      outPl<<node->getName()<<"\t"<<node->getX()<<"\t"<<node->getY();
      outPl<<"\tDIMS = ("<<node->getWidth()<<", "<<node->getHeight()<<")";
      outPl<<"\t: "<<toChar(node->getOrient())<<endl;
    }
  outPl<<endl;

  for(node = terminalsBegin(); node != terminalsEnd(); node++)
    {
      outPl<<node->getName()<<"\t"<<node->getX()<<"\t"<<node->getY()
	   <<"\t: "<<toChar(node->getOrient())<<endl;
    }
  outPl<<endl;
  outPl.close();
}

void Nodes::saveCapoNodes(const char* baseFileName)
{
  
  char fileName[1024];
  strcpy(fileName, baseFileName);
  strcat(fileName, ".nodes");
  ofstream file(fileName);
  file<<"UCLA nodes  1.0"<<endl<<endl<<endl;
  
  file<<"NumNodes : \t"<<_nodes.size()+_terminals.size()<<endl;
  file<<"NumTerminals : \t"<<_terminals.size()<<endl;

  itNode node;

  float width, height, temp;
  for(node = nodesBegin(); node != nodesEnd(); ++node)
    {
      width = node->getWidth();
      height = node->getHeight();
      if(int(node->getOrient())%2 == 1)
	{
	  temp = width;
	  width = height;
	  height = temp;
	}
      file<<"\t"<<node->getName()<<"\t"<<width<<"\t"<<height<<endl;      
    }

  for(node = terminalsBegin(); node != terminalsEnd(); ++node)
    {
      width = node->getWidth();
      height = node->getHeight();
      if(int(node->getOrient())%2 == 1)
	{
	  temp = width;
	  width = height;
	  height = temp;
	}
      file<<"\t"<<node->getName()<<"\t"<<width<<"\t"<<height<<"\tterminal "
	  <<endl;      
    }
  file.close();
}

void Nodes::saveNodes(const char* baseFileName)
{
  char fileName[1024];
  strcpy(fileName, baseFileName);
  strcat(fileName, ".blocks");
  ofstream file(fileName);
  file<<"UCSC blocks  1.0"<<endl<<endl<<endl;
  
  file<<"NumSoftRectangularBlocks : \t"<<_nodes.size()<<endl;
  file<<"NumHardRectilinearBlocks : \t0"<<endl;
  file<<"NumTerminals : \t"<<_terminals.size()<<endl<<endl;

  itNode node;
     
  for(node = nodesBegin(); node != nodesEnd(); ++node)
    {
      file<<node->getName()<<" softrectangular "<<node->getArea()<<"\t"
          <<node->getminAR()<<"\t"<<node->getmaxAR()<<endl;      
    }
  file<<endl;
  for(node = terminalsBegin(); node != terminalsEnd(); ++node)
    file<<node->getName()<<" terminal"<<endl;

  file.close();
}

void Nodes::saveCapoPl(const char* baseFileName)
{
  char fileName[1024];
  strcpy(fileName, baseFileName);
  strcat(fileName, ".pl");
  ofstream file(fileName);

  file<<"UCLA pl   1.0"<<endl<<endl<<endl;
  
  itNode node;
     
  for(node = nodesBegin(); node != nodesEnd(); ++node)
    {
      file<<"\t"<<node->getName()<<"\t"<<node->getX()<<"\t"
	  <<node->getY()<<" : \t"<<toChar(node->getOrient())<<endl;      
    }

    
  for(node = terminalsBegin(); node != terminalsEnd(); ++node)
    {
      file<<"\t"<<node->getName()<<"\t"<<node->getX()<<"\t"
          <<node->getY()<<" : \t"<<toChar(node->getOrient())<<endl;
    }
    
  file.close();
}

void Nodes::saveCapoScl(const char* baseFileName, float reqdAR, float reqdWS, const BBox &nonTrivialBBox)
{
  //default required aspect ratio of 1
  float AR;
  if(reqdAR == -9999)
    AR = 1;
  else
    AR = reqdAR;
    
  float area = (1+reqdWS/100.0)*getNodesArea();  //add WS
  float height = sqrt(area/AR);
  float width = AR*height;

  float siteWidth = 1.;
  unsigned numSites = static_cast<unsigned>(width/siteWidth) + 1;

  float minHeight = getMinHeight();
  float minWidth = getMinWidth();

  float rowHeight = std::max(ceil(0.1*std::min(minHeight, minWidth)),1.);

  unsigned numRows = static_cast<unsigned>(height/rowHeight) + 1;

  float rowCoord = nonTrivialBBox.isValid() ? nonTrivialBBox.getMinY() : 0.f;
  float colCoord = nonTrivialBBox.isValid() ? nonTrivialBBox.getMinX() : 0.f;

  string fileName = string(baseFileName) + string(".scl");
  ofstream file(fileName.c_str());

  file<<"UCLA scl   1.0"<<endl<<endl<<endl;
  file<<"Numrows : "<<numRows<<endl;
  
  for(unsigned i = 0; i < numRows; ++i)
  {
    file<<"CoreRow Horizontal"<<endl;
    file<<" Coordinate\t: "<<rowCoord<<endl;
    file<<" Height\t: "<<rowHeight<<endl;
    file<<" Sitewidth\t: "<<siteWidth<<endl;
    file<<" Sitespacing\t: "<<siteWidth<<endl;
    file<<" Siteorient\t: N"<<endl;
    file<<" Sitesymmetry\t: Y"<<endl;
    file<<" SubrowOrigin\t: " << colCoord << " Numsites\t: "<<numSites<<endl;
    file<<"End"<<endl;

    rowCoord += rowHeight;
  }

  file.close();
}

float Nodes::getMinHeight()
{
  itNode node;
  float minHeight = std::numeric_limits<float>::max();
  for(node = nodesBegin(); node != nodesEnd(); ++node)
   {
     if(minHeight > node->getHeight())
       minHeight = node->getHeight();
   }
  return minHeight;
}

float Nodes::getMinWidth()
{
  itNode node;
  float minWidth = numeric_limits<float>::max();
  for(node = nodesBegin(); node != nodesEnd(); ++node)
   {
     if(minWidth > node->getWidth())
       minWidth = node->getWidth();
   }
  return minWidth;
}

void Nodes::initNodesFastPOAccess(Nets& nets, bool reset)
{
  itNode node;
  if (reset)
    for(node = nodesBegin(); node != nodesEnd(); ++node)
      node->allPinsAtCenter = false;
  else
    {
      for(node = nodesBegin(); node != nodesEnd(); ++node)
	node->allPinsAtCenter =  node->calcAllPinsAtCenter(nets);
    }
}
