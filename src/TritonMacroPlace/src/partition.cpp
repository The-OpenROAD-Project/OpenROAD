#include "circuit.h"
#include "Parquet.h"
#include "mixedpackingfromdb.h"
#include "btreeanneal.h"
#include "logger.h"

#include <iostream>
#include <climits>
#include <cfloat>
#include <fstream>

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::pair;
using std::unordered_map;

namespace MacroPlace {

Partition::Partition() 
  : partClass(PartClass::None), 
  lx(FLT_MAX), ly(FLT_MAX), 
  width(FLT_MAX), height(FLT_MAX), 
  netTable(0), tableCnt(0) {}

Partition::Partition(
    PartClass _partClass, 
    double _lx, double _ly, 
    double _width, double _height ) :
  partClass(_partClass), lx(_lx), ly(_ly), 
  width(_width), height(_height), 
  netTable(0), tableCnt(0) {}
    
Partition::~Partition(){ 
  if( netTable) { 
    delete [] netTable; 
    netTable=0; 
    tableCnt=0;
  } 
}

Partition::Partition(const Partition& prev)   
  : partClass(prev.partClass), 
  lx(prev.lx), ly(prev.ly),
  width(prev.width), height(prev.height),
  macroStor(prev.macroStor), 
  tableCnt(prev.tableCnt), 
  macroMap(prev.macroMap) {
    if( prev.netTable ) {
      netTable = new double[tableCnt];
      for(int i=0; i<tableCnt; i++) {
        netTable[i] = prev.netTable[i];
      }
    }
    else {
      netTable = 0;
    }
  }

Partition& Partition::operator= (const Partition& prev) {
  partClass = prev.partClass;
  lx = prev.lx;
  ly = prev.ly;
  width = prev.width;
  height = prev.height;
  macroStor = prev.macroStor;
  tableCnt = prev.tableCnt;
  macroMap = prev.macroMap; 

  if( prev.netTable ) {
    netTable = new double[tableCnt];
    for(int i=0; i<tableCnt; i++) {
      netTable[i] = prev.netTable[i];
    }
  }
  else {
    netTable = 0;
  }
  return *this;
}

void Partition::Dump() {    
  cout << "partClass: " << partClass << endl;
  cout << lx << " " << ly << " " << width << " " << height << endl;
  for(auto& curMacro : macroStor) {
    curMacro.Dump();
  }
  cout << endl;
}


void Partition::PrintParquetFormat(string origName){
  string blkName = origName + ".blocks";
  string netName = origName + ".nets";
  string wtsName = origName + ".wts"; 
  string plName = origName + ".pl";
  
  // For *.nets and *.wts writing
  vector< pair<int, int> > netStor;
  vector<int> costStor;

  netStor.reserve( (macroStor.size()+4)*(macroStor.size()+3)/2 );
  costStor.reserve( (macroStor.size()+4)*(macroStor.size()+3)/2 );
  for(size_t i=0; i<macroStor.size()+4; i++) {
    for(size_t j=i+1; j<macroStor.size()+4; j++) {
      int cost = 0;
      if( netTable ) {
        cost = netTable[ i*(macroStor.size()+4) + j] 
          + netTable[ j*(macroStor.size()+4) + i ];
      }
      if( cost != 0 ) {
        netStor.push_back( std::make_pair( std::min(i,j), std::max(i,j) ) );
        costStor.push_back(cost);
      }
    }
  }
  
  WriteBlkFile( blkName );
  WriteNetFile( netStor, netName );
  WriteWtsFile( costStor, wtsName );
  WritePlFile( plName );
  
}

void Partition::WriteBlkFile( string blkName ) {
  std::ofstream blkFile(blkName);
  if( !blkFile.good() ) {
    cout << "** ERROR: Cannot Open BlkFile to write : " << blkName << endl;
    exit(1);
  }

  std::stringstream feed;
  feed << "UCSC blocks 1.0" << endl;
  feed << "# Created" << endl;
  feed << "# User" << endl;
  feed << "# Platform" << endl << endl;

  feed << "NumSoftRectangularBlocks : 0" << endl;
  feed << "NumHardRectilinearBlocks : " << macroStor.size() << endl;
  feed << "NumTerminals : 4" << endl << endl;

  for(auto& curMacro : macroStor) {
    feed << curMacro.name << " hardrectilinear 4 ";
    feed << "(" << curMacro.lx << ", " << curMacro.ly << ") ";
    feed << "(" << curMacro.lx << ", " << curMacro.ly + curMacro.h << ") ";
    feed << "(" << curMacro.lx + curMacro.w << ", " << curMacro.ly + curMacro.h << ") ";
    feed << "(" << curMacro.lx + curMacro.w << ", " << curMacro.ly << ") " << endl;
  }

  feed << endl;

  feed << "West terminal" << endl;
  feed << "East terminal" << endl;
  feed << "North terminal" << endl;
  feed << "South terminal" << endl;
  
  blkFile << feed.str();
  blkFile.close();
  feed.clear();  
}

#define EAST_IDX (macroStor.size())
#define WEST_IDX (macroStor.size()+1)
#define NORTH_IDX (macroStor.size()+2)
#define SOUTH_IDX (macroStor.size()+3)

#define GLOBAL_EAST_IDX (mckt.macroStor.size())
#define GLOBAL_WEST_IDX (mckt.macroStor.size()+1)
#define GLOBAL_NORTH_IDX (mckt.macroStor.size()+2)
#define GLOBAL_SOUTH_IDX (mckt.macroStor.size()+3)

string Partition::GetName(int macroIdx ) {
  if( macroIdx < macroStor.size()) {
      return macroStor[macroIdx].name;
  }
  else {
    if( macroIdx == EAST_IDX ) {
      return "East"; 
    }
    else if( macroIdx == WEST_IDX) {
      return "West";
    }
    else if( macroIdx == NORTH_IDX) {
      return "North";
    }
    else if( macroIdx == SOUTH_IDX) { 
      return "South";
    }
    else {
      return "None";
    }
  }
}

void Partition::WriteNetFile( vector< pair<int, int> >& netStor, string netName ) {
  std::ofstream netFile(netName);
  if( !netFile.good() ) {
    cout << "** ERROR: Cannot Open NetFile to write : " << netName << endl;
    exit(1);
  }

  std::stringstream feed;
  feed << "UCLA nets 1.0" << endl;
  feed << "# Created" << endl;
  feed << "# User" << endl;
  feed << "# Platform" << endl << endl;


  feed << "NumNets : " << netStor.size() << endl;
  feed << "NumPins : " << 2 * netStor.size() << endl;

  for(auto& curNet : netStor) { 
    int idx = &curNet - &netStor[0];
    feed << "NetDegree : 2  n" << std::to_string(idx) << endl;
    feed << GetName( curNet.first ) << " B : %0.0 %0.0" << endl;
    feed << GetName( curNet.second ) << " B : %0.0 %0.0" << endl;
  }

  feed << endl;

  netFile << feed.str();
  netFile.close();
  feed.clear();  

}

void Partition::WriteWtsFile( vector< int >& costStor, string wtsName ) {
  std::ofstream wtsFile(wtsName);
  if( !wtsFile.good() ) {
    cout << "** ERROR: Cannot Open WtsFile to write : " << wtsName << endl;
    exit(1);
  }

  std::stringstream feed;
  feed << "UCLA wts 1.0" << endl;
  feed << "# Created" << endl;
  feed << "# User" << endl;
  feed << "# Platform" << endl << endl;


  for(auto& curWts : costStor ) { 
    int idx = &curWts - &costStor[0];
    feed << "n" << std::to_string(idx) << " " << curWts << endl;
  }

  feed << endl;

  wtsFile << feed.str();
  wtsFile.close();
  feed.clear();  

}

void Partition::WritePlFile( string plName ) {
  std::ofstream plFile(plName);
  if( !plFile.good() ) {
    cout << "** ERROR: Cannot Open NetFile to write : " << plName << endl;
    exit(1);
  }

  std::stringstream feed;
  feed << "UCLA pl 1.0" << endl;
  feed << "# Created" << endl;
  feed << "# User" << endl;
  feed << "# Platform" << endl << endl;

  for(auto& curMacro : macroStor) {
    feed << curMacro.name << " 0 0" << endl;
  }

  feed << "East " << lx + width << " " << ly + height/2.0 << endl;
  feed << "West " << lx << " " << ly + height/2.0 << endl;
  feed << "North " << lx + width/2.0 << " " << ly + height << endl;
  feed << "South " << lx + width/2.0 << " " << ly << endl;

  feed << endl;

  plFile << feed.str();
  plFile.close();
  feed.clear();  
}

   


void Partition::FillNetlistTable(MacroCircuit& mckt, 
    unordered_map<PartClass, vector<int>, PartClassHash, PartClassEqual>& macroPartMap ) {
  tableCnt = (macroStor.size()+4)*(macroStor.size()+4);
  netTable = new double[tableCnt];
  for(int i=0; i<tableCnt; i++) {
    netTable[i] = 0.0f;
  }
  // FillNetlistTableIncr();
  // FillNetlistTableDesc();
  
  auto mpPtr = macroPartMap.find( partClass );
  if( mpPtr == macroPartMap.end()) {
    cout << "ERROR: Partition: " << partClass 
      << " not exists MacroCell (macroPartMap)" << endl;
    exit(1);
  }

  // Just Copy to the netlistTable.
  if( partClass == ALL ) {
    for(size_t i=0; i< (macroStor.size()+4); i++) {
      for(size_t j=0; j< macroStor.size()+4; j++) {
        netTable[ i*(macroStor.size()+4)+j ] = (double)mckt.macroWeight[i][j]; 
      }
    }
    return; 
  }

  // row
  for(size_t i=0; i<macroStor.size()+4; i++) {
    // column
    for(size_t j=0; j<macroStor.size()+4; j++) {
      if( i == j ) {
        continue;
      }

      // from: macro case
      if ( i < macroStor.size() ) {
        auto mPtr = mckt.macroNameMap.find( macroStor[i].name );
        if( mPtr == mckt.macroNameMap.end()) {
          cout << "ERROR on macroNameMap: " << endl;
          exit(1);
        }
        int globalIdx1 = mPtr->second;

        // to macro case
        if( j < macroStor.size() ) {
          auto mPtr = mckt.macroNameMap.find( macroStor[j].name );
          if( mPtr == mckt.macroNameMap.end()) {
            cout << "ERROR on macroNameMap: " << endl;
            exit(1);
          }
          int globalIdx2 = mPtr->second;
          netTable[ i*(macroStor.size()+4) + j ] = mckt.macroWeight[globalIdx1][globalIdx2];
        }
        // to IO-west case
        else if( j == WEST_IDX ) {
          int westSum = mckt.macroWeight[globalIdx1][GLOBAL_WEST_IDX];

          if( partClass == PartClass::NE ) {
            auto mpPtr = macroPartMap.find( PartClass::NW );
            if( mpPtr != macroPartMap.end() ) {
              for(auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name];
                westSum += mckt.macroWeight[globalIdx1][curGlobalIdx]; 
              }
            }
          }
          if( partClass == PartClass::SE ) {
            auto mpPtr = macroPartMap.find( PartClass::SW );
            if( mpPtr != macroPartMap.end() ) {
              for(auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name];
                westSum += mckt.macroWeight[globalIdx1][curGlobalIdx]; 
              }
            }
          }
          netTable[ i*(macroStor.size()+4) + j] = westSum;
        }
        else if (j == EAST_IDX ) {
          int eastSum = mckt.macroWeight[globalIdx1][GLOBAL_EAST_IDX];

          if( partClass == PartClass::NW ) {
            auto mpPtr = macroPartMap.find( PartClass::NE );
            if( mpPtr != macroPartMap.end() ) {
              for(auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name];
                eastSum += mckt.macroWeight[globalIdx1][curGlobalIdx]; 
              }
            }
          }
          if( partClass == PartClass::SW ) {
            auto mpPtr = macroPartMap.find( PartClass::SE );
            if( mpPtr != macroPartMap.end() ) {
              for(auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name];
                eastSum += mckt.macroWeight[globalIdx1][curGlobalIdx]; 
              }
            }
          }
          netTable[ i*(macroStor.size()+4) + j] = eastSum ;
        }
        else if (j == NORTH_IDX ) { 
          int northSum = mckt.macroWeight[globalIdx1][GLOBAL_NORTH_IDX];

          if( partClass == PartClass::SE ) {
            auto mpPtr = macroPartMap.find( PartClass::NE );
            if( mpPtr != macroPartMap.end() ) {
              for(auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name];
                northSum += mckt.macroWeight[globalIdx1][curGlobalIdx]; 
              }
            }
          }
          if( partClass == PartClass::SW ) {
            auto mpPtr = macroPartMap.find( PartClass::NW );
            if( mpPtr != macroPartMap.end() ) {
              for(auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name];
                northSum += mckt.macroWeight[globalIdx1][curGlobalIdx]; 
              }
            }
          }
          netTable[ i*(macroStor.size()+4) + j] = northSum ;
        }
        else if (j == SOUTH_IDX ) {
          int southSum = mckt.macroWeight[globalIdx1][GLOBAL_SOUTH_IDX];

          if( partClass == PartClass::NE ) {
            auto mpPtr = macroPartMap.find( PartClass::SE );
            if( mpPtr != macroPartMap.end() ) {
              for(auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name];
                southSum += mckt.macroWeight[globalIdx1][curGlobalIdx]; 
              }
            }
          }
          if( partClass == PartClass::NW ) {
            auto mpPtr = macroPartMap.find( PartClass::SW );
            if( mpPtr != macroPartMap.end() ) {
              for(auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name];
                southSum += mckt.macroWeight[globalIdx1][curGlobalIdx]; 
              }
            }
          }
          netTable[ i*(macroStor.size()+4) + j] = southSum;
        }
      }
      //from IO
      else if( i == WEST_IDX ){
        // to Macro
        if( j < macroStor.size() ) {
          auto mPtr = mckt.macroNameMap.find( macroStor[j].name );
          if( mPtr == mckt.macroNameMap.end()) {
            cout << "ERROR on macroNameMap: " << endl;
            exit(1);
          }
          int globalIdx2 = mPtr->second;
          netTable[ i*(macroStor.size()+4) + j] = 
            mckt.macroWeight[GLOBAL_WEST_IDX][globalIdx2];
        }
      }
      else if( i == EAST_IDX) {
        // to Macro
        if( j < macroStor.size() ) {
          auto mPtr = mckt.macroNameMap.find( macroStor[j].name );
          if( mPtr == mckt.macroNameMap.end()) {
            cout << "ERROR on macroNameMap: " << endl;
            exit(1);
          }
          int globalIdx2 = mPtr->second;
          netTable[ i*(macroStor.size()+4) + j] = 
            mckt.macroWeight[GLOBAL_EAST_IDX][globalIdx2];
        }
      }
      else if(i == NORTH_IDX) {
        // to Macro
        if( j < macroStor.size() ) {
          auto mPtr = mckt.macroNameMap.find( macroStor[j].name );
          if( mPtr == mckt.macroNameMap.end()) {
            cout << "ERROR on macroNameMap: " << endl;
            exit(1);
          }
          int globalIdx2 = mPtr->second;
          netTable[ i*(macroStor.size()+4) + j] = 
            mckt.macroWeight[GLOBAL_NORTH_IDX][globalIdx2];
        }
      }
      else if(i == SOUTH_IDX ) {
        // to Macro
        if( j < macroStor.size() ) {
          auto mPtr = mckt.macroNameMap.find( macroStor[j].name );
          if( mPtr == mckt.macroNameMap.end()) {
            cout << "ERROR on macroNameMap: " << endl;
            exit(1);
          }
          int globalIdx2 = mPtr->second;
          netTable[ i*(macroStor.size()+4) + j] = 
            mckt.macroWeight[GLOBAL_SOUTH_IDX][globalIdx2];
        }
      }
    }
  }
}

