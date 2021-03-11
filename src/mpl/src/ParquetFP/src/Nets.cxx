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
using std::map;
using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::ifstream;

Nets::Nets(const string &baseName)//://_numPins(0)
{
  string fname = baseName + string(".nets");
  parseNets(fname);
  initName2IdxMap();
  fname = baseName + string(".wts");
  parseWts(fname);
}

void Nets::parseNets(const string &fnameNets)
{
  char block_name[1024];
  ifstream nets(fnameNets.c_str());
  char tempWord1[1024];
  char netName[1024];
  float poffsetX;
  float poffsetY;
  int netIndex = 0;
  int numNets = 0;
  int numPins=0;
  unsigned netDegree=0;
  unsigned netCtr=0;
  Net tempEdge;

  if(!nets)
  {
    cout<<"ERROR: .nets file could not be opened successfully"<<endl;
    return;
  }
  skiptoeol(nets);
  while(!nets.eof())
  {
    nets>>tempWord1;
    if(!(strcmp(tempWord1,"NumNets")))
      break;
  }
  nets>>tempWord1;
  nets>>numNets;
  while(!nets.eof())
  {
    nets>>tempWord1;
    if(!(strcmp(tempWord1,"NumPins")))
      break;
  }
  nets>>tempWord1;
  nets>>numPins;

  if(numNets > 0)
  {
    while(!nets.eof())
    {
      nets>>tempWord1;
      if(!(strcmp(tempWord1,"NetDegree")))
        break;
    }
    nets>>tempWord1;
    nets>>netDegree;

    eatblank(nets);
    if(nets.peek() == '\n' || nets.peek() == '\r')
    {
      sprintf(netName,"N%d",netIndex);
    }
    else  //netName present
    {
      nets>>netName;
    }
    skiptoeol(nets);
    tempEdge.putName(netName);
  }

  eatblank(nets);
  if(nets.peek() == EOF)
    nets.get();

  while(!nets.eof())
  { 
    eatblank(nets);
    if(nets.eof())
      break;
    if(nets.peek()=='#')
      eathash(nets);
    else
    {
      eatblank(nets);
      if(nets.peek() == '\n' || nets.peek() == '\r' || nets.peek() == EOF)
      {
        nets.get();
        continue;
      }

      nets>>block_name;
      nets>>tempWord1;

      if(!strcmp(tempWord1,"B") || !strcmp(tempWord1,"O") || 
          !strcmp(tempWord1,"I"))
      {
        eatblank(nets);

        if(nets.peek()=='\n' || nets.peek()=='\r' || nets.eof())
        {
          nets.get();
          //put terminal info in vector
          pin tempPin(block_name,true,0,0,netIndex);
          tempEdge.addNode(tempPin);
          //cout<<block_name<<"\t"<<tempWord1<<"\t"<<endl;
          ++netCtr;
          continue;
        }
        else
        {
          nets>>tempWord1;
          if(!strcmp(tempWord1,":"))
          {
            eatblank(nets);
          }
          else
            cout << "error in parsing"<<endl;

          if(nets.peek()!='%')
          {cout<<"expecting %"<<endl;}
          else
          {
            nets.get();
            nets>>poffsetX;
            eatblank(nets);
            nets.get();
            nets>>poffsetY;
            nets.get();
            //convert from %
            poffsetX /= 100;
            poffsetY /= 100;
            //put block info here
            pin tempPin(block_name,false,poffsetX,poffsetY,netIndex);
            tempEdge.addNode(tempPin);
            //cout<<block_name<<"\t"<<tempWord1<<"\t"<<poffsetX<<"\t"<<poffsetY<<endl;
            ++netCtr;
          }
        }
      }

      else if(!strcmp(block_name,"NetDegree"))//new net starts
      {
        tempEdge.putIndex(netIndex);
        _nets.push_back(tempEdge);
        //_numPins+=netCtr;

        if(netCtr != netDegree)
        { 
          cout<<"ERROR in parsing .nets file. For net "<<tempEdge.getName()<<" netDegree do not match with no: of pins. "<<netCtr<<" vs "<<netDegree<<"\n"<<endl;
        }
        netCtr = 0;
        tempEdge.clean();
        netIndex++;

        nets>>netDegree;
        eatblank(nets);
        if(nets.peek() == '\n' || nets.peek() == '\r')
        {
          sprintf(netName,"N%d",netIndex);
        }
        else  //netName present
        { 
          nets>>netName;
        }
        skiptoeol(nets);
        tempEdge.putName(netName);
      }
    }
  }
  nets.close();

  if(numNets > 0)
  {
    //put the last net info inside
    tempEdge.putIndex(netIndex);
    _nets.push_back(tempEdge);
    ++netIndex;
    //_numPins+=netCtr;
  }

  if(netIndex != numNets)
    cout<<"Error in parsing .nets file. Number of nets do not tally "<<netIndex<<" vs "<<numNets<<endl;


  int actNumPins = getNumPins();
  if(numPins != actNumPins)
  {
    cout<<"Error in parsing .nets file. Number of pins do not tally "<<actNumPins<<" vs "<<numPins<<endl;
  }
}


