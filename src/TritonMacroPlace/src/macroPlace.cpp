#include "circuit.h"
#include "partition.h" 
#include "opendb/db.h"
#include "logger.h"
#include <unordered_set>
#include <memory>

using std::string;
using std::vector;
using std::pair;
using std::unordered_map;
using std::unordered_set;
using std::cout;
using std::endl;
//using MacroPlace::Partition;

using namespace MacroPlace;
using namespace odb;

namespace MacroPlace { 

typedef vector<pair<Partition, Partition>> TwoPartitions;

static vector<pair<Partition, Partition>> GetPart(
    Layout &layout,  
    const double siteSizeX,
    const double siteSizeY,
    Partition& partition, 
    bool isHorizontal,
    std::shared_ptr<Logger> log);

static void UpdateMacroPartMap( 
    MacroCircuit& mckt,
    MacroPlace::Partition& part, 
    unordered_map<MacroPlace::PartClass, vector<int>, 
    PartClassHash, PartClassEqual> &macroPartMap );


static void 
CutRoundUp( Layout& layout, 
    const double siteSizeX, 
    const double siteSizeY, 
    double& cutLine, bool isHorizontal );

static void 
PrintAllSets(FILE* fp, Layout& layout, 
    vector< vector<Partition> >& allSets);

static void 
UpdateOpendbCoordi(dbDatabase* db, MacroCircuit& mckt); 

void 
MacroCircuit::PlaceMacros(int& solCount) {
  init();
  Layout layout(lx_, ly_, ux_, uy_);


  //  RandomPlace for special needs. 
  //  Really not recommended to execute this functioning 
  //if( mckt.isRandomPlace() == true ) {
  //  double snapGrid = 0.02f;
  //  mckt.StubPlacer(snapGrid);
  //}

  bool isHorizontal = true;

  Partition topLayout(PartClass::ALL, 
      layout.lx(), layout.ly(), layout.ux()-layout.lx(), layout.uy()-layout.ly());
  topLayout.macroStor = macroStor;

  log_->procBegin("One Level Partition");

  TwoPartitions oneLevelPart 
    = GetPart(layout, siteSizeX_, siteSizeY_, topLayout, isHorizontal, log_);
  
  log_->procEnd("One Level Partition");
  TwoPartitions eastStor, westStor;

  vector< vector<Partition> > allSets;

  // Fill the MacroPlace for ALL circuits

  unordered_map< PartClass, vector<int>, 
    PartClassHash, PartClassEqual> globalMacroPartMap;
  UpdateMacroPartMap( *this, topLayout, globalMacroPartMap );

  if( isTiming_ ) {
    topLayout.FillNetlistTable( *this, globalMacroPartMap );
    UpdateNetlist(topLayout);
  }

 
  // push to the outer vector 
  vector<Partition> layoutSet;
  layoutSet.push_back(topLayout);

  // push
  allSets.push_back(layoutSet);

  for(auto& curSet : oneLevelPart ) {
    if( isHorizontal ) {
      log_->procBegin("Horizontal Partition");
      Layout eastInfo(layout, curSet.first);
      Layout westInfo(layout, curSet.second);

      log_->procBegin("East Partition");
      TwoPartitions eastStor 
        = GetPart(eastInfo, siteSizeX_, siteSizeY_, curSet.first, !isHorizontal, log_);
      log_->procEnd("East Partition");

      log_->procBegin("West Partition");
      TwoPartitions westStor 
        = GetPart(westInfo, siteSizeX_, siteSizeY_, curSet.second, !isHorizontal, log_);
      log_->procEnd("West Partition");

     
      // Zero case handling when eastStor = 0 
      if( eastStor.size() == 0 && westStor.size() != 0 ) {
        for(size_t i=0; i<westStor.size(); i++) {
          vector<Partition> oneSet;

          // one set is composed of two subblocks
          oneSet.push_back( westStor[i].first );
          oneSet.push_back( westStor[i].second );

          // Fill Macro Netlist
          // update macroPartMap
          unordered_map< PartClass, vector<int>, 
            PartClassHash, PartClassEqual> macroPartMap;
          for(auto& curSet: oneSet) {
            UpdateMacroPartMap( *this, curSet, macroPartMap );
          }
          
          if( isTiming_ ) { 
            for(auto& curSet: oneSet) {
              curSet.FillNetlistTable( *this, macroPartMap );
            }
          }

          allSets.push_back( oneSet );
        }
      }
      // Zero case handling when westStor = 0 
      else if( eastStor.size() != 0 && westStor.size() == 0 ) {
        for(size_t i=0; i<eastStor.size(); i++) {
          vector<Partition> oneSet;

          // one set is composed of two subblocks
          oneSet.push_back( eastStor[i].first );
          oneSet.push_back( eastStor[i].second );

          // Fill Macro Netlist
          // update macroPartMap
          unordered_map< PartClass, vector<int>, 
            PartClassHash, PartClassEqual> macroPartMap;
          for(auto& curSet: oneSet) {
            UpdateMacroPartMap( *this, curSet, macroPartMap );
          }
          
          if( isTiming_ ) { 
            for(auto& curSet: oneSet) {
              curSet.FillNetlistTable( *this, macroPartMap );
            }
          }

          allSets.push_back( oneSet );
        } 
      }
      else {
        // for all possible combinations in partitions
        for(size_t i=0; i<eastStor.size(); i++) {
          for(size_t j=0; j<westStor.size(); j++) {

            vector<Partition> oneSet;

            // one set is composed of four subblocks
            oneSet.push_back( eastStor[i].first );
            oneSet.push_back( eastStor[i].second );
            oneSet.push_back( westStor[j].first );
            oneSet.push_back( westStor[j].second );

            // Fill Macro Netlist
            // update macroPartMap
            unordered_map< PartClass, vector<int>, 
              PartClassHash, PartClassEqual> macroPartMap;
            for(auto& curSet: oneSet) {
              UpdateMacroPartMap( *this, curSet, macroPartMap );
            }

            if( isTiming_ ) { 
              for(auto& curSet: oneSet) {
                curSet.FillNetlistTable( *this, macroPartMap );
              }
            }

            allSets.push_back( oneSet );
          }
        }
      } 
      log_->procEnd("Horizontal Partition");
    }
    else {
      log_->procBegin("Vertical Partition");
      // TODO
      log_->procEnd("Vertical Partition");
    }
  }
  log_->infoInt( "NumExtractedSets", allSets.size() -1);

  solCount = 0;
  int bestSetIdx = 0;
  double bestWwl = -DBL_MAX;
  for(auto& curSet: allSets) {
    // skip for top-topLayout partition
    if( curSet.size() == 1) {
      continue;
    }
    // For each partitions (four partition)
    //
    bool isFailed = false;
    for(auto& curPart : curSet) {
      // Annealing based on ParquetFP Engine
      if( !curPart.DoAnneal(log_) ) {
        isFailed = true;
        break;
      }
      // Update mckt frequently
      UpdateMacroCoordi(curPart);
    }
    if( isFailed ) {
      continue;
    }

    // update partitons' macro info
    for(auto& curPart : curSet) { 
      curPart.UpdateMacroCoordi(*this);
    }

    double curWwl = GetWeightedWL();
    log_->infoInt("SetId", &curSet - &allSets[0]);
    log_->infoFloat("WeightedWL", curWwl);

    if( curWwl > bestWwl ) {
      bestWwl = curWwl;
      bestSetIdx = &curSet - &allSets[0]; 
    }
    solCount++;
  }
  
  log_->infoInt( "NumFinalSols", solCount);

  // bestset DEF writing
  std::vector<MacroPlace::Partition> bestSet = allSets[bestSetIdx];

  for( auto& curBestPart: bestSet) { 
    UpdateMacroCoordi(curBestPart);
  }
  UpdateOpendbCoordi(db_, *this); 
  
}


// 
// update opendb dataset from mckt.
static void 
UpdateOpendbCoordi(dbDatabase* db, MacroCircuit& mckt) {
  dbTech* tech = db->getTech();
  const int dbu = tech->getDbUnitsPerMicron();
  
  for(auto& curMacro : mckt.macroStor) {
    curMacro.dbInstPtr->setLocation( 
        static_cast<int>(round(curMacro.lx * dbu)), 
        static_cast<int>(round(curMacro.ly * dbu))) ;
    curMacro.dbInstPtr->setPlacementStatus( dbPlacementStatus::LOCKED ) ;
  }
}

static void 
CutRoundUp( 
    Layout& layout,
    const double siteSizeX, 
    const double siteSizeY,  
    double& cutLine, bool isHorizontal ) {

  if( isHorizontal ) {
    int integer = static_cast<int>( round( static_cast<float>(cutLine) / siteSizeX) );
    cutLine = integer * siteSizeX;
    cutLine = fmin(cutLine, layout.ux());
    cutLine = fmax(cutLine, layout.lx());
  }
  else {
    int integer = static_cast<int>( round( static_cast<float>(cutLine) / siteSizeY) );
    cutLine = integer * siteSizeY;
    cutLine = fmin(cutLine, layout.uy());
    cutLine = fmax(cutLine, layout.ly());
  }
}

// using mckt.macroInstMap and partition, 
// fill in macroPartMap
//
// macroPartMap will contain 
// 
// first: macro partition class info 
// second: macro candidates.
static void UpdateMacroPartMap( 
    MacroCircuit& mckt,
    MacroPlace::Partition& part, 
    unordered_map<MacroPlace::PartClass, vector<int>, 
    PartClassHash, PartClassEqual>& macroPartMap ) {


  auto mpPtr = macroPartMap.find( part.partClass );
  if( mpPtr == macroPartMap.end() ) {
    vector<int> curMacroStor;
    // convert macro Information into macroIdx
    for(auto& curMacro: part.macroStor) {
      auto miPtr = mckt.macroInstMap.find( curMacro.staInstPtr );
      if( miPtr == mckt.macroInstMap.end() ) {
        cout << "ERROR: macro " << curMacro.name 
          << " not exists in macroInstMap: " << curMacro.staInstPtr << endl;
        exit(1);
      }
      curMacroStor.push_back( miPtr->second) ;
    }
    macroPartMap[ part.partClass ] = curMacroStor; 
  }
  else {
    cout << "ERROR: Partition- " << part.partClass 
      << " already updated (UpdateMacroPartMap)" << endl; 
    exit(1);
  }
}


// only considers lx or ly coordinates for sorting
static bool 
SortMacroPair(const std::pair<int, double> &p1, 
    const std::pair<int, double> &p2 ) {
  return p1.second < p2.second;
}

// Two partitioning functions:
// first : lower part
// second : upper part
// 
// cutLine is sweeping from lower to upper coordinates in x / y
static vector<pair<Partition, Partition>> GetPart(
    Layout &layout,  
    const double siteSizeX,
    const double siteSizeY,
    Partition& partition, 
    bool isHorizontal,
    std::shared_ptr<Logger> log) {
  log->procBegin("Partition");
  log->infoInt("NumMacros", partition.macroStor.size());

  // Return vector
  vector<pair<Partition, Partition>> ret;
  
  double maxWidth = -1e30;
  double maxHeight = -1e30;
  
  // segment stor
  // first: macroStor index
  // second: lx or ly values
  vector<std::pair<int, double>> segStor;
  
  // in parent partition, traverse macros
  for(auto& curMacro: partition.macroStor) {
    segStor.push_back( 
        std::make_pair( &curMacro - &partition.macroStor[0], 
          (isHorizontal)? curMacro.lx : curMacro.ly ));
    
    maxWidth = std::fmax( maxWidth, curMacro.w );
    maxHeight = std::fmax( maxHeight, curMacro.h );
  }

  double cutLineLimit = (isHorizontal)? maxWidth * 0.25 : maxHeight * 0.25;
  double prevPushLimit = -1e30;
  bool isFirst = true;
  vector<double> cutLineStor;
 
  // less than 4
  if( partition.macroStor.size() <= 4 ) {
    sort(segStor.begin(), segStor.end(), SortMacroPair);

    // first : macroStor index
    // second : macro lower coordinates
    for(auto& segPair: segStor) {
      if( isFirst ) {
        cutLineStor.push_back( segPair.second );
        prevPushLimit = segPair.second;
        isFirst = false;
      }
      else if( std::abs(segPair.second -prevPushLimit) > cutLineLimit ) {
        cutLineStor.push_back( segPair.second );
        prevPushLimit = segPair.second;
      }
    }
  }
  // more than 4
  else {
    int hardLimit = int( sqrt( 1.0*partition.macroStor.size()/3.0 ) + 0.5f);
    for(int i=0; i<=hardLimit; i++) {
      cutLineStor.push_back( (isHorizontal)? 
          layout.lx() + (layout.ux() - layout.lx())/hardLimit * i :
          layout.ly() + (layout.uy() - layout.ly())/hardLimit * i );
    }
  }
  log->infoInt("NumCutLines", cutLineStor.size());
  
  // Macro checker array
  // 0 for uninitialize
  // 1 for lower
  // 2 for upper
  // 3 for both
  int* chkArr = new int[partition.macroStor.size()];
  
  for(auto& cutLine : cutLineStor ) {
    log->infoInt("CutLine", cutLine);
    CutRoundUp(layout, siteSizeX, siteSizeY, cutLine, isHorizontal);
    
    log->infoInt("RoundUpCutLine", cutLine);

   
    // chkArr initialize 
    for(size_t i=0; i<partition.macroStor.size(); i++) {
      chkArr[i] = 0;
    }
  
    bool isImpossible = false;
    for(auto& curMacro : partition.macroStor) {
      int i = &curMacro - &partition.macroStor[0];
      if( isHorizontal ) {
        // lower is possible
        if( curMacro.w <= cutLine ) {
          chkArr[i] += 1;
        }
        // upper is possible
        if ( curMacro.w <= partition.lx + partition.width - cutLine) {
          chkArr[i] += 2; 
        }
        // none of them
        if( chkArr[i] == 0 ) {
          isImpossible = true;
          break;
        }
      }
      else {
        // lower is possible
        if( curMacro.h <= cutLine ) {
          chkArr[i] += 1;
        }
        // upper is possible
        if (curMacro.h <= partition.ly + partition.height - cutLine) {
          chkArr[i] += 2;
        }
        // none of 
        if( chkArr[i] == 0 ) {
          isImpossible = true;
          break;
        }
      }
    } 
    // impossible cuts, then skip 
    if( isImpossible ) {
      continue;
    }

    // Fill in the Partitioning information
    PartClass lClass = None, uClass = None;
    if( partition.partClass == MacroPlace::PartClass::ALL ) {

      lClass = (isHorizontal)? W : S;
      uClass = (isHorizontal)? E : N;
    }

    if( partition.partClass == W) {
      lClass = SW;
      uClass = NW;
    
    }
    if( partition.partClass == E) {
      lClass = SE;
      uClass = NE;
    }

    if( partition.partClass == N) {
      lClass = NW;
      uClass = NE;
    }

    if( partition.partClass == S) {
      lClass = SW;
      uClass = SE;
    }

    Partition lowerPart( lClass, 
      partition.lx, 
      partition.ly, 
      (isHorizontal)? cutLine - partition.lx : partition.width, 
      (isHorizontal)? partition.height : cutLine - partition.ly); 

    Partition upperPart( uClass, 
      (isHorizontal)? cutLine : partition.lx, 
      (isHorizontal)? partition.ly : cutLine,
      (isHorizontal)? partition.lx + partition.width - cutLine : partition.width, 
      (isHorizontal)? partition.height : partition.ly + partition.height - cutLine);

    //
    // Fill in child partitons' macroStor
    for(auto& curMacro : partition.macroStor) {
      int i=&curMacro - &partition.macroStor[0];
      if( chkArr[i] == 1 ) {
        lowerPart.macroStor.push_back( 
            Macro( curMacro.name, curMacro.type,
              curMacro.lx, curMacro.ly,
              curMacro.w, curMacro.h,
              curMacro.haloX, curMacro.haloY,
              curMacro.channelX, curMacro.channelY, 
              curMacro.ptr, curMacro.staInstPtr,
              curMacro.dbInstPtr )) ; 
      }
      else if( chkArr[i] == 2 ) {
        upperPart.macroStor.push_back(
            Macro( curMacro.name, curMacro.type,
              (isHorizontal)? curMacro.lx-cutLine : curMacro.lx, 
              (isHorizontal)? curMacro.ly : curMacro.ly-cutLine,
              curMacro.w, curMacro.h,
              curMacro.haloX, curMacro.haloY,
              curMacro.channelX, curMacro.channelY, 
              curMacro.ptr, curMacro.staInstPtr,
              curMacro.dbInstPtr));
      }
      else if( chkArr[i] == 3 ) {
        double centerPoint = 
          (isHorizontal)? 
          curMacro.lx + curMacro.w/2.0 :
          curMacro.ly + curMacro.h/2.0;

        if( centerPoint < cutLine ) {
          lowerPart.macroStor.push_back( 
              Macro( curMacro.name, curMacro.type,
                curMacro.lx, curMacro.ly,
                curMacro.w, curMacro.h,
                curMacro.haloX, curMacro.haloY,
                curMacro.channelX, curMacro.channelY, 
                curMacro.ptr, curMacro.staInstPtr,
                curMacro.dbInstPtr )) ; 
        
        }
        else {
          upperPart.macroStor.push_back(
              Macro( curMacro.name, curMacro.type,
                (isHorizontal)? curMacro.lx-cutLine : curMacro.lx, 
                (isHorizontal)? curMacro.ly : curMacro.ly-cutLine,
                curMacro.w, curMacro.h,
                curMacro.haloX, curMacro.haloY,
                curMacro.channelX, curMacro.channelY, 
                curMacro.ptr, curMacro.staInstPtr,
                curMacro.dbInstPtr));
        }
      }
    }
    
    double lowerArea = lowerPart.width * lowerPart.height;
    double upperArea = upperPart.width * upperPart.height;

    double upperMacroArea = 0.0f;
    double lowerMacroArea = 0.0f;

    for(auto& curMacro : upperPart.macroStor) {
      upperMacroArea += curMacro.w * curMacro.h;
    } 
    for(auto& curMacro : lowerPart.macroStor) {
      lowerMacroArea += curMacro.w * curMacro.h;
    }
    
    // impossible partitioning
    if( upperMacroArea > upperArea || lowerMacroArea > lowerArea) {
      log->infoString("Meets impossible partition, continue");
      continue;
    }

    pair<Partition, Partition> curPart( lowerPart, upperPart );
    ret.push_back( curPart );
  }
  delete[] chkArr;
  log->procEnd("Partition");
  
  return ret; 
}

}