void Partition::FillNetlistTableIncr() {
  //      if( macroStor.size() <= 1 ) {
  //        return;
  //      }

  for(size_t i=0; i<macroStor.size()+4; i++) {
    for(size_t j=0; j<macroStor.size()+4; j++) {
      double val = (i + j + 1)* 100;
      if( i == j || 
          (i >= macroStor.size() && j >= macroStor.size())) {
        val = 0;
      }
      netTable[ i*(macroStor.size()+4) + j] = val;
    }
  } 
}
    
void Partition::FillNetlistTableDesc() {
  //      if( macroStor.size() <= 1 ) {
  //        return;
  //      }

  for(size_t i=0; i<macroStor.size()+4; i++) {
    for(size_t j=0; j<macroStor.size()+4; j++) {
      double val = (2*macroStor.size()+8-(i+j)) * 100;
      if( i == j || 
          (i >= macroStor.size() && j >= macroStor.size())) {
        val = 0;
      }
      netTable[ i*(macroStor.size()+4) + j] = val;
    }
  } 
}


void Partition::UpdateMacroCoordi(MacroCircuit& mckt) {
  for(auto& curPartMacro : macroStor) {
    auto mPtr = mckt.macroNameMap.find(curPartMacro.name);
    int macroIdx = mPtr->second;
    curPartMacro.lx = mckt.macroStor[macroIdx].lx;
    curPartMacro.ly = mckt.macroStor[macroIdx].ly;
  }
}