void Nets::parseWts(const string &fnameWts)
{
  ifstream wts(fnameWts.c_str());
  //  char tempWord1[1024];
  char netName[1024];
  float netWeight=1;

  if(!wts)
  {
    cout<<"WARNING: .wts file could not be opened successfully"<<endl;
    return;
  }

  skiptoeol(wts);

  while(!wts.eof())
  { 
    eatblank(wts);
    if(wts.eof())
      break;
    if(wts.peek()=='#')
      eathash(wts);
    else
    {
      eatblank(wts);
      if(wts.peek() == '\n' || wts.peek() == '\r' || wts.peek() == EOF)
      {
        wts.get();
        continue;
      }
      wts>>netName;
      wts>>netWeight;
      if(_name2IdxMap.find(netName) != _name2IdxMap.end())
      {
        unsigned idx = getIdxFrmName(netName);
        Net& net = getNet(idx);
        net.putWeight(netWeight);
      }
      else
      {
        cout<<"ERROR in parsing .wts file. Net "<<netName<<" has weight, but is not defined in .nets file"<<endl;
      }
    }
  }
  wts.close();
}

void Nets::updateNodeInfo(Nodes& nodes)
{
  itNet net;
  itPin pin;
  itNode node;

  map<string, int> index;
  map<string, bool> type;

  for(node = nodes.nodesBegin(); node != nodes.nodesEnd(); node++)
    {
      index[node->getName()] = node->getIndex();
      type[node->getName()] = 0;
    }
  for(node = nodes.terminalsBegin(); node != nodes.terminalsEnd(); node++)
    {
      index[node->getName()] = node->getIndex();
      type[node->getName()] = 1;
    }

  for(net = _nets.begin(); net != _nets.end(); net++)
    {
      for(pin = net->pinsBegin(); pin != net->pinsEnd(); pin++)
        {
	  int thisIndex = index[pin->getName()];
	  bool thisType = type[pin->getName()];
	  pin->putNodeIndex(thisIndex);
	  pin->putType(thisType);
	    
	  //  cout<<"ERROR: Node "<<pin->getName()<<" not found in updateNodeInfo"<<endl;
	}
    }
}


int Nets::getNumPins(void) const
{
  itNetConst net;
  unsigned numPins = 0;
  for(net = netsBegin(); net != netsEnd(); ++net)
   {
     numPins += net->_pins.size();
   }
  //if(numPins != _numPins)
  //{
    //cout<<" Long computation: "<<numPins << " stored version: "<<_numPins<<endl;
    //abkfatal( numPins == _numPins, "numPins mismatch");
  //}
  return numPins;
}


void Nets::putName2IdxEntry(string netName, int idx)
{
  _name2IdxMap[netName] = idx;
}

int Nets::getIdxFrmName(string netName)
{
  return _name2IdxMap[netName];
}

void Nets::initName2IdxMap(void)
{
  unsigned netCtr=0;
  for(itNet net=netsBegin(); net != netsEnd(); ++net)
    {
      putName2IdxEntry(net->getName(), netCtr);
      ++netCtr;
    }
}