// Call ParquetFP
bool Partition::DoAnneal(std::shared_ptr<Logger> log) {
  
  // No macro, no need to execute
  if( macroStor.size() == 0 ) {
    return true;
  }
  
  log->procBegin("Parquet");
  
  
  // Preprocessing in macroPlacer side
  // For nets and wts 
  vector< pair<int, int> > netStor;
  vector<int> costStor;

  netStor.reserve( (macroStor.size()+4)*(macroStor.size()+3)/2 );
  costStor.reserve( (macroStor.size()+4)*(macroStor.size()+3)/2 );

  for(size_t i=0; i<macroStor.size()+4; i++) {
    for(size_t j=i+1; j<macroStor.size()+4; j++) {
      int cost = 0;
      if( netTable ) {
        cost = netTable[ i*(macroStor.size()+4) + j] 
          + netTable[ j*(macroStor.size()+4) + i ];
      }
      if( cost != 0 ) {
        netStor.push_back( std::make_pair( std::min(i,j), std::max(i,j) ) );
        costStor.push_back(cost);
      }
    }
  }

  if( netStor.size() == 0 ) {
    for(size_t i=0; i<4; i++) {
      for(size_t j=i+1; j<4; j++) {
        netStor.push_back(std::make_pair(i, j));
        costStor.push_back(1);
      }
    }
  }
  
  using namespace parquetfp;

  // Populating DB structure
  // Instantiate Parquet DB structure
  DB db;
  Nodes* nodes = db.getNodes();
  Nets* nets = db.getNets();


  //////////////////////////////////////////////////////
  // Feed node structure: macro Info
  for(auto& curMacro : macroStor) {
  
    double padMacroWidth = curMacro.w + 2*(curMacro.haloX + curMacro.channelX);
    double padMacroHeight = curMacro.h + 2*(curMacro.haloY + curMacro.channelY);
//    cout << curMacro.w << " -> " << padMacroWidth << endl;
//    exit(1);

    Node tmpMacro ( std::string(curMacro.name.c_str()) , padMacroWidth * padMacroHeight, 
        padMacroWidth/padMacroHeight, padMacroWidth/padMacroHeight,
        &curMacro - &macroStor[0], false);

    tmpMacro.addSubBlockIndex(&curMacro - &macroStor[0]);

    // TODO
    // tmpMacro.putSnapX();
    // tmpMacro.putHaloX();
    // tmpMacro.putChannelX();

    nodes->putNewNode(tmpMacro);
  }

  // Feed node structure: terminal Info
  int indexTerm = 0;
  std::string pinNames[4] = {"West", "East", "North", "South"};
  double posX[4] = {0.0,         width,      width/2.0,  width/2.0};
  double posY[4] = {height/2.0,  height/2.0, height,     0.0f };
  for(int i=0; i<4; i++) {
    Node tmpPin(pinNames[i], 0, 1, 1, indexTerm++, true); 
    tmpPin.putX(posX[i]);
    tmpPin.putY(posY[i]);
    nodes->putNewTerm(tmpPin);
  }

  
  //////////////////////////////////////////////////////
  // Feed net / weight structure
  for(auto& curNet : netStor) {
    int idx = &curNet - &netStor[0];
    Net tmpEdge;

    parquetfp::pin tempPin1( GetName(curNet.first).c_str(), true, 0, 0, idx );
    parquetfp::pin tempPin2( GetName(curNet.second).c_str(), true, 0, 0, idx );

    tmpEdge.addNode(tempPin1);
    tmpEdge.addNode(tempPin2);
    tmpEdge.putIndex(idx);
    tmpEdge.putName(std::string("n"+std::to_string(idx)).c_str());
    tmpEdge.putWeight(costStor[idx]);

    nets->putNewNet(tmpEdge);
  }

  nets->updateNodeInfo(*nodes);
  nodes->updatePinsInfo(*nets);


  // Populate MixedBlockInfoType object
  // It is from DB object
  MixedBlockInfoTypeFromDB dbBlockInfo(db);
  MixedBlockInfoType* blockInfo = reinterpret_cast<MixedBlockInfoType*> (&dbBlockInfo);
 
  // Command_Line object populate
  Command_Line param;
  param.minWL = true;
  param.noRotation = true;
  param.FPrep = "BTree";
  param.seed = 100;
  param.scaleTerms = false;

  // Fixed-outline mode in Parquet
  param.nonTrivialOutline = parquetfp::BBox(0, 0, width, height);
  param.reqdAR = width/height;
  param.maxWS = 0;
  param.verb = "0 0 0";

  // Instantiate BTreeAnnealer Object
  BTreeAreaWireAnnealer* annealer = 
    new BTreeAreaWireAnnealer(*blockInfo, const_cast<Command_Line*>(&param), &db);

  annealer->go();

  const BTree& sol = annealer->currSolution();
  // Failed annealing
  if( sol.totalWidth() > width ||
      sol.totalHeight() > height ) {
    log->infoString("Parquet BBOX exceed the given area");
    log->infoFloatSignificantPair("ParquetSolLayout", sol.totalWidth(), sol.totalHeight());
    log->infoFloatSignificantPair("TargetLayout", width, height);
    return false;
  }
  delete annealer;

  // 
  // flip info initialization for each partition
  bool isFlipX = false, isFlipY = false;
  switch(partClass) {
    // y flip
    case NW:
      isFlipY = true;
      break;
    // x, y flip
    case NE:
      isFlipX = isFlipY = true; 
      break;
    // NonFlip 
    case SW:
      break;
    // x flip
    case SE: 
      isFlipX = true;
      break;
    // very weird
    default: 
      break;
  }

  // update back into macroPlacer 
  for(size_t i=0; i<nodes->getNumNodes(); i++) {
    Node& curNode = nodes->getNode(i);

    macroStor[i].lx = (isFlipX)? 
      width - curNode.getX() - curNode.getWidth() + lx :
      curNode.getX() + lx;
    macroStor[i].ly = (isFlipY)?
      height - curNode.getY() - curNode.getHeight() + ly :
      curNode.getY() + ly;

    macroStor[i].lx += (macroStor[i].haloX + macroStor[i].channelX);
    macroStor[i].ly += (macroStor[i].haloY + macroStor[i].channelY); 
  }

//  db.plot( "out.plt", 0, 0, 0, 0, 0,
//      0, 1, 1,  // slack, net, name 
//      true, 
//      0, 0, width, height);

  log->procEnd("Parquet");
  return true;

}

void Partition::PrintSetFormat(FILE* fp) {
  string sliceStr = "";
  if( partClass == ALL ) {
    sliceStr = "ALL";
  }
  else if( partClass == NW ) {
    sliceStr = "NW";
  }
  else if( partClass == NE ) {
    sliceStr = "NE";
  }
  else if (partClass == SW) {
    sliceStr = "SW";
  }
  else if (partClass == SE) {
    sliceStr = "SE";
  }
  else if (partClass == E) {
    sliceStr = "E";
  }
  else if (partClass == W) {
    sliceStr = "W";
  }
  else if (partClass == S) {
    sliceStr = "S";
  }
  else if (partClass == N) {
    sliceStr = "N";
  }

  fprintf(fp,"  BEGIN SLICE %s %ld ;\n", sliceStr.c_str(), macroStor.size() );
  fprintf(fp,"    LX %f ;\n", lx);
  fprintf(fp,"    LY %f ;\n", ly);
  fprintf(fp,"    WIDTH %f ;\n", width);
  fprintf(fp,"    HEIGHT %f ;\n", height);
  for(auto& curMacro : macroStor) {
    fprintf(fp,"    MACRO %s %s %f %f %f %f ;\n", 
        curMacro.name.c_str(), curMacro.type.c_str(), 
        curMacro.lx, curMacro.ly, curMacro.w, curMacro.h);
  }

  if( netTable ) {
    fprintf(fp, "    NETLISTTABLE \n    ");
    for(size_t i=0; i<macroStor.size()+4; i++) {
      for(size_t j=0; j<macroStor.size()+4; j++) {
        fprintf(fp, "%.3f ", netTable[(macroStor.size()+4)*i + j]);
      }
      if( i == macroStor.size()+3 ) {
        fprintf(fp, "; \n");
      }
      else {
        fprintf(fp, "\n    ");
      }
    }
  }
  else {
    fprintf(fp,"    NETLISTTABLE ;\n");
  }
  
  fprintf(fp,"  END SLICE ;\n");
  fflush(fp);
}

}
