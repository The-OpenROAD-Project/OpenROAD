// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "odb/dbBlockSet.h"
#include "odb/dbCCSegSet.h"
#include "odb/dbDatabaseObserver.h"
#include "odb/dbMatrix.h"
#include "odb/dbNetSet.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/dbViaParams.h"
#include "odb/geom.h"
#include "odb/isotropy.h"

inline constexpr int ADS_MAX_CORNER = 10;
inline constexpr const char* kDefaultBufBaseName{"buf"};
inline constexpr const char* kDefaultNetBaseName{"net"};

namespace utl {
class Logger;
}

namespace odb {

class dbShape;
class lefout;
class dbViaParams;
class dbTransform;

template <class T>
class dbId;

// Forward declarations of all database objects
class dbBox;
class dbJournalEntry;

// Property objects
class dbBoolProperty;
class dbStringProperty;
class dbIntProperty;
class dbDoubleProperty;

// Design objects
class dbBlock;
class dbBTerm;
class dbNet;
class dbInst;
class dbITerm;
class dbVia;
class dbTrackGrid;
class dbObstruction;
class dbBlockage;
class dbWire;
class dbSWire;
class dbSBox;
class dbCapNode;
class dbRSeg;
class dbCCSeg;
class dbBlockSearch;
class dbRow;
class dbFill;
class dbTechAntennaPinModel;
class dbBlockCallBackObj;
class dbRegion;
class dbBPin;

// Lib objects
class dbLib;
class dbSite;
class dbMaster;
class dbMTerm;
class dbMPin;
class dbGDSLib;

// Tech objects
class dbTech;
class dbTechVia;
class dbTechViaRule;
class dbTechViaLayerRule;
class dbTechViaGenerateRule;
class dbTechNonDefaultRule;
class dbTechLayerRule;
class dbTechLayerSpacingRule;
class dbTechLayerAntennaRule;
class dbTechMinCutRule;
class dbTechMinEncRule;
class dbTechV55InfluenceEntry;
class dbTechSameNetRule;
class dbViaParams;

// Generator Code Begin ClassDeclarations
class dbAccessPoint;
class dbBusPort;
class dbCellEdgeSpacing;
class dbChip;
class dbChipBump;
class dbChipBumpInst;
class dbChipConn;
class dbChipInst;
class dbChipNet;
class dbChipRegion;
class dbChipRegionInst;
class dbDatabase;
class dbDft;
class dbGCellGrid;
class dbGDSARef;
class dbGDSBoundary;
class dbGDSBox;
class dbGDSPath;
class dbGDSSRef;
class dbGDSStructure;
class dbGDSText;
class dbGlobalConnect;
class dbGroup;
class dbGuide;
class dbIsolation;
class dbLevelShifter;
class dbLogicPort;
class dbMarker;
class dbMarkerCategory;
class dbMasterEdgeType;
class dbMetalWidthViaMap;
class dbModBTerm;
class dbModInst;
class dbModITerm;
class dbModNet;
class dbModule;
class dbNetTrack;
class dbPolygon;
class dbPowerDomain;
class dbPowerSwitch;
class dbProperty;
class dbScanChain;
class dbScanInst;
class dbScanList;
class dbScanPartition;
class dbScanPin;
class dbTechLayer;
class dbTechLayerAreaRule;
class dbTechLayerArraySpacingRule;
class dbTechLayerCornerSpacingRule;
class dbTechLayerCutClassRule;
class dbTechLayerCutEnclosureRule;
class dbTechLayerCutSpacingRule;
class dbTechLayerCutSpacingTableDefRule;
class dbTechLayerCutSpacingTableOrthRule;
class dbTechLayerEolExtensionRule;
class dbTechLayerEolKeepOutRule;
class dbTechLayerForbiddenSpacingRule;
class dbTechLayerKeepOutZoneRule;
class dbTechLayerMaxSpacingRule;
class dbTechLayerMinCutRule;
class dbTechLayerMinStepRule;
class dbTechLayerSpacingEolRule;
class dbTechLayerSpacingTablePrlRule;
class dbTechLayerTwoWiresForbiddenSpcRule;
class dbTechLayerVoltageSpacing;
class dbTechLayerWidthTableRule;
class dbTechLayerWrongDirSpacingRule;
// Generator Code End ClassDeclarations

// Extraction Objects
class dbExtControl;

// Custom iterators
class dbModuleBusPortModBTermItr;

///////////////////////////////////////////////////////////////////////////////
///
/// A box is the element used to represent layout shapes.
///
///////////////////////////////////////////////////////////////////////////////
class dbBox : public dbObject
{
 public:
  ///
  /// Get the lower coordinate.
  ///
  int xMin() const;

  ///
  /// Get the lower y coordinate.
  ///
  int yMin() const;

  ///
  /// Get the high x coordinate.
  ///
  int xMax() const;

  ///
  /// Get the high y coordinate.
  ///
  int yMax() const;

  ///
  /// Returns true if this box represents a via
  ///
  bool isVia() const;

  ///
  /// Get tech-via this box represents.
  /// returns nullptr if this box does not represent a tech-via
  ///
  dbTechVia* getTechVia() const;

  ///
  /// Get block-via this box represents.
  /// returns nullptr if this box does not represent a block-via
  ///
  dbVia* getBlockVia() const;

  ///
  /// Return the placed location of this via.
  ///
  Point getViaXY() const;

  ///
  /// Get the box bounding points.
  ///
  Rect getBox() const;

  ///
  /// Get the translated boxes of this via
  ///
  void getViaBoxes(std::vector<dbShape>& shapes) const;

  ///
  /// Get the translated boxes of this via on the given layer
  ///
  void getViaLayerBoxes(dbTechLayer* layer, std::vector<dbShape>& shapes) const;

  ///
  /// Get the orientation of the box.
  ///
  Orientation2D getDir() const;

  ///
  /// Get the width (xMax-xMin) of the box.
  ///
  uint32_t getDX() const;

  int getDesignRuleWidth() const;

  void setDesignRuleWidth(int);

  ///
  /// Get the height (yMax-yMin) of the box.
  ///
  uint32_t getDY() const;

  ///
  /// Set temporary flag visited
  ///
  void setVisited(bool value);
  bool isVisited() const;

  ///
  /// Get the owner of this box
  ///
  dbObject* getBoxOwner() const;

  ///
  /// Get the owner type of this box
  ///
  dbBoxOwner getOwnerType() const;

  ///
  /// Get the layer of this box.
  /// Returns nullptr if this shape is an object bbox.
  /// These bboxes have no layer.
  ///     dbBlock    - bbox has no layer
  ///     dbInst     - bbox has no layer
  ///     dbVia      - bbox has no layer
  ///     dbTechVia  - bbox has no layer
  ///
  /// These bboxes have no layer.
  ///    All dbBox(s) that represent VIA's.
  ///
  dbTechLayer* getTechLayer() const;

  ///
  /// Get the layer mask assigned to this box.
  /// Returns 0 is not assigned or bbox has no layer
  ///
  uint32_t getLayerMask() const;

  ///
  /// Sets the layer mask for this box.
  ///
  void setLayerMask(uint32_t mask);

  ///
  /// Add a physical pin to a dbBPin.
  /// Returns nullptr if this dbBPin already has a pin.
  ///
  static dbBox* create(dbBPin* bpin,
                       dbTechLayer* layer,
                       int x1,
                       int y1,
                       int x2,
                       int y2,
                       uint32_t mask = 0);

  ///
  /// Add a box to a block-via.
  ///
  static dbBox* create(dbVia* via,
                       dbTechLayer* layer,
                       int x1,
                       int y1,
                       int x2,
                       int y2);

  ///
  /// Add an obstruction to a master.
  ///
  static dbBox* create(dbMaster* master,
                       dbTechLayer* layer,
                       int x1,
                       int y1,
                       int x2,
                       int y2);

  ///
  /// Add a via obstrction to a master.
  /// This function may fail and return nullptr if this via has no shapes.
  ///
  static dbBox* create(dbMaster* master, dbTechVia* via, int x, int y);

  ///
  /// Add a wire-shape to a master-pin.
  ///
  static dbBox* create(dbMPin* pin,
                       dbTechLayer* layer,
                       int x1,
                       int y1,
                       int x2,
                       int y2);

  ///
  /// Add a wire-shape to a polygon.
  ///
  static dbBox* create(dbPolygon* pbox, int x1, int y1, int x2, int y2);

  ///
  /// Add a via obstrction to a master-pin.
  /// This function may fail and return nullptr if this via has no shapes.
  ///
  static dbBox* create(dbMPin* pin, dbTechVia* via, int x, int y);

  ///
  /// Add a shape to a tech-via;
  ///
  static dbBox* create(dbTechVia* via,
                       dbTechLayer* layer,
                       int x1,
                       int y1,
                       int x2,
                       int y2);

  ///
  /// Add a boundary to a region
  ///
  static dbBox* create(dbRegion* region, int x1, int y1, int x2, int y2);

  ///
  /// Create a halo on an instance.
  ///
  static dbBox* create(dbInst* inst, int x1, int y1, int x2, int y2);

  // Destroy box
  static void destroy(dbBox* box);

  ///
  /// Translate a database-id back to a pointer.
  /// This function translates any dbBox which is part of a block.
  ///
  static dbBox* getBox(dbBlock* block, uint32_t oid);

  /// Translate a database-id back to a pointer.
  /// This function translates any dbBox which is part of a tech.
  ///
  static dbBox* getBox(dbTech* tech, uint32_t oid);

  ///
  /// Translate a database-id back to a pointer.
  /// This function translates any dbBox whichs is part of a master.
  ///
  static dbBox* getBox(dbMaster* master, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A sbox is the element used to represent special layout shapes.
///
///////////////////////////////////////////////////////////////////////////////
class dbSBox : public dbBox
{
 public:
  /// Direction of segment
  enum Direction
  {
    UNDEFINED = 0,
    HORIZONTAL = 1,
    VERTICAL = 2,
    OCTILINEAR = 3
  };

  ///
  /// Get the shape type of this wire.
  ///
  dbWireShapeType getWireShapeType() const;

  ///
  /// Return the specified direction of this segment
  ///
  Direction getDirection() const;

  ///
  /// Get the swire of this shape
  ///
  dbSWire* getSWire() const;

  ///
  /// Get Oct Wire Shape
  ///
  Oct getOct() const;

  ///
  /// Get via mask for bottom layer of via
  ///
  uint32_t getViaBottomLayerMask() const;

  ///
  /// Get via mask for cut layer of via
  ///
  uint32_t getViaCutLayerMask() const;

  ///
  /// Get via mask for top layer of via
  ///
  uint32_t getViaTopLayerMask() const;

  ///
  /// Set via masks
  ///
  void setViaLayerMask(uint32_t bottom, uint32_t cut, uint32_t top);

  ///
  /// Has via mask
  ///
  bool hasViaLayerMasks() const;

  ///
  /// Create a set of new sboxes from a via array
  ///
  std::vector<dbSBox*> smashVia();

  ///
  /// Add a rect to a dbSWire.
  ///
  /// If direction == UNDEFINED
  ///    |(x2-x1)| must be an even number or |(y2-y1)| must be an even number
  ///
  /// If direction == HORIZONTAL
  ///    |(y2-y1)| must be an even number
  ///
  /// If direction == VERTICAL
  ///    |(x2-x1)| must be an even number
  ///
  /// If the direction sementics are not met, this function will return nullptr.
  ///
  /// These requirements are a result that the current DEF semantics (5.5) use
  /// PATH statements to output these rectangles, the paths must have even
  /// widths.
  static dbSBox* create(dbSWire* swire,
                        dbTechLayer* layer,
                        int x1,
                        int y1,
                        int x2,
                        int y2,
                        dbWireShapeType type,
                        Direction dir = UNDEFINED,
                        int width = 0);

  ///
  /// Add a block-via to a dbSWire
  /// This function may fail and return nullptr if this via has no shapes.
  ///
  static dbSBox* create(dbSWire* swire,
                        dbVia* via,
                        int x,
                        int y,
                        dbWireShapeType type);

  ///
  /// Add a tech-via to a dbSWire.
  /// This function may fail and return nullptr if this via has no shapes.
  ///
  static dbSBox* create(dbSWire* swire,
                        dbTechVia* via,
                        int x,
                        int y,
                        dbWireShapeType type);

  ///
  /// Translate a database-id back to a pointer.
  /// This function translates any dbBox whichs is part of a block
  ///
  static dbSBox* getSBox(dbBlock* block, uint32_t oid);

  ///
  /// Destroy a SBox.
  ///
  static void destroy(dbSBox* box);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A Block is the element used to represent a layout-netlist.
/// A Block can have multiple children, however, currently only two-levels
/// of hierarchy is supported.
///
///////////////////////////////////////////////////////////////////////////////
class dbBlock : public dbObject
{
 public:
  struct dbBTermGroup
  {
    std::vector<dbBTerm*> bterms;
    bool order = false;
  };

  struct dbBTermTopLayerGrid
  {
    // The single top-most routing layer of the placement grid.
    dbTechLayer* layer = nullptr;
    // The distance between each valid position on the grid in the x- and
    // y-directions, respectively.
    int x_step = 0;
    int y_step = 0;
    // The region of the placement grid.
    Polygon region;
    // The width and height of the pins assigned to this grid. The centers of
    // the pins are placed on the grid positions.
    int pin_width = 0;
    int pin_height = 0;
    // The boundary around existing routing obstructions that the pins should
    // avoid.
    int keepout = 0;
  };

  ///
  /// Get block chip name.
  ///
  std::string getName();

  ///
  /// Get the block chip name.
  ///
  const char* getConstName();

  ///
  /// Get the bounding box of this block.
  ///
  dbBox* getBBox();

  ///
  /// Get the chip this block belongs too.
  ///
  dbChip* getChip();

  ///
  /// Get the database this block belongs too.
  ///
  dbDatabase* getDataBase();

  ///
  /// Get the technology of this block
  ///
  dbTech* getTech();

  ///
  /// Get the parent block this block. Returns nullptr if this block is the
  /// top-block of the chip.
  ///
  dbBlock* getParent();

  ///
  /// Returns the hierarchical parent of this block if it exists.
  ///
  dbInst* getParentInst();

  ///
  /// Returns the top module of this block.
  ///
  dbModule* getTopModule() const;

  ///
  /// Get the child blocks of this block.
  ///
  dbSet<dbBlock> getChildren();

  ///
  /// Find a specific child-block of this block.
  /// Returns nullptr if the object was not found.
  ///
  dbBlock* findChild(const char* name);

  ///
  /// Get all the block-terminals of this block.
  ///
  dbSet<dbBTerm> getBTerms();

  ///
  /// Find a specific bterm of this block.
  /// Returns nullptr if the object was not found.
  ///
  dbBTerm* findBTerm(const char* name);

  ///
  /// Get all the bterm groups of this block.
  ///
  std::vector<dbBTermGroup> getBTermGroups();

  ///
  /// Get all the block-terminals of this block.
  /// The flag order places the pins ordered in ascending x/y position.
  ///
  void addBTermGroup(const std::vector<dbBTerm*>& bterms, bool order);

  ///
  /// Define the top layer grid for pin placement.
  ///
  void setBTermTopLayerGrid(const dbBTermTopLayerGrid& top_layer_grid);

  ///
  /// Get the top layer grid for pin placement.
  ///
  std::optional<dbBTermTopLayerGrid> getBTermTopLayerGrid();

  ///
  /// Get only the polygon corresponding to the top layer grid region.
  ///
  Polygon getBTermTopLayerGridRegion();

  ///
  /// Find the rectangle corresponding to the constraint region in a specific
  /// edge of the die area.
  ///
  Rect findConstraintRegion(const Direction2D& edge, int begin, int end);

  ///
  /// Add region constraint for dbBTerms according to their IO type.
  ///
  void addBTermConstraintByDirection(dbIoType direction,
                                     const Rect& constraint_region);

  ///
  /// Add region constraint for dbBTerms.
  ///
  void addBTermsToConstraint(const std::vector<dbBTerm*>& bterms,
                             const Rect& constraint_region);

  ///
  /// Get all the instance-terminals of this block.
  ///
  dbSet<dbITerm> getITerms();

  ///
  /// Get the instances of this block.
  ///
  dbSet<dbInst> getInsts();

  ///
  /// Get the modules of this block.
  ///
  dbSet<dbModule> getModules();

  ///
  /// Get the modinsts of this block.
  ///
  dbSet<dbModInst> getModInsts();
  dbSet<dbModNet> getModNets();
  dbSet<dbModBTerm> getModBTerms();
  dbSet<dbModITerm> getModITerms();

  ///
  /// Get the Power Domains of this block.
  ///
  dbSet<dbPowerDomain> getPowerDomains();

  ///
  /// Get the Logic Ports of this block.
  ///
  dbSet<dbLogicPort> getLogicPorts();

  ///
  /// Get the Power Switches of this block.
  ///
  dbSet<dbPowerSwitch> getPowerSwitches();

  ///
  /// Get the Isolations of this block.
  ///
  dbSet<dbIsolation> getIsolations();

  ///
  /// Get the LevelShifters of this block.
  ///
  dbSet<dbLevelShifter> getLevelShifters();

  ///
  /// Get the groups of this block.
  ///
  dbSet<dbGroup> getGroups();

  ///
  /// Get the access points of this block.
  ///
  dbSet<dbAccessPoint> getAccessPoints();

  ///
  /// Get the global connects of this block.
  ///
  dbSet<dbGlobalConnect> getGlobalConnects();

  ///
  /// Evaluate global connect rules on this block.
  /// and helper functions for global connections
  /// on this block.
  ///
  int globalConnect(bool force, bool verbose);
  int globalConnect(dbGlobalConnect* gc, bool force, bool verbose);
  int addGlobalConnect(dbRegion* region,
                       const char* instPattern,
                       const char* pinPattern,
                       dbNet* net,
                       bool do_connect);
  void reportGlobalConnect();
  void clearGlobalConnect();

  ///
  /// Get the component mask shift layers.
  ///
  std::vector<dbTechLayer*> getComponentMaskShift();

  ///
  /// Set the component mask shift layers.
  ///
  void setComponentMaskShift(const std::vector<dbTechLayer*>& layers);

  ///
  /// Find a specific instance of this block.
  /// Returns nullptr if the object was not found.
  ///
  dbInst* findInst(const char* name);

  ///
  /// Find a specific module in this block.
  /// Returns nullptr if the object was not found.
  ///
  dbModule* findModule(const char* name);

  ///
  /// Find a specific modinst in this block. path is
  /// master_module_name/modinst_name Returns nullptr if the object was not
  /// found.
  ///
  dbModInst* findModInst(const char* path);

  ///
  /// Find a specific moditerm in this block. path is
  /// master_module_name/modinst_name/term_name Returns nullptr if the object
  /// was not found.
  ///
  dbModITerm* findModITerm(const char* hierarchical_name);

  ///
  /// Find a specific modbterm in this block. path is
  /// master_module_name/modinst_name/term_name Returns nullptr if the object
  /// was not found.
  ///
  dbModBTerm* findModBTerm(const char* hierarchical_name);

  ///
  /// Find a specific PowerDomain in this block.
  /// Returns nullptr if the object was not found.
  ///
  dbPowerDomain* findPowerDomain(const char* name);

  ///
  /// Find a specific LogicPort in this block.
  /// Returns nullptr if the object was not found.
  ///
  dbLogicPort* findLogicPort(const char* name);

  ///
  /// Find a specific PowerSwitch in this block.
  /// Returns nullptr if the object was not found.
  ///
  dbPowerSwitch* findPowerSwitch(const char* name);

  ///
  /// Find a specific Isolation in this block.
  /// Returns nullptr if the object was not found.
  ///
  dbIsolation* findIsolation(const char* name);

  ///
  /// Find a specific LevelShifter in this block.
  /// Returns nullptr if the object was not found.
  ///
  dbLevelShifter* findLevelShifter(const char* name);

  ///
  /// Find a specific group in this block.
  /// Returns nullptr if the object was not found.
  ///
  dbGroup* findGroup(const char* name);

  ///
  /// Find a specific iterm of this block.
  ///
  /// The iterm name must be of the form:
  ///
  ///     <instanceName><hierDelimiter><termName>
  ///
  /// For example:   inst0/A
  ///
  dbITerm* findITerm(const char* name);

  ///
  /// Get the obstructions of this block
  ///
  dbSet<dbObstruction> getObstructions();

  ///
  /// Get the blockages of this block
  ///
  dbSet<dbBlockage> getBlockages();

  ///
  /// Get the nets of this block
  ///
  dbSet<dbNet> getNets();

  ///
  /// Get the capNodes of this block
  ///
  dbSet<dbCapNode> getCapNodes();

  ///
  /// Get the rsegs of this block
  ///
  dbSet<dbRSeg> getRSegs();

  ///
  /// Find a specific net of this block.
  /// Returns nullptr if the object was not found.
  ///
  dbNet* findNet(const char* name) const;

  ///
  /// Find a specific mod net of this block.
  /// Returns nullptr if the object was not found.
  ///
  dbModNet* findModNet(const char* hierarchical_name) const;

  //
  // Utility to write db file
  //
  // void dbBlock::writeDb(char *filename, int allNode=0);
  void writeDb(char* filename, int allNode = 0);

  //
  // Utility to write guides file
  //
  void writeGuides(const char* filename) const;

  ///
  /// Find a specific via of this block.
  /// Returns nullptr if the object was not found.
  ///
  dbVia* findVia(const char* name);

  ///
  /// Get the vias of this block
  ///
  dbSet<dbVia> getVias();

  ///
  /// Get the GCell grid of this block
  /// Returns nullptr if no grid exists.
  ///
  dbGCellGrid* getGCellGrid();

  ///
  /// Get the DEF units of this technology.
  ///
  int getDefUnits();

  ///
  /// Set the DEF units of this technology.
  ///
  void setDefUnits(int units);

  ///
  /// Get the Database units per micron.
  ///
  int getDbUnitsPerMicron();

  ///
  /// Convert a length from database units (DBUs) to microns.
  ///
  double dbuToMicrons(int dbu);
  double dbuToMicrons(unsigned int dbu);
  double dbuToMicrons(int64_t dbu);
  double dbuToMicrons(double dbu);

  ///
  /// Convert an area from database units squared (DBU^2) to square microns.
  ///
  double dbuAreaToMicrons(int64_t dbu_area);

  ///
  /// Convert a length from microns to database units (DBUs).
  ///
  int micronsToDbu(double microns);

  ///
  /// Convert an area from square microns to database units squared (DBU^2).
  ///
  int64_t micronsAreaToDbu(double micronsArea);

  ///
  /// Get the hierarchy delimiter.
  /// Returns (0) if the delimiter was not set.
  /// A hierarchy delimiter can only be set at the time
  /// a block is created.
  ///
  char getHierarchyDelimiter() const;

  ///
  /// Set the bus name delimiters
  ///
  void setBusDelimiters(char left, char right);

  ///
  /// Get the bus name delimiters
  /// Left and Right are set to "zero" if the bus delimiters
  /// were not set.
  ///
  void getBusDelimiters(char& left, char& right);

  ///
  /// Get extraction counters
  ///
  void getExtCount(int& numOfNet,
                   int& numOfRSeg,
                   int& numOfCapNode,
                   int& numOfCCSeg);

  ///
  /// Copy RC values from one extDb to another.
  ///
  void copyExtDb(uint32_t fr,
                 uint32_t to,
                 uint32_t extDbCnt,
                 double resFactor,
                 double ccFactor,
                 double gndcFactor);

  ///
  /// Adjust RC values.
  ///
  void adjustRC(double resFactor, double ccFactor, double gndcFactor);

  ///
  /// add cc capacitance to gnd capacitance of capNodes of this block
  ///
  bool groundCC(float gndFactor);

  ///
  /// adjust CC's of nets
  ///
  bool adjustCC(float adjFactor,
                double ccThreshHold,
                std::vector<dbNet*>& nets,
                std::vector<dbNet*>& halonets);

  ///
  /// undo adjusted CC
  ///
  void undoAdjustedCC(std::vector<dbNet*>& nets, std::vector<dbNet*>& halonets);

  ///
  /// Get the number of process corners.
  ///
  int getCornerCount();

  ///
  /// having independent extraction corners ?
  ///
  bool extCornersAreIndependent();

  ///
  /// Get the number of corners kept n this block
  ///
  int getCornersPerBlock();

  ///
  /// Get the number of ext dbs
  ///
  int getExtDbCount();

  ///
  /// Get ext corner name by the index in ext Db
  ///
  std::string getExtCornerName(int corner);

  ///
  /// Get the index in ext Db by name
  ///
  int getExtCornerIndex(const char* cornerName);

  ///
  /// Set corner name list
  ///
  void setCornerNameList(const char* name_list);

  ///
  /// Get corner name list
  ///
  char* getCornerNameList();

  ///
  /// Set the number of process corners. The maximum number of
  /// process corners is limited to 256. This method will
  /// delete all dbRCSeg, dbCCSeg, which depend on this value.
  ///
  void setCornerCount(int cornerCnt, int extDbCnt, const char* name_list);
  void setCornerCount(int cnt);

  ///
  /// Set the number of corners kept in this block
  ///
  void setCornersPerBlock(int cornersPerBlock);

  ///
  /// Initialize the parasitics value tables
  ///
  void initParasiticsValueTables();

  ///
  /// create child block for one extraction corner
  ///
  dbBlock* createExtCornerBlock(uint32_t corner);
  ///
  /// find child block for one extraction corner
  ///
  dbBlock* findExtCornerBlock(uint32_t corner);
  ///
  /// get extraction data block for one extraction corner
  ///
  dbBlock* getExtCornerBlock(uint32_t corner);

  ///
  /// Get the track-grids of this block.
  ///
  dbSet<dbTrackGrid> getTrackGrids();

  ///
  /// Find a specific track-grid.
  /// Returns nullptr if a track-grid has not be defined for this layer.
  ///
  dbTrackGrid* findTrackGrid(dbTechLayer* layer);

  ///
  /// Get the rows of this block
  ///
  dbSet<dbRow> getRows();

  ///
  /// Get the fills in this block
  ///
  dbSet<dbFill> getFills();

  ///
  /// Get the list of masters used in this block.
  ///
  void getMasters(std::vector<dbMaster*>& masters);

  ///
  /// Set the die area. The die-area is considered a constant regardless
  /// of the geometric elements of the dbBlock. It is generally a constant
  /// declared in DEF.
  ///
  void setDieArea(const Rect& new_rect);

  ///
  /// Set the die area with polygon. Allows for non-rectangular floorplans
  ///
  void setDieArea(const Polygon& new_area);

  ///
  /// Get the die area. The default die-area is (0,0,0,0).
  ///
  Rect getDieArea();

  ///
  /// Get the die area as a polygon. The default die-area is (0,0,0,0).
  ///
  Polygon getDieAreaPolygon();

  ///
  /// Compute the core area based on rows
  ///
  odb::Polygon computeCoreArea();

  ///
  /// Set the core area.
  ///
  void setCoreArea(const Rect& new_area);

  ///
  /// Set the core area with polygon. Allows for non-rectangular floorplans
  ///
  void setCoreArea(const Polygon& new_area);

  ///
  /// Get the core area.
  ///
  Rect getCoreArea();

  ///
  /// Get the core area.
  ///
  Polygon getCoreAreaPolygon();

  ///
  /// Add region in the die area where IO pins cannot be placed
  ///
  void addBlockedRegionForPins(const Rect& region);

  ///
  /// Get the regions in the die area where IO pins cannot be placed
  ///
  const std::vector<Rect>& getBlockedRegionsForPins();

  ///
  /// Set the extmain instance.
  ///
  void setExtmi(void* ext);

  ///
  /// Get the extmain instance.
  ///
  void* getExtmi();

  ///
  /// Get the extraction control settings
  ///
  dbExtControl* getExtControl();

  ///
  /// Get the dbDft object for persistent dft structs
  ///
  dbDft* getDft() const;

  ///
  /// Get the minimum routing layer
  ///
  int getMinRoutingLayer() const;

  ///
  /// Set the minimum routing layer
  ///
  void setMinRoutingLayer(int min_routing_layer);

  ///
  /// Get the maximum routing layer
  ///
  int getMaxRoutingLayer() const;

  ///
  /// Set the maximum routing layer
  ///
  void setMaxRoutingLayer(int max_routing_layer);

  ///
  /// Set the minimum layer for clock
  ///
  int getMinLayerForClock() const;

  ///
  /// Set the minimum layer for clock
  ///
  void setMinLayerForClock(int min_layer_for_clock);

  ///
  /// Set the maximum layer for clock
  ///
  int getMaxLayerForClock() const;

  ///
  /// Set the maximum layer for clock
  ///
  void setMaxLayerForClock(int max_layer_for_clock);

  ///
  /// Get the gcell tile size
  ///
  int getGCellTileSize();

  ///
  /// Get the extraction corner names
  ///
  void getExtCornerNames(std::list<std::string>& ecl);

  ///
  /// Get the capacitor-coupled segments.
  ///
  dbSet<dbCCSeg> getCCSegs();

  ///
  /// Build search database for fast area searches for insts
  ///
  // uint32_t makeInstSearchDB();

  ///
  /// Get search database object for fast area searches on physical objects
  ///
  dbBlockSearch* getSearchDb();

  ///
  /// destroy coupling caps of nets
  ///
  void destroyCCs(std::vector<dbNet*>& nets);

  ///
  /// destroy RC segments of nets
  ///
  void destroyRSegs(std::vector<dbNet*>& nets);

  ///
  /// destroy capnodes of nets
  ///
  void destroyCNs(std::vector<dbNet*>& nets, bool cleanExtid);

  ///
  /// destroy parasitics of nets
  ///
  void destroyParasitics(std::vector<dbNet*>& nets);
  void destroyCornerParasitics(std::vector<dbNet*>& nets);

  ///
  /// get cc_halo_net's of input nets
  ///
  void getCcHaloNets(std::vector<dbNet*>& changedNets,
                     std::vector<dbNet*>& ccHaloNets);

  ///
  /// merge rsegs before doing exttree
  ///
  void preExttreeMergeRC(double max_cap, uint32_t corner);

  ///
  /// check if signal, clock and special nets are routed
  ///
  bool designIsRouted(bool verbose);

  ///
  /// Destroy wires of nets
  ///
  void destroyNetWires();

  ///
  /// clear
  ///
  void clear();

  ///
  /// get wire_updated nets
  ///
  void getWireUpdatedNets(std::vector<dbNet*>& nets);

  ///
  /// Make a unique net/instance name
  /// If parent is nullptr, the net name will be unique in top module.
  /// If base_name is nullptr, the default net name will be used.
  /// If uniquify is IF_NEEDED*, unique suffix will be added when necessary.
  /// If uniquify is *_WITH_UNDERSCORE, an underscore will be added before the
  /// unique suffix.
  ///
  std::string makeNewNetName(dbModInst* parent = nullptr,
                             const char* base_name = "net",
                             const dbNameUniquifyType& uniquify
                             = dbNameUniquifyType::ALWAYS);

  std::string makeNewModNetName(dbModule* parent,
                                const char* base_name = "net",
                                const dbNameUniquifyType& uniquify
                                = dbNameUniquifyType::ALWAYS,
                                dbNet* corresponding_flat_net = nullptr);
  std::string makeNewInstName(dbModInst* parent = nullptr,
                              const char* base_name = "inst",
                              const dbNameUniquifyType& uniquify
                              = dbNameUniquifyType::ALWAYS);

  const char* getBaseName(const char* full_name) const;

  ///
  /// return the regions of this design
  ///
  dbSet<dbRegion> getRegions();

  ///
  /// Find a specific region. Returns nullptr if the region was not found.
  ///
  dbRegion* findRegion(const char* name);

  ///
  ///  Find the non-default-rule
  ///
  dbTechNonDefaultRule* findNonDefaultRule(const char* name);

  ///
  ///  Get the non-default-rules specific to this block.
  ///
  dbSet<dbTechNonDefaultRule> getNonDefaultRules();

  ///
  ///  Get marker categories for this block.
  ///
  dbSet<dbMarkerCategory> getMarkerCategories();

  ///
  ///  Find marker group for this block.
  ///
  dbMarkerCategory* findMarkerCategory(const char* name);

  //
  //  Write marker information to file
  //
  void writeMarkerCategories(const std::string& file);
  void writeMarkerCategories(std::ofstream& reports);

  ///
  /// set First driving iterm on all signal nets; set 0 is none exists
  void setDrivingItermsforNets();

  void clearUserInstFlags();

  std::map<dbTechLayer*, dbTechVia*> getDefaultVias();

  ///
  /// Destroy all the routing wires from signal and clock nets in this block.
  ///
  void destroyRoutes();

 public:
  ///
  /// Create a chip's top-block. Returns nullptr of a top-block already
  /// exists.
  /// If tech is null then the db must contain only one dbTech.
  ///
  static dbBlock* create(dbChip* chip,
                         const char* name,
                         char hier_delimiter = '/');

  ///
  /// Create a hierachical/child block. This block has no connectivity.
  /// If tech is null then the tech will be taken from 'block'
  /// Returns nullptr if a block with the same name exists.
  ///
  static dbBlock* create(dbBlock* block,
                         const char* name,
                         char hier_delimiter = '/');

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbBlock* getBlock(dbChip* chip, uint32_t oid);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbBlock* getBlock(dbBlock* block, uint32_t oid);

  ///
  /// Destroy a block.
  ///
  static void destroy(dbBlock* block);

  ///
  /// Delete the bterm from the block.
  ///
  static dbSet<dbBlock>::iterator destroy(dbSet<dbBlock>::iterator& itr);

  //
  // For debugging only.  Print block content to an ostream.
  //
  void debugPrintContent(std::ostream& str_db);
  void debugPrintContent() { debugPrintContent(std::cout); }
};

///////////////////////////////////////////////////////////////////////////////
///
/// A block-terminal is the element used to represent connections in/out of
/// a block.
///
///////////////////////////////////////////////////////////////////////////////
class dbBTerm : public dbObject
{
 public:
  ///
  /// Get the block-terminal name.
  ///
  std::string getName() const;

  ///
  /// Get the block-terminal name.
  ///
  const char* getConstName() const;

  ///
  /// Change the name of the bterm.
  /// Returns true if successful.
  /// Returns false if a bterm with the same name already exists.
  ///
  bool rename(const char* name);

  ///
  /// Get bbox of this term (ie the bbox of the bpins)
  ///
  Rect getBBox();

  ///
  /// Set the signal type of this block-terminal.
  ///
  void setSigType(dbSigType type);

  ///
  /// Get the signal type of this block-terminal.
  ///
  dbSigType getSigType() const;

  ///
  /// Set the IO direction of this block-terminal.
  ///
  void setIoType(dbIoType type);

  ///
  /// Get the IO direction of this block-terminal.
  ///
  dbIoType getIoType() const;

  ///
  /// Set spef mark of this block-terminal.
  ///
  void setSpefMark(uint32_t v);

  ///
  /// get spef mark of this block-terminal.
  ///
  bool isSetSpefMark();

  ///
  /// Set mark of this block-terminal.
  ///
  void setMark(uint32_t v);

  ///
  /// get mark of this block-terminal.
  ///
  bool isSetMark();

  ///
  /// set ext id of this block-terminal.
  ///
  void setExtId(uint32_t v);

  ///
  /// get ext id of this block-terminal.
  ///
  uint32_t getExtId();

  ///
  /// is this terminal SPECIAL (i.e. not for regular signal routing).
  ///
  bool isSpecial() const;

  ///
  /// set SPECIAL attribute -- expected to be done once by DEF parser.
  ///
  void setSpecial();

  ///
  /// Get the net of this block-terminal.
  ///
  dbNet* getNet() const;

  ///
  /// Get the mod net of this block-terminal.
  dbModNet* getModNet() const;
  ///

  /// Disconnect the block-terminal from its net.
  /// kills a dbModNet and dbNet connection
  void disconnect();
  // Fine level apis to control which net removed from pin.
  /// Disconnect the block-terminal from its db net.
  void disconnectDbNet();
  /// Disconnect the block-terminal from its mod net.
  void disconnectDbModNet();

  /// Connect the block-terminal to net.
  ///
  void connect(dbNet* db_net, dbModNet* modnet);
  void connect(dbNet* net);
  void connect(dbModNet* mod_net);

  ///
  /// Get the block of this block-terminal.
  ///
  dbBlock* getBlock() const;

  ///
  /// Get the hierarchical parent iterm of this bterm.
  ///
  /// Returns nullptr if this bterm has no parent iterm.
  ///
  ///
  ///     (top-block)
  ///     +------------------------------------------------------------------+
  ///     |                                                                  |
  ///     |                                                                  |
  ///     |                                                                  |
  ///     |               (child-block / instance)                           |
  ///     |               +----------------------------------+               |
  ///     |               |                                  |               |
  ///     |B             I|B                                 |               |
  ///     |o.............o|o..........                       |               |
  ///     |  (net in      |  (net in child-block)            |               |
  ///     |   top-block)  |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               +----------------------------------+               |
  ///     |                                                                  |
  ///     |                                                                  |
  ///     +------------------------------------------------------------------+
  ///
  ///
  /// B = dbBterm
  /// I = dbIterm
  ///
  dbITerm* getITerm();

  ///
  /// Get the bpins of this bterm.
  ///
  dbSet<dbBPin> getBPins() const;

  ///
  /// This method finds the first "placed" dbPin box.
  /// returns false if there are no placed bpins.
  ///
  bool getFirstPin(dbShape& shape);

  ///
  /// This method finds the location the first "placed" dbPin box.
  /// The location is the computed center of the bbox.
  /// returns false if there are no placed bpins. x and y are set to zero.
  //
  bool getFirstPinLocation(int& x, int& y) const;

  ///
  /// This method returns the placementstatus of the first dbBPin.
  /// Returns NONE if bterm has no dbPins.
  ///
  dbPlacementStatus getFirstPinPlacementStatus();

  ///
  /// Get the ground sensistivity pin (5.6 DEF)
  ///
  dbBTerm* getGroundPin();

  ///
  /// Set the ground sensistivity pin (5.6 DEF)
  ///
  void setGroundPin(dbBTerm* pin);

  ///
  /// Get the supply sensistivity pin (5.6 DEF)
  ///
  dbBTerm* getSupplyPin();

  ///
  /// Set the supply sensistivity pin (5.6 DEF)
  ///
  void setSupplyPin(dbBTerm* pin);

  ///
  /// Create a new block-terminal.
  /// Returns nullptr if a bterm with this name already exists
  ///
  static dbBTerm* create(dbNet* net, const char* name);

  ///
  /// Delete the bterm from the block.
  ///
  static void destroy(dbBTerm* bterm);

  ///
  /// Delete the bterm from the block.

  static dbSet<dbBTerm>::iterator destroy(dbSet<dbBTerm>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbBTerm* getBTerm(dbBlock* block, uint32_t oid);

  uint32_t staVertexId();
  void staSetVertexId(uint32_t id);

  ///
  /// Set the region where the BTerm is constrained
  ///
  void setConstraintRegion(const Rect& constraint_region);

  ///
  /// Get the region where the BTerm is constrained
  ///
  std::optional<Rect> getConstraintRegion();

  ///
  /// Reset constraint region.
  ///
  void resetConstraintRegion();

  ///
  /// Set the bterm which position is mirrored to this bterm
  ///
  void setMirroredBTerm(dbBTerm* mirrored_bterm);

  ///
  /// Get the bterm that is mirrored to this bterm
  ///
  dbBTerm* getMirroredBTerm();

  ///
  /// Returns true if the current BTerm has a mirrored BTerm.
  ///
  bool hasMirroredBTerm();

  ///
  /// Return true if this BTerm is mirrored with another pin.
  ///
  bool isMirrored();
};

///////////////////////////////////////////////////////////////////////////////
///
/// A BPIn is the element that represents a physical connection to a block
/// terminal.
///
///////////////////////////////////////////////////////////////////////////////

class dbBPin : public dbObject
{
 public:
  ///
  /// Get the placement status of this block-terminal.
  ///
  dbPlacementStatus getPlacementStatus() const;

  ///
  /// Set the placement status of this block-terminal.
  ///
  void setPlacementStatus(dbPlacementStatus status);

  ///
  /// Get bterm of this pin.
  ///
  dbBTerm* getBTerm() const;

  ///
  /// Get boxes of this pin
  ///
  dbSet<dbBox> getBoxes();

  ///
  /// Get bbox of this pin (ie the bbox of getBoxes())
  ///
  Rect getBBox();

  ///
  /// Returns true if this bpin has an effective-width rule.
  ///
  bool hasEffectiveWidth() const;

  ///
  /// Set the effective width rule.
  ///
  void setEffectiveWidth(int w);

  ///
  /// Return the effective width rule.
  ///
  int getEffectiveWidth() const;

  ///
  /// Returns true if this bpin has an min-spacing rule.
  ///
  bool hasMinSpacing() const;

  ///
  /// Set the min spacing rule.
  ///
  void setMinSpacing(int w);

  ///
  /// Return the min spacing rule.
  ///
  int getMinSpacing() const;

  std::vector<dbAccessPoint*> getAccessPoints() const;

  ///
  /// Create a new block-terminal-pin
  ///
  static dbBPin* create(dbBTerm* bterm);

  ///
  /// Delete the bpin from this bterm
  ///
  static void destroy(dbBPin* bpin);

  ///
  /// Delete the bterm from the bterm.
  static dbSet<dbBPin>::iterator destroy(dbSet<dbBPin>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbBPin* getBPin(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A Net is the element that represents a "net" on a block.
///
///////////////////////////////////////////////////////////////////////////////
class dbNet : public dbObject
{
 public:
  ///
  /// Get the hierarchical net name (not a base name).
  ///
  std::string getName() const;

  ///
  /// Need a version that does not do strdup every time
  ///
  const char* getConstName() const;

  ///
  /// Print net name with or without id and newline
  ///
  void printNetName(FILE* fp, bool idFlag = true, bool newLine = true);

  ///
  /// Change the name of the net.
  /// Returns true if successful.
  /// Returns false if a net with the same name already exists.
  ///
  bool rename(const char* name);

  ///
  /// Swaps the current db net name with the source db net
  void swapNetNames(dbNet* source, bool journal = true);

  ///
  /// RC netowork disconnect
  ///
  bool isRCDisconnected();

  ///
  ///
  ///
  void setRCDisconnected(bool value);

  ///
  /// Get the weight assigned to this net.
  /// (Default: 1)
  ///
  int getWeight();

  ///
  /// Set the weight assigned of this net.
  ///
  void setWeight(int weight);

  ///
  /// Get the source assigned to this net.
  ///
  dbSourceType getSourceType();

  ///
  /// Set the source assigned of this net.
  ///
  void setSourceType(dbSourceType type);

  ///
  /// Get the x-talk-class assigned to this net.
  /// (Default: 0)
  ///
  int getXTalkClass();

  ///
  /// Set the x-talk-class assigned of this net.
  ///
  void setXTalkClass(int value);

  ///
  /// Set the driving term id assigned of this net.
  ///
  void setDrivingITerm(const dbITerm* iterm);

  ///
  /// Returns the driving dbITerm* of this net.
  ///
  dbITerm* getDrivingITerm() const;

  ///
  /// Returns true if a fixed-bump flag has been set.
  ///
  bool hasFixedBump();

  ///
  /// Set the value of the fixed-bump flag.
  ///
  void setFixedBump(bool value);

  ///
  /// Get the Regular Wiring of a net (TODO: per path)
  ///
  dbWireType getWireType() const;

  ///
  /// Set the Regular Wiring of a net (TODO: per path)
  ///
  void setWireType(dbWireType wire_type);

  ///
  /// Get the signal type of this block-net.
  ///
  dbSigType getSigType() const;

  ///
  /// Get the signal type of this block-net.
  ///
  void setSigType(dbSigType sig_type);

  ///
  /// Assuming no intersection, check if the net is in the bbox.
  ///
  bool isEnclosed(Rect* bbox);

  ///
  /// Returns the mark flag value. This flag specified that the
  /// net has been marked.
  ///
  bool isMarked();

  ///
  /// Returns the mark_1 flag value. This flag specified that the
  /// net has been mark_1ed.
  ///
  bool isMark_1ed();

  ///
  /// Set the mark flag to the specified value.
  ///
  void setMark(bool value);

  ///
  /// Set the mark_1 flag to the specified value.
  ///
  void setMark_1(bool value);

  ///
  /// Returns the select flag value. This flag specified that the
  /// net has been select.
  ///
  bool isSelect();

  /// Net
  /// Set the select flag to the specified value.
  ///
  void setSelect(bool value);

  ///
  /// Returns the wire-ordered flag value. This flag specified that the
  /// wires of this net have been ordered into a single dbWire.
  ///
  bool isWireOrdered();

  ///
  /// Set the wire-ordered flag to the specified value.
  /// Note: This flag is set to false any time a dbWire
  /// is created on this net.
  ///
  void setWireOrdered(bool value);

  ///
  /// Returns the disconnected flag value. This flag specified that the
  /// wire are connected to all the iterms of this net.
  ///
  bool isDisconnected();

  ///
  /// Set the disconnected flag to the specified value.
  /// Note: This flag is set to false any time a dbWire
  /// is created on this net.
  ///
  void setDisconnected(bool value);

  ///
  /// wire_update flag to be used at when the wire is replaced with a new wire
  /// NOTE: rcgraph, extracted, ordered, reduced all have to be reset
  ///
  void setWireAltered(bool value);
  bool isWireAltered();

  ///
  /// rc_graph flag set when Rseg and CapNodes were created
  ///
  void setRCgraph(bool value);
  bool isRCgraph();

  ///
  /// extracted flag set when net was extracted
  ///
  void setExtracted(bool value);
  bool isExtracted();

  ///
  /// Sinlge bit general purpose flag to be used at spef
  ///
  void setSpef(bool value);
  bool isSpef();

  ///
  /// Set/Reset the don't-touch flag
  ///
  /// Setting this implies:
  /// - The net can't be destroyed
  /// - The net can't have any bterm or iterms connected or disconnected
  /// - The net CAN be routed or unrouted (wire or swire)
  ///
  void setDoNotTouch(bool v);

  ///
  /// Returns true if the don't-touch flag is set.
  ///
  bool isDoNotTouch() const;

  ///
  /// Get the block this net belongs to.
  ///
  dbBlock* getBlock() const;

  ///
  /// Get all the instance-terminals of this net.
  ///
  dbSet<dbITerm> getITerms() const;

  ///
  /// Get the 1st instance-terminal of this net.
  ///
  dbITerm* get1stITerm();

  ///
  /// Get the 1st inputSignal Iterm; can be
  ///
  dbITerm* get1stSignalInput(bool io);

  ///
  /// Get the 1st driver terminal (dbITerm or dbBTerm)
  ///
  dbObject* getFirstDriverTerm() const;

  ///
  /// Get the 1st driver instance
  ///
  dbInst* getFirstDriverInst() const;

  ///
  /// Get the 1st output Iterm; can be
  ///
  dbITerm* getFirstOutput() const;

  ///
  /// Get all the block-terminals of this net.
  ///
  dbSet<dbBTerm> getBTerms() const;

  ///
  /// Get the 1st block-terminal of this net.
  ///
  dbBTerm* get1stBTerm();

  ///
  /// Get the special-wires of this net.
  ///
  dbSet<dbSWire> getSWires();

  ///
  /// Get the wire of this net.
  /// Returns nullptr if this net has no wire.
  ///
  dbWire* getWire();

  ///
  /// Get the first swire of this net.
  /// Returns nullptr if this net has no swires.
  ///
  dbSWire* getFirstSWire();

  ///
  /// Get the global wire of thie net.
  /// Returns nullptr if this net has no global wire.
  ///
  dbWire* getGlobalWire();

  ///
  /// Returns true if this dbNet is marked as special. Special nets/iterms are
  /// declared in the SPECIAL NETS section of a DEF file.
  ///
  bool isSpecial() const;

  ///
  /// Mark this dbNet as special.
  ///
  void setSpecial();

  ///
  /// Unmark this dbNet as special.
  ///
  void clearSpecial();

  ///
  /// Returns true if this dbNet is connected to other dbNet.
  ///
  bool isConnected(const dbNet* other) const;

  ///
  /// Returns true if this dbNet is connected to other dbModNet.
  ///
  bool isConnected(const dbModNet* other) const;

  ///
  /// Returns true if this dbNet has its pins connected by abutment
  ///
  bool isConnectedByAbutment();

  ///
  /// Set the IO flag if there are any BTerms on net
  bool setIOflag();

  ///
  /// returns true if there are BTerms on net
  bool isIO();

  ///
  /// Returns true if this dbNet is was connected using a wild-card.
  ///
  bool isWildConnected();

  ///
  /// Mark this dbNet as wild-connected.
  ///
  void setWildConnected();

  ///
  /// Unmark this dbNet as wild-connected.
  ///
  void clearWildConnected();

  ///
  /// Get the gndc calibration factor of this net
  ///
  float getGndcCalibFactor();

  ///
  /// Set the gndc calibration factor of this net
  ///
  void setGndcCalibFactor(float gndcCalib);

  ///
  /// Calibrate the capacitance of this net
  ///
  void calibrateCapacitance();

  ///
  /// Calibrate the ground capacitance of this net
  ///
  void calibrateGndCap();

  ///
  /// Calibrate the coupling capacitance of this net
  ///
  void calibrateCouplingCap();
  void calibrateCouplingCap(int corner);

  ///
  /// Get the cc calibration factor of this net
  ///
  float getCcCalibFactor();

  ///
  /// Set the cc calibration factor of this net
  ///
  void setCcCalibFactor(float ccCalib);

  ///
  /// Adjust resistances of this net
  ///
  void adjustNetRes(float factor);

  ///
  /// Adjust resistances of this net for a corner
  ///
  void adjustNetRes(float factor, uint32_t corner);

  ///
  /// Adjust ground cap of this net
  ///
  void adjustNetGndCap(float factor);

  ///
  /// Adjust ground cap of this net for a corner
  ///
  void adjustNetGndCap(uint32_t corner, float factor);

  ///
  /// get ccAdjustFactor of this net
  ///
  float getCcAdjustFactor();

  ///
  /// set ccAdjustFactor of this net
  ///
  void setCcAdjustFactor(float factor);

  ///
  /// get ccAdjustOrder of this net
  ///
  uint32_t getCcAdjustOrder();

  ///
  /// set ccAdjustOrder of this net
  ///
  void setCcAdjustOrder(uint32_t order);

  ///
  /// adjust CC's of this net
  ///
  bool adjustCC(uint32_t adjOrder,
                float adjFactor,
                double ccThreshHold,
                std::vector<dbCCSeg*>& adjustedCC,
                std::vector<dbNet*>& halonets);

  ///
  /// undo adjusted CC
  ///
  void undoAdjustedCC(std::vector<dbCCSeg*>& adjustedCC,
                      std::vector<dbNet*>& halonets);

  ///
  /// add cc capacitance to gnd capacitance of capNodes of this net
  ///
  bool groundCC(float gndFactor);

  ///
  /// Add to the dbCC of this net
  ///
  void addDbCc(float cap);

  ///
  /// Get dbCC of this net
  ///
  float getDbCc();

  ///
  /// Set dbCC of this net
  ///
  void setDbCc(float cap);

  ///
  /// Get refCC of this net
  ///
  float getRefCc();

  ///
  /// Set refCC of this net
  ///
  void setRefCc(float cap);

  ///
  /// get the CC match ratio against this net
  ///
  float getCcMatchRatio();

  ///
  /// set the CC match ratio against this net
  ///
  void setCcMatchRatio(float ratio);

  ///
  /// Get the gdn cap of this net to *gndcap, total cap to *totalcap
  ///
  void getGndTotalCap(double* gndcap, double* totalcap, double miller_mult);

  ///
  /// merge rsegs before doing exttree
  ///
  void preExttreeMergeRC(double max_cap, uint32_t corner);

  ///
  /// Get Cap Node given a node_num
  ///
  dbCapNode* findCapNode(uint32_t nodeId);

  ///
  /// Get the Cap Nodes of this net.
  ///
  dbSet<dbCapNode> getCapNodes();

  ///
  /// delete the Cap Nodes of this net.
  ///
  void destroyCapNodes(bool cleanExtid);

  ///
  /// Reverse the rsegs seqence of this net.
  ///
  void reverseRSegs();

  ///
  /// Set the 1st R segment of this net.
  ///
  void set1stRSegId(uint32_t rseg_id);

  ///
  /// Get the zeroth R segment of this net.
  ///
  dbRSeg* getZeroRSeg();

  ///
  /// Get the 1st R segment id of this net.
  ///
  uint32_t get1stRSegId();

  ///
  /// find the rseg having srcn and tgtn
  ///
  dbRSeg* findRSeg(uint32_t srcn, uint32_t tgtn);

  ///
  /// Set the 1st Cap node of this net.
  ///
  void set1stCapNodeId(uint32_t capn_id);

  ///
  /// Get the 1st Cap node of this net.
  ///
  uint32_t get1stCapNodeId();

  ///
  /// Reset, or Set the extid of the bterms and iterms to the capnode id's
  ///
  void setTermExtIds(int capId);

  ///
  /// get rseg  count
  ///
  uint32_t getRSegCount();

  ///
  /// Get the RSegs segments.
  ///
  dbSet<dbRSeg> getRSegs();

  ///
  /// compact internal capnode number'
  ///
  void collapseInternalCapNum(FILE* cap_node_map);

  ///
  /// find max number of cap nodes that are internal
  ///
  uint32_t maxInternalCapNum();

  ///
  /// get capNode count
  ///
  uint32_t getCapNodeCount();

  ///
  /// get CC seg count
  ///
  uint32_t getCcCount();

  ///
  /// delete the R segments of this net.
  ///
  void destroyRSegs();

  ///
  /// reverse the link order of CCsegs of capNodes
  ///
  void reverseCCSegs();

  ///
  /// Get the source capacitor-coupled segments of this net..
  ///
  void getSrcCCSegs(std::vector<dbCCSeg*>& segs);

  ///
  /// Get the target capacitor-coupled segments of this net..
  ///
  void getTgtCCSegs(std::vector<dbCCSeg*>& segs);

  ///
  /// Get the nets having coupling caps with this net
  ///
  void getCouplingNets(uint32_t corner,
                       double ccThreshold,
                       std::set<dbNet*>& cnets);

  ///
  /// delete the capacitor-coupled segments.
  ///
  void destroyCCSegs();

  ///
  /// destroy parasitics
  ///
  void destroyParasitics();

  ///
  /// Get total capacitance in FF
  ///
  double getTotalCapacitance(uint32_t corner = 0, bool cc = false);

  ///
  /// Get total coupling capacitance in FF
  ///
  double getTotalCouplingCap(uint32_t corner = 0);

  ///
  /// Get total resistance in mil ohms
  ///
  double getTotalResistance(uint32_t corner = 0);

  ///
  /// Set the nondefault rule applied to this net for wiring.
  ///
  void setNonDefaultRule(dbTechNonDefaultRule* rule);

  ///
  /// Get the nondefault rule applied to this net for wiring.
  /// Returns nullptr if there is no nondefault rule.
  ///
  dbTechNonDefaultRule* getNonDefaultRule();

  ///
  /// Get stats of this net
  ///
  void getNetStats(uint32_t& wireCnt,
                   uint32_t& viaCnt,
                   uint32_t& len,
                   uint32_t& layerCnt,
                   uint32_t* levelTable);

  ///
  /// Get wire counts of this net
  ///
  void getWireCount(uint32_t& wireCnt, uint32_t& viaCnt);

  ///
  /// Get wire counts of this signal net
  ///
  void getSignalWireCount(uint32_t& wireCnt, uint32_t& viaCnt);

  ///
  /// Get wire counts of this power net
  ///
  void getPowerWireCount(uint32_t& wireCnt, uint32_t& viaCnt);

  ///
  /// Get term counts of this net
  ///
  uint32_t getTermCount();

  ///
  /// Get iterm counts of this signal net
  ///
  uint32_t getITermCount();

  ///
  /// Get bterm counts of this signal net
  ///
  uint32_t getBTermCount();

  //
  // Get the bounding box of the iterms and bterms.
  //
  Rect getTermBBox();

  ///
  /// Delete the swires of this net
  ///
  void destroySWires();

  ///
  /// Create a new net.
  /// Returns nullptr if a net with this name already exists
  ///
  static dbNet* create(dbBlock* block,
                       const char* name,
                       bool skipExistingCheck = false);

  static dbNet* create(dbBlock* block,
                       const char* base_name,
                       const dbNameUniquifyType& uniquify,
                       dbModule* parent_module = nullptr);

  ///
  /// Delete this net from this block.
  ///
  static void destroy(dbNet* net);

  ///
  /// mark nets of a block.
  ///
  static void markNets(std::vector<dbNet*>& nets, dbBlock* block, bool mk);

  ///
  /// Delete the net from the block.
  ///
  static dbSet<dbNet>::iterator destroy(dbSet<dbNet>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbNet* getNet(dbBlock* block, uint32_t oid);

  ///
  /// Translate a valid database-id back to a pointer.
  ///
  static dbNet* getValidNet(dbBlock* block, uint32_t oid);

  ///
  /// True if can merge the iterms and bterms of the in_net with this net
  ///
  bool canMergeNet(dbNet* in_net);

  ///
  /// Merge the iterms and bterms of the in_net with this net
  ///
  void mergeNet(dbNet* in_net);

  ///
  /// Find the parent module instance of this net by parsing its hierarchical
  /// name.
  /// Returns nullptr if the parent is the top module.
  /// Note that a dbNet can be located at multiple hierarchical modules.
  ///
  dbModInst* findMainParentModInst() const;

  ///
  /// Find the parent module of this net by parsing its hierarchical name.
  /// Returns the top module if the parent is the top module.
  ///
  dbModule* findMainParentModule() const;

  dbSet<dbGuide> getGuides() const;

  void clearGuides();

  dbSet<dbNetTrack> getTracks() const;

  void clearTracks();

  bool hasJumpers();

  void setJumpers(bool has_jumpers);

  ///
  /// Return true if the input net is in higher hierarchy than this net
  /// e.g., If this net name = "a/b/c" and input `net` name = "a/d",
  ///       this API returns true.
  ///
  bool isDeeperThan(const dbNet* net) const;

  ///
  /// Find all dbModNets related to the given dbNet.
  /// Go through all the pins on the dbNet and find all dbModNets.
  ///
  /// A dbNet could have many modnets (e.g., a dbNet might connect
  /// two objects in different parts of the hierarchy, each connected
  /// by different dbModNets in different parts of the hierarchy).
  ///
  bool findRelatedModNets(std::set<dbModNet*>& modnet_set) const;

  ///
  /// Find the modnet in the highest hierarchy related to this net.
  ///
  dbModNet* findModNetInHighestHier() const;

  ///
  /// Rename this net with the name of the modnet in the highest hierarchy
  /// related to this flat net.
  ///
  void renameWithModNetInHighestHier();

  ///
  /// Check if this net is internal to the given module.
  /// A net is internal if all its iterms belong to instances within the module
  /// and it has no bterms.
  ///
  bool isInternalTo(dbModule* module) const;

  ///
  /// Check issues such as multiple drivers, no driver, or dangling net
  ///
  void checkSanity() const;

  ///
  /// Dump dbNet info for debugging
  ///
  void dump(bool show_modnets = false) const;

  ///
  /// Check consistency between the terminals connected to this dbNet and
  /// the terminals connected to all related dbModNets. This ensures that
  /// the flat and hierarchical representations of the net's connectivity
  /// are consistent
  //
  void checkSanityModNetConsistency() const;

  ///
  /// Dump dbNet connectivity for debugging
  ///
  void dumpConnectivity(int level = 1) const;

  ///
  /// Load-pin buffering.
  /// - Inserts a buffer on the driving net of the load pin (iterm/bterm).
  /// - Returns the newly created buffer instance.
  /// - If loc is null, the buffer is inserted at the load pin.
  ///
  dbInst* insertBufferBeforeLoad(
      dbObject* load_input_term,
      const dbMaster* buffer_master,
      const Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const dbNameUniquifyType& uniquify = dbNameUniquifyType::ALWAYS);

  ///
  /// Driver-pin buffering.
  /// - Inserts a buffer on the net driven by the driver pin (iterm/bterm).
  /// - Returns the newly created buffer instance.
  /// - If loc is null, the buffer is inserted at the driver pin.
  ///
  dbInst* insertBufferAfterDriver(
      dbObject* drvr_output_term,
      const dbMaster* buffer_master,
      const Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const dbNameUniquifyType& uniquify = dbNameUniquifyType::ALWAYS);

  ///
  /// Partial-loads buffering.
  /// - Inserts a buffer on the net driving the specified load pins.
  /// - Returns the newly created buffer instance.
  /// - If loc is null, the buffer is inserted at the center of the load pins.
  /// - Note that the new buffer drives the specified load pins only.
  ///   It does not drive other unspecified loads driven by the same net.
  /// - loads_on_diff_nets: Flag indicating if loads can be on different dbNets.
  ///   If true, the loads can be on different dbNets. This should be carefully
  ///   used because it may break the function of the design if the loads
  ///   contain an irrelevant load.
  ///
  dbInst* insertBufferBeforeLoads(
      const std::set<dbObject*>& load_pins,
      const dbMaster* buffer_master,
      const Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const dbNameUniquifyType& uniquify = dbNameUniquifyType::ALWAYS,
      bool loads_on_diff_nets = false);

  ///
  /// Partial-loads buffering with vector load_pins support.
  ///
  dbInst* insertBufferBeforeLoads(
      const std::vector<dbObject*>& load_pins,
      const dbMaster* buffer_master,
      const Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const dbNameUniquifyType& uniquify = dbNameUniquifyType::ALWAYS,
      bool loads_on_diff_nets = false);

  ///
  /// Connect a driver iterm to a load iterm, punching ports through hierarchy
  /// as needed.
  ///
  void hierarchicalConnect(dbObject* driver, dbObject* load);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbInstance is the element used to represent instances of master-cells in
/// a block.
///
///////////////////////////////////////////////////////////////////////////////
class dbInst : public dbObject
{
 public:
  ///
  /// Get the hierarchical instance name (not a base name).
  ///
  std::string getName() const;

  ///
  /// Need a version that does not do strdup every time
  ///
  const char* getConstName() const;

  ///
  /// Compare, like !strcmp
  ///
  bool isNamed(const char* name);

  ///
  /// Change the name of the inst.
  /// Returns true if successful.
  /// Returns false if a inst with the same name already exists.
  ///
  bool rename(const char* name);

  /////////////////////////////////////////////////////////////////
  ///
  /// IMPORTANT -  INSTANCE PLACEMENT
  ///
  /// There are seven methods used to get/set the placement.
  ///
  ///     getOrigin           - Get the origin of this instance
  ///     setOrigin           - Set the origin of this instance
  ///     getOrient           - Get orient of this instance
  ///     setOrient           - Set orient of this instance
  ///     getLocation         - Get the lower-left corner of this instance
  ///     setLocation         - Set the lower-left corner of this instance
  ///     setLocationOrient   - Set the orient of this instance and maintain the
  ///     lower-left corner.
  ///
  /// The getLocation/setLocation are used to get and set the lower-left corner
  /// of the bounding box of the instance. These methods use the DEF semantics.
  ///
  ///
  ///  MASTER COORDINATE SYSTEM:
  ///
  ///                                              |
  ///                                            --o-- (0,0)
  ///                                              |
  ///
  ///
  ///   +----------------------+ (Master bbox after rotation applied)
  ///   |                      |
  ///   |                      |
  ///   |                      |
  ///   |                      |
  ///   |                      |
  ///   +----------------------+
  ///  Mx,My
  ///
  ///
  ///  BLOCK COORDINATE SYSTEM:
  ///
  ///                                              |
  ///                                            --o-- (x,y) (True origin
  ///                                            (getOrigin/setOrigin)
  ///                                              |
  ///
  ///
  ///   +----------------------+ (Master bbox after rotation applied)
  ///   |                      |
  ///   |                      |
  ///   |                      |
  ///   |                      |
  ///   |                      |
  ///   +----------------------+
  ///  Ix,Ix (Location getLocation/setLocation)
  ///
  ///
  /// getLocation returns:(Note Mx/My is the location of the bbox AFTER
  /// rotation)
  ///
  ///     Ix = x + Mx
  ///     Iy = y + My
  ///
  /// setLocation(x,y) is equivalent to:
  ///
  ///    dbMaster * master = inst->getMaster();
  ///    Rect bbox;
  ///    master->getBBox(bbox);
  ///    dbTransform t(getOrient());
  ///    t.apply(bbox);
  ///    inst->setOrigin(x - bbox.xMin(),y - bbox.yMin());
  ///
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ///  WARNING:
  ///
  ///     If dbInst::setLocation() is called BEFORE dbInst::setOrient() is
  ///     called with the proper orient the "real" origin will be computed
  ///     incorrectly and the instance will be placed INCORRECTLY.
  ///
  ///     If you want to change the orient relative to the "location" use
  ///     setLocationOrient(). Otherwise the bounding box will be recomputed
  ///     incorrectly.
  ///
  ///     getLocation/setLocation are provided for backward compatibility and
  ///     there use should be avoided.
  ///
  /// TRANSFORMS:
  ///
  ///     When using dbTransform() to translate the shapes/pins of an instance,
  ///     use getOrigin() to correctly set up the transform:
  ///
  ///         int x, y;
  ///         inst->getOrigin(x,y);
  ///         dbTransform transform( inst->getOrient(), Point(x,y) );
  ///
  ///         for all shapes of inst:
  ///             transform.apply( shape )
  ///
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////

  ///
  /// Get the "placed" origin of this instance.
  ///
  Point getOrigin();

  ///
  /// Set the "placed" origin of this instance.
  ///
  void setOrigin(int x, int y);

  ///
  /// Get the orientation of this instance.
  ///
  dbOrientType getOrient();

  ///
  /// Set the orientation of this instance.
  ///
  void setOrient(dbOrientType orient);

  ///
  /// This method returns the lower-left corner
  /// of the bounding box of this instance.
  ///
  void getLocation(int& x, int& y) const;

  ///
  /// This method returns the lower-left corner
  /// of the bounding box of this instance.
  ///
  Point getLocation() const;

  ///
  /// This method sets the lower-left corner
  /// of the bounding box of this instance.
  ///
  void setLocation(int x, int y);

  ///
  /// Set the orientation of this instance.
  /// This method holds the instance lower-left corner in place and
  /// rotates the instance relative to the lower-left corner.
  ///
  /// This method is equivalent to the following:
  ///
  ///     int x, y;
  ///     inst->getLocation();
  ///     inst->setOrient( orient );
  ///     inst->setLocation(x,y);
  ///
  ///
  void setLocationOrient(dbOrientType orient);

  ///
  /// Get the transform of this instance.
  /// Equivalent to getOrient() and getOrigin()
  ///
  dbTransform getTransform();

  ///
  /// Set the transform of this instance.
  /// Equivalent to setOrient() and setOrigin()
  ///
  void setTransform(const dbTransform& t);

  ///
  /// Get the hierarchical transform of this instance.
  ///
  void getHierTransform(dbTransform& t);

  /////////////////////////////////////////////////////////////////

  ///
  /// This method returns the lower-left corner
  /// of the bounding box of this instance.
  ///
  dbBox* getBBox();

  ///
  /// Get the placement status of this instance.
  ///
  dbPlacementStatus getPlacementStatus();

  ///
  /// Is the placement status of this instance fixed
  ///
  bool isFixed() { return getPlacementStatus().isFixed(); }

  ///
  /// Is the placement status of this instance placed
  ///
  bool isPlaced() { return getPlacementStatus().isPlaced(); }

  ///
  /// Set the placement status of this instance.
  ///
  void setPlacementStatus(dbPlacementStatus status);

  ///
  /// Get the eco state bits to be used when an ECO block is created
  ///
  bool getEcoCreate();
  bool getEcoDestroy();
  bool getEcoModify();

  ///
  /// Set the eco state bits to be used when an ECO block is created
  ///
  void setEcoCreate(bool v);
  void setEcoDestroy(bool v);
  void setEcoModify(bool v);

  ///
  /// Get the user-defined flag bit.
  ///
  bool getUserFlag1();

  ///
  /// Set the user-defined flag bit.
  ///
  void setUserFlag1();

  ///
  /// Clear the user-defined flag bit.
  ///
  void clearUserFlag1();

  ///
  /// Get the user-defined flag bit.
  ///
  bool getUserFlag2();

  ///
  /// Set the user-defined flag bit.
  ///
  void setUserFlag2();

  ///
  /// Clear the user-defined flag bit.
  ///
  void clearUserFlag2();

  ///
  /// Get the user-defined flag bit.
  ///
  bool getUserFlag3();

  ///
  /// Set the user-defined flag bit.
  ///
  void setUserFlag3();

  ///
  /// Clear the user-defined flag bit.
  ///
  void clearUserFlag3();

  ///
  /// Set/Reset the don't-touch flag
  ///
  /// Setting this implies:
  /// - The instance can't be destroyed
  /// - The instance can't be resized (ie swapMaster)
  /// - The associated iterms can't be connected or disconnected
  /// - The parent module can't be changed
  /// - The instance CAN be moved, have its orientation changed, or be
  ///   placed or unplaced
  ///
  void setDoNotTouch(bool v);

  ///
  /// Returns true if the don't-touch flag is set.
  ///
  bool isDoNotTouch();

  ///
  /// Get the block of this instance.
  ///
  dbBlock* getBlock() const;

  ///
  /// Get the Master of this instance.
  ///
  dbMaster* getMaster() const;

  ///
  /// Get the group of this instance.
  ///
  dbGroup* getGroup();

  ///
  /// Get the instance-terminals of this instance.
  ///
  dbSet<dbITerm> getITerms() const;

  ///
  /// Get the first input terminal of this instance.
  ///
  dbITerm* getFirstInput() const;

  ///
  /// Get the first output terminal of this instance.
  ///
  dbITerm* getFirstOutput() const;

  ///
  /// Get the region this instance belongs to. Returns nullptr if instance has
  /// no assigned region.
  ///
  dbRegion* getRegion();

  ///
  /// Get the module this instance belongs to. Returns nullptr if instance has
  /// no assigned module.
  ///
  dbModule* getModule();

  ///
  /// Find the iterm of the given terminal name.
  /// Returns nullptr if no terminal was found.
  ///
  dbITerm* findITerm(const char* terminal_name);

  ///
  /// Find the iterm of the given terminal name given the master term order
  ///
  dbITerm* getITerm(uint32_t mterm_order_id);

  ///
  /// Get the all the instances connected to the net of each iterm of this
  /// instance. Only traverse nets of the specified SigType. Default is
  /// dbSigType::SIGNAL.
  ///
  void getConnectivity(std::vector<dbInst*>& neighbors,
                       dbSigType::Value value = dbSigType::SIGNAL);

  ///
  /// Bind the hierarchical (child) block to this instance.
  ///
  /// This method creates connectivity across the hierarchy.
  ///
  ///     (block)
  ///     +------------------------------------------------------------------+
  ///     |                                                                  |
  ///     |                                                                  |
  ///     |                                                                  |
  ///     |               (child-block / instance)                           |
  ///     |               +----------------------------------+               |
  ///     |               |                                  |               |
  ///     |B             I|B                                 |               |
  ///     |o.............o|o..........                       |               |
  ///     |  (net in      |  (net in child-block)            |               |
  ///     |   top-block)  |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               +----------------------------------+               |
  ///     |                                                                  |
  ///     |                                                                  |
  ///     +------------------------------------------------------------------+
  ///
  /// B = dbBterm
  /// I = dbIterm
  ///
  /// This method will return false under the following situations:
  ///
  ///    1) This instance is already bound to a block.
  ///    2) This block cannot already be bound to an instance.
  ///    2) The block must be a direct child of the instance block.
  ///    3) block bterms must map 1-to-1 (by name) to the master mterms.
  ///
  bool bindBlock(dbBlock* block, bool force = false);

  ///
  /// Unbind the hierarchical (child) block from this instance.
  /// Does nothing if this instance has no child block.
  ///
  void unbindBlock();

  //
  // reset _hierachy to 0; not fully understood and tested!!!
  //
  bool resetHierarchy(bool verbose);
  ///
  /// Get the hierarchical (child) block bound to this instance.
  /// Returns nullptr if this instance has no child.
  ///
  dbBlock* getChild();

  ///
  /// Get the parent instance of this instance.
  /// Returns nullptr if this instance has no parent.
  ///
  dbInst* getParent();

  ///
  /// Get the children of this instance.
  ///
  dbSet<dbInst> getChildren();

  ///
  /// Returns true if this instance has hierarchy.
  ///
  bool isHierarchical();

  ///
  /// Returns true if this instance is physical only.
  ///
  bool isPhysicalOnly();

  ///
  /// Returns a halo assigned to this instance.
  /// Returns nullptr if this instance has no halo.
  ///
  dbBox* getHalo();

  ///
  /// Get the weight assigned to this instance.
  /// (Default: 1)
  ///
  int getWeight();

  ///
  /// Set the weight assigned of this instance.
  ///
  void setWeight(int weight);

  ///
  /// Get the source assigned to this instance.
  ///
  dbSourceType getSourceType();

  ///
  /// Set the source assigned of this instance.
  ///
  void setSourceType(dbSourceType type);

  ///
  /// Get the iterm that represents this mterm
  ///
  dbITerm* getITerm(dbMTerm* mterm);

  ///
  /// Swap the master of this instance.
  ///
  /// Returns true if the operations succeeds.
  /// NOTE: If this instance is bound to a block hierarchy, the master cannot be
  /// swapped.
  ///
  /// This method invalidates any existing dbSet<dbITerm>::iterator.
  ///
  bool swapMaster(dbMaster* master);

  ///
  /// Is the master's type BLOCK or any of its subtypes
  ///
  bool isBlock() const;

  ///
  /// Is the master's type CORE or any of its subtypes
  ///
  bool isCore() const;

  ///
  /// Is the master's type PAD or any of its subtypes
  ///
  bool isPad() const;

  ///
  /// Is the master's type ENDCAP or any of its subtypes
  ///
  bool isEndCap() const;

  ///
  /// Get the scan version of this instance.
  /// Returns nullptr if this instance has no scan version.
  ///
  dbScanInst* getScanInst() const;

  void setPinAccessIdx(uint32_t idx);

  uint32_t getPinAccessIdx() const;

  ///
  /// Create a new instance.
  /// If physical_only is true, the instance can only be added to a top module.
  /// If false, it will be added to the parent module.
  /// Returns nullptr if an instance with this name already exists.
  /// Returns nullptr if the master is not FROZEN.
  /// If dbmodule is non null the dbInst is added to that module.
  ///
  static dbInst* create(dbBlock* block,
                        dbMaster* master,
                        const char* name,
                        bool physical_only = false,
                        dbModule* parent_module = nullptr);

  ///
  /// Create a new instance with a unique name.
  ///
  static dbInst* create(dbBlock* block,
                        dbMaster* master,
                        const char* base_name,
                        const dbNameUniquifyType& uniquify,
                        dbModule* parent_module = nullptr);

  static dbInst* create(dbBlock* block,
                        dbMaster* master,
                        const char* name,
                        dbRegion* region,
                        bool physical_only = false,
                        dbModule* parent_module = nullptr);

  static dbInst* makeUniqueDbInst(dbBlock* block,
                                  dbMaster* master,
                                  const char* name,
                                  bool physical_only = false,
                                  dbModule* target_module = nullptr);

  ///
  /// Create a new instance of child_block in top_block.
  /// This is a convenience method to create the instance, an
  /// interface dbMaster from child_block, and bind the instance
  /// to the child_block.
  ///
  static dbInst* create(dbBlock* top_block,
                        dbBlock* child_block,
                        const char* name);

  ///
  /// Delete the instance from the block.
  ///
  static void destroy(dbInst* inst);

  ///
  /// Safely delete the inst from the block within an iterator
  ///
  static dbSet<dbInst>::iterator destroy(dbSet<dbInst>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbInst* getInst(dbBlock* block, uint32_t oid);

  ///
  /// Translate a valid database-id back to a pointer.
  ///
  static dbInst* getValidInst(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbITerm (instance-terminal) is the element that represents a connection
/// to a master-terminal of an instance.
///
///////////////////////////////////////////////////////////////////////////////
class dbITerm : public dbObject
{
 public:
  ///
  /// Get the instance of this instance-terminal.
  ///
  dbInst* getInst() const;

  ///
  /// Get the net of this instance-terminal.
  /// Returns nullptr if this instance-terminal has NOT been connected
  /// to a net.
  ///
  dbNet* getNet() const;

  dbModNet* getModNet() const;

  ///
  /// Get the master-terminal that this instance-terminal is representing.
  ///
  dbMTerm* getMTerm() const;

  ///
  /// Get the name of this iterm.  This is not persistently stored
  /// and is constructed on the fly.
  ///
  std::string getName(char separator = '/') const;

  ///
  /// Get bbox of this iterm (ie the transfromed bbox of the mterm)
  ///
  Rect getBBox();

  ///
  /// Get the block this instance-terminal belongs too.
  ///
  dbBlock* getBlock() const;

  ///
  /// Get the signal type of this instance-terminal.
  ///
  dbSigType getSigType() const;

  ///
  /// Get the IO direction of this instance-terminal.
  ///
  dbIoType getIoType() const;

  ///
  /// True is iterm is input of signal type; if io false INOUT is not considered
  /// iput
  ///
  bool isInputSignal(bool io = true);

  ///
  /// True is iterm is output of signal type; if io false INOUT is not
  /// considered iput
  ///
  bool isOutputSignal(bool io = true);

  ///
  /// Mark this dbITerm as spef. v should 1 or 0
  ///
  void setSpef(uint32_t v);

  ///
  /// Return true if this dbITerm flag spef is set to 1
  ///
  bool isSpef();

  ///
  /// set ext id
  ///
  void setExtId(uint32_t v);

  ///
  /// get ext id
  ///
  uint32_t getExtId();

  ///
  /// Returns true if this dbITerm is marked as special. Special nets/iterms are
  /// declared in the SPECIAL NETS section of a DEF file.
  ///
  bool isSpecial();

  ///
  /// Mark this dbITerm as special.
  ///
  void setSpecial();

  ///
  /// Unmark this dbITerm as special.
  ///
  void clearSpecial();

  ///
  /// Set clocked of this instance-terminal.
  ///
  void setClocked(bool v);

  ///
  /// get clocked of this instance-terminal.
  ///
  bool isClocked();

  ///
  /// Set mark of this instance-terminal.
  ///
  void setMark(uint32_t v);

  ///
  /// get mark of this instance-terminal.
  ///
  bool isSetMark();

  ///
  /// Returns true if this dbITerm has been marked as physically connected.
  ///
  bool isConnected();

  ///
  /// Mark this dbITerm as physically connected.
  ///
  void setConnected();

  ///
  /// Unmark this dbITerm as physically connected.
  ///
  void clearConnected();

  ///
  /// Get the hierarchical child bterm of this iterm.
  /// Returns nullptr if there is no child bterm.
  ///
  ///
  ///     (top-block)
  ///     +------------------------------------------------------------------+
  ///     |                                                                  |
  ///     |                                                                  |
  ///     |                                                                  |
  ///     |               (child-block / instance)                           |
  ///     |               +----------------------------------+               |
  ///     |               |                                  |               |
  ///     |B             I|B                                 |               |
  ///     |o.............o|o..........                       |               |
  ///     |  (net in      |  (net in child-block)            |               |
  ///     |   top-block)  |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               |                                  |               |
  ///     |               +----------------------------------+               |
  ///     |                                                                  |
  ///     |                                                                  |
  ///     +------------------------------------------------------------------+
  ///
  ///
  /// B = dbBterm
  /// I = dbIterm
  ///
  dbBTerm* getBTerm();

  ///
  /// Connect this iterm to a single "flat" net.
  ///
  void connect(dbNet* net);

  // connect this iterm to a single dbmodNet

  void connect(dbModNet* net);

  // simultaneously connect this iterm to both a dbnet and a mod net.
  // but do not do a reassociate (that is done by higher level api
  // call in dbNetwork::connectPin)
  //
  void connect(dbNet* db_net, dbModNet* db_mod_net);

  ///
  /// Disconnect this iterm from the net it is connected to.
  /// Will remove from both mod net and dbNet.
  ///
  void disconnect();

  ///
  /// Disconnect just the db net
  ///
  void disconnectDbNet();

  ///
  /// Disconnect just the mod net
  ///

  void disconnectDbModNet();

  ///
  /// Get the average of the centers for the iterm shapes
  /// Returns false if iterm has no shapes
  ///
  bool getAvgXY(int* x, int* y) const;

  ///
  /// Returns all geometries of all dbMPin associated with
  /// the dbMTerm.
  ///
  std::vector<std::pair<dbTechLayer*, Rect>> getGeometries() const;

  void setAccessPoint(dbMPin* pin, dbAccessPoint* ap);

  ///
  /// Returns preferred access points per each pin.
  /// One preffered access point, if available, for each pin.
  ///
  std::vector<dbAccessPoint*> getPrefAccessPoints() const;

  ///
  /// Returns all access points for each pin.
  ///
  std::map<dbMPin*, std::vector<dbAccessPoint*>> getAccessPoints() const;

  ///
  /// Destroys all access points of each pin.
  ///
  void clearPrefAccessPoints();

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbITerm* getITerm(dbBlock* block, uint32_t oid);

  uint32_t staVertexId();
  void staSetVertexId(uint32_t id);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbVia is the element that represents a block specific via definition.
/// These vias are typically generated to be used in power routing.
///
///////////////////////////////////////////////////////////////////////////////
class dbVia : public dbObject
{
 public:
  ///
  /// Get the via name.
  ///
  std::string getName();

  ///
  /// Get the via name.
  ///
  const char* getConstName();

  ///
  /// Get the pattern value of this via.
  /// Returns and empty ("") string if a pattern has not been set.
  ///
  std::string getPattern();

  ///
  /// Set the pattern value of this via.
  /// The pattern is ignored if the pattern is already set on this via
  ///
  void setPattern(const char* pattern);

  ///
  /// Set generate rule that was used to genreate this via.
  ///
  void setViaGenerateRule(dbTechViaGenerateRule* rule);

  ///
  /// Get the generate rule that was used to genreate this via.
  ///
  dbTechViaGenerateRule* getViaGenerateRule();

  ///
  /// Returns true if this via has params.
  ///
  bool hasParams();

  ///
  /// Set via params to generate this via. This method will create the shapes
  /// of this via. All previous shapes are destroyed.
  ///
  void setViaParams(const dbViaParams& params);

  ///
  /// Get the via params used to generate this via.
  ///
  dbViaParams getViaParams();

  ///
  /// Get the block this via belongs too.
  ///
  dbBlock* getBlock();

  ///
  /// Get the bbox of this via
  /// Returns nullptr if this via has no shapes.
  ///
  dbBox* getBBox();

  ///
  /// Get the boxes of this via.
  ///
  dbSet<dbBox> getBoxes();

  ///
  /// Get the upper-most layer of this via reaches
  /// Returns nullptr if this via has no shapes.
  ///
  dbTechLayer* getTopLayer();

  ///
  /// Get the lower-most layer of this via reaches
  /// Returns nullptr if this via has no shapes.
  ///
  dbTechLayer* getBottomLayer();

  ///
  /// Returns true if this via is a rotated version of a block or tech via.
  ///
  /// Vias in DEF can now be rotated.
  ///
  bool isViaRotated();

  ///
  /// Get the rotation of this via.
  ///
  dbOrientType getOrient();

  //
  // Get the tech-via that this roated via represents.
  /// Returns nullptr if this via does not represent a tech via
  //
  dbTechVia* getTechVia();

  //
  // Get the block-via that this roated via represents.
  /// Returns nullptr if this via does not represent a block via
  //
  dbVia* getBlockVia();

  void setDefault(bool);

  bool isDefault();

  ///
  /// Create a block specific via.
  /// Returns nullptr if a via with this name already exists.
  ///
  static dbVia* create(dbBlock* block, const char* name);

  ///
  /// Created a rotated version of the specified block-via.
  /// Returns nullptr if a via with this name already exists.
  ///
  static dbVia* create(dbBlock* block,
                       const char* name,
                       dbVia* via,
                       dbOrientType type);

  ///
  /// Created a rotated version of the specified tech-via.
  /// Returns nullptr if a via with this name already exists.
  ///
  static dbVia* create(dbBlock* block,
                       const char* name,
                       dbTechVia* via,
                       dbOrientType type);

  /// Copy all the VIA's from the src-block to the dst-block.
  /// Returns false the copy failed.
  static bool copy(dbBlock* dst, dbBlock* src);

  /// Copy the VIA to the dst-block.
  static dbVia* copy(dbBlock* dst, dbVia* src);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbVia* getVia(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
///  A dbWire is the element used to represent net wires. A wire object
///  represents a sequence of from-to paths, which may form either
///  a disjoint or non-disjoint set of paths. A wire is encoded using the
///  dbWireEncoder class, see "dbWireCodec.h". A wire is decoded using
///  the dbWireDecoder class.
///
///////////////////////////////////////////////////////////////////////////////
class dbWire : public dbObject
{
 public:
  ///
  /// Get the block this wire belongs too.
  ///
  dbBlock* getBlock();

  ///
  /// Get the net this wire is attached too.
  /// Returns nullptr if this wire is not attached to a net.
  ///
  dbNet* getNet();

  ///
  /// Append the wire to this wire. This operation will fail if the wire
  /// is from another block and the wire contains bterms or iterms.
  ///
  void append(dbWire* wire, bool singleSegmentWire = false);

  ///
  /// Get junction id associated with the term
  ///
  uint32_t getTermJid(int termid) const;

  ///
  /// Get the shape of this shape-id.
  /// PRECONDITION: shape-id is a segment or via
  ///
  void getShape(int shape_id, dbShape& shape);

  ///
  /// Get the segment of this segment-id
  ///
  /// PRECONDITION: segment_id is a segment
  ///
  void getSegment(int segment_id, dbShape& shape);

  ///
  /// Get the segment of this segment-id, where layer is the layer of the
  /// segment
  ///
  /// PRECONDITION: segment_id is a segment
  ///
  void getSegment(int segment_id, dbTechLayer* layer, dbShape& shape);

  ///
  /// Get the previous via of this shape-id.
  /// returns false if the previous shape is not a via.
  ///
  bool getPrevVia(int shape_id, dbShape& shape);

  ///
  /// Get the via that follows of this shape-id.
  /// returns false if the next shape is not a via.
  ///
  bool getNextVia(int shape_id, dbShape& shape);

  ///
  /// Get the via boxes of this via_shape-id.
  ///
  /// returns false if this shape_id is not a via.
  ///
  bool getViaBoxes(int via_shape_id, std::vector<dbShape>& shapes);

  ///
  /// Returns true if this wire is a global-wire
  ///
  bool isGlobalWire();

  ///
  /// Get the bounding box of this wire
  ///
  std::optional<Rect> getBBox();

  ///
  /// Get the total path length contained in this wire.
  ///
  uint64_t getLength();

  ///
  /// Get the number of entries contained in this wire.
  ///
  uint32_t length();

  ///
  /// Get the count of wire segments contained in this wire.
  ///
  uint32_t count();

  ///
  /// Get junction coordinate.
  ///
  Point getCoord(int jid);

  ///
  /// Get junction property
  ///
  bool getProperty(int jid, int& property);

  ///
  /// Set junction property
  ///
  bool setProperty(int jid, int property);

  ///
  /// Set one data entry
  ///
  int getData(int n);

  ///
  /// Set one opcode entry
  ///
  unsigned char getOpcode(int n);

  ///
  /// Attach this wire to a net.
  ///   1) If the net is already attached to another wire, the other wire will
  ///      be destroyed.
  ///   2) If this wire is already attached to another net, thie wire will be
  ///   detachd from
  //       the other net.
  ///
  void attach(dbNet* net);

  ///
  /// Detach this wire from a net.
  ///
  void detach();

  ///
  /// Create a wire.
  /// Returns nullptr if this net already has the specified wire dbWire.
  ///
  static dbWire* create(dbNet* net, bool global_wire = false);

  ///
  /// Create an unattached wire.
  ///
  static dbWire* create(dbBlock* block, bool global_wire = false);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbWire* getWire(dbBlock* block, uint32_t oid);

  ///
  /// Destroy a wire.
  ///
  static void destroy(dbWire* wire);

 private:
  void addOneSeg(unsigned char op,
                 int value,
                 uint32_t jj,
                 int* did,
                 dbRSeg** new_rsegs);
  void addOneSeg(unsigned char op, int value);

  friend class dbNet;
};

///////////////////////////////////////////////////////////////////////////////
///
///  A dbSWire is the element used to represent special-net wires.
///
///////////////////////////////////////////////////////////////////////////////
class dbSWire : public dbObject
{
 public:
  ///
  /// Get the block this wire belongs too.
  ///
  dbBlock* getBlock();

  ///
  /// Get the net this wire is attached too.
  ///
  dbNet* getNet();

  ///
  /// Return the wire-type.
  ///
  dbWireType getWireType();

  ///
  /// Returns the shield net if the wire-type is dbWireType::SHIELD
  ///
  dbNet* getShield();

  ///
  /// Get the wires of this special wire.
  ///
  dbSet<dbSBox> getWires();

  ///
  /// Create a new special-wire.
  ///
  static dbSWire* create(dbNet* net, dbWireType type, dbNet* shield = nullptr);

  ///
  /// Delete this wire
  ///
  static void destroy(dbSWire* swire);

  ///
  /// Delete this wire
  ///
  static dbSet<dbSWire>::iterator destroy(dbSet<dbSWire>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbSWire* getSWire(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbGCellGrid is the element that represents a block specific grid
/// definition.
///
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
/// A dbTrackGrid is the element that represents a block tracks.
///
///////////////////////////////////////////////////////////////////////////////
class dbTrackGrid : public dbObject
{
 public:
  ///
  /// Get the layer for this track-grid.
  ///
  dbTechLayer* getTechLayer();

  ///
  /// Get the "X" track coordinates for a this tech-layer.
  ///
  void getGridX(std::vector<int>& x_grid);
  const std::vector<int>& getGridX();

  ///
  /// Get the "Y" track coordinates for a this tech-layer.
  ///
  void getGridY(std::vector<int>& y_grid);
  const std::vector<int>& getGridY();

  ///
  /// Get the block this grid belongs too.
  ///
  dbBlock* getBlock();

  ///
  /// Add a "X" grid pattern.
  ///
  void addGridPatternX(int origin_x,
                       int line_count,
                       int step,
                       int first_mask = 0,
                       bool samemask = false);

  ///
  /// Add a "Y" grid pattern.
  ///
  void addGridPatternY(int origin_y,
                       int line_count,
                       int step,
                       int first_mask = 0,
                       bool samemask = false);

  ///
  /// Get the number of "X" grid patterns.
  ///
  int getNumGridPatternsX();

  ///
  /// Get the number of "Y" grid patterns.
  ///
  int getNumGridPatternsY();

  ///
  /// Get the "ith" "X" grid pattern.
  ///
  void getGridPatternX(int i, int& origin_x, int& line_count, int& step);
  void getGridPatternX(int i,
                       int& origin_x,
                       int& line_count,
                       int& step,
                       int& first_mask,
                       bool& samemask);

  ///
  /// Get the "ith" "Y" grid pattern.
  ///
  void getGridPatternY(int i, int& origin_y, int& line_count, int& step);
  void getGridPatternY(int i,
                       int& origin_y,
                       int& line_count,
                       int& step,
                       int& first_mask,
                       bool& samemask);
  ///
  /// Create an empty Track grid.
  /// Returns nullptr if a the grid for this layer already exists.
  ///
  static dbTrackGrid* create(dbBlock* block, dbTechLayer* layer);

  ///
  /// Get the spacing between tracks for this grid.
  /// If the layer has a multi pattern spacing, returns the average.
  ///
  void getAverageTrackSpacing(int& track_step,
                              int& track_init,
                              int& num_tracks);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTrackGrid* getTrackGrid(dbBlock* block, uint32_t oid);

  ///
  /// destroy a grid
  ///
  static void destroy(dbTrackGrid* grid_);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbObstruction is the element that represents a routing
/// obstruction in a block.
///
///////////////////////////////////////////////////////////////////////////////
class dbObstruction : public dbObject
{
 public:
  ///
  /// Get the bbox of this obstruction.
  ///
  dbBox* getBBox();

  ///
  /// Get the instance associated with this obstruction.
  /// Returns nullptr of no instance was associated with this obstruction
  ///
  dbInst* getInstance();

  ///
  /// Declare this obstruction to be a "slot" obstruction.
  ///
  void setSlotObstruction();

  ///
  /// Returns true if this obstruction is a "slot" obstruction.
  ///
  bool isSlotObstruction();

  ///
  /// Declare this obstruction to be a "fill" obstruction.
  ///
  void setFillObstruction();

  ///
  /// Returns true if this obstruction is a "fill" obstruction.
  ///
  bool isFillObstruction();

  ///
  /// Declare this obstruction to be non "power/ground" obstruction.
  ///
  void setExceptPGNetsObstruction();

  ///
  /// Returns true if this obstruction is a non "power/ground" obstruction.
  ///
  bool isExceptPGNetsObstruction();

  ///
  /// Declare this obstruction to have been pushed into this block.
  ///
  void setPushedDown();

  ///
  /// Returns true if this obstruction was pushed into this block.
  ///
  bool isPushedDown();
  ///
  /// Returns true if this bpin has an effective-width rule.
  ///
  bool hasEffectiveWidth();

  ///
  /// Set the effective width rule.
  ///
  void setEffectiveWidth(int w);

  ///
  /// Return the effective width rule.
  ///
  int getEffectiveWidth();

  ///
  /// Returns true if this bpin has an min-spacing rule.
  ///
  bool hasMinSpacing();

  ///
  /// Set the min spacing rule.
  ///
  void setMinSpacing(int w);

  ///
  /// Return the min spacing rule.
  ///
  int getMinSpacing();

  ///
  /// Get the block this obstruction belongs too.
  ///
  dbBlock* getBlock();

  ///
  /// Delete this obstruction from this block.
  ///
  static void destroy(dbObstruction* obstruction);

  ///
  /// Delete the blockage from the block.
  ///
  static dbSet<dbObstruction>::iterator destroy(
      dbSet<dbObstruction>::iterator& itr);

  ///
  /// Returns true if this obstruction is system created. System created
  /// obstructions represent obstructions created by non-rectangular floorplans.
  /// The general flow is the polygonal floorplan is subtracted
  /// from its general bounding box and the shapes that are created
  /// by that difference are then decomposed into rectangles which
  /// create obstructions with the system created annotation.
  ///
  bool isSystemReserved();

  ///
  /// Sets this obstruction as system created.
  ///
  void setIsSystemReserved(bool is_system_reserved);

  ///
  /// Create a routing obstruction.
  ///
  static dbObstruction* create(dbBlock* block,
                               dbTechLayer* layer,
                               int x1,
                               int y1,
                               int x2,
                               int y2,
                               dbInst* inst = nullptr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbObstruction* getObstruction(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbBlockage is the element that represents a placement blockage in a block.
///
///////////////////////////////////////////////////////////////////////////////
class dbBlockage : public dbObject
{
 public:
  ///
  /// Get the bbox of this blockage.
  ///
  dbBox* getBBox();

  ///
  /// Get the instance associated with this blockage.
  /// Returns nullptr of no instance was associated with this blockage
  ///
  dbInst* getInstance();

  ///
  /// Declare this blockage to have been pushed into this block.
  ///
  void setPushedDown();

  ///
  /// Returns true if this blockage was pushed into this block.
  ///
  bool isPushedDown();

  ///
  /// Declare this blockage is soft.
  ///
  void setSoft();

  ///
  /// Returns true if this blockage is soft.
  ///
  bool isSoft();

  ///
  /// Returns true if this blockage is system created. System created blockages
  /// represent blockages created by non-rectangular floorplans.
  /// The general flow is the polygonal floorplan is subtracted
  /// from its general bounding box and the shapes that are created
  /// by that difference are then decomposed into rectangles which
  /// create blockages with the system created annotation.
  ///
  bool isSystemReserved();

  ///
  /// Sets this blockage as system created.
  ///
  void setIsSystemReserved(bool is_system_reserved);

  ///
  /// Set the max placement density percentage in [0,100]
  ///
  void setMaxDensity(float max_density);

  ///
  /// Returns the max placement density percentage
  ///
  float getMaxDensity();

  ///
  /// Get the block this blockage belongs too.
  ///
  dbBlock* getBlock();

  ///
  /// Create a placement blockage.
  ///
  static dbBlockage* create(dbBlock* block,
                            int x1,
                            int y1,
                            int x2,
                            int y2,
                            dbInst* inst = nullptr);

  static void destroy(dbBlockage* blockage);

  ///
  /// Delete the blockage from the block.
  ///
  static dbSet<dbBlockage>::iterator destroy(dbSet<dbBlockage>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbBlockage* getBlockage(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A RCSeg is the element that represents an RC element in a RC network.
///
/// The segment junction nodes are denoted "source" and "target". However,
/// this namimg is just a convention and there is no implied directional
/// meaning.
///
///////////////////////////////////////////////////////////////////////////////
class dbCapNode : public dbObject
{
 public:
  ///
  /// Add the capacitances of other capnode to this capnode
  ///
  void addCapnCapacitance(dbCapNode* other);

  ///
  /// Add the gndCap of this capnode to *gndcap and *totalcap
  ///
  void addGndCap(double* gndcap, double* totalcap);

  /// Add the gndCap to *gndcap and *totalcap, ccCap to *totalcap
  ///
  void addGndTotalCap(double* gndcap, double* totalcap, double miller_mult);

  ///
  /// Get the gndCap of this capnode to *gndcap and *totalcap
  ///
  void getGndCap(double* gndcap, double* totalcap);

  ///
  /// Get the gndCap to *gndcap and *totalcap, ccCap to *totalcap
  ///
  void getGndTotalCap(double* gndcap, double* totalcap, double miller_mult);

  ///
  /// Add the caps of all corners of CC's from this capnode to *totalcap
  ///
  void accAllCcCap(double* totalcap, double miller_mult);

  ///
  /// Set the capacitance of this CapNode segment for this process corner. Value
  /// must be in femto-fards.
  ///
  void setCapacitance(double cap, int corner = 0);

  ///
  /// Add the capacitance of this CapNode segment for this process corner. Value
  /// must be in femto-fards.
  ///
  void addCapacitance(double cap, int corner = 0);

  ///
  /// add cc capacitance to gnd capacitance of this capNode
  ///
  bool groundCC(float gndFactor);

  ///
  ///  Adjust the capacitance of this capNode for this process corner
  ///
  void adjustCapacitance(float factor, uint32_t corner);

  ///
  ///  Adjust the capacitance of this capNode
  ///
  void adjustCapacitance(float factor);

  ///
  /// check if having CC's with capacitnce greater than ccThreshHold
  ///
  bool needAdjustCC(double ccThreshHold);

  ///
  /// adjust CC's of this capNode
  ///
  void adjustCC(uint32_t adjOrder,
                float adjFactor,
                std::vector<dbCCSeg*>& adjustedCC,
                std::vector<dbNet*>& halonets);

  ///
  /// Get the capacitance of this capNode segment for this process corner.
  /// Returns value in femto-fards.
  ///
  double getCapacitance(uint32_t corner = 0);

  ///
  /// Get the rc-network cap node.
  ///
  uint32_t getNode();

  ///
  /// Get the shapeId of the cap node.
  ///
  uint32_t getShapeId();

  ///
  /// Set the rc-network cap node.
  ///
  void setNode(uint32_t nodeid);

  ///
  /// Get next cap node in same net
  ///
  //  dbCapNode *getNext(dbBlock *block_);

  ///
  ///  is this node iterm/bterm/internal/branch/dangling/foreign .
  ///
  bool isName();
  bool isITerm();
  bool isBTerm();
  bool isInternal();
  bool isBranch();
  bool isDangling();
  bool isForeign();
  bool isTreeNode();  // bterm, iterm, branch
  // bool isSourceNodeBterm();
  bool isSourceTerm(dbBlock* mblock = nullptr);
  bool isInoutTerm(dbBlock* mblock = nullptr);

  ///
  /// Returns the select flag value. This flag specified that the
  /// net has been select.
  ///
  bool isSelect();

  ///
  /// Set the select flag to the specified value.
  ///
  void setSelect(bool value);

  ///
  ///  increase children cnt; capNode is a branch of the rooted tree.
  ///
  uint32_t incrChildrenCnt();
  uint32_t getChildrenCnt();
  void setChildrenCnt(uint32_t cnt);

  ///
  ///  set iterm/bterm/internal/branch/foreign flag of this cap node.
  ///
  void setNameFlag();
  void setBTermFlag();
  void setITermFlag();
  void setInternalFlag();
  void setBranchFlag();
  void setForeignFlag();

  ///
  ///  reset iterm/bterm/internal/branch/foreign flag of this cap node.
  ///
  void resetNameFlag();
  void resetBTermFlag();
  void resetITermFlag();
  void resetInternalFlag();
  void resetBranchFlag();
  void resetForeignFlag();

  ///
  /// Get the sort index of this node
  ///
  uint32_t getSortIndex();

  ///
  /// Set the sort index of this node
  ///
  void setSortIndex(uint32_t idx);

  ///
  /// Get the coordinates of this node if iterm or bterm
  ///
  bool getTermCoords(int& x, int& y, dbBlock* mblock = nullptr);

  ///
  /// Get the iterm of this node
  ///
  dbITerm* getITerm(dbBlock* mblock = nullptr);

  ///
  /// Get the bterm of this node
  ///
  dbBTerm* getBTerm(dbBlock* mblock = nullptr);

  ///
  /// Set the _ycoord of this node
  ///
  /// void setCoordY(int y);

  ///
  /// Get the _ycoord of this node
  ///
  /// void getCoordY(int & y);

  ///
  /// print the ccsegs of the capn
  ///
  void printCC();

  ///
  /// check the ccsegs of the capn
  ///
  bool checkCC();

  ///
  /// Get the coupling caps bound to this node
  ///
  dbSet<dbCCSeg> getCCSegs();

  ///
  /// Net given the capNode id
  ///
  dbNet* getNet();

  ///
  /// set net
  ///
  void setNet(uint32_t netid);

  ///
  /// set next
  ///
  void setNext(uint32_t nextid);

  ///
  /// Create a new rc-segment
  /// The default values for each process corner is 0.0.
  ///
  static dbCapNode* create(dbNet* net, uint32_t node, bool foreign);

  ///
  /// add a seg onto a net
  ///
  void addToNet();

  ///
  /// Destroy a rc-segment.
  ///
  static void destroy(dbCapNode* seg, bool destroyCC = true);

  ///
  /// Destroy a rc-segment.
  ///
  static dbSet<dbCapNode>::iterator destroy(dbSet<dbCapNode>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbCapNode* getCapNode(dbBlock* block, uint32_t oid);

 private:
  ///
  /// Get the capacitance of this capNode segment for this process corner.
  /// Returns value in femto-fards.
  ///
  void getCapTable(double* cap);

  friend class dbRSeg;
};

///////////////////////////////////////////////////////////////////////////////
///
/// A RSeg is the element that represents an Res element in a Res network.
///
/// The segment junction nodes are denoted "source" and "target". However,
/// this namimg is just a convention and there is no implied directional
/// meaning.
///
///////////////////////////////////////////////////////////////////////////////
class dbRSeg : public dbObject
{
 public:
  ///
  /// Add the resistances of other rseg to this rseg
  ///
  void addRSegResistance(dbRSeg* other);

  ///
  /// Add the capacitances of other rseg to this rseg
  ///
  void addRSegCapacitance(dbRSeg* other);

  ///
  /// Get the resistance of this RC segment for this process corner. Returns
  /// value in ohms.
  ///
  double getResistance(int corner = 0);

  ///
  /// Get the resistance of this RC segment to *res
  ///
  void getAllRes(double* res);

  ///
  /// Add the resistance of this RC segment to *res
  ///
  void addAllRes(double* res);

  ///
  /// Get the gdn cap of this RC segment to *gndcap and *totalcap
  ///
  void getGndCap(double* gndcap, double* totalcap);

  ///
  /// Add the gdn cap of this RC segment to *gndcap and *totalcap
  ///
  void addGndCap(double* gndcap, double* totalcap);

  ///
  /// Get the gdn cap of this RC segment to *gndcap, total cap to *totalcap
  ///
  void getGndTotalCap(double* gndcap, double* totalcap, double miller_mult);

  ///
  /// Add the gdn cap of this RC segment to *gndcap, total cap to *totalcap
  ///
  void addGndTotalCap(double* gndcap, double* totalcap, double miller_mult);

  ///
  /// do merge rsegs
  ///
  void mergeRCs(std::vector<dbRSeg*>& mrsegs);

  ///
  /// Adjust the capacitance of this RC segment for this process corner.
  ///
  void adjustCapacitance(float factor, uint32_t corner);

  ///
  /// Adjust the capacitance of the src capNode of this RC segment for the
  /// process corner.
  ///
  void adjustSourceCapacitance(float factor, uint32_t corner);

  ///
  /// Adjust the capacitance of this RC segment.
  ///
  void adjustCapacitance(float factor);

  ///
  /// Set the capacitance of this RC segment for this process corner. Value must
  /// in FF.
  ///
  void setCapacitance(double cap, int corner = 0);

  ///
  /// Returns the _update_cap flag value. This flag specified that the
  /// rseg has been updated
  ///
  bool updatedCap();

  ///
  /// Get the capacitance of this RC segment for this process corner. Returns
  /// value in FF.
  ///
  double getCapacitance(int corner = 0);

  ///
  /// Get the capacitance of this RC segment for this process corner,
  /// plus coupling capacitance. Returns value in FF.
  ///
  double getSourceCapacitance(int corner = 0);

  ///
  /// Get the first capnode capacitance of this RC segment
  /// for this process corner, if foreign,
  /// plus coupling capacitance. Returns value in FF.
  ///
  double getCapacitance(int corner, double miller_mult);

  ///
  /// Get the CC segs of this RC segment,
  ///
  void getCcSegs(std::vector<dbCCSeg*>& ccsegs);

  ///
  /// print the CC segs of this RC segment,
  ///
  void printCcSegs();
  void printCC();
  bool checkCC();

  ///
  /// Get the capacitance table of this RC segment. value is in FF
  ///
  void getCapTable(double* cap);

  ///
  /// Set the resistance of this RC segment for this process corner. Value must
  /// be in ohms.
  ///
  void setResistance(double res, int corner = 0);

  ///
  ///  Adjust the resistance of this RC segment for this process corner
  ///
  void adjustResistance(float factor, int corner);

  ///
  ///  Adjust the resistance of this RC segment
  ///
  void adjustResistance(float factor);

  ///
  /// Set the next rseg
  ///
  void setNext(uint32_t next_id);

  ///
  /// Get the rc-network source node of this segment,
  ///
  uint32_t getSourceNode();

  ///
  /// Get the rc-network source node of this segment,
  ///
  dbCapNode* getSourceCapNode();

  ///
  /// Set the rc-network source node of this segment,
  ///
  void setSourceNode(uint32_t nodeid);

  ///
  /// Get the rc-network target node of this segment,
  ///
  uint32_t getTargetNode();

  ///
  /// Get the rc-network target node of this segment,
  ///
  dbCapNode* getTargetCapNode();

  ///
  /// Set the rc-network target node of this segment,
  ///
  void setTargetNode(uint32_t nodeid);

  ///
  /// Get shape-id of this RC-segment.
  ///
  uint32_t getShapeId();

  ///
  /// Set coordinates of this RC-segment.
  ///
  void setCoords(int x, int y);

  ///
  /// Get coordinates of this RC-segment.
  ///
  void getCoords(int& x, int& y);

  ///
  /// Set shape-id of this RC-segment, and the target capNode if internal.
  ///
  void updateShapeId(uint32_t nsid);

  ///
  /// check path direction
  ///
  bool pathLowToHigh();

  ///
  /// check if cap allocated
  ///
  bool allocatedCap();

  ///
  /// returns length and width.
  ///
  uint32_t getLengthWidth(uint32_t& w);

  ///
  /// add a seg onto a net
  ///
  bool addToNet();

  ///
  /// Get the net of this RC-segment.
  ///
  dbNet* getNet();

  ///
  /// Create a new r-segment
  /// The default values for each process corner is 0.0.
  ///
  static dbRSeg* create(dbNet* net,
                        int x,
                        int y,
                        uint32_t path_dir,
                        bool allocate_cap);

  ///
  /// Destroy a rc-segment.
  ///
  static void destroy(dbRSeg* seg);
  static void destroy(dbRSeg* seg, dbNet* net);

  ///
  /// simple destroy a disconnected rc-segment
  ///
  static void destroyS(dbRSeg* seg);

  ///
  /// Destroy a rc-segment.
  ///
  static dbSet<dbRSeg>::iterator destroy(dbSet<dbRSeg>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbRSeg* getRSeg(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A CCSeg is the element that represents an coupling capacitance element
/// in a RC network.
///
/// The segment junction nodes are denoted "source" and "target". However,
/// this namimg is just a convention and there is no implied directional
/// meaning.
///
///////////////////////////////////////////////////////////////////////////////
class dbCCSeg : public dbObject
{
 public:
  ///
  /// Adjust the capacitance of this CC segment
  ///
  void adjustCapacitance(float factor);

  ///
  /// Adjust the capacitance of a corner this CC segment
  ///
  void adjustCapacitance(float factor, int corner);

  ///
  /// Get the capacitance of this CC segment for this process corner. Returns
  /// value in femto-fards.
  ///
  double getCapacitance(int corner = 0);

  ///
  /// Set the capacitance of this CC segment for this process corner. Value must
  /// be in femto-fards.
  ///
  void setCapacitance(double cap, int corner = 0);

  ///
  /// Add the capacitance of this CC segment for this process corner. Value must
  /// be in femto-fards.
  ///
  void addCapacitance(double cap, int corner = 0);

  ///
  /// Add the capacitance of all corners of this CC segment to *ttcap
  ///
  void accAllCcCap(double* ttcap, double miller_mult);

  ///
  /// Get the capacitance of all corners of this CC segment to *ttcap
  ///
  void getAllCcCap(double* ttcap);

  ///
  /// Set the capacitance of all corners of this CC segment by *ttcap
  ///
  void setAllCcCap(double* ttcap);

  dbCapNode* getSourceCapNode();
  dbCapNode* getTargetCapNode();

  ///
  /// Add capacitance of other CC segment to this CC segment
  ///
  void addCcCapacitance(dbCCSeg* other);

  ///
  /// Change this CC segement's capNode orig to capNode new
  ///
  void swapCapnode(dbCapNode* orig, dbCapNode* newn);

  ///
  /// Get the capNode of this CC segment, other than oneCap
  ///
  dbCapNode* getTheOtherCapn(dbCapNode* oneCap, uint32_t& cid);

  /// Get the rc-network source node of this segment,
  ///
  uint32_t getSourceNodeNum();

  ///
  /// Set the rc-network source node of this segment,
  ///
  // void setSourceNode( uint32_t nodeid );

  ///
  /// Get the rc-network target node of this segment,
  ///
  uint32_t getTargetNodeNum();

  ///
  /// Set the rc-network target node of this segment,
  ///
  // void setTargetNode( uint32_t nodeid );

  ///
  /// Get the source net of this CC-segment.
  ///
  dbNet* getSourceNet();

  ///
  /// Get the target net of this CC-segment.
  ///
  dbNet* getTargetNet();

  ///
  /// Get the infile cnt of this CC-segment.
  ///
  uint32_t getInfileCnt();

  ///
  /// Increment the infile cnt of this CC-segment.
  ///
  void incrInfileCnt();

  ///
  /// Returns the mark flag value. This flag specified that the
  /// CC seg has been marked.
  ///
  bool isMarked();

  ///
  /// Set the mark flag to the specified value.
  ///
  void setMark(bool value);

  ///
  /// print CC's of capn
  ///
  void printCapnCC(uint32_t capn);

  ///
  /// check CC's of capn
  ///
  bool checkCapnCC(uint32_t capn);

  ///
  /// unlink cc from capn
  ///
  void unLink_cc_seg(dbCapNode* capn);

  ///
  /// link cc to capn
  ///
  void Link_cc_seg(dbCapNode* capn, uint32_t cseq);

  ///
  /// relink _cc_tgt_segs of a net
  /// Used in re-reading the CC part of a spef file.
  ///

  // static dbCCSeg * relinkTgtCC (dbNet *net_, dbCCSeg *pseg_, uint32_t
  // src_cap_node, uint32_t tgt_cap_node);

  ///
  /// Returns nullptr if not found
  ///
  static dbCCSeg* findCC(dbCapNode* nodeA, dbCapNode* nodeB);

  ///
  /// Create a new cc-segment.
  /// The default values for each process corner is 0.0.
  ///
  static dbCCSeg* create(dbCapNode* nodeA,
                         dbCapNode* nodeB,
                         bool mergeParallel = false);

  ///
  /// Destroy a cc-segment.
  ///
  static void destroy(dbCCSeg* seg);

  ///
  /// simple destroy disconnected cc-segment
  ///
  static void destroyS(dbCCSeg* seg);

  ///
  /// Destroy a cc-segment.
  ///
  static dbSet<dbCCSeg>::iterator destroy(dbSet<dbCCSeg>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbCCSeg* getCCSeg(dbBlock* block, uint32_t oid);

  ///
  /// disconnect a cc-segment
  ///
  static void disconnect(dbCCSeg* tcc_);

  ///
  /// connect a cc-segment
  ///
  static void connect(dbCCSeg* tcc_);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A Row is the element that represents placement sites.
///
///////////////////////////////////////////////////////////////////////////////
class dbRow : public dbObject
{
 public:
  ///
  /// Get the row name.
  ///
  std::string getName();

  ///
  /// Get the row name.
  ///
  const char* getConstName();

  ///
  /// Get the row site.
  ///
  dbSite* getSite();

  ///
  /// Get the origin of this row
  ///
  Point getOrigin();

  ///
  /// Get the site-orientation of this row
  ///
  dbOrientType getOrient();

  ///
  /// Get the direction of this row
  ///
  dbRowDir getDirection();

  ///
  /// Get the number of sites in this row.
  ///
  int getSiteCount();

  ///
  /// Get the spacing between sites. The spacing is measured from the
  /// origin of each site.
  ///
  int getSpacing();

  ///
  /// Get the bounding box of this row
  ///
  Rect getBBox();

  ///
  /// Get the block this row belongs too.
  ///
  dbBlock* getBlock();

  ///
  /// Create a new row.
  ///
  static dbRow* create(dbBlock* block,
                       const char* name,
                       dbSite* site,
                       int origin_x,
                       int origin_y,
                       dbOrientType orient,
                       dbRowDir direction,
                       int num_sites,
                       int spacing);

  ///
  /// Destroy a row.
  ///
  static void destroy(dbRow* row);

  ///
  /// Destroy a row.
  ///
  static dbSet<dbRow>::iterator destroy(dbSet<dbRow>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbRow* getRow(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A fill is the element that one metal fill shape
///
///////////////////////////////////////////////////////////////////////////////
class dbFill : public dbObject
{
 public:
  ///
  /// Get the fill bounding box.
  ///
  void getRect(Rect& rect);

  ///
  /// Returns true if this fill requires OPC (Optical proximity correction)
  ///
  bool needsOPC();

  ///
  /// Which mask is used for double or triple patterning.  Zero is returned for
  /// unassigned.  Values are typically in [1,3].
  ///
  uint32_t maskNumber();

  ///
  /// Get the layer of this fill.
  ///
  dbTechLayer* getTechLayer();

  ///
  /// Create a new fill.
  ///
  static dbFill* create(dbBlock* block,
                        bool needs_opc,
                        uint32_t mask_number,
                        dbTechLayer* layer,
                        int x1,
                        int y1,
                        int x2,
                        int y2);

  ///
  /// Destroy a fill.
  ///
  static void destroy(dbFill* fill);

  ///
  /// Destroy fills.
  ///
  static dbSet<dbFill>::iterator destroy(dbSet<dbFill>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbFill* getFill(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A Region is the element that represents a placement region.
///
///////////////////////////////////////////////////////////////////////////////
class dbRegion : public dbObject
{
 public:
  ///
  /// Get the region name.
  ///
  std::string getName();

  ///
  /// Get the region type.
  ///
  dbRegionType getRegionType();

  ///
  /// Set the region type.
  ///
  void setRegionType(dbRegionType type);

  ///
  /// Get the instances of this region.
  ///
  dbSet<dbInst> getRegionInsts();

  ///
  // Set the value of the invalid flag.
  ///
  void setInvalid(bool v);

  ///
  /// Returns true if the invalid flag is set.
  ///
  bool isInvalid();

  ///
  /// Get the boundaries of this region.
  /// A region may have no boundaries. In this case, you may have to check the
  /// parents of this region. This case can occur when reading DEF GROUPS and
  /// REGIONS. The result is two levels of hierarchy with the boundaries on the
  /// parent.
  ///
  dbSet<dbBox> getBoundaries();

  ///
  /// Add this instance to the region
  ///
  void addInst(dbInst* inst);

  ///
  /// Remove this instance from the region
  ///
  void removeInst(dbInst* inst);

  ///
  /// Remove this group from the region
  ///
  void removeGroup(dbGroup* group);

  ///
  /// Add group to this region.
  ///
  void addGroup(dbGroup* group);

  ///
  /// Get the groups of this region.
  ///
  dbSet<dbGroup> getGroups();

  ///
  /// Get the block of this region
  ///
  dbBlock* getBlock();

  ///
  /// Create a new region. Returns nullptr if a region with this name already
  /// exists in the block.
  ///
  static dbRegion* create(dbBlock* block, const char* name);

  ///
  /// Destroy a region.
  ///
  static void destroy(dbRegion* region);

  ///
  /// Destroy a region.
  ///
  static dbSet<dbRegion>::iterator destroy(dbSet<dbRegion>::iterator& itr);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbRegion* getRegion(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A Library is the element that represents a collection of library-cells,
/// called Masters.
///
///////////////////////////////////////////////////////////////////////////////
class dbLib : public dbObject
{
 public:
  ///
  /// Get the library name.
  ///
  std::string getName();

  ///
  /// Get the library name.
  ///
  const char* getConstName();

  ///
  /// Get the Database units per micron.
  ///
  int getDbUnitsPerMicron();

  ///
  /// Get the technology of this library
  ///
  dbTech* getTech();

  ///
  /// Get the master-cells of this library
  ///
  dbSet<dbMaster> getMasters();

  ///
  /// Finds a specific master-cell in the library
  /// Returns nullptr if the object was not found.
  ///
  dbMaster* findMaster(const char* name);

  ///
  /// Get the sites of this library
  ///
  dbSet<dbSite> getSites();

  ///
  /// Finds a specific site in the library
  /// Returns nullptr if the object was not found.
  ///
  dbSite* findSite(const char* name);

  ///
  /// Get the LEF units of this technology.
  ///
  int getLefUnits();

  ///
  /// Set the LEF units of this technology.
  ///
  void setLefUnits(int units);

  ///
  /// Get the HierarchyDelimiter.
  /// Returns (0) if the delimiter was not set.
  /// A hierarchy delimiter can only be set at the time
  /// a library is created.
  ///
  char getHierarchyDelimiter() const;

  ///
  /// Set the Bus name delimiters
  ///
  void setBusDelimiters(char left, char right);

  ///
  /// Get the Bus name delimiters
  /// Left and Right are set to "zero" if the bus delimiters
  /// were not set.
  ///
  void getBusDelimiters(char& left, char& right);

  ///
  /// Create a new library.
  ///
  static dbLib* create(dbDatabase* db,
                       const char* name,
                       dbTech* tech,
                       char hierarchy_delimiter = '/');

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbLib* getLib(dbDatabase* db, uint32_t oid);

  ///
  /// Destroy a library.
  ///
  static void destroy(dbLib* lib);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A Site is the element that represents a placement site for cells in this
/// library.
///
///////////////////////////////////////////////////////////////////////////////
class dbSite : public dbObject
{
 public:
  struct OrientedSite
  {
    dbSite* site;
    dbOrientType orientation;
    friend bool operator==(const OrientedSite& lhs, const OrientedSite& rhs);
    friend bool operator!=(const OrientedSite& lhs, const OrientedSite& rhs);
  };
  using RowPattern = std::vector<OrientedSite>;

  ///
  /// Get the site name.
  ///
  std::string getName() const;

  ///
  /// Get the site name.
  ///
  const char* getConstName();

  ///
  /// Get the width of this site
  ///
  int getWidth();

  ///
  /// Set the width of this site
  ///
  void setWidth(int width);

  ///
  /// Get the height of this site
  ///
  int getHeight() const;

  ///
  /// Set the height of this site
  ///
  void setHeight(int height);

  ///
  /// Get the class of this site.
  ///
  dbSiteClass getClass();

  ///
  /// Set the class of this site
  ///
  void setClass(dbSiteClass site_class);

  ///
  /// Mark that this site has X-Symmetry
  ///
  void setSymmetryX();

  ///
  /// Returns true if this site has X-Symmetry
  ///
  bool getSymmetryX();

  ///
  /// Mark that this site has Y-Symmetry
  ///
  void setSymmetryY();

  ///
  /// Returns true if this site has Y-Symmetry
  ///
  bool getSymmetryY();

  ///
  /// Mark that this site has R90-Symmetry
  ///
  void setSymmetryR90();

  ///
  /// Returns true if this site has R90-Symmetry
  ///
  bool getSymmetryR90();

  ///
  /// set the row pattern of this site
  ///
  void setRowPattern(const RowPattern& row_pattern);

  ///
  /// Returns true if the row pattern is not empty
  ///
  bool hasRowPattern() const;

  ///
  /// Is this site in a row pattern or does it have a row pattern
  ///
  bool isHybrid() const;

  ///
  /// returns the row pattern if available
  ///
  RowPattern getRowPattern();

  ///
  /// Get the library of this site.
  ///
  dbLib* getLib();

  ///
  /// Create a new site.
  /// Returns nullptr if a site with this name already exists
  ///
  static dbSite* create(dbLib* lib, const char* name);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbSite* getSite(dbLib* lib, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A Master is the element that represents a master-cell from the library.
///
///////////////////////////////////////////////////////////////////////////////
class dbMaster : public dbObject
{
 public:
  ///
  /// Get the master cell name.
  ///
  std::string getName() const;

  ///
  /// Get the master cell name.
  ///
  const char* getConstName() const;

  ///
  /// Get the x,y origin of this master
  ///
  Point getOrigin();

  ///
  /// Set the x,y origin of this master, default is (0,0)
  ///
  void setOrigin(int x, int y);

  ///
  /// Get the width of this master cell.
  ///
  uint32_t getWidth() const;

  ///
  /// Set the width of this master cell.
  ///
  void setWidth(uint32_t width);

  ///
  /// Get the height of this master cell.
  ///
  uint32_t getHeight() const;

  ///
  /// Set the height of this master cell.
  ///
  void setHeight(uint32_t height);

  ///
  /// Get the area of this master cell.
  ///
  int64_t getArea() const;

  ///
  /// is filler cell
  ///
  bool isFiller();

  ///
  /// Get the type of this master cell
  ///
  dbMasterType getType() const;

  ///
  /// Is the type BLOCK or any of its subtypes
  ///
  bool isBlock() const { return getType().isBlock(); }

  ///
  /// Is the type CORE or any of its subtypes
  ///
  bool isCore() const { return getType().isCore(); }

  ///
  /// Is the type PAD or any of its subtypes
  ///
  bool isPad() const { return getType().isPad(); }

  ///
  /// Is the type ENDCAP or any of its subtypes
  ///
  bool isEndCap() const { return getType().isEndCap(); }

  ///
  /// Is the master's type COVER or any of its subtypes
  ///
  bool isCover() const { return getType().isCover(); };

  ///
  /// This master can be placed automatically in the core.
  /// Pad, ring, cover, and none are false.
  ///
  bool isCoreAutoPlaceable();

  ///
  /// Set the type of this master cell
  ///
  void setType(dbMasterType type);

  ///
  /// Get the Logical equivalent of this master
  /// Returns nullptr if no equivalent was set.
  ///
  dbMaster* getLEQ();

  ///
  /// Set the Logical equivalent of this master
  /// NOTE: When setting the LEQ, the LEQ should be
  /// constructed to form a LEQ ring. The database
  /// does not enforce this. Typically, if the LEQ is
  /// set in the LEF file, than the LEQ's form a ring.
  ///
  void setLEQ(dbMaster* master);

  ///
  /// Get the Electical equivalent of this master
  /// Returns nullptr if no equivalent was set.
  ///
  dbMaster* getEEQ();

  ///
  /// Set the Electical equivalent of this master
  /// NOTE: When setting the EEQ, the EEQ should be
  /// constructed to form a EEQ ring. The database
  /// does not enforce this. Typically, if the EEQ is
  /// set in the LEF file, than the EEQ's form a ring.
  ///
  void setEEQ(dbMaster* master);

  ///
  /// Mark that this site has X-Symmetry
  ///
  void setSymmetryX();

  ///
  /// Returns true if this site has X-Symmetry
  ///
  bool getSymmetryX();

  ///
  /// Mark that this site has Y-Symmetry
  ///
  void setSymmetryY();

  ///
  /// Returns true if this site has Y-Symmetry
  ///
  bool getSymmetryY();

  ///
  /// Mark that this site has R90-Symmetry
  ///
  void setSymmetryR90();

  ///
  /// Returns true if this site has R90-Symmetry
  ///
  bool getSymmetryR90();

  ///
  /// Get the terminals of this master.
  ///
  dbSet<dbMTerm> getMTerms();

  ///
  /// Get the LEF58_EDGETYPE properties.
  ///
  dbSet<dbMasterEdgeType> getEdgeTypes();

  ///
  /// Find a specific master-terminal
  /// Returns nullptr if the object was not found.
  ///
  dbMTerm* findMTerm(const char* name);
  dbMTerm* findMTerm(dbBlock* block, const char* name);

  ///
  /// Get the library of this master.
  ///
  dbLib* getLib();

  ///
  /// Get the obstructions of this master
  ///
  dbSet<dbBox> getObstructions(bool include_decomposed_polygons = true);

  ///
  /// Get the polygon obstructions of this master
  ///
  dbSet<dbPolygon> getPolygonObstructions();

  ///
  /// Get the placement bounding box of this master.
  ///
  void getPlacementBoundary(Rect& r);

  ///
  /// Apply the suppiled transform to the master obsutrctions and pin
  /// geometries.
  ///
  void transform(dbTransform& t);

  ///
  /// Freeze this master. dbMTerms cannot be added or delete from the master
  /// once it is frozen.
  ///
  void setFrozen();

  ///
  /// Returns true if the master is frozen
  ///
  bool isFrozen();

  ///
  /// Set _sequential of this master.
  ///
  void setSequential(bool v);

  ///
  /// Returns _sequential this master
  ///
  bool isSequential();

  ///
  /// Set _mark of this master.
  ///
  void setMark(uint32_t mark);

  ///
  /// Returns _mark this master
  ///
  uint32_t isMarked();

  bool isSpecialPower();
  void setSpecialPower(bool v);

  ///
  /// Returns the number of mterms of this master.
  ///
  int getMTermCount();

  ///
  /// Set the site of this master.
  /// h
  void setSite(dbSite* site);

  ///
  /// Set the site of this master.
  /// Returns nullptr if no site has been set.
  ///
  dbSite* getSite();

  ///
  /// Returns a database unique id for this master.
  ///
  int getMasterId();

  ///
  /// Clear the access points of all pins.
  ///
  void clearPinAccess(int pin_access_idx);

  ///
  /// Create a new master.
  /// Returns nullptr if a master with this name already exists
  ///
  static dbMaster* create(dbLib* lib, const char* name);

  ///
  /// Destroy a dbMaster.
  ///
  static void destroy(dbMaster* master);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbMaster* getMaster(dbLib* lib, uint32_t oid);

  void* staCell();
  void staSetCell(void* cell);
};

class dbGDSLib : public dbObject
{
 public:
  void setLibname(std::string libname);

  std::string getLibname() const;

  void setUnits(double uu_per_dbu, double dbu_per_meter);

  std::pair<double, double> getUnits() const;

  dbGDSStructure* findGDSStructure(const char* name) const;

  dbSet<dbGDSStructure> getGDSStructures();

  static dbGDSLib* create(dbDatabase* db, const std::string& name);
  static void destroy(dbGDSLib* lib);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A MTerm is the element that represents a terminal on a Master.
///
///////////////////////////////////////////////////////////////////////////////
class dbMTerm : public dbObject
{
 public:
  ///
  /// Get the master term name.
  ///
  std::string getName();

  ///
  /// Get the master term name.
  ///
  const char* getConstName();

  ///
  /// Get the master term name. Change lib_bus_del to blk_bus_del if needed
  ///
  char* getName(dbInst* inst, char* ttname);
  char* getName(dbBlock* block, dbMaster* master, char* ttname);

  ///
  /// Get the signal type of this master-terminal.
  ///
  dbSigType getSigType();

  ///
  /// Set the signal type of this master-terminal.
  ///
  void setSigType(dbSigType type);

  ///
  /// Get the IO direction of this master-terminal.
  ///
  dbIoType getIoType();

  ///
  /// Get the shape of this master-terminal.
  ///
  dbMTermShapeType getShape();

  ///
  /// Set mark of this master-terminal.
  ///
  void setMark(uint32_t v);

  ///
  /// get mark of this master-terminal.
  ///
  bool isSetMark();

  ///
  /// Get the master this master-terminal belongs too.
  ///
  dbMaster* getMaster();

  ///
  /// Get the physical pins of this terminal.
  ///
  dbSet<dbMPin> getMPins();

  ///
  /// Get bbox of this term (ie the bbox of the getMPins())
  ///
  Rect getBBox();

  ///
  /// Add antenna info that is not specific to an oxide model.
  ///
  void addPartialMetalAreaEntry(double inval, dbTechLayer* refly = nullptr);
  void addPartialMetalSideAreaEntry(double inval, dbTechLayer* refly = nullptr);
  void addPartialCutAreaEntry(double inval, dbTechLayer* refly = nullptr);
  void addDiffAreaEntry(double inval, dbTechLayer* refly = nullptr);

  ///
  /// Antenna info that is specific to an oxide model.
  ///
  dbTechAntennaPinModel* createDefaultAntennaModel();
  dbTechAntennaPinModel* createOxide2AntennaModel();

  ///
  /// Access and write antenna rule models -- get functions will return nullptr
  /// if model not created.
  ///
  bool hasDefaultAntennaModel() const;
  bool hasOxide2AntennaModel() const;
  dbTechAntennaPinModel* getDefaultAntennaModel() const;
  dbTechAntennaPinModel* getOxide2AntennaModel() const;
  void writeAntennaLef(lefout& writer) const;

  // From LEF's ANTENNADIFFAREA on the MACRO's PIN
  void getDiffArea(std::vector<std::pair<double, dbTechLayer*>>& data);

  void* staPort();
  void staSetPort(void* port);

  ///
  /// Return the index of this mterm on this master.
  /// PREQ: master must be frozen.
  ///
  int getIndex();

  ///
  /// Create a new master terminal.
  /// Returns nullptr if a master terminal with this name already exists
  ///
  static dbMTerm* create(dbMaster* master,
                         const char* name,
                         dbIoType io_type = dbIoType(),
                         dbSigType sig_type = dbSigType(),
                         dbMTermShapeType shape_type = dbMTermShapeType());

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbMTerm* getMTerm(dbMaster* master, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A MPin is the element that represents a physical pin on a master-terminal.
///
///////////////////////////////////////////////////////////////////////////////
class dbMPin : public dbObject
{
 public:
  ///
  /// Get the master-terminal this pin belongs too.
  ///
  dbMTerm* getMTerm();

  ///
  /// Get the master this pin belongs too.
  ///
  dbMaster* getMaster();

  ///
  /// Get the geometry of this pin.
  ///
  dbSet<dbBox> getGeometry(bool include_decomposed_polygons = true);

  ///
  /// Get the polygon geometry of this pin.
  ///
  dbSet<dbPolygon> getPolygonGeometry();

  ///
  /// Get bbox of this pin (ie the bbox of getGeometry())
  ///
  Rect getBBox();

  std::vector<std::vector<odb::dbAccessPoint*>> getPinAccess() const;

  void clearPinAccess(int pin_access_idx);

  ///
  /// Create a new physical pin.
  ///
  static dbMPin* create(dbMTerm* mterm);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbMPin* getMPin(dbMaster* master, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A Tech is the element that represents technology specific data.
///
///////////////////////////////////////////////////////////////////////////////
class dbTech : public dbObject
{
 public:
  ///
  /// Get the tech name.
  ///
  std::string getName();

  ///
  /// Get the Database units per micron.
  ///
  int getDbUnitsPerMicron();

  ///
  /// Get the technolgy layers. The layers are ordered from the
  /// bottom mask number to the top mask number.
  ///
  dbSet<dbTechLayer> getLayers();

  ///
  /// Find the technology layer.
  /// Returns nullptr if the object was not found.
  ///
  dbTechLayer* findLayer(const char* name);

  ///
  /// Find the technology layer.
  /// Returns nullptr if the object was not found.
  ///
  dbTechLayer* findLayer(int layer_number);

  ///
  /// Find the technology routing layer.
  /// Returns nullptr if the object was not found.
  ///
  dbTechLayer* findRoutingLayer(int level_number);

  ///
  /// Get the technolgy vias. This includes non-default-rule-vias.
  ///
  dbSet<dbTechVia> getVias();

  ///
  /// Find the technology via.
  /// Returns nullptr if the object was not found.
  ///
  dbTechVia* findVia(const char* name);

  ///
  /// Get the LEF units of this technology.
  ///
  int getLefUnits();

  ///
  /// Set the LEF units of this technology.
  ///
  void setLefUnits(int units);

  ///
  /// Get the LEF version in this technology as a number or as a string.
  ///
  double getLefVersion() const;
  std::string getLefVersionStr() const;

  ///
  /// Set the LEF version of this technology, in both number and string form.
  ///
  void setLefVersion(double inver);

  ///
  ///  Get and set the NOWIREEXTENSIONATPIN construct
  ///
  bool hasNoWireExtAtPin() const;
  dbOnOffType getNoWireExtAtPin() const;
  void setNoWireExtAtPin(dbOnOffType intyp);

  ///
  ///  Get and set the NAMESCASESENSITIVE construct
  ///
  dbOnOffType getNamesCaseSensitive() const;
  void setNamesCaseSensitive(dbOnOffType intyp);

  ///
  /// Handle LEF CLEARANCEMEASURE construct
  ///
  bool hasClearanceMeasure() const;
  dbClMeasureType getClearanceMeasure() const;
  void setClearanceMeasure(dbClMeasureType inmeas);

  ///
  /// Handle LEF USEMINSPACING for pins and obstruction separately.
  ///
  bool hasUseMinSpacingObs() const;
  dbOnOffType getUseMinSpacingObs() const;
  void setUseMinSpacingObs(dbOnOffType inval);

  bool hasUseMinSpacingPin() const;
  dbOnOffType getUseMinSpacingPin() const;
  void setUseMinSpacingPin(dbOnOffType inval);

  ///
  ///  Handle MANUFACTURINGGRID construct
  ///  NOTE: Assumes conversion to internal DB units,
  ///  NOT microns or LEF/DEF units
  ///
  bool hasManufacturingGrid() const;
  int getManufacturingGrid() const;
  void setManufacturingGrid(int ingrd);

  ///
  /// Get the number of layers in this technology.
  ///
  int getLayerCount();

  ///
  /// Get the number of routing-layers in this technology.
  ///
  int getRoutingLayerCount();

  ///
  /// Get the number of vias in this technolgy.
  ///
  int getViaCount();

  ///
  /// Get the non-default rules
  ///
  dbSet<dbTechNonDefaultRule> getNonDefaultRules();

  ///
  /// Find a specific rule
  ///
  dbTechNonDefaultRule* findNonDefaultRule(const char* rulename);

  ///
  /// Find a specific rule
  /// Returns nullptr if no rule exists.
  ///
  dbTechSameNetRule* findSameNetRule(dbTechLayer* l1, dbTechLayer* l2);

  ///
  /// Get the same-net rules of this non-default rule.
  ///
  void getSameNetRules(std::vector<dbTechSameNetRule*>& rules);

  ///
  ///
  ///
  dbSet<dbTechViaRule> getViaRules();

  ///
  ///
  ///
  dbSet<dbTechViaGenerateRule> getViaGenerateRules();

  ///
  ///
  ///
  dbSet<dbMetalWidthViaMap> getMetalWidthViaMap();

  ///
  /// Get the LEF58_CELLEDGESPACINGTABLE
  ///
  dbSet<dbCellEdgeSpacing> getCellEdgeSpacingTable();

  ///
  ///
  ///
  dbTechViaRule* findViaRule(const char* name);

  ///
  ///
  ///
  dbTechViaGenerateRule* findViaGenerateRule(const char* name);

  ///
  ///
  ///
  void checkLayer(bool typeChk, bool widthChk, bool pitchChk, bool spacingChk);

  ///
  /// Create a new technology.
  /// Returns nullptr if a database technology already exists
  ///
  static dbTech* create(dbDatabase* db, const char* name);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTech* getTech(dbDatabase* db, uint32_t oid);

  ///
  /// Destroy a technology.
  /// TODO: Define what happens to the libs and the chip.
  ///
  static void destroy(dbTech* tech);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A TechVia is the element that represents a specific process VIA in
/// a technolgy.
///
///////////////////////////////////////////////////////////////////////////////
class dbTechVia : public dbObject
{
 public:
  ///
  /// Get the via name.
  ///
  std::string getName();

  ///
  /// Get the via name.
  ///
  const char* getConstName();

  ///
  /// Returns true if this via is a default
  ///
  bool isDefault();

  ///
  /// Set the default flag to true.
  ///
  void setDefault();

  ///
  /// Returns true if this via is a top-of-stack
  ///
  bool isTopOfStack();

  ///
  /// Set the top-of-stack flag to true.
  ///
  void setTopOfStack();

  ///
  /// Get the resitance per square nm
  ///
  double getResistance();

  ///
  /// Set the resitance per square nm
  ///
  void setResistance(double res);

  ///
  /// Set the pattern value of this via.
  /// The pattern is ignored if the pattern is already set on this via
  ///
  void setPattern(const char* pattern);

  ///
  /// Get the pattern value of this via.
  /// Returns and empty ("") string if a pattern has not been set.
  ///
  std::string getPattern();

  ///
  /// Set generate rule that was used to genreate this via.
  ///
  void setViaGenerateRule(dbTechViaGenerateRule* rule);

  ///
  /// Get the generate rule that was used to genreate this via.
  ///
  dbTechViaGenerateRule* getViaGenerateRule();

  ///
  /// Returns true if this via has params.
  ///
  bool hasParams();

  ///
  /// Set via params to generate this via. This method will create the shapes
  /// of this via. All previous shapes are destroyed.
  ///
  void setViaParams(const dbViaParams& params);

  ///
  /// Get the via params used to generate this via.
  ///
  dbViaParams getViaParams();

  ///
  /// Get the technology this via belongs too.
  ///
  dbTech* getTech();

  ///
  /// Get the bbox of this via.
  /// Returns nullptr if this via has no shapes.
  ///
  dbBox* getBBox();

  ///
  /// Get the boxes of this VIA
  ///
  dbSet<dbBox> getBoxes();

  ///
  /// Get the upper-most layer of this via reaches
  /// Returns nullptr if this via has no shapes.
  ///
  dbTechLayer* getTopLayer();

  ///
  /// Get the lower-most layer of this via reaches
  /// Returns nullptr if this via has no shapes.
  ///
  dbTechLayer* getBottomLayer();

  ///
  /// Returns the non-default rule this via belongs too.
  /// Returns nullptr if this via is not part of a non-default rule.
  ///
  dbTechNonDefaultRule* getNonDefaultRule();

  ///
  /// Create a new via.
  /// Returns nullptr if a via with this name already exists.
  ///
  static dbTechVia* create(dbTech* tech, const char* name);

  ///
  /// Create a new non-default-rule via.
  /// Returns nullptr if a via with this name already exists.
  ///
  static dbTechVia* create(dbTechNonDefaultRule* rule, const char* name);
  ///
  /// Create a new non-default-rule via by cloning an existing via (not
  /// necessarily from the same non-default rule
  /// Returns nullptr if a via with this name already exists.
  ///
  static dbTechVia* clone(dbTechNonDefaultRule* rule,
                          dbTechVia* invia_,
                          const char* new_name);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechVia* getTechVia(dbTech* tech, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A TechViaRule is the element that represents a LEF VIARULE
///
///////////////////////////////////////////////////////////////////////////////
class dbTechViaRule : public dbObject
{
 public:
  ///
  /// Get the via-rule name.
  ///
  std::string getName();

  ///
  /// Add this via to this rule
  ///
  void addVia(dbTechVia* via);

  ///
  /// Get the number of vias assigned to this rule
  ///
  uint32_t getViaCount();

  ///
  /// Return the via of this index. The index ranges from [0 ... (viaCount-1)]
  ///
  dbTechVia* getVia(uint32_t indx);

  ///
  /// Get the number of layer-rules assigned to this rule
  ///
  uint32_t getViaLayerRuleCount();

  ///
  /// Return the layer-rule of this index. The index ranges from [0 ...
  /// (viaCount-1)]
  ///
  dbTechViaLayerRule* getViaLayerRule(uint32_t indx);

  ///
  /// Create a new via.
  /// Returns nullptr if a via-rule with this name already exists.
  ///
  static dbTechViaRule* create(dbTech* tech, const char* name);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechViaRule* getTechViaRule(dbTech* tech, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A TechViaLayerRule is the element that represents a LEF VIARULE LAYER
///
///////////////////////////////////////////////////////////////////////////////
class dbTechViaLayerRule : public dbObject
{
 public:
  ///
  /// Get the layer
  ///
  dbTechLayer* getLayer();

  ///
  /// Get the rule direction
  ///
  dbTechLayerDir getDirection();

  ///
  /// Set the rule direction
  ///
  void setDirection(dbTechLayerDir dir);

  ///
  /// Returns true if width rule is set
  ///
  bool hasWidth();

  ///
  /// Returns the width rule
  ///
  void getWidth(int& minWidth, int& maxWidth);

  ///
  /// Set the width rule
  ///
  void setWidth(int minWidth, int maxWidth);

  ///
  /// Returns true if the enclosure rule is set.
  ///
  bool hasEnclosure();

  ///
  /// Returns the enclosure rule
  ///
  void getEnclosure(int& overhang1, int& overhang2);

  ///
  /// Set the enclosure rule
  ///
  void setEnclosure(int overhang1, int overhang2);

  ///
  /// Returns true if the overhang rule is set.
  ///
  bool hasOverhang();

  ///
  /// Returns the overhang rule
  ///
  int getOverhang();

  ///
  /// Set the overhang rule
  ///
  void setOverhang(int overhang);

  ///
  /// Returns true if the metal-overhang rule is set.
  ///
  bool hasMetalOverhang();

  ///
  /// Returns the overhang rule
  ///
  int getMetalOverhang();

  ///
  /// Set the overhang rule
  ///
  void setMetalOverhang(int overhang);

  ///
  /// returns true if the rect rule is set
  ///
  bool hasRect();

  ///
  /// Get the rect rule
  ///
  void getRect(Rect& r);

  ///
  /// Set the rect rule
  ///
  void setRect(const Rect& r);

  ///
  /// returns true if the spacing rule is set
  ///
  bool hasSpacing();

  ///
  /// Get the spacing rule.
  ///
  void getSpacing(int& x_spacing, int& y_spacing);

  ///
  /// Set the spacing rule.
  ///
  void setSpacing(int x_spacing, int y_spacing);

  ///
  /// Returns true if the resistance rule is set.
  ///
  bool hasResistance();

  ///
  /// Set the resistance
  ///
  void setResistance(double r);

  ///
  /// Get the resistance
  ///
  double getResistance();

  ///
  /// Create a new via-layer_rule.
  ///
  static dbTechViaLayerRule* create(dbTech* tech,
                                    dbTechViaRule* rule,
                                    dbTechLayer* layer);

  ///
  /// Create a new via-layer_rule.
  ///
  static dbTechViaLayerRule* create(dbTech* tech,
                                    dbTechViaGenerateRule* rule,
                                    dbTechLayer* layer);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechViaLayerRule* getTechViaLayerRule(dbTech* tech, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A TechViaGenerateRule is the element that represents a LEF VIARULE GENERATE
///
///////////////////////////////////////////////////////////////////////////////
class dbTechViaGenerateRule : public dbObject
{
 public:
  ///
  /// Get the via-rule name.
  ///
  std::string getName();

  ///
  /// Returns true if this is the default rule.
  ///
  bool isDefault();

  ///
  /// Get the number of layer-rules assigned to this rule
  ///
  uint32_t getViaLayerRuleCount();

  ///
  /// Return the layer-rule of this index. The index ranges from [0 ...
  /// (viaCount-1)]
  ///
  dbTechViaLayerRule* getViaLayerRule(uint32_t indx);

  ///
  /// Create a new via.
  /// Returns nullptr if a via-rule with this name already exists.
  ///
  static dbTechViaGenerateRule* create(dbTech* tech,
                                       const char* name,
                                       bool is_default);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechViaGenerateRule* getTechViaGenerateRule(dbTech* tech,
                                                       uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A TechLayerSpacingRule stores a design rule in LEF V5.4 format.
/// These are bound to layers -- a layer may have several design rules to
/// describe required spacing among different widths and parallel lengths.
///
///////////////////////////////////////////////////////////////////////////////

class dbTechLayerSpacingRule : public dbObject
{
 public:
  /// Combine data and predicates for elements of rule
  bool isUnconditional() const;
  uint32_t getSpacing() const;
  bool getLengthThreshold(uint32_t& threshold) const;
  bool getLengthThresholdRange(uint32_t& rmin, uint32_t& rmax) const;
  bool getRange(uint32_t& rmin, uint32_t& rmax) const;
  void setSpacingNotchLengthValid(bool val);
  void setSpacingEndOfNotchWidthValid(bool val);
  bool hasSpacingNotchLength() const;
  bool hasSpacingEndOfNotchWidth() const;
  bool hasRange() const;
  bool hasLengthThreshold() const;
  bool hasUseLengthThreshold() const;
  bool getInfluence(uint32_t& influence) const;
  bool getInfluenceRange(uint32_t& rmin, uint32_t& rmax) const;
  bool getRangeRange(uint32_t& rmin, uint32_t& rmax) const;
  bool getAdjacentCuts(uint32_t& numcuts,
                       uint32_t& within,
                       uint32_t& spacing,
                       bool& except_same_pgnet) const;
  bool getCutLayer4Spacing(dbTechLayer*& outly) const;
  bool getCutStacking() const;
  bool getCutCenterToCenter() const;
  bool getCutSameNet() const;
  bool getCutParallelOverlap() const;
  uint32_t getCutArea() const;
  void writeLef(lefout& writer) const;

  void setSameNetPgOnly(bool pgonly);
  bool getSameNetPgOnly();
  void setLengthThreshold(uint32_t threshold);
  void setSpacing(uint32_t spacing);
  void setLengthThresholdRange(uint32_t rmin, uint32_t rmax);
  void setRange(uint32_t rmin, uint32_t rmax);
  void setUseLengthThreshold();
  void setInfluence(uint32_t influence);
  void setInfluenceRange(uint32_t rmin, uint32_t rmax);
  void setRangeRange(uint32_t rmin, uint32_t rmax);
  void setAdjacentCuts(uint32_t numcuts,
                       uint32_t within,
                       uint32_t spacing,
                       bool except_same_pgnet);
  void setCutLayer4Spacing(dbTechLayer* cutly);
  void setCutStacking(bool stacking);
  void setCutCenterToCenter(bool c2c);
  void setCutSameNet(bool same_net);
  void setCutParallelOverlap(bool overlap);
  void setCutArea(uint32_t area);
  void setEol(uint32_t width,
              uint32_t within,
              bool parallelEdge,
              uint32_t parallelSpace,
              uint32_t parallelWithin,
              bool twoEdges);
  bool getEol(uint32_t& width,
              uint32_t& within,
              bool& parallelEdge,
              uint32_t& parallelSpace,
              uint32_t& parallelWithin,
              bool& twoEdges) const;

  ///
  /// Create a new layer spacing rule.
  /// Returns pointer to newly created object
  ///
  static dbTechLayerSpacingRule* create(dbTechLayer* inly);
  static dbTechLayerSpacingRule* getTechLayerSpacingRule(dbTechLayer* inly,
                                                         uint32_t dbid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbTechMinCutRule stores rules for minimum cuts
/// in LEF V5.5 format.
/// These are bound to layers -- a layer may have several minimum cut rules to
/// describe required cuts at intersections of  different widths and
/// protrusion lengths.
///
///////////////////////////////////////////////////////////////////////////////

class dbTechMinCutRule : public dbObject
{
 public:
  bool getMinimumCuts(uint32_t& numcuts, uint32_t& width) const;
  void setMinimumCuts(uint32_t numcuts,
                      uint32_t width,
                      bool above_only,
                      bool below_only);
  bool getCutDistance(uint32_t& cut_distance) const;
  void setCutDistance(uint32_t cut_distance);
  bool getLengthForCuts(uint32_t& length, uint32_t& distance) const;
  void setLengthForCuts(uint32_t length, uint32_t distance);
  bool isAboveOnly() const;
  bool isBelowOnly() const;
  void writeLef(lefout& writer) const;
  static dbTechMinCutRule* create(dbTechLayer* inly);
  static dbTechMinCutRule* getMinCutRule(dbTechLayer* inly, uint32_t dbid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbTechMinEncRule stores rules for minimum enclosure area
/// in LEF V5.5 format.
/// These are bound to layers -- a layer may have several minimum enclosure
/// rules to describe connections to wires of different widths
///
///////////////////////////////////////////////////////////////////////////////

class dbTechMinEncRule : public dbObject
{
 public:
  bool getEnclosure(uint32_t& area) const;
  void setEnclosure(uint32_t area);
  bool getEnclosureWidth(uint32_t& width) const;
  void setEnclosureWidth(uint32_t width);
  void writeLef(lefout& writer) const;

  static dbTechMinEncRule* create(dbTechLayer* inly);
  static dbTechMinEncRule* getMinEncRule(dbTechLayer* inly, uint32_t dbid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbTechV55InfluenceEntry stores an entry in the table for V5.5 format
/// influence spacing rules.
/// Influence spacing in V5.5 describes the required spacing (_spacing) for
/// any wire within a distance (_within) a wire of width (_width).
/// These are bound to layers.
///
///////////////////////////////////////////////////////////////////////////////

class dbTechV55InfluenceEntry : public dbObject
{
 public:
  bool getV55InfluenceEntry(uint32_t& width,
                            uint32_t& within,
                            uint32_t& spacing) const;
  void setV55InfluenceEntry(const uint32_t& width,
                            const uint32_t& within,
                            const uint32_t& spacing);
  void writeLef(lefout& writer) const;

  static dbTechV55InfluenceEntry* create(dbTechLayer* inly);
  static dbTechV55InfluenceEntry* getV55InfluenceEntry(dbTechLayer* inly,
                                                       uint32_t dbid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbTechLayerAntennaRule expresses a single antenna rule for a given layer.
///
///////////////////////////////////////////////////////////////////////////////

class dbTechLayerAntennaRule : public dbObject
{
 public:
  bool isValid() const;
  void writeLef(lefout& writer) const;

  void setGatePlusDiffFactor(double factor);
  void setAreaMinusDiffFactor(double factor);

  void setAreaFactor(double factor, bool diffuse = false);
  void setSideAreaFactor(double factor, bool diffuse = false);

  bool hasAreaFactor() const;
  bool hasSideAreaFactor() const;

  double getAreaFactor() const;
  double getSideAreaFactor() const;

  bool isAreaFactorDiffUseOnly() const;
  bool isSideAreaFactorDiffUseOnly() const;

  bool hasAntennaCumRoutingPlusCut() const;
  void setAntennaCumRoutingPlusCut(bool value = true);

  // If return value is 0 then the value is unset
  double getPAR() const;
  double getCAR() const;
  double getPSR() const;
  double getCSR() const;
  double getGatePlusDiffFactor() const;
  double getAreaMinusDiffFactor() const;

  void setPAR(double ratio);
  void setCAR(double ratio);
  void setPSR(double ratio);
  void setCSR(double ratio);

  // if indices.size()==0 then these are unset
  // if indices.size()==1 then this is a single value rather than a PWL
  struct pwl_pair
  {
    const std::vector<double>& indices;
    const std::vector<double>& ratios;
  };

  pwl_pair getDiffPAR() const;
  pwl_pair getDiffCAR() const;
  pwl_pair getDiffPSR() const;
  pwl_pair getDiffCSR() const;
  pwl_pair getAreaDiffReduce() const;

  // PWL
  void setDiffPAR(const std::vector<double>& diff_idx,
                  const std::vector<double>& ratios);
  void setDiffCAR(const std::vector<double>& diff_idx,
                  const std::vector<double>& ratios);
  void setDiffPSR(const std::vector<double>& diff_idx,
                  const std::vector<double>& ratios);
  void setDiffCSR(const std::vector<double>& diff_idx,
                  const std::vector<double>& ratios);

  // Single value
  void setDiffPAR(double ratio);
  void setDiffCAR(double ratio);
  void setDiffPSR(double ratio);
  void setDiffCSR(double ratio);

  void setAreaDiffReduce(const std::vector<double>& areas,
                         const std::vector<double>& factors);

  static dbTechLayerAntennaRule* getAntennaRule(dbTech* inly, uint32_t dbid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A dbTechAntennaPinModel contains model specific antenna info for a pin
///
///////////////////////////////////////////////////////////////////////////////

class dbTechAntennaPinModel : public dbObject
{
 public:
  void addGateAreaEntry(double inval, dbTechLayer* refly = nullptr);
  void addMaxAreaCAREntry(double inval, dbTechLayer* refly = nullptr);
  void addMaxSideAreaCAREntry(double inval, dbTechLayer* refly = nullptr);
  void addMaxCutCAREntry(double inval, dbTechLayer* refly = nullptr);

  void getGateArea(std::vector<std::pair<double, dbTechLayer*>>& data);
  void getMaxAreaCAR(std::vector<std::pair<double, dbTechLayer*>>& data);
  void getMaxSideAreaCAR(std::vector<std::pair<double, dbTechLayer*>>& data);
  void getMaxCutCAR(std::vector<std::pair<double, dbTechLayer*>>& data);

  void writeLef(dbTech* tech, lefout& writer) const;

  static dbTechAntennaPinModel* getAntennaPinModel(dbMaster* master,
                                                   uint32_t dbid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A NonDefaultRule is the element that represents a Non-default technology
/// rule.
///
///////////////////////////////////////////////////////////////////////////////
class dbTechNonDefaultRule : public dbObject
{
 public:
  ///
  /// Get the rule name.
  ///
  std::string getName();

  ///
  /// Get the rule name.
  ///
  const char* getConstName();

  ///
  /// Returns true if this rule is a block rule
  ///
  bool isBlockRule();

  ///
  /// Find a specific layer-rule.
  /// Returns nullptr if there is no layer-rule.
  ///
  dbTechLayerRule* getLayerRule(dbTechLayer* layer);

  ///
  /// Get the layer-rules of this non-default rule.
  ///
  void getLayerRules(std::vector<dbTechLayerRule*>& layer_rules);

  ///
  /// Get the vias of this non-default rule.
  ///
  void getVias(std::vector<dbTechVia*>& vias);

  ///
  /// Find a specific rule
  /// Returns nullptr if no rule exists.
  ///
  dbTechSameNetRule* findSameNetRule(dbTechLayer* l1, dbTechLayer* l2);

  ///
  /// Get the same-net rules of this non-default rule.
  ///
  void getSameNetRules(std::vector<dbTechSameNetRule*>& rules);

  /////////////////////////
  // 5.6 DEF additions
  /////////////////////////

  ///
  /// Set the hard spacing rule.
  ///
  bool getHardSpacing();

  ///
  /// Get the hard spacing rule.
  ///
  void setHardSpacing(bool value);

  ///
  ///  Add a use via.
  ///
  void addUseVia(dbTechVia* via);

  ///
  ///  Get vias to use.
  ///
  void getUseVias(std::vector<dbTechVia*>& vias);

  ///
  ///  Add a use via.
  ///
  void addUseViaRule(dbTechViaGenerateRule* rule);

  ///
  ///  Get vias to use.
  ///
  void getUseViaRules(std::vector<dbTechViaGenerateRule*>& rules);

  ///
  /// Assign a minimum number of cuts to this cut-layer
  ///
  void setMinCuts(dbTechLayer* cut_layer, int count);

  ///
  /// Get the minimum number of cuts for this cut-layer.
  ///
  /// Returns false if a value has not been specified.
  ///
  bool getMinCuts(dbTechLayer* cut_layer, int& count);

  ///
  /// Create a new non-default-rule.
  /// Returns nullptr if a non-default-rule with this name already exists
  ///
  static dbTechNonDefaultRule* create(dbTech* tech, const char* name);

  ///
  /// Create a new non-default-rule.
  /// Returns nullptr if a non-default-rule with this name already exists
  ///
  static dbTechNonDefaultRule* create(dbBlock* block, const char* name);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechNonDefaultRule* getTechNonDefaultRule(dbTech* tech,
                                                     uint32_t oid);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechNonDefaultRule* getTechNonDefaultRule(dbBlock* block,
                                                     uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A TechLayerRule is the element that represents a non-default layer
/// rule.
///
///////////////////////////////////////////////////////////////////////////////
class dbTechLayerRule : public dbObject
{
 public:
  ///
  /// Get the layer this rule represents
  ///
  dbTechLayer* getLayer();

  ///
  /// Returns true if this rule is a block rule
  ///
  bool isBlockRule();

  ///
  /// Get the non-default-rule this layer-rule belongs too.
  ///
  dbTechNonDefaultRule* getNonDefaultRule();

  ///
  /// Get the minimum path-width.
  ///
  int getWidth();

  ///
  /// Set the minimum path-width.
  ///
  void setWidth(int width);

  ///
  /// Get the minimum object-to-object spacing.
  ///
  int getSpacing();

  ///
  /// Set the minimum object-to-object spacing.
  ///
  void setSpacing(int spacing);

  ///
  /// Get the resitance per square nm
  ///
  double getResistance();

  ///
  /// Set the resitance per square nm
  ///
  void setResistance(double res);

  ///
  /// Get the capacitance per square nm
  ///
  double getCapacitance();

  ///
  /// Set the capacitance per square nm
  ///
  void setCapacitance(double cap);

  ///
  /// Get the edge capacitance
  ///
  double getEdgeCapacitance();

  ///
  /// Set the edge capacitance
  ///
  void setEdgeCapacitance(double cap);

  ///
  /// Get the edge capacitance
  ///
  uint32_t getWireExtension();

  ///
  /// Set the edge capacitance
  ///
  void setWireExtension(uint32_t ext);

  ///
  /// Create a new layer-rule.
  /// Returns nullptr if a layer-rule for this layer already exists.
  ///
  static dbTechLayerRule* create(dbTechNonDefaultRule* rule,
                                 dbTechLayer* layer);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechLayerRule* getTechLayerRule(dbTech* tech, uint32_t oid);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechLayerRule* getTechLayerRule(dbBlock* block, uint32_t oid);
};

///////////////////////////////////////////////////////////////////////////////
///
/// A TechSameNetRule
///
///////////////////////////////////////////////////////////////////////////////
class dbTechSameNetRule : public dbObject
{
 public:
  ///
  /// Get the layer this rule represents
  ///
  dbTechLayer* getLayer1();

  ///
  /// Get the layer this rule represents
  ///
  dbTechLayer* getLayer2();

  ///
  /// Get the minimum net-to-net spacing.
  ///
  int getSpacing();

  ///
  /// Set the minimum net-to-net spacing.
  ///
  void setSpacing(int spacing);

  ///
  /// Set the flag to allow stacked vias, the default value is false.
  ///
  void setAllowStackedVias(bool value);

  ///
  /// Get the allow stacked vias flag.
  ///
  bool getAllowStackedVias();

  ///
  /// Create a new default samenet rule.
  /// Returns nullptr if a rule already exists between these layers.
  ///
  static dbTechSameNetRule* create(dbTechLayer* layer1, dbTechLayer* layer2);
  ///
  /// Create a new non-default samenet rule.
  /// Returns nullptr if a rule already exists between these layers.
  ///
  static dbTechSameNetRule* create(dbTechNonDefaultRule* rule,
                                   dbTechLayer* layer1,
                                   dbTechLayer* layer2);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechSameNetRule* getTechSameNetRule(dbTech* tech, uint32_t oid);
};

class dbViaParams : private _dbViaParams
{
 public:
  dbViaParams();
  dbViaParams(const dbViaParams& p) = default;

  int getXCutSize() const;
  int getYCutSize() const;
  int getXCutSpacing() const;
  int getYCutSpacing() const;
  int getXTopEnclosure() const;
  int getYTopEnclosure() const;
  int getXBottomEnclosure() const;
  int getYBottomEnclosure() const;
  int getNumCutRows() const;
  int getNumCutCols() const;
  int getXOrigin() const;
  int getYOrigin() const;
  int getXTopOffset() const;
  int getYTopOffset() const;
  int getXBottomOffset() const;
  int getYBottomOffset() const;
  dbTechLayer* getTopLayer() const;
  dbTechLayer* getCutLayer() const;
  dbTechLayer* getBottomLayer() const;

  void setXCutSize(int value);
  void setYCutSize(int value);
  void setXCutSpacing(int value);
  void setYCutSpacing(int value);
  void setXTopEnclosure(int value);
  void setYTopEnclosure(int value);
  void setXBottomEnclosure(int value);
  void setYBottomEnclosure(int value);
  void setNumCutRows(int value);
  void setNumCutCols(int value);
  void setXOrigin(int value);
  void setYOrigin(int value);
  void setXTopOffset(int value);
  void setYTopOffset(int value);
  void setXBottomOffset(int value);
  void setYBottomOffset(int value);
  void setTopLayer(dbTechLayer* layer);
  void setCutLayer(dbTechLayer* layer);
  void setBottomLayer(dbTechLayer* layer);

 private:
  dbViaParams(const _dbViaParams& p);

  dbTech* _tech;

  friend class dbVia;
  friend class dbTechVia;
};

// Generator Code Begin ClassDefinition

class dbAccessPoint : public dbObject
{
 public:
  void setPoint(Point point);

  Point getPoint() const;

  void setLayer(dbTechLayer* layer);

  // User Code Begin dbAccessPoint
  void setAccesses(const std::vector<dbDirection>& accesses);

  void getAccesses(std::vector<dbDirection>& tbl) const;

  void setLowType(dbAccessType type_low);

  dbAccessType getLowType() const;

  void setHighType(dbAccessType type_high);

  dbAccessType getHighType() const;

  void setAccess(bool access, dbDirection dir);

  bool hasAccess(dbDirection dir = dbDirection::NONE)
      const;  // NONE refers to access in any direction

  dbTechLayer* getLayer() const;

  dbMPin* getMPin() const;

  dbBPin* getBPin() const;

  std::vector<std::vector<dbObject*>> getVias() const;

  void addTechVia(int num_cuts, dbTechVia* via);

  void addBlockVia(int num_cuts, dbVia* via);

  void addSegment(const Rect& segment,
                  const bool& begin_style_trunc,
                  const bool& end_style_trunc);

  const std::vector<std::tuple<Rect, bool, bool>>& getSegments() const;

  static dbAccessPoint* create(dbBlock* block,
                               dbMPin* pin,
                               uint32_t pin_access_idx);

  static dbAccessPoint* create(dbBPin*);

  static dbAccessPoint* getAccessPoint(dbBlock* block, uint32_t dbid);

  static void destroy(dbAccessPoint* ap);
  // User Code End dbAccessPoint
};

class dbBusPort : public dbObject
{
 public:
  int getFrom() const;

  int getTo() const;

  dbModBTerm* getPort() const;

  void setMembers(dbModBTerm* members);

  dbModBTerm* getMembers() const;

  void setLast(dbModBTerm* last);

  dbModBTerm* getLast() const;

  dbModule* getParent() const;

  // User Code Begin dbBusPort
  // get element by bit index in bus (allows for up/down)
  // linear access
  dbModBTerm* getBusIndexedElement(int index);
  dbSet<dbModBTerm> getBusPortMembers();
  int getSize() const;
  bool getUpdown() const;

  static dbBusPort* create(dbModule* parentModule,
                           dbModBTerm* port,
                           int from_ix,
                           int to_ix);

  // User Code End dbBusPort
};

class dbCellEdgeSpacing : public dbObject
{
 public:
  void setFirstEdgeType(const std::string& first_edge_type);

  std::string getFirstEdgeType() const;

  void setSecondEdgeType(const std::string& second_edge_type);

  std::string getSecondEdgeType() const;

  void setSpacing(int spacing);

  int getSpacing() const;

  void setExceptAbutted(bool except_abutted);

  bool isExceptAbutted() const;

  void setExceptNonFillerInBetween(bool except_non_filler_in_between);

  bool isExceptNonFillerInBetween() const;

  void setOptional(bool optional);

  bool isOptional() const;

  void setSoft(bool soft);

  bool isSoft() const;

  void setExact(bool exact);

  bool isExact() const;

  // User Code Begin dbCellEdgeSpacing

  static dbCellEdgeSpacing* create(dbTech*);

  static void destroy(dbCellEdgeSpacing*);

  // User Code End dbCellEdgeSpacing
};

class dbChip : public dbObject
{
 public:
  enum class ChipType
  {
    DIE,
    RDL,
    IP,
    SUBSTRATE,
    HIER
  };

  const char* getName() const;

  void setOffset(Point offset);

  Point getOffset() const;

  void setWidth(int width);

  int getWidth() const;

  void setHeight(int height);

  int getHeight() const;

  void setThickness(int thickness);

  int getThickness() const;

  void setShrink(float shrink);

  float getShrink() const;

  void setSealRingEast(int seal_ring_east);

  int getSealRingEast() const;

  void setSealRingWest(int seal_ring_west);

  int getSealRingWest() const;

  void setSealRingNorth(int seal_ring_north);

  int getSealRingNorth() const;

  void setSealRingSouth(int seal_ring_south);

  int getSealRingSouth() const;

  void setScribeLineEast(int scribe_line_east);

  int getScribeLineEast() const;

  void setScribeLineWest(int scribe_line_west);

  int getScribeLineWest() const;

  void setScribeLineNorth(int scribe_line_north);

  int getScribeLineNorth() const;

  void setScribeLineSouth(int scribe_line_south);

  int getScribeLineSouth() const;

  void setTsv(bool tsv);

  bool isTsv() const;

  dbSet<dbChipRegion> getChipRegions() const;

  dbSet<dbMarkerCategory> getMarkerCategories() const;

  // User Code Begin dbChip

  ChipType getChipType() const;
  ///
  /// Get the top-block of this chip.
  /// Returns nullptr if a top-block has NOT been created.
  ///
  dbBlock* getBlock();

  dbSet<dbChipInst> getChipInsts() const;

  dbSet<dbChipConn> getChipConns() const;

  dbSet<dbChipNet> getChipNets() const;

  dbChipInst* findChipInst(const std::string& name) const;

  dbChipRegion* findChipRegion(const std::string& name) const;

  dbTech* getTech() const;

  Rect getBBox() const;

  Cuboid getCuboid() const;

  dbMarkerCategory* findMarkerCategory(const char* name) const;

  ///
  /// Create a new chip.
  /// Returns nullptr if there is no database technology.
  ///
  static dbChip* create(dbDatabase* db,
                        dbTech* tech,
                        const std::string& name = "",
                        ChipType type = ChipType::DIE);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbChip* getChip(dbDatabase* db, uint32_t oid);

  ///
  /// Destroy a chip.
  ///
  static void destroy(dbChip* chip);
  // User Code End dbChip
};

class dbChipBump : public dbObject
{
 public:
  // User Code Begin dbChipBump
  dbChip* getChip() const;

  dbChipRegion* getChipRegion() const;

  dbInst* getInst() const;

  dbNet* getNet() const;

  dbBTerm* getBTerm() const;

  void setNet(dbNet* net);

  void setBTerm(dbBTerm* bterm);

  static dbChipBump* create(dbChipRegion* chip_region, dbInst* inst);

  // User Code End dbChipBump
};

class dbChipBumpInst : public dbObject
{
 public:
  // User Code Begin dbChipBumpInst

  dbChipBump* getChipBump() const;

  dbChipRegionInst* getChipRegionInst() const;

  // User Code End dbChipBumpInst
};

class dbChipConn : public dbObject
{
 public:
  std::string getName() const;

  void setThickness(int thickness);

  int getThickness() const;

  // User Code Begin dbChipConn

  dbChip* getParentChip() const;

  dbChipRegionInst* getTopRegion() const;

  dbChipRegionInst* getBottomRegion() const;

  std::vector<dbChipInst*> getTopRegionPath() const;

  std::vector<dbChipInst*> getBottomRegionPath() const;

  static dbChipConn* create(const std::string& name,
                            dbChip* parent_chip,
                            const std::vector<dbChipInst*>& top_region_path,
                            dbChipRegionInst* top_region,
                            const std::vector<dbChipInst*>& bottom_region_path,
                            dbChipRegionInst* bottom_region);

  static void destroy(dbChipConn* chipConn);
  // User Code End dbChipConn
};

class dbChipInst : public dbObject
{
 public:
  std::string getName() const;

  dbOrientType3D getOrient() const;

  dbChip* getMasterChip() const;

  dbChip* getParentChip() const;

  // User Code Begin dbChipInst

  dbTransform getTransform() const;

  void setOrient(dbOrientType3D orient);

  void setLoc(const Point3D& loc);

  Point3D getLoc() const;

  Rect getBBox() const;

  Cuboid getCuboid() const;

  dbSet<dbChipRegionInst> getRegions() const;

  dbChipRegionInst* findChipRegionInst(dbChipRegion* chip_region) const;

  dbChipRegionInst* findChipRegionInst(const std::string& name) const;

  static odb::dbChipInst* create(dbChip* parent_chip,
                                 dbChip* master_chip,
                                 const std::string& name);

  static void destroy(dbChipInst* chipInst);
  // User Code End dbChipInst
};

class dbChipNet : public dbObject
{
 public:
  std::string getName() const;

  // User Code Begin dbChipNet
  dbChip* getChip() const;

  uint32_t getNumBumpInsts() const;

  dbChipBumpInst* getBumpInst(uint32_t index,
                              std::vector<dbChipInst*>& path) const;

  void addBumpInst(dbChipBumpInst* bump_inst,
                   const std::vector<dbChipInst*>& path);

  static dbChipNet* create(dbChip* chip, const std::string& name);

  static void destroy(dbChipNet* net);
  // User Code End dbChipNet
};

class dbChipRegion : public dbObject
{
 public:
  enum class Side
  {
    FRONT,
    BACK,
    INTERNAL,
    INTERNAL_EXT
  };

  std::string getName() const;

  void setBox(const Rect& box);

  Rect getBox() const;

  dbSet<dbChipBump> getChipBumps() const;

  // User Code Begin dbChipRegion
  Cuboid getCuboid() const;

  dbChip* getChip() const;

  Side getSide() const;

  dbTechLayer* getLayer() const;

  static dbChipRegion* create(dbChip* chip,
                              const std::string& name,
                              Side side,
                              dbTechLayer* layer);

  // User Code End dbChipRegion
};

class dbChipRegionInst : public dbObject
{
 public:
  // User Code Begin dbChipRegionInst
  Cuboid getCuboid() const;

  dbChipInst* getChipInst() const;

  dbChipRegion* getChipRegion() const;

  dbSet<dbChipBumpInst> getChipBumpInsts() const;

  // User Code End dbChipRegionInst
};

class dbDatabase : public dbObject
{
 public:
  void setDbuPerMicron(uint32_t dbu_per_micron);

  uint32_t getDbuPerMicron() const;

  dbSet<dbChip> getChips() const;

  dbChip* findChip(const char* name) const;

  dbSet<dbProperty> getProperties() const;

  dbSet<dbChipInst> getChipInsts() const;

  dbSet<dbChipRegionInst> getChipRegionInsts() const;

  dbSet<dbChipConn> getChipConns() const;

  dbSet<dbChipBumpInst> getChipBumpInsts() const;

  dbSet<dbChipNet> getChipNets() const;

  // User Code Begin dbDatabase

  void setHierarchy(bool value);
  bool hasHierarchy() const;

  void setTopChip(dbChip* chip);
  ///
  /// Return the libs contained in the database. A database can contain
  /// multiple libs.
  ///
  dbSet<dbLib> getLibs();

  ///
  /// Find a specific lib.
  /// Returns nullptr if no lib was found.
  ///
  dbLib* findLib(const char* name);

  ///
  /// Return the techs contained in the database. A database can contain
  /// multiple techs.
  ///
  dbSet<dbTech> getTechs();

  ///
  /// Find a specific tech.
  /// Returns nullptr if no tech was found.
  ///
  dbTech* findTech(const char* name);

  ///
  /// Find a specific master
  /// Returns nullptr if no master is found.
  ///
  dbMaster* findMaster(const char* name);

  ///
  /// This function is used to delete unused master-cells.
  /// Returns the number of unused master-cells that have been deleted.
  ///
  int removeUnusedMasters();

  ///
  /// Get the chip of this database.
  /// Returns nullptr if no chip has been created.
  ///
  dbChip* getChip();

  ////////////////////////
  /// DEPRECATED
  ////////////////////////
  ///
  /// This is replaced by dbBlock::getTech() or dbLib::getTech().
  /// This is temporarily kept for legacy migration.
  /// Get the technology of this database if there is exactly one dbTech.
  /// Returns nullptr if no tech has been created.
  ///
  dbTech* getTech();

  ///
  /// Returns the number of masters
  ///
  uint32_t getNumberOfMasters();

  ///
  /// Read a database from this stream.
  /// WARNING: This function destroys the data currently in the database.
  /// Throws ZIOError..
  ///
  void read(std::istream& f);

  ///
  /// Write a database to this stream.
  /// Throws ZIOError..
  ///
  void write(std::ostream& file);

  ///
  /// ECO - The following methods implement a simple ECO mechanism for capturing
  /// netlist changes. The intent of the ECO mechanism is to support delta
  /// changes that occur in a "remote" node that must be applied back to the
  /// "master" node. Being as such, the database on the "remote" must be an
  /// exact copy of the "master" database, prior to changes. Furthermore, the
  /// "master" database cannot change prior to commiting the eco to the block.
  ///
  /// WARNING: If these invariants does not hold then the results will be
  ///          unpredictable.
  ///

  ///
  /// Start collecting ECO changes on the specified block.
  ///
  static void beginEco(dbBlock* block);

  ///
  /// Stop collecting ECO changes on the specified block.
  ///
  static void endEco(dbBlock* block);

  ///
  /// Commit the last ECO changes on the specified block.
  ///
  static void commitEco(dbBlock* block);

  ///
  /// Undo the last ECO changes on the specified block.
  ///
  static void undoEco(dbBlock* block);

  ///
  /// Returns true if the current ECO is empty
  ///
  static bool ecoEmpty(dbBlock* block);

  ///
  /// Return true if the ECO stack is empty. The ECO stack holds
  /// the nested uncommitted ECOs that can still be undone.
  ///
  static bool ecoStackEmpty(dbBlock* block);

  ///
  /// Read the ECO changes from the specified file to be applied to the
  /// specified block.
  ///
  static void readEco(dbBlock* block, const char* filename);

  ///
  /// Write the ECO changes to the specified file.
  ///
  static void writeEco(dbBlock* block, const char* filename);

  ///
  /// links to utl::Logger
  ///
  void setLogger(utl::Logger* logger);

  ///
  /// Initializes the database to nothing.
  ///
  void clear();

  ///
  /// Generates a report of memory usage.
  ///   Not perfectly byte accurate.  Intended for developers.
  ///
  void report();

  ///
  /// Used to be notified when lef/def/odb are read.
  ///
  void addObserver(dbDatabaseObserver* observer);
  void removeObserver(dbDatabaseObserver* observer);

  ///
  /// Notify observers when one of these operations is complete.
  /// Fine-grained callbacks during construction are not as helpful
  /// as knowing when the data is fully loaded into odb.
  ///
  void triggerPostReadLef(dbTech* tech, dbLib* library);
  void triggerPostReadDef(dbBlock* block, bool floorplan);
  void triggerPostReadDb();
  void triggerPostRead3Dbx(dbChip* chip);

  ///
  /// Create an instance of a database
  ///
  static dbDatabase* create();

  ///
  /// Detroy an instance of a database
  ///
  static void destroy(dbDatabase* db);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbDatabase* getDatabase(uint32_t oid);
  // User Code End dbDatabase
};

// Top level DFT (Design for Testing) class
class dbDft : public dbObject
{
 public:
  void setScanInserted(bool scan_inserted);

  bool isScanInserted() const;

  dbSet<dbScanChain> getScanChains() const;
};

class dbGCellGrid : public dbObject
{
 public:
  struct GCellData
  {
    float usage = 0;
    float capacity = 0;
  };

  // User Code Begin dbGCellGrid

  ///
  /// Get the "X" grid coordinates
  ///
  void getGridX(std::vector<int>& x_grid);

  ///
  /// Get the "Y" grid coordinates
  ///
  void getGridY(std::vector<int>& y_grid);

  ///
  /// Get the block this grid belongs too.
  ///
  dbBlock* getBlock();

  ///
  /// Add a "X" grid pattern.
  ///
  void addGridPatternX(int origin_x, int line_count, int step);

  ///
  /// Add a "Y" grid pattern.
  ///
  void addGridPatternY(int origin_y, int line_count, int step);

  ///
  /// Get the number of "X" grid patterns.
  ///
  int getNumGridPatternsX();

  ///
  /// Get the number of "Y" grid patterns.
  ///
  int getNumGridPatternsY();

  ///
  /// Get the "ith" "X" grid pattern.
  ///
  void getGridPatternX(int i, int& origin_x, int& line_count, int& step);

  ///
  /// Get the "ith" "Y" grid pattern.
  ///
  void getGridPatternY(int i, int& origin_y, int& line_count, int& step);
  ///
  /// Create an empty GCell grid.
  /// Returns nullptr if a grid already exists.
  ///
  static dbGCellGrid* create(dbBlock* block);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbGCellGrid* getGCellGrid(dbBlock* block, uint32_t oid);

  uint32_t getXIdx(int x);

  uint32_t getYIdx(int y);

  float getCapacity(dbTechLayer* layer, uint32_t x_idx, uint32_t y_idx) const;

  float getUsage(dbTechLayer* layer, uint32_t x_idx, uint32_t y_idx) const;

  void setCapacity(dbTechLayer* layer,
                   uint32_t x_idx,
                   uint32_t y_idx,
                   float capacity);

  void setUsage(dbTechLayer* layer, uint32_t x_idx, uint32_t y_idx, float use);

  void resetCongestionMap();

  void resetGrid();

  dbMatrix<dbGCellGrid::GCellData> getLayerCongestionMap(dbTechLayer* layer);

  dbMatrix<dbGCellGrid::GCellData> getDirectionCongestionMap(
      const dbTechLayerDir& direction);
  // User Code End dbGCellGrid
};

class dbGDSARef : public dbObject
{
 public:
  void setOrigin(Point origin);

  Point getOrigin() const;

  void setLr(Point lr);

  Point getLr() const;

  void setUl(Point ul);

  Point getUl() const;

  void setTransform(dbGDSSTrans transform);

  dbGDSSTrans getTransform() const;

  void setNumRows(int16_t num_rows);

  int16_t getNumRows() const;

  void setNumColumns(int16_t num_columns);

  int16_t getNumColumns() const;

  // User Code Begin dbGDSARef
  dbGDSStructure* getStructure() const;
  std::vector<std::pair<std::int16_t, std::string>>& getPropattr();

  static dbGDSARef* create(dbGDSStructure* parent, dbGDSStructure* child);
  static void destroy(dbGDSARef* aref);
  // User Code End dbGDSARef
};

class dbGDSBoundary : public dbObject
{
 public:
  void setLayer(int16_t layer);

  int16_t getLayer() const;

  void setDatatype(int16_t datatype);

  int16_t getDatatype() const;

  void setXy(const std::vector<Point>& xy);

  void getXy(std::vector<Point>& tbl) const;

  // User Code Begin dbGDSBoundary
  const std::vector<Point>& getXY();
  std::vector<std::pair<std::int16_t, std::string>>& getPropattr();

  static dbGDSBoundary* create(dbGDSStructure* structure);
  static void destroy(dbGDSBoundary* boundary);
  // User Code End dbGDSBoundary
};

class dbGDSBox : public dbObject
{
 public:
  void setLayer(int16_t layer);

  int16_t getLayer() const;

  void setDatatype(int16_t datatype);

  int16_t getDatatype() const;

  void setBounds(Rect bounds);

  Rect getBounds() const;

  // User Code Begin dbGDSBox
  std::vector<std::pair<std::int16_t, std::string>>& getPropattr();

  static dbGDSBox* create(dbGDSStructure* structure);
  static void destroy(dbGDSBox* box);
  // User Code End dbGDSBox
};

class dbGDSPath : public dbObject
{
 public:
  void setLayer(int16_t layer);

  int16_t getLayer() const;

  void setDatatype(int16_t datatype);

  int16_t getDatatype() const;

  void setXy(const std::vector<Point>& xy);

  void getXy(std::vector<Point>& tbl) const;

  void setWidth(int width);

  int getWidth() const;

  void setPathType(int16_t path_type);

  int16_t getPathType() const;

  // User Code Begin dbGDSPath
  const std::vector<Point>& getXY();
  std::vector<std::pair<std::int16_t, std::string>>& getPropattr();

  static dbGDSPath* create(dbGDSStructure* structure);
  static void destroy(dbGDSPath* path);
  // User Code End dbGDSPath
};

class dbGDSSRef : public dbObject
{
 public:
  void setOrigin(Point origin);

  Point getOrigin() const;

  void setTransform(dbGDSSTrans transform);

  dbGDSSTrans getTransform() const;

  // User Code Begin dbGDSSRef
  dbGDSStructure* getStructure() const;
  std::vector<std::pair<std::int16_t, std::string>>& getPropattr();

  static dbGDSSRef* create(dbGDSStructure* parent, dbGDSStructure* child);
  static void destroy(dbGDSSRef* sref);
  // User Code End dbGDSSRef
};

class dbGDSStructure : public dbObject
{
 public:
  char* getName() const;

  dbSet<dbGDSBoundary> getGDSBoundaries() const;

  dbSet<dbGDSBox> getGDSBoxs() const;

  dbSet<dbGDSPath> getGDSPaths() const;

  dbSet<dbGDSSRef> getGDSSRefs() const;

  dbSet<dbGDSARef> getGDSARefs() const;

  dbSet<dbGDSText> getGDSTexts() const;

  // User Code Begin dbGDSStructure

  dbGDSLib* getGDSLib();

  static dbGDSStructure* create(dbGDSLib* lib, const char* name);
  static void destroy(dbGDSStructure* structure);

  // User Code End dbGDSStructure
};

class dbGDSText : public dbObject
{
 public:
  void setLayer(int16_t layer);

  int16_t getLayer() const;

  void setDatatype(int16_t datatype);

  int16_t getDatatype() const;

  void setOrigin(Point origin);

  Point getOrigin() const;

  void setPresentation(dbGDSTextPres presentation);

  dbGDSTextPres getPresentation() const;

  void setTransform(dbGDSSTrans transform);

  dbGDSSTrans getTransform() const;

  void setText(const std::string& text);

  std::string getText() const;

  // User Code Begin dbGDSText
  std::vector<std::pair<std::int16_t, std::string>>& getPropattr();

  static dbGDSText* create(dbGDSStructure* structure);
  static void destroy(dbGDSText* text);
  // User Code End dbGDSText
};

class dbGlobalConnect : public dbObject
{
 public:
  dbRegion* getRegion() const;

  dbNet* getNet() const;

  std::string getInstPattern() const;

  std::string getPinPattern() const;

  // User Code Begin dbGlobalConnect
  std::vector<dbInst*> getInsts() const;

  int connect(dbInst* inst, bool force);

  static dbGlobalConnect* create(dbNet* net,
                                 dbRegion* region,
                                 const std::string& inst_pattern,
                                 const std::string& pin_pattern);

  static void destroy(dbGlobalConnect* global_connect);

  static dbSet<dbGlobalConnect>::iterator destroy(
      dbSet<dbGlobalConnect>::iterator& itr);

  // User Code End dbGlobalConnect
};

class dbGroup : public dbObject
{
 public:
  const char* getName() const;

  dbGroup* getParentGroup() const;

  dbRegion* getRegion() const;

  // User Code Begin dbGroup

  void setType(dbGroupType type);

  dbGroupType getType() const;

  void addModInst(dbModInst* modinst);

  void removeModInst(dbModInst* modinst);

  dbSet<dbModInst> getModInsts();

  void addInst(dbInst* inst);

  void removeInst(dbInst* inst);

  dbSet<dbInst> getInsts();

  void addGroup(dbGroup* group);

  void removeGroup(dbGroup* group);

  dbSet<dbGroup> getGroups();

  void addPowerNet(dbNet* net);

  void addGroundNet(dbNet* net);

  void removeNet(dbNet* net);

  dbSet<dbNet> getPowerNets();

  dbSet<dbNet> getGroundNets();

  static dbGroup* create(dbBlock* block, const char* name);

  static dbGroup* create(dbRegion* parent, const char* name);

  static dbGroup* create(dbGroup* parent, const char* name);

  static void destroy(dbGroup* group);

  static dbGroup* getGroup(dbBlock* block_, uint32_t dbid_);

  // User Code End dbGroup
};

class dbGuide : public dbObject
{
 public:
  Rect getBox() const;

  // User Code Begin dbGuide

  dbNet* getNet() const;

  dbTechLayer* getLayer() const;

  dbTechLayer* getViaLayer() const;

  bool isCongested() const;

  static dbGuide* create(dbNet* net,
                         dbTechLayer* layer,
                         dbTechLayer* via_layer,
                         Rect box,
                         bool is_congested);

  static dbGuide* getGuide(dbBlock* block, uint32_t dbid);

  static void destroy(dbGuide* guide);

  static dbSet<dbGuide>::iterator destroy(dbSet<dbGuide>::iterator& itr);

  bool isJumper() const;

  void setIsJumper(bool jumper);

  bool isConnectedToTerm() const;

  void setIsConnectedToTerm(bool is_connected);

  // User Code End dbGuide
};

class dbIsolation : public dbObject
{
 public:
  const char* getName() const;

  std::string getAppliesTo() const;

  std::string getClampValue() const;

  std::string getIsolationSignal() const;

  std::string getIsolationSense() const;

  std::string getLocation() const;

  void setPowerDomain(dbPowerDomain* power_domain);

  dbPowerDomain* getPowerDomain() const;

  // User Code Begin dbIsolation
  static dbIsolation* create(dbBlock* block, const char* name);
  static void destroy(dbIsolation* iso);

  void setAppliesTo(const std::string& applies_to);

  void setClampValue(const std::string& clamp_value);

  void setIsolationSignal(const std::string& isolation_signal);

  void setIsolationSense(const std::string& isolation_sense);

  void setLocation(const std::string& location);

  void addIsolationCell(const std::string& master);

  std::vector<dbMaster*> getIsolationCells();

  bool appliesTo(const dbIoType& io);

  // User Code End dbIsolation
};

class dbLevelShifter : public dbObject
{
 public:
  const char* getName() const;

  dbPowerDomain* getDomain() const;

  void setSource(const std::string& source);

  std::string getSource() const;

  void setSink(const std::string& sink);

  std::string getSink() const;

  void setUseFunctionalEquivalence(bool use_functional_equivalence);

  bool isUseFunctionalEquivalence() const;

  void setAppliesTo(const std::string& applies_to);

  std::string getAppliesTo() const;

  void setAppliesToBoundary(const std::string& applies_to_boundary);

  std::string getAppliesToBoundary() const;

  void setRule(const std::string& rule);

  std::string getRule() const;

  void setThreshold(float threshold);

  float getThreshold() const;

  void setNoShift(bool no_shift);

  bool isNoShift() const;

  void setForceShift(bool force_shift);

  bool isForceShift() const;

  void setLocation(const std::string& location);

  std::string getLocation() const;

  void setInputSupply(const std::string& input_supply);

  std::string getInputSupply() const;

  void setOutputSupply(const std::string& output_supply);

  std::string getOutputSupply() const;

  void setInternalSupply(const std::string& internal_supply);

  std::string getInternalSupply() const;

  void setNamePrefix(const std::string& name_prefix);

  std::string getNamePrefix() const;

  void setNameSuffix(const std::string& name_suffix);

  std::string getNameSuffix() const;

  void setCellName(const std::string& cell_name);

  std::string getCellName() const;

  void setCellInput(const std::string& cell_input);

  std::string getCellInput() const;

  void setCellOutput(const std::string& cell_output);

  std::string getCellOutput() const;

  // User Code Begin dbLevelShifter

  static dbLevelShifter* create(dbBlock* block,
                                const char* name,
                                dbPowerDomain* domain);
  static void destroy(dbLevelShifter* shifter);

  void addElement(const std::string& element);
  void addExcludeElement(const std::string& element);
  void addInstance(const std::string& instance, const std::string& port);
  std::vector<std::string> getElements() const;
  std::vector<std::string> getExcludeElements() const;
  std::vector<std::pair<std::string, std::string>> getInstances() const;
  // User Code End dbLevelShifter
};

class dbLogicPort : public dbObject
{
 public:
  const char* getName() const;

  std::string getDirection() const;

  // User Code Begin dbLogicPort
  static dbLogicPort* create(dbBlock* block,
                             const char* name,
                             const std::string& direction);
  static void destroy(dbLogicPort* lp);
  // User Code End dbLogicPort
};

class dbMarker : public dbObject
{
 public:
  void setComment(const std::string& comment);

  std::string getComment() const;

  void setLineNumber(int line_number);

  int getLineNumber() const;

  void setVisited(bool visited);

  bool isVisited() const;

  void setVisible(bool visible);

  bool isVisible() const;

  void setWaived(bool waived);

  bool isWaived() const;

  // User Code Begin dbMarker

  std::string getName() const;

  using MarkerShape = std::variant<Point, Line, Rect, Polygon>;

  dbMarkerCategory* getCategory() const;
  std::vector<MarkerShape> getShapes() const;
  dbTechLayer* getTechLayer() const;
  Rect getBBox() const;

  std::set<dbObject*> getSources() const;

  void addShape(const Point& pt);
  void addShape(const Line& line);
  void addShape(const Rect& rect);
  void addShape(const Polygon& polygon);

  void setTechLayer(dbTechLayer* layer);

  void addSource(dbObject* obj);
  bool addObstructionFromBlock(dbBlock* block);

  static dbMarker* create(dbMarkerCategory* category);
  static void destroy(dbMarker* marker);

  // User Code End dbMarker
};

class dbMarkerCategory : public dbObject
{
 public:
  const char* getName() const;

  void setDescription(const std::string& description);

  std::string getDescription() const;

  void setSource(const std::string& source);

  void setMaxMarkers(int max_markers);

  int getMaxMarkers() const;

  dbSet<dbMarker> getMarkers() const;

  dbSet<dbMarkerCategory> getMarkerCategories() const;

  dbMarkerCategory* findMarkerCategory(const char* name) const;

  // User Code Begin dbMarkerCategory

  dbMarkerCategory* getTopCategory() const;
  dbObject* getParent() const;
  std::string getSource() const;

  std::set<dbMarker*> getAllMarkers() const;

  bool rename(const char* name);

  int getMarkerCount() const;

  void writeJSON(const std::string& path) const;
  void writeJSON(std::ofstream& report) const;
  void writeTR(const std::string& path) const;
  void writeTR(std::ofstream& report) const;

  static std::set<dbMarkerCategory*> fromJSON(dbChip* chip,
                                              const std::string& path);
  static std::set<dbMarkerCategory*> fromJSON(dbChip* chip,
                                              const char* source,
                                              std::ifstream& report);
  static dbMarkerCategory* fromTR(dbChip* chip,
                                  const char* name,
                                  const std::string& path);
  static dbMarkerCategory* fromTR(dbChip* chip,
                                  const char* name,
                                  const char* source,
                                  std::ifstream& report);

  static dbMarkerCategory* create(dbBlock* block, const char* name);
  static dbMarkerCategory* createOrReplace(dbBlock* block, const char* name);
  static dbMarkerCategory* createOrGet(dbBlock* block, const char* name);
  static dbMarkerCategory* create(dbChip* chip, const char* name);
  static dbMarkerCategory* createOrReplace(dbChip* chip, const char* name);
  static dbMarkerCategory* createOrGet(dbChip* chip, const char* name);
  static dbMarkerCategory* create(dbMarkerCategory* category, const char* name);
  static dbMarkerCategory* createOrGet(dbMarkerCategory* category,
                                       const char* name);
  static dbMarkerCategory* createOrReplace(dbMarkerCategory* category,
                                           const char* name);
  static void destroy(dbMarkerCategory* category);

  // User Code End dbMarkerCategory
};

class dbMasterEdgeType : public dbObject
{
 public:
  enum EdgeDir
  {
    TOP,
    RIGHT,
    LEFT,
    BOTTOM
  };

  void setEdgeType(const std::string& edge_type);

  std::string getEdgeType() const;

  void setCellRow(int cell_row);

  int getCellRow() const;

  void setHalfRow(int half_row);

  int getHalfRow() const;

  void setRangeBegin(int range_begin);

  int getRangeBegin() const;

  void setRangeEnd(int range_end);

  int getRangeEnd() const;

  // User Code Begin dbMasterEdgeType
  void setEdgeDir(dbMasterEdgeType::EdgeDir edge_dir);

  dbMasterEdgeType::EdgeDir getEdgeDir() const;

  static dbMasterEdgeType* create(dbMaster* master);

  static void destroy(dbMasterEdgeType* edge_type);

  // User Code End dbMasterEdgeType
};

class dbMetalWidthViaMap : public dbObject
{
 public:
  void setViaCutClass(bool via_cut_class);

  bool isViaCutClass() const;

  void setCutLayer(dbTechLayer* cut_layer);

  void setBelowLayerWidthLow(int below_layer_width_low);

  int getBelowLayerWidthLow() const;

  void setBelowLayerWidthHigh(int below_layer_width_high);

  int getBelowLayerWidthHigh() const;

  void setAboveLayerWidthLow(int above_layer_width_low);

  int getAboveLayerWidthLow() const;

  void setAboveLayerWidthHigh(int above_layer_width_high);

  int getAboveLayerWidthHigh() const;

  void setViaName(const std::string& via_name);

  std::string getViaName() const;

  void setPgVia(bool pg_via);

  bool isPgVia() const;

  // User Code Begin dbMetalWidthViaMap

  dbTechLayer* getCutLayer() const;

  static dbMetalWidthViaMap* create(dbTech* tech);

  static void destroy(dbMetalWidthViaMap* via_map);

  static dbMetalWidthViaMap* getMetalWidthViaMap(dbTech* tech, uint32_t dbid);

  // User Code End dbMetalWidthViaMap
};

class dbModBTerm : public dbObject
{
 public:
  const char* getName() const;

  dbModule* getParent() const;

  // User Code Begin dbModBTerm
  std::string getHierarchicalName() const;
  void setParentModITerm(dbModITerm* parent_pin);
  dbModITerm* getParentModITerm() const;
  void setModNet(dbModNet* modNet);
  dbModNet* getModNet() const;
  void setSigType(const dbSigType& type);
  dbSigType getSigType() const;
  void setIoType(const dbIoType& type);
  dbIoType getIoType() const;
  void connect(dbModNet* net);
  void disconnect();
  bool isBusPort() const;
  void setBusPort(dbBusPort*);
  dbBusPort* getBusPort() const;

  ///
  /// Returns the module instance that contains this module boundary terminal.
  /// - It can be connected to a dbModITerm of a dbModInst that instantiates
  ///   this module. This function returns that dbModInst.
  /// - Returns nullptr if there is no instantiated module or dbModBTerm is not
  ///   connected to a dbModITerm.
  ///
  dbModInst* getModInst() const;

  static dbModBTerm* create(dbModule* parentModule, const char* name);
  static void destroy(dbModBTerm*);
  static dbSet<dbModBTerm>::iterator destroy(dbSet<dbModBTerm>::iterator& itr);
  static dbModBTerm* getModBTerm(dbBlock* block, uint32_t dbid);
  // User Code End dbModBTerm
};

class dbModInst : public dbObject
{
 public:
  const char* getName() const;

  dbModule* getParent() const;

  dbModule* getMaster() const;

  dbGroup* getGroup() const;

  // User Code Begin dbModInst
  std::string getHierarchicalName() const;
  dbModITerm* findModITerm(const char* name);
  dbSet<dbModITerm> getModITerms();
  void removeUnusedPortsAndPins();

  dbModNet* findHierNet(const char* base_name) const;
  dbNet* findFlatNet(const char* base_name) const;
  bool findNet(const char* base_name,
               dbNet*& flat_net,
               dbModNet*& hier_net) const;

  /// Swap the module of this instance.
  /// Returns new mod inst if the operations succeeds.
  /// Old mod inst is deleted along with its child insts.
  dbModInst* swapMaster(dbModule* module);

  // Recursive search a given dbInst through child mod insts
  bool containsDbInst(dbInst* inst) const;

  // Recursive search a given dbModInst through child mod insts
  bool containsDbModInst(dbModInst* mod_inst) const;

  static dbModInst* create(dbModule* parentModule,
                           dbModule* masterModule,
                           const char* name);

  // This destroys this modinst but does not destroy the master dbModule.
  static void destroy(dbModInst* modinst);

  static dbSet<dbModInst>::iterator destroy(dbSet<dbModInst>::iterator& itr);

  static dbModInst* getModInst(dbBlock* block_, uint32_t dbid_);
  // User Code End dbModInst
};

class dbModITerm : public dbObject
{
 public:
  const char* getName() const;

  dbModInst* getParent() const;

  // User Code Begin dbModITerm
  std::string getHierarchicalName() const;
  void setModNet(dbModNet* modNet);
  dbModNet* getModNet() const;
  void setChildModBTerm(dbModBTerm* child_port);
  dbModBTerm* getChildModBTerm() const;
  void connect(dbModNet* modnet);
  void disconnect();

  static dbModITerm* create(dbModInst* parentInstance,
                            const char* name,
                            dbModBTerm* modbterm = nullptr);
  static void destroy(dbModITerm*);
  static dbSet<dbModITerm>::iterator destroy(dbSet<dbModITerm>::iterator& itr);
  static dbModITerm* getModITerm(dbBlock* block, uint32_t dbid);
  // User Code End dbModITerm
};

class dbModNet : public dbObject
{
 public:
  dbModule* getParent() const;

  // User Code Begin dbModNet
  dbSet<dbModITerm> getModITerms() const;
  dbSet<dbModBTerm> getModBTerms() const;
  dbSet<dbITerm> getITerms() const;
  dbSet<dbBTerm> getBTerms() const;
  unsigned connectionCount() const;
  std::string getName() const;
  const char* getConstName() const;
  std::string getHierarchicalName() const;
  void rename(const char* new_name);
  void disconnectAllTerms();
  void dump() const;

  // Find the flat net (dbNet) associated with this hierarchical net (dbModNet).
  // A dbModNet should be associated with a single dbNet.
  // This function traverses the terminals connected to this dbModNet
  // and returns the first dbNet it finds.
  dbNet* findRelatedNet() const;
  void checkSanity() const;

  ///
  /// Merge the terminals of the in_modnet with this modnet
  ///
  void mergeModNet(dbModNet* in_modnet);

  ///
  /// Connect the terminals of the in_net with this modnet
  ///
  void connectTermsOf(dbNet* in_net);

  ///
  /// Returns true if this dbModNet is connected to other dbNet.
  ///
  bool isConnected(const dbNet* other) const;

  ///
  /// Returns true if this dbModNet is connected to other dbModNet.
  ///
  bool isConnected(const dbModNet* other) const;

  ///
  /// Returns the next dbModNets in the fanin of this dbModNet.
  /// Traverses up to parent inputs or down to child outputs.
  ///
  std::vector<dbModNet*> getNextModNetsInFanin() const;

  ///
  /// Returns the next dbModNets in the fanout of this dbModNet.
  /// Traverses down to child inputs or up to parent outputs.
  ///
  std::vector<dbModNet*> getNextModNetsInFanout() const;

  ///
  /// Traverses the hierarchy in search of the first mod net that satisfies the
  /// given condition.
  ///
  dbModNet* findInHierarchy(const std::function<bool(dbModNet*)>& condition,
                            dbHierSearchDir dir) const;

  static dbModNet* getModNet(dbBlock* block, uint32_t id);
  static dbModNet* create(dbModule* parent_module, const char* base_name);
  static dbModNet* create(dbModule* parent_module,
                          const char* base_name,
                          const dbNameUniquifyType& uniquify,
                          dbNet* corresponding_flat_net = nullptr);
  static dbSet<dbModNet>::iterator destroy(dbSet<dbModNet>::iterator& itr);
  static void destroy(dbModNet*);
  // User Code End dbModNet
};

class dbModule : public dbObject
{
 public:
  const char* getName() const;

  void setModInst(dbModInst* mod_inst);

  dbModInst* getModInst() const;

  // User Code Begin dbModule

  ///
  /// Returns the parent module, or nullptr if this is the top-level module.
  ///
  dbModule* getParentModule() const;

  std::string getHierarchicalName() const;

  // Get a mod net by name
  dbModNet* getModNet(const char* net_name) const;

  // Adding an inst to a new module will remove it from its previous
  // module.
  void addInst(dbInst* inst);

  dbBlock* getOwner() const;

  dbSet<dbModInst> getChildren() const;
  dbSet<dbModInst> getModInsts() const;
  dbSet<dbModNet> getModNets();
  // Get the ports of a module (STA world uses ports, which contain members).
  dbSet<dbModBTerm> getPorts();
  // Get the leaf level connections on a module (flat connected view).
  dbSet<dbModBTerm> getModBTerms() const;
  dbModBTerm* getModBTerm(uint32_t id);
  dbSet<dbInst> getInsts() const;

  dbModInst* findModInst(const char* name) const;
  dbInst* findDbInst(const char* name) const;
  dbModBTerm* findModBTerm(const char* name) const;

  std::vector<dbInst*> getLeafInsts();

  int getModInstCount();
  int getDbInstCount();

  const dbModBTerm* getHeadDbModBTerm() const;
  bool canSwapWith(dbModule* new_module) const;
  bool isTop() const;
  bool containsDbInst(dbInst* inst) const;
  bool containsDbModInst(dbModInst* inst) const;

  static dbModule* create(dbBlock* block, const char* name);

  static void destroy(dbModule* module);

  static dbModule* getModule(dbBlock* block_, uint32_t dbid_);

  static dbModule* makeUniqueDbModule(const char* cell_name,
                                      const char* inst_name,
                                      dbBlock* block);

  // User Code End dbModule
};

class dbNetTrack : public dbObject
{
 public:
  Rect getBox() const;

  // User Code Begin dbNetTrack

  dbNet* getNet() const;

  dbTechLayer* getLayer() const;

  static dbNetTrack* create(dbNet* net, dbTechLayer* layer, Rect box);

  static dbNetTrack* getNetTrack(dbBlock* block, uint32_t dbid);

  static void destroy(dbNetTrack* guide);

  static dbSet<dbNetTrack>::iterator destroy(dbSet<dbNetTrack>::iterator& itr);

  // User Code End dbNetTrack
};

class dbPolygon : public dbObject
{
 public:
  Polygon getPolygon() const;

  int getDesignRuleWidth() const;

  // User Code Begin dbPolygon
  dbTechLayer* getTechLayer();
  dbSet<dbBox> getGeometry();
  void setDesignRuleWidth(int design_rule_width);

  ///
  /// Add an obstruction to a master.
  ///
  static dbPolygon* create(dbMaster* master,
                           dbTechLayer* layer,
                           const std::vector<Point>& polygon);

  ///
  /// Add a wire-shape to a master-pin.
  ///
  static dbPolygon* create(dbMPin* pin,
                           dbTechLayer* layer,
                           const std::vector<Point>& polygon);

  // User Code End dbPolygon
};

class dbPowerDomain : public dbObject
{
 public:
  const char* getName() const;

  dbGroup* getGroup() const;

  void setTop(bool top);

  bool isTop() const;

  void setParent(dbPowerDomain* parent);

  dbPowerDomain* getParent() const;

  void setVoltage(float voltage);

  float getVoltage() const;

  // User Code Begin dbPowerDomain
  void setGroup(dbGroup* group);
  static dbPowerDomain* create(dbBlock* block, const char* name);
  static void destroy(dbPowerDomain* pd);

  void addElement(const std::string& element);
  std::vector<std::string> getElements();

  void addPowerSwitch(dbPowerSwitch* ps);
  void addIsolation(dbIsolation* iso);
  void addLevelShifter(dbLevelShifter* shifter);

  std::vector<dbPowerSwitch*> getPowerSwitches();
  std::vector<dbIsolation*> getIsolations();
  std::vector<dbLevelShifter*> getLevelShifters();

  void setArea(const Rect& area);
  bool getArea(Rect& area);

  // User Code End dbPowerDomain
};

class dbPowerSwitch : public dbObject
{
 public:
  struct UPFIOSupplyPort
  {
    std::string port_name;
    std::string supply_net_name;
  };
  struct UPFControlPort
  {
    std::string port_name;
    std::string net_name;
  };
  struct UPFAcknowledgePort
  {
    std::string port_name;
    std::string net_name;
    std::string boolean_expression;
  };
  struct UPFOnState
  {
    std::string state_name;
    std::string input_supply_port;
    std::string boolean_expression;
  };

  const char* getName() const;

  void setPowerDomain(dbPowerDomain* power_domain);

  dbPowerDomain* getPowerDomain() const;

  // User Code Begin dbPowerSwitch
  static dbPowerSwitch* create(dbBlock* block, const char* name);
  static void destroy(dbPowerSwitch* ps);
  void addInSupplyPort(const std::string& in_port, const std::string& net);
  void setOutSupplyPort(const std::string& out_port, const std::string& net);
  void addControlPort(const std::string& control_port,
                      const std::string& control_net);
  void addAcknowledgePort(const std::string& port_name,
                          const std::string& net_name,
                          const std::string& boolean_expression);
  void addOnState(const std::string& on_state,
                  const std::string& port_name,
                  const std::string& boolean_expression);
  void setLibCell(dbMaster* master);
  void addPortMap(const std::string& model_port,
                  const std::string& switch_port);

  void addPortMap(const std::string& model_port, dbMTerm* mterm);
  std::vector<UPFControlPort> getControlPorts();
  std::vector<UPFIOSupplyPort> getInputSupplyPorts();
  UPFIOSupplyPort getOutputSupplyPort();
  std::vector<UPFAcknowledgePort> getAcknowledgePorts();
  std::vector<UPFOnState> getOnStates();

  // Returns library cell that was defined in the upf for this power switch
  dbMaster* getLibCell();

  // returns a map associating the model ports to actual instances of dbMTerms
  // belonging to the first
  //  lib cell defined in the upf
  std::map<std::string, dbMTerm*> getPortMap();
  // User Code End dbPowerSwitch
};

class dbProperty : public dbObject
{
 public:
  // User Code Begin dbProperty
  enum Type
  {
    // Do not change the order or the values of this enum.
    STRING_PROP = 0,
    BOOL_PROP = 1,
    INT_PROP = 2,
    DOUBLE_PROP = 3
  };

  /// Get the type of this property.
  Type getType();

  /// Get thetname of this property.
  std::string getName();

  /// Get the owner of this property
  dbObject* getPropOwner();

  /// Find the named property. Returns nullptr if the property does not exist.
  static dbProperty* find(dbObject* object, const char* name);

  /// Find the named property of the specified type. Returns nullptr if the
  /// property does not exist.
  static dbProperty* find(dbObject* object, const char* name, Type type);

  /// Destroy a specific property
  static void destroy(dbProperty* prop);
  /// Destroy all properties of the specific object
  static void destroyProperties(dbObject* obj);
  static dbSet<dbProperty> getProperties(dbObject* object);
  static dbSet<dbProperty>::iterator destroy(dbSet<dbProperty>::iterator itr);
  // 5.8
  static std::string writeProperties(dbObject* object);
  static std::string writePropValue(dbProperty* prop);
  // User Code End dbProperty
};

// A scan chain is a collection of dbScanLists that contains dbScanInsts.  Here,
// scan_in, scan_out and scan_enable are the top level ports/pins to where this
// scan chain is connected.  Each scan chain is also associated with a
// particular test mode and test mode pin that puts the Circuit Under Test (CUT)
// in test. The Scan Enable port/pin puts the scan chain into shifting mode.
class dbScanChain : public dbObject
{
 public:
  dbSet<dbScanPartition> getScanPartitions() const;

  // User Code Begin dbScanChain
  const std::string& getName() const;

  void setName(std::string_view name);

  void setScanIn(dbBTerm* scan_in);
  void setScanIn(dbITerm* scan_in);
  std::variant<dbBTerm*, dbITerm*> getScanIn() const;

  void setScanOut(dbBTerm* scan_out);
  void setScanOut(dbITerm* scan_out);
  std::variant<dbBTerm*, dbITerm*> getScanOut() const;

  void setScanEnable(dbBTerm* scan_enable);
  void setScanEnable(dbITerm* scan_enable);
  std::variant<dbBTerm*, dbITerm*> getScanEnable() const;

  void setTestMode(dbBTerm* test_mode);
  void setTestMode(dbITerm* test_mode);
  std::variant<dbBTerm*, dbITerm*> getTestMode() const;

  void setTestModeName(const std::string& test_mode_name);
  const std::string& getTestModeName() const;

  static dbScanChain* create(dbDft* dft);
  // User Code End dbScanChain
};

// A scan inst is a cell with a scan in, scan out and an optional scan enable.
// If no scan enable is provided, then this is an stateless component (because
// we don't need to enable scan for it) and the number of bits is set to 0.  It
// may be possible that two or more dbScanInst contains the same dbInst if the
// dbInst has more than one scan_in/scan_out pair. Examples of those cases are
// multibit cells with external scan or black boxes.  In this case, the scan_in,
// scan_out and scan enables are pins in the dbInst. The scan clock is the pin
// that we use to shift patterns in and out of the scan chain.
class dbScanInst : public dbObject
{
 public:
  struct AccessPins
  {
    std::variant<dbBTerm*, dbITerm*> scan_in;
    std::variant<dbBTerm*, dbITerm*> scan_out;
  };

  enum class ClockEdge
  {
    Rising,
    Falling
  };

  // User Code Begin dbScanInst
  void setScanClock(std::string_view scan_clock);
  const std::string& getScanClock() const;

  void setClockEdge(ClockEdge clock_edge);
  ClockEdge getClockEdge() const;
  std::string getClockEdgeString() const;

  // The number of bits that are in this scan inst from the scan in to the scan
  // out. For simple flops this is just 1.
  void setBits(uint32_t bits);
  uint32_t getBits() const;

  void setScanEnable(dbBTerm* scan_enable);
  void setScanEnable(dbITerm* scan_enable);
  std::variant<dbBTerm*, dbITerm*> getScanEnable() const;

  void setAccessPins(const AccessPins& access_pins);
  AccessPins getAccessPins() const;

  dbInst* getInst() const;

  void insertAtFront(dbScanList* scan_list);

  static dbScanInst* create(dbScanList* scan_list, dbInst* inst);
  // User Code End dbScanInst
};

// A scan list is a collection of dbScanInsts in a particular order that must be
// respected when performing scan reordering and repartitioning. For ScanList
// with two or more elements we say that they are ORDERED. If the ScanList
// contains only one element then they are FLOATING elements that don't have any
// restriccion when optimizing the scan chain.
class dbScanList : public dbObject
{
 public:
  // User Code Begin dbScanList
  dbSet<dbScanInst> getScanInsts() const;
  dbScanInst* add(dbInst* inst);
  static dbScanList* create(dbScanPartition* scan_partition);
  // User Code End dbScanList
};

// A scan partition is way to split the scan chains into sub chains with
// compatible scan flops (same clock, edge and voltage). The biggest partition
// possible is the whole chain if all the scan flops inside it are compatible
// between them for reordering and repartitioning. The name of this partition is
// not unique, as multiple partitions may have the same same and therefore
// contain the same type of flops.
class dbScanPartition : public dbObject
{
 public:
  dbSet<dbScanList> getScanLists() const;

  // User Code Begin dbScanPartition
  const std::string& getName() const;
  void setName(const std::string& name);

  static dbScanPartition* create(dbScanChain* chain);
  // User Code End dbScanPartition
};

// This is a helper class to contain dbBTerms and dbITerms in the same field. We
// need this difference because some pins may need to be conected to top level
// ports or cell's pins.  For example: a scan chain may be connected to a top
// level design port (dbBTerm) or to an output/input pin of a cell that is part
// of a decompressor/compressor
class dbScanPin : public dbObject
{
 public:
  // User Code Begin dbScanPin
  std::variant<dbBTerm*, dbITerm*> getPin() const;
  void setPin(dbBTerm* bterm);
  void setPin(dbITerm* iterm);
  static dbId<dbScanPin> create(dbDft* dft, dbBTerm* bterm);
  static dbId<dbScanPin> create(dbDft* dft, dbITerm* iterm);
  // User Code End dbScanPin
};

class dbTechLayer : public dbObject
{
 public:
  enum LEF58_TYPE
  {
    NONE,
    NWELL,
    PWELL,
    ABOVEDIEEDGE,
    BELOWDIEEDGE,
    DIFFUSION,
    TRIMPOLY,
    MIMCAP,
    STACKEDMIMCAP,
    TSVMETAL,
    TSV,
    PASSIVATION,
    HIGHR,
    TRIMMETAL,
    REGION,
    MEOL,
    WELLDISTANCE,
    CPODE,
    PADMETAL,
    POLYROUTING
  };

  void setWrongWayWidth(uint32_t wrong_way_width);

  uint32_t getWrongWayWidth() const;

  void setWrongWayMinWidth(uint32_t wrong_way_min_width);

  uint32_t getWrongWayMinWidth() const;

  void setLayerAdjustment(float layer_adjustment);

  float getLayerAdjustment() const;

  void getOrthSpacingTable(std::vector<std::pair<int, int>>& tbl) const;

  dbSet<dbTechLayerCutClassRule> getTechLayerCutClassRules() const;

  dbTechLayerCutClassRule* findTechLayerCutClassRule(const char* name) const;

  dbSet<dbTechLayerSpacingEolRule> getTechLayerSpacingEolRules() const;

  dbSet<dbTechLayerCutSpacingRule> getTechLayerCutSpacingRules() const;

  dbSet<dbTechLayerMinStepRule> getTechLayerMinStepRules() const;

  dbSet<dbTechLayerCornerSpacingRule> getTechLayerCornerSpacingRules() const;

  dbSet<dbTechLayerSpacingTablePrlRule> getTechLayerSpacingTablePrlRules()
      const;

  dbSet<dbTechLayerCutSpacingTableOrthRule>
  getTechLayerCutSpacingTableOrthRules() const;

  dbSet<dbTechLayerCutSpacingTableDefRule> getTechLayerCutSpacingTableDefRules()
      const;

  dbSet<dbTechLayerCutEnclosureRule> getTechLayerCutEnclosureRules() const;

  dbSet<dbTechLayerEolExtensionRule> getTechLayerEolExtensionRules() const;

  dbSet<dbTechLayerArraySpacingRule> getTechLayerArraySpacingRules() const;

  dbSet<dbTechLayerEolKeepOutRule> getTechLayerEolKeepOutRules() const;

  dbSet<dbTechLayerMaxSpacingRule> getTechLayerMaxSpacingRules() const;

  dbSet<dbTechLayerWidthTableRule> getTechLayerWidthTableRules() const;

  dbSet<dbTechLayerMinCutRule> getTechLayerMinCutRules() const;

  dbSet<dbTechLayerAreaRule> getTechLayerAreaRules() const;

  dbSet<dbTechLayerForbiddenSpacingRule> getTechLayerForbiddenSpacingRules()
      const;

  dbSet<dbTechLayerKeepOutZoneRule> getTechLayerKeepOutZoneRules() const;

  dbSet<dbTechLayerWrongDirSpacingRule> getTechLayerWrongDirSpacingRules()
      const;

  dbSet<dbTechLayerTwoWiresForbiddenSpcRule>
  getTechLayerTwoWiresForbiddenSpcRules() const;

  dbSet<dbTechLayerVoltageSpacing> getTechLayerVoltageSpacings() const;

  void setRectOnly(bool rect_only);

  bool isRectOnly() const;

  void setRightWayOnGridOnly(bool right_way_on_grid_only);

  bool isRightWayOnGridOnly() const;

  void setRightWayOnGridOnlyCheckMask(bool right_way_on_grid_only_check_mask);

  bool isRightWayOnGridOnlyCheckMask() const;

  void setRectOnlyExceptNonCorePins(bool rect_only_except_non_core_pins);

  bool isRectOnlyExceptNonCorePins() const;

  // User Code Begin dbTechLayer
  int findV55Spacing(int width, int prl) const;

  int findTwSpacing(int width1, int width2, int prl) const;

  void setLef58Type(LEF58_TYPE type);

  LEF58_TYPE getLef58Type() const;
  std::string getLef58TypeString() const;

  ///
  /// Get the layer name.
  ///
  std::string getName() const;

  ///
  /// Get the layer name.
  ///
  const char* getConstName() const;

  ///
  /// Returns true if this layer has an alias.
  ///
  bool hasAlias();

  ///
  /// Get the layer alias.
  ///
  std::string getAlias();

  ///
  /// Set the layer alias.
  ///
  void setAlias(const char* alias);

  ///
  /// Get the default width.
  ///
  uint32_t getWidth() const;
  void setWidth(int width);

  ///
  /// Get the minimum object-to-object spacing.
  ///
  int getSpacing();
  void setSpacing(int spacing);

  ///
  /// Get the minimum spacing to a wide line.
  ///
  int getSpacing(int width, int length = 0);

  ///
  /// The number of masks for this layer (aka double/triple patterning).
  /// Allowable values are in [1, 3].
  ///
  uint32_t getNumMasks() const;
  void setNumMasks(uint32_t number);

  ///
  /// Get the low end of the uppermost range for wide wire design rules.
  ///
  void getMaxWideDRCRange(int& owidth, int& olength);
  void getMinWideDRCRange(int& owidth, int& olength);

  ///
  /// Get the collection of spacing rules for the object, assuming
  /// coding in LEF 5.4 format.
  ///
  dbSet<dbTechLayerSpacingRule> getV54SpacingRules() const;

  ///
  /// API for version 5.5 spacing rules, expressed as a 2D matrix with
  /// index tables  LEF 5.4 and 5.5 rules should not co-exist -- although
  /// this is not enforced here.
  /// V5.4 and V5.5 spacing rules are optional -- in this case there is a
  /// single spacing value for all length/width combinations.
  ///
  bool hasV55SpacingRules() const;
  bool getV55SpacingWidthsAndLengths(std::vector<uint32_t>& width_idx,
                                     std::vector<uint32_t>& length_idx) const;
  void printV55SpacingRules(lefout& writer) const;
  bool getV55SpacingTable(std::vector<std::vector<uint32_t>>& sptbl) const;

  void initV55LengthIndex(uint32_t numelems);
  void addV55LengthEntry(uint32_t length);
  void initV55WidthIndex(uint32_t numelems);
  void addV55WidthEntry(uint32_t width);
  void initV55SpacingTable(uint32_t numrows, uint32_t numcols);
  void addV55SpacingTableEntry(uint32_t inrow,
                               uint32_t incol,
                               uint32_t spacing);

  dbSet<dbTechV55InfluenceEntry> getV55InfluenceRules();

  ///
  /// API for version 5.7 two widths spacing rules, expressed as a 2D matrix
  /// with index tables
  ///
  bool hasTwoWidthsSpacingRules() const;
  void printTwoWidthsSpacingRules(lefout& writer) const;
  bool getTwoWidthsSpacingTable(
      std::vector<std::vector<uint32_t>>& sptbl) const;
  uint32_t getTwoWidthsSpacingTableNumWidths() const;
  uint32_t getTwoWidthsSpacingTableWidth(uint32_t row) const;
  bool getTwoWidthsSpacingTableHasPRL(uint32_t row) const;
  uint32_t getTwoWidthsSpacingTablePRL(uint32_t row) const;
  uint32_t getTwoWidthsSpacingTableEntry(uint32_t row, uint32_t col) const;

  void initTwoWidths(uint32_t num_widths);
  void addTwoWidthsIndexEntry(uint32_t width, int parallel_run_length = -1);
  void addTwoWidthsSpacingTableEntry(uint32_t inrow,
                                     uint32_t incol,
                                     uint32_t spacing);
  ///
  ///  create container for layer specific antenna rules
  ///  currently only oxide1 (default) and oxide2 models supported.
  ///
  dbTechLayerAntennaRule* createDefaultAntennaRule();
  dbTechLayerAntennaRule* createOxide2AntennaRule();

  ///
  /// Access and write antenna rule models -- get functions will return nullptr
  /// if model not created.
  ///
  bool hasDefaultAntennaRule() const;
  bool hasOxide2AntennaRule() const;
  dbTechLayerAntennaRule* getDefaultAntennaRule() const;
  dbTechLayerAntennaRule* getOxide2AntennaRule() const;
  void writeAntennaRulesLef(lefout& writer) const;

  ///
  /// Get collection of minimum cuts, minimum enclosure rules, if exist
  ///
  bool getMinimumCutRules(std::vector<dbTechMinCutRule*>& cut_rules);
  bool getMinEnclosureRules(std::vector<dbTechMinEncRule*>& enc_rules);

  dbSet<dbTechMinCutRule> getMinCutRules();
  dbSet<dbTechMinEncRule> getMinEncRules();

  ///
  /// Get/Set the minimum feature size (pitch).
  ///
  int getPitch();
  int getPitchX();
  int getPitchY();
  int getFirstLastPitch();
  void setPitch(int pitch);
  void setPitchXY(int pitch_x, int pitch_y);
  void setFirstLastPitch(int first_last_pitch);
  bool hasXYPitch();

  int getOffset();
  int getOffsetX();
  int getOffsetY();
  void setOffset(int pitch);
  void setOffsetXY(int pitch_x, int pitch_y);
  bool hasXYOffset();

  ///
  ///  Get THICKNESS in DB units, and return indicator of existence.
  ///  Do not trust value of output parm if return value is false.
  ///
  bool getThickness(uint32_t& inthk) const;
  void setThickness(uint32_t thickness);

  ///
  ///  Get/set AREA parameter.  This interface is used when a
  ///  reasonable default exists.
  ///
  bool hasArea() const;
  double getArea() const;
  void setArea(double area);

  ///
  ///  Get/set MAXWIDTH parameter.  This interface is used when a
  ///  reasonable default exists.
  ///
  bool hasMaxWidth() const;
  uint32_t getMaxWidth() const;
  void setMaxWidth(uint32_t max_width);

  ///
  ///  Get/set min width parameter.
  ///
  uint32_t getMinWidth() const;
  void setMinWidth(uint32_t max_width);

  ///
  ///  Get/set MINSTEP parameter.  This interface is used when a
  ///  reasonable default exists.
  ///
  bool hasMinStep() const;
  uint32_t getMinStep() const;
  void setMinStep(uint32_t min_step);

  dbTechLayerMinStepType getMinStepType() const;
  void setMinStepType(dbTechLayerMinStepType type);

  bool hasMinStepMaxLength() const;
  uint32_t getMinStepMaxLength() const;
  void setMinStepMaxLength(uint32_t length);

  bool hasMinStepMaxEdges() const;
  uint32_t getMinStepMaxEdges() const;
  void setMinStepMaxEdges(uint32_t edges);

  ///
  ///  Get/set PROTRUSIONWIDTH parameter.  This interface is used when a
  ///  reasonable default exists.
  ///
  bool hasProtrusion() const;
  uint32_t getProtrusionWidth() const;
  uint32_t getProtrusionLength() const;
  uint32_t getProtrusionFromWidth() const;
  void setProtrusion(uint32_t pt_width,
                     uint32_t pt_length,
                     uint32_t pt_from_width);

  /// Get the layer-type
  ///
  dbTechLayerType getType();

  ///
  /// Get/Set the layer-direction
  ///
  dbTechLayerDir getDirection();
  void setDirection(dbTechLayerDir direction);

  ///
  /// Get/Set the resistance (ohms per square for routing layers;
  ///                         ohms per cut on via layers)
  ///
  double getResistance();
  void setResistance(double res);

  ///
  /// Get/Set the capacitance (pF per square micron)
  ///
  double getCapacitance();
  void setCapacitance(double cap);

  ///
  /// Get/Set the edge capacitance (pF per micron)
  ///
  double getEdgeCapacitance();
  void setEdgeCapacitance(double cap);

  ///
  /// Get/Set the wire extension
  ///
  uint32_t getWireExtension();
  void setWireExtension(uint32_t ext);

  ///
  /// Get mask-order number of this layer.
  ///
  int getNumber() const;

  ///
  /// Get routing-level of this routing layer. The routing level
  /// is from [1-num_layers]. This function returns 0, if this
  /// layer is not a routing layer.
  ///
  /// This layer is really intended for signal routing.  In LEF you
  /// can have layers that have "TYPE ROUTING" but aren't really
  /// for routing signal nets (e.g. MIMCAP, STACKEDMIMCAP).
  /// These layers will return zero.
  ///
  int getRoutingLevel();

  ///
  /// Get the layer below this layer.
  /// Returns nullptr if at bottom of layer stack.
  ///
  dbTechLayer* getLowerLayer();

  ///
  /// Get the layer above this layer.
  /// Returns nullptr if at top of layer stack.
  ///
  dbTechLayer* getUpperLayer();

  ///
  /// Get the technology this layer belongs too.
  ///
  dbTech* getTech() const;

  bool hasOrthSpacingTable() const;

  void addOrthSpacingTableEntry(int within, int spacing);

  ///
  /// Create a new layer. The mask order is implicit in the create order.
  /// Returns nullptr if a layer with this name already exists
  ///
  static dbTechLayer* create(dbTech* tech,
                             const char* name,
                             dbTechLayerType type);

  ///
  /// Translate a database-id back to a pointer.
  ///
  static dbTechLayer* getTechLayer(dbTech* tech, uint32_t oid);
  // User Code End dbTechLayer
};

class dbTechLayerAreaRule : public dbObject
{
 public:
  void setArea(int area);

  int getArea() const;

  void setExceptMinWidth(int except_min_width);

  int getExceptMinWidth() const;

  void setExceptEdgeLength(int except_edge_length);

  int getExceptEdgeLength() const;

  void setExceptEdgeLengths(const std::pair<int, int>& except_edge_lengths);

  std::pair<int, int> getExceptEdgeLengths() const;

  void setExceptMinSize(const std::pair<int, int>& except_min_size);

  std::pair<int, int> getExceptMinSize() const;

  void setExceptStep(const std::pair<int, int>& except_step);

  std::pair<int, int> getExceptStep() const;

  void setMask(int mask);

  int getMask() const;

  void setRectWidth(int rect_width);

  int getRectWidth() const;

  void setExceptRectangle(bool except_rectangle);

  bool isExceptRectangle() const;

  void setOverlap(uint32_t overlap);

  uint32_t getOverlap() const;

  // User Code Begin dbTechLayerAreaRule

  static dbTechLayerAreaRule* create(dbTechLayer* _layer);

  void setTrimLayer(dbTechLayer* trim_layer);

  dbTechLayer* getTrimLayer() const;

  static void destroy(dbTechLayerAreaRule* rule);

  // User Code End dbTechLayerAreaRule
};

class dbTechLayerArraySpacingRule : public dbObject
{
 public:
  void setViaWidth(int via_width);

  int getViaWidth() const;

  void setCutSpacing(int cut_spacing);

  int getCutSpacing() const;

  void setWithin(int within);

  int getWithin() const;

  void setArrayWidth(int array_width);

  int getArrayWidth() const;

  void setCutClass(dbTechLayerCutClassRule* cut_class);

  void setParallelOverlap(bool parallel_overlap);

  bool isParallelOverlap() const;

  void setLongArray(bool long_array);

  bool isLongArray() const;

  void setViaWidthValid(bool via_width_valid);

  bool isViaWidthValid() const;

  void setWithinValid(bool within_valid);

  bool isWithinValid() const;

  // User Code Begin dbTechLayerArraySpacingRule

  void setCutsArraySpacing(int num_cuts, int spacing);

  const std::map<int, int>& getCutsArraySpacing() const;

  dbTechLayerCutClassRule* getCutClass() const;

  static dbTechLayerArraySpacingRule* create(dbTechLayer* layer);

  static dbTechLayerArraySpacingRule* getTechLayerArraySpacingRule(
      dbTechLayer* inly,
      uint32_t dbid);

  static void destroy(dbTechLayerArraySpacingRule* rule);

  // User Code End dbTechLayerArraySpacingRule
};

class dbTechLayerCornerSpacingRule : public dbObject
{
 public:
  enum CornerType
  {
    CONVEXCORNER,
    CONCAVECORNER
  };

  void setWithin(int within);

  int getWithin() const;

  void setEolWidth(int eol_width);

  int getEolWidth() const;

  void setJogLength(int jog_length);

  int getJogLength() const;

  void setEdgeLength(int edge_length);

  int getEdgeLength() const;

  void setMinLength(int min_length);

  int getMinLength() const;

  void setExceptNotchLength(int except_notch_length);

  int getExceptNotchLength() const;

  void setSameMask(bool same_mask);

  bool isSameMask() const;

  void setCornerOnly(bool corner_only);

  bool isCornerOnly() const;

  void setExceptEol(bool except_eol);

  bool isExceptEol() const;

  void setExceptJogLength(bool except_jog_length);

  bool isExceptJogLength() const;

  void setEdgeLengthValid(bool edge_length_valid);

  bool isEdgeLengthValid() const;

  void setIncludeShape(bool include_shape);

  bool isIncludeShape() const;

  void setMinLengthValid(bool min_length_valid);

  bool isMinLengthValid() const;

  void setExceptNotch(bool except_notch);

  bool isExceptNotch() const;

  void setExceptNotchLengthValid(bool except_notch_length_valid);

  bool isExceptNotchLengthValid() const;

  void setExceptSameNet(bool except_same_net);

  bool isExceptSameNet() const;

  void setExceptSameMetal(bool except_same_metal);

  bool isExceptSameMetal() const;

  void setCornerToCorner(bool corner_to_corner);

  bool isCornerToCorner() const;

  // User Code Begin dbTechLayerCornerSpacingRule
  void setType(CornerType _type);

  CornerType getType() const;

  void addSpacing(uint32_t width, uint32_t spacing1, uint32_t spacing2 = 0);

  void getSpacingTable(std::vector<std::pair<int, int>>& tbl);

  void getWidthTable(std::vector<int>& tbl);

  static dbTechLayerCornerSpacingRule* create(dbTechLayer* layer);

  static dbTechLayerCornerSpacingRule* getTechLayerCornerSpacingRule(
      dbTechLayer* inly,
      uint32_t dbid);
  static void destroy(dbTechLayerCornerSpacingRule* rule);
  // User Code End dbTechLayerCornerSpacingRule
};

class dbTechLayerCutClassRule : public dbObject
{
 public:
  const char* getName() const;

  void setWidth(int width);

  int getWidth() const;

  void setLength(int length);

  int getLength() const;

  void setNumCuts(int num_cuts);

  int getNumCuts() const;

  void setLengthValid(bool length_valid);

  bool isLengthValid() const;

  void setCutsValid(bool cuts_valid);

  bool isCutsValid() const;

  // User Code Begin dbTechLayerCutClassRule
  static dbTechLayerCutClassRule* getTechLayerCutClassRule(dbTechLayer* inly,
                                                           uint32_t dbid);

  static dbTechLayerCutClassRule* create(dbTechLayer* _layer, const char* name);

  static void destroy(dbTechLayerCutClassRule* rule);
  // User Code End dbTechLayerCutClassRule
};

class dbTechLayerCutEnclosureRule : public dbObject
{
 public:
  enum ENC_TYPE
  {
    DEFAULT,
    EOL,
    ENDSIDE,
    HORZ_AND_VERT
  };

  // User Code Begin dbTechLayerCutEnclosureRuleEnums
  /*
  ENC_TYPE describes the enclosure overhang values as following (from the
  lefdefref):
  * DEFAULT       : overhang1 overhang2
  * EOL           : EOL eolWidth ... eolOverhang otherOverhang ...
  * ENDSIDE       : END overhang1 SIDE overhang2
  * HORZ_AND_VERT : HORIZONTAL overhang1 VERTICAL overhang2
  */
  // User Code End dbTechLayerCutEnclosureRuleEnums

  void setCutClass(dbTechLayerCutClassRule* cut_class);

  dbTechLayerCutClassRule* getCutClass() const;

  void setEolWidth(int eol_width);

  int getEolWidth() const;

  void setEolMinLength(int eol_min_length);

  int getEolMinLength() const;

  void setFirstOverhang(int first_overhang);

  int getFirstOverhang() const;

  void setSecondOverhang(int second_overhang);

  int getSecondOverhang() const;

  void setSpacing(int spacing);

  int getSpacing() const;

  void setExtension(int extension);

  int getExtension() const;

  void setForwardExtension(int forward_extension);

  int getForwardExtension() const;

  void setBackwardExtension(int backward_extension);

  int getBackwardExtension() const;

  void setMinWidth(int min_width);

  int getMinWidth() const;

  void setCutWithin(int cut_within);

  int getCutWithin() const;

  void setMinLength(int min_length);

  int getMinLength() const;

  void setParLength(int par_length);

  int getParLength() const;

  void setSecondParLength(int second_par_length);

  int getSecondParLength() const;

  void setParWithin(int par_within);

  int getParWithin() const;

  void setSecondParWithin(int second_par_within);

  int getSecondParWithin() const;

  void setBelowEnclosure(int below_enclosure);

  int getBelowEnclosure() const;

  void setNumCorners(uint32_t num_corners);

  uint32_t getNumCorners() const;

  void setCutClassValid(bool cut_class_valid);

  bool isCutClassValid() const;

  void setAbove(bool above);

  bool isAbove() const;

  void setBelow(bool below);

  bool isBelow() const;

  void setEolMinLengthValid(bool eol_min_length_valid);

  bool isEolMinLengthValid() const;

  void setEolOnly(bool eol_only);

  bool isEolOnly() const;

  void setShortEdgeOnEol(bool short_edge_on_eol);

  bool isShortEdgeOnEol() const;

  void setSideSpacingValid(bool side_spacing_valid);

  bool isSideSpacingValid() const;

  void setEndSpacingValid(bool end_spacing_valid);

  bool isEndSpacingValid() const;

  void setOffCenterLine(bool off_center_line);

  bool isOffCenterLine() const;

  void setWidthValid(bool width_valid);

  bool isWidthValid() const;

  void setIncludeAbutted(bool include_abutted);

  bool isIncludeAbutted() const;

  void setExceptExtraCut(bool except_extra_cut);

  bool isExceptExtraCut() const;

  void setPrl(bool prl);

  bool isPrl() const;

  void setNoSharedEdge(bool no_shared_edge);

  bool isNoSharedEdge() const;

  void setLengthValid(bool length_valid);

  bool isLengthValid() const;

  void setExtraCutValid(bool extra_cut_valid);

  bool isExtraCutValid() const;

  void setExtraOnly(bool extra_only);

  bool isExtraOnly() const;

  void setRedundantCutValid(bool redundant_cut_valid);

  bool isRedundantCutValid() const;

  void setParallelValid(bool parallel_valid);

  bool isParallelValid() const;

  void setSecondParallelValid(bool second_parallel_valid);

  bool isSecondParallelValid() const;

  void setSecondParWithinValid(bool second_par_within_valid);

  bool isSecondParWithinValid() const;

  void setBelowEnclosureValid(bool below_enclosure_valid);

  bool isBelowEnclosureValid() const;

  void setConcaveCornersValid(bool concave_corners_valid);

  bool isConcaveCornersValid() const;

  // User Code Begin dbTechLayerCutEnclosureRule
  void setType(ENC_TYPE type);

  ENC_TYPE getType() const;

  static dbTechLayerCutEnclosureRule* create(dbTechLayer* layer);

  static dbTechLayerCutEnclosureRule* getTechLayerCutEnclosureRule(
      dbTechLayer* inly,
      uint32_t dbid);
  static void destroy(dbTechLayerCutEnclosureRule* rule);
  // User Code End dbTechLayerCutEnclosureRule
};

class dbTechLayerCutSpacingRule : public dbObject
{
 public:
  enum CutSpacingType
  {
    NONE,
    MAXXY,
    SAMEMASK,
    LAYER,
    ADJACENTCUTS,
    PARALLELOVERLAP,
    PARALLELWITHIN,
    SAMEMETALSHAREDEDGE,
    AREA
  };

  void setCutSpacing(int cut_spacing);

  int getCutSpacing() const;

  void setSecondLayer(dbTechLayer* second_layer);

  void setOrthogonalSpacing(int orthogonal_spacing);

  int getOrthogonalSpacing() const;

  void setWidth(int width);

  int getWidth() const;

  void setEnclosure(int enclosure);

  int getEnclosure() const;

  void setEdgeLength(int edge_length);

  int getEdgeLength() const;

  void setParWithin(int par_within);

  int getParWithin() const;

  void setParEnclosure(int par_enclosure);

  int getParEnclosure() const;

  void setEdgeEnclosure(int edge_enclosure);

  int getEdgeEnclosure() const;

  void setAdjEnclosure(int adj_enclosure);

  int getAdjEnclosure() const;

  void setAboveEnclosure(int above_enclosure);

  int getAboveEnclosure() const;

  void setAboveWidth(int above_width);

  int getAboveWidth() const;

  void setMinLength(int min_length);

  int getMinLength() const;

  void setExtension(int extension);

  int getExtension() const;

  void setEolWidth(int eol_width);

  int getEolWidth() const;

  void setNumCuts(uint32_t num_cuts);

  uint32_t getNumCuts() const;

  void setWithin(int within);

  int getWithin() const;

  void setSecondWithin(int second_within);

  int getSecondWithin() const;

  void setCutClass(dbTechLayerCutClassRule* cut_class);

  void setTwoCuts(uint32_t two_cuts);

  uint32_t getTwoCuts() const;

  void setPrl(uint32_t prl);

  uint32_t getPrl() const;

  void setParLength(uint32_t par_length);

  uint32_t getParLength() const;

  void setCutArea(int cut_area);

  int getCutArea() const;

  void setCenterToCenter(bool center_to_center);

  bool isCenterToCenter() const;

  void setSameNet(bool same_net);

  bool isSameNet() const;

  void setSameMetal(bool same_metal);

  bool isSameMetal() const;

  void setSameVia(bool same_via);

  bool isSameVia() const;

  void setStack(bool stack);

  bool isStack() const;

  void setOrthogonalSpacingValid(bool orthogonal_spacing_valid);

  bool isOrthogonalSpacingValid() const;

  void setAboveWidthEnclosureValid(bool above_width_enclosure_valid);

  bool isAboveWidthEnclosureValid() const;

  void setShortEdgeOnly(bool short_edge_only);

  bool isShortEdgeOnly() const;

  void setConcaveCornerWidth(bool concave_corner_width);

  bool isConcaveCornerWidth() const;

  void setConcaveCornerParallel(bool concave_corner_parallel);

  bool isConcaveCornerParallel() const;

  void setConcaveCornerEdgeLength(bool concave_corner_edge_length);

  bool isConcaveCornerEdgeLength() const;

  void setConcaveCorner(bool concave_corner);

  bool isConcaveCorner() const;

  void setExtensionValid(bool extension_valid);

  bool isExtensionValid() const;

  void setNonEolConvexCorner(bool non_eol_convex_corner);

  bool isNonEolConvexCorner() const;

  void setEolWidthValid(bool eol_width_valid);

  bool isEolWidthValid() const;

  void setMinLengthValid(bool min_length_valid);

  bool isMinLengthValid() const;

  void setAboveWidthValid(bool above_width_valid);

  bool isAboveWidthValid() const;

  void setMaskOverlap(bool mask_overlap);

  bool isMaskOverlap() const;

  void setWrongDirection(bool wrong_direction);

  bool isWrongDirection() const;

  void setAdjacentCuts(uint32_t adjacent_cuts);

  uint32_t getAdjacentCuts() const;

  void setExactAligned(bool exact_aligned);

  bool isExactAligned() const;

  void setCutClassToAll(bool cut_class_to_all);

  bool isCutClassToAll() const;

  void setNoPrl(bool no_prl);

  bool isNoPrl() const;

  void setSameMask(bool same_mask);

  bool isSameMask() const;

  void setExceptSamePgnet(bool except_same_pgnet);

  bool isExceptSamePgnet() const;

  void setSideParallelOverlap(bool side_parallel_overlap);

  bool isSideParallelOverlap() const;

  void setExceptSameNet(bool except_same_net);

  bool isExceptSameNet() const;

  void setExceptSameMetal(bool except_same_metal);

  bool isExceptSameMetal() const;

  void setExceptSameMetalOverlap(bool except_same_metal_overlap);

  bool isExceptSameMetalOverlap() const;

  void setExceptSameVia(bool except_same_via);

  bool isExceptSameVia() const;

  void setAbove(bool above);

  bool isAbove() const;

  void setExceptTwoEdges(bool except_two_edges);

  bool isExceptTwoEdges() const;

  void setTwoCutsValid(bool two_cuts_valid);

  bool isTwoCutsValid() const;

  void setSameCut(bool same_cut);

  bool isSameCut() const;

  void setLongEdgeOnly(bool long_edge_only);

  bool isLongEdgeOnly() const;

  void setPrlValid(bool prl_valid);

  bool isPrlValid() const;

  void setBelow(bool below);

  bool isBelow() const;

  void setParWithinEnclosureValid(bool par_within_enclosure_valid);

  bool isParWithinEnclosureValid() const;

  // User Code Begin dbTechLayerCutSpacingRule
  dbTechLayerCutClassRule* getCutClass() const;

  dbTechLayer* getSecondLayer() const;

  dbTechLayer* getTechLayer() const;

  void setType(CutSpacingType _type);

  CutSpacingType getType() const;

  static dbTechLayerCutSpacingRule* getTechLayerCutSpacingRule(
      dbTechLayer* inly,
      uint32_t dbid);

  static dbTechLayerCutSpacingRule* create(dbTechLayer* _layer);

  static void destroy(dbTechLayerCutSpacingRule* rule);
  // User Code End dbTechLayerCutSpacingRule
};

class dbTechLayerCutSpacingTableDefRule : public dbObject
{
 public:
  enum LOOKUP_STRATEGY
  {
    FIRST,
    SECOND,
    MAX,
    MIN
  };

  // User Code Begin dbTechLayerCutSpacingTableDefRuleEnums
  /*
  LOOKUP_STRATEGY:
  * FIRST     : first spacing value
  * SECOND    : second spacing value
  * MAX       : max spacing value
  * MIN       : min spacing value
  */
  // User Code End dbTechLayerCutSpacingTableDefRuleEnums

  void setDefault(int spacing);

  int getDefault() const;

  void setSecondLayer(dbTechLayer* second_layer);

  void setPrl(int prl);

  int getPrl() const;

  void setExtension(int extension);

  int getExtension() const;

  void setDefaultValid(bool default_valid);

  bool isDefaultValid() const;

  void setSameMask(bool same_mask);

  bool isSameMask() const;

  void setSameNet(bool same_net);

  bool isSameNet() const;

  void setSameMetal(bool same_metal);

  bool isSameMetal() const;

  void setSameVia(bool same_via);

  bool isSameVia() const;

  void setLayerValid(bool layer_valid);

  bool isLayerValid() const;

  void setNoStack(bool no_stack);

  bool isNoStack() const;

  void setNonZeroEnclosure(bool non_zero_enclosure);

  bool isNonZeroEnclosure() const;

  void setPrlForAlignedCut(bool prl_for_aligned_cut);

  bool isPrlForAlignedCut() const;

  void setCenterToCenterValid(bool center_to_center_valid);

  bool isCenterToCenterValid() const;

  void setCenterAndEdgeValid(bool center_and_edge_valid);

  bool isCenterAndEdgeValid() const;

  void setNoPrl(bool no_prl);

  bool isNoPrl() const;

  void setPrlValid(bool prl_valid);

  bool isPrlValid() const;

  void setMaxXY(bool max_x_y);

  bool isMaxXY() const;

  void setEndExtensionValid(bool end_extension_valid);

  bool isEndExtensionValid() const;

  void setSideExtensionValid(bool side_extension_valid);

  bool isSideExtensionValid() const;

  void setExactAlignedSpacingValid(bool exact_aligned_spacing_valid);

  bool isExactAlignedSpacingValid() const;

  void setHorizontal(bool horizontal);

  bool isHorizontal() const;

  void setPrlHorizontal(bool prl_horizontal);

  bool isPrlHorizontal() const;

  void setVertical(bool vertical);

  bool isVertical() const;

  void setPrlVertical(bool prl_vertical);

  bool isPrlVertical() const;

  void setNonOppositeEnclosureSpacingValid(
      bool non_opposite_enclosure_spacing_valid);

  bool isNonOppositeEnclosureSpacingValid() const;

  void setOppositeEnclosureResizeSpacingValid(
      bool opposite_enclosure_resize_spacing_valid);

  bool isOppositeEnclosureResizeSpacingValid() const;

  // User Code Begin dbTechLayerCutSpacingTableDefRule
  void addPrlForAlignedCutEntry(const std::string& from, const std::string& to);

  void addCenterToCenterEntry(const std::string& from, const std::string& to);

  void addCenterAndEdgeEntry(const std::string& from, const std::string& to);

  void addPrlEntry(const std::string& from, const std::string& to, int ccPrl);

  void addEndExtensionEntry(const std::string& cls, int ext);

  void addSideExtensionEntry(const std::string& cls, int ext);

  void addExactElignedEntry(const std::string& cls, int spacing);

  void addNonOppEncSpacingEntry(const std::string& cls, int spacing);

  void addOppEncSpacingEntry(const std::string& cls,
                             int rsz1,
                             int rsz2,
                             int spacing);

  dbTechLayer* getSecondLayer() const;

  bool isCenterToCenter(const std::string& cutClass1,
                        const std::string& cutClass2);

  bool isCenterAndEdge(const std::string& cutClass1,
                       const std::string& cutClass2);

  bool isPrlForAlignedCutClasses(const std::string& cutClass1,
                                 const std::string& cutClass2);

  int getPrlEntry(const std::string& cutClass1, const std::string& cutClass2);

  void setSpacingTable(
      const std::vector<std::vector<std::pair<int, int>>>& table,
      const std::map<std::string, uint32_t>& row_map,
      const std::map<std::string, uint32_t>& col_map);

  void getSpacingTable(std::vector<std::vector<std::pair<int, int>>>& table,
                       std::map<std::string, uint32_t>& row_map,
                       std::map<std::string, uint32_t>& col_map);

  int getMaxSpacing(std::string cutClass, bool SIDE) const;

  int getExactAlignedSpacing(const std::string& cutClass) const;

  int getMaxSpacing(const std::string& cutClass1,
                    const std::string& cutClass2,
                    LOOKUP_STRATEGY strategy = MAX) const;

  int getSpacing(std::string class1,
                 bool SIDE1,
                 std::string class2,
                 bool SIDE2,
                 LOOKUP_STRATEGY strategy = MAX) const;

  dbTechLayer* getTechLayer() const;

  static dbTechLayerCutSpacingTableDefRule* create(dbTechLayer* parent);

  static dbTechLayerCutSpacingTableDefRule*
  getTechLayerCutSpacingTableDefSubRule(dbTechLayer* parent, uint32_t dbid);

  static void destroy(dbTechLayerCutSpacingTableDefRule* rule);
  // User Code End dbTechLayerCutSpacingTableDefRule
};

class dbTechLayerCutSpacingTableOrthRule : public dbObject
{
 public:
  void getSpacingTable(std::vector<std::pair<int, int>>& tbl) const;

  // User Code Begin dbTechLayerCutSpacingTableOrthRule
  void setSpacingTable(const std::vector<std::pair<int, int>>& tbl);

  static dbTechLayerCutSpacingTableOrthRule* create(dbTechLayer* parent);

  static dbTechLayerCutSpacingTableOrthRule*
  getTechLayerCutSpacingTableOrthSubRule(dbTechLayer* parent, uint32_t dbid);

  static void destroy(dbTechLayerCutSpacingTableOrthRule* rule);
  // User Code End dbTechLayerCutSpacingTableOrthRule
};

class dbTechLayerEolExtensionRule : public dbObject
{
 public:
  void setSpacing(int spacing);

  int getSpacing() const;

  void getExtensionTable(std::vector<std::pair<int, int>>& tbl) const;

  void setParallelOnly(bool parallel_only);

  bool isParallelOnly() const;

  // User Code Begin dbTechLayerEolExtensionRule

  void addEntry(int eol, int ext);

  static dbTechLayerEolExtensionRule* create(dbTechLayer* layer);

  static dbTechLayerEolExtensionRule* getTechLayerEolExtensionRule(
      dbTechLayer* inly,
      uint32_t dbid);

  static void destroy(dbTechLayerEolExtensionRule* rule);
  // User Code End dbTechLayerEolExtensionRule
};

class dbTechLayerEolKeepOutRule : public dbObject
{
 public:
  void setEolWidth(int eol_width);

  int getEolWidth() const;

  void setBackwardExt(int backward_ext);

  int getBackwardExt() const;

  void setForwardExt(int forward_ext);

  int getForwardExt() const;

  void setSideExt(int side_ext);

  int getSideExt() const;

  void setWithinLow(int within_low);

  int getWithinLow() const;

  void setWithinHigh(int within_high);

  int getWithinHigh() const;

  void setClassName(const std::string& class_name);

  std::string getClassName() const;

  void setClassValid(bool class_valid);

  bool isClassValid() const;

  void setCornerOnly(bool corner_only);

  bool isCornerOnly() const;

  void setExceptWithin(bool except_within);

  bool isExceptWithin() const;

  // User Code Begin dbTechLayerEolKeepOutRule
  static dbTechLayerEolKeepOutRule* create(dbTechLayer* layer);

  static dbTechLayerEolKeepOutRule* getTechLayerEolKeepOutRule(
      dbTechLayer* inly,
      uint32_t dbid);
  static void destroy(dbTechLayerEolKeepOutRule* rule);
  // User Code End dbTechLayerEolKeepOutRule
};

class dbTechLayerForbiddenSpacingRule : public dbObject
{
 public:
  void setForbiddenSpacing(const std::pair<int, int>& forbidden_spacing);

  std::pair<int, int> getForbiddenSpacing() const;

  void setWidth(int width);

  int getWidth() const;

  void setWithin(int within);

  int getWithin() const;

  void setPrl(int prl);

  int getPrl() const;

  void setTwoEdges(int two_edges);

  int getTwoEdges() const;

  // User Code Begin dbTechLayerForbiddenSpacingRule

  bool hasWidth();

  bool hasWithin();

  bool hasPrl();

  bool hasTwoEdges();

  static dbTechLayerForbiddenSpacingRule* create(dbTechLayer* _layer);

  static void destroy(dbTechLayerForbiddenSpacingRule* rule);

  // User Code End dbTechLayerForbiddenSpacingRule
};

class dbTechLayerKeepOutZoneRule : public dbObject
{
 public:
  void setFirstCutClass(const std::string& first_cut_class);

  std::string getFirstCutClass() const;

  void setSecondCutClass(const std::string& second_cut_class);

  std::string getSecondCutClass() const;

  void setAlignedSpacing(int aligned_spacing);

  int getAlignedSpacing() const;

  void setSideExtension(int side_extension);

  int getSideExtension() const;

  void setForwardExtension(int forward_extension);

  int getForwardExtension() const;

  void setEndSideExtension(int end_side_extension);

  int getEndSideExtension() const;

  void setEndForwardExtension(int end_forward_extension);

  int getEndForwardExtension() const;

  void setSideSideExtension(int side_side_extension);

  int getSideSideExtension() const;

  void setSideForwardExtension(int side_forward_extension);

  int getSideForwardExtension() const;

  void setSpiralExtension(int spiral_extension);

  int getSpiralExtension() const;

  void setSameMask(bool same_mask);

  bool isSameMask() const;

  void setSameMetal(bool same_metal);

  bool isSameMetal() const;

  void setDiffMetal(bool diff_metal);

  bool isDiffMetal() const;

  void setExceptAlignedSide(bool except_aligned_side);

  bool isExceptAlignedSide() const;

  void setExceptAlignedEnd(bool except_aligned_end);

  bool isExceptAlignedEnd() const;

  // User Code Begin dbTechLayerKeepOutZoneRule

  static dbTechLayerKeepOutZoneRule* create(dbTechLayer* _layer);

  static void destroy(dbTechLayerKeepOutZoneRule* rule);

  // User Code End dbTechLayerKeepOutZoneRule
};

class dbTechLayerMaxSpacingRule : public dbObject
{
 public:
  void setCutClass(const std::string& cut_class);

  std::string getCutClass() const;

  void setMaxSpacing(int max_spacing);

  int getMaxSpacing() const;

  // User Code Begin dbTechLayerMaxSpacingRule
  bool hasCutClass() const;

  static dbTechLayerMaxSpacingRule* create(dbTechLayer* _layer);

  static void destroy(dbTechLayerMaxSpacingRule* rule);

  // User Code End dbTechLayerMaxSpacingRule
};

class dbTechLayerMinCutRule : public dbObject
{
 public:
  void setNumCuts(int num_cuts);

  int getNumCuts() const;

  std::map<std::string, int> getCutClassCutsMap() const;

  void setWidth(int width);

  int getWidth() const;

  void setWithinCutDist(int within_cut_dist);

  int getWithinCutDist() const;

  void setLength(int length);

  int getLength() const;

  void setLengthWithinDist(int length_within_dist);

  int getLengthWithinDist() const;

  void setArea(int area);

  int getArea() const;

  void setAreaWithinDist(int area_within_dist);

  int getAreaWithinDist() const;

  void setPerCutClass(bool per_cut_class);

  bool isPerCutClass() const;

  void setWithinCutDistValid(bool within_cut_dist_valid);

  bool isWithinCutDistValid() const;

  void setFromAbove(bool from_above);

  bool isFromAbove() const;

  void setFromBelow(bool from_below);

  bool isFromBelow() const;

  void setLengthValid(bool length_valid);

  bool isLengthValid() const;

  void setAreaValid(bool area_valid);

  bool isAreaValid() const;

  void setAreaWithinDistValid(bool area_within_dist_valid);

  bool isAreaWithinDistValid() const;

  void setSameMetalOverlap(bool same_metal_overlap);

  bool isSameMetalOverlap() const;

  void setFullyEnclosed(bool fully_enclosed);

  bool isFullyEnclosed() const;

  // User Code Begin dbTechLayerMinCutRule

  void setCutsPerCutClass(const std::string& cut_class, int num_cuts);

  static dbTechLayerMinCutRule* create(dbTechLayer* layer);

  static dbTechLayerMinCutRule* getTechLayerMinCutRule(dbTechLayer* inly,
                                                       uint32_t dbid);

  static void destroy(dbTechLayerMinCutRule* rule);

  // User Code End dbTechLayerMinCutRule
};

class dbTechLayerMinStepRule : public dbObject
{
 public:
  void setMinStepLength(int min_step_length);

  int getMinStepLength() const;

  void setMaxEdges(uint32_t max_edges);

  uint32_t getMaxEdges() const;

  void setMinAdjLength1(int min_adj_length1);

  int getMinAdjLength1() const;

  void setMinAdjLength2(int min_adj_length2);

  int getMinAdjLength2() const;

  void setEolWidth(int eol_width);

  int getEolWidth() const;

  void setMinBetweenLength(int min_between_length);

  int getMinBetweenLength() const;

  void setMaxEdgesValid(bool max_edges_valid);

  bool isMaxEdgesValid() const;

  void setMinAdjLength1Valid(bool min_adj_length1_valid);

  bool isMinAdjLength1Valid() const;

  void setNoBetweenEol(bool no_between_eol);

  bool isNoBetweenEol() const;

  void setMinAdjLength2Valid(bool min_adj_length2_valid);

  bool isMinAdjLength2Valid() const;

  void setConvexCorner(bool convex_corner);

  bool isConvexCorner() const;

  void setMinBetweenLengthValid(bool min_between_length_valid);

  bool isMinBetweenLengthValid() const;

  void setExceptSameCorners(bool except_same_corners);

  bool isExceptSameCorners() const;

  void setConcaveCorner(bool concave_corner);

  bool isConcaveCorner() const;

  void setExceptRectangle(bool except_rectangle);

  bool isExceptRectangle() const;

  void setNoAdjacentEol(bool no_adjacent_eol);

  bool isNoAdjacentEol() const;

  // User Code Begin dbTechLayerMinStepRule
  static dbTechLayerMinStepRule* create(dbTechLayer* layer);

  static dbTechLayerMinStepRule* getTechLayerMinStepRule(dbTechLayer* inly,
                                                         uint32_t dbid);

  static void destroy(dbTechLayerMinStepRule* rule);
  // User Code End dbTechLayerMinStepRule
};

class dbTechLayerSpacingEolRule : public dbObject
{
 public:
  void setEolSpace(int eol_space);

  int getEolSpace() const;

  void setEolWidth(int eol_width);

  int getEolWidth() const;

  void setWrongDirSpace(int wrong_dir_space);

  int getWrongDirSpace() const;

  void setOppositeWidth(int opposite_width);

  int getOppositeWidth() const;

  void setEolWithin(int eol_within);

  int getEolWithin() const;

  void setWrongDirWithin(int wrong_dir_within);

  int getWrongDirWithin() const;

  void setExactWidth(int exact_width);

  int getExactWidth() const;

  void setOtherWidth(int other_width);

  int getOtherWidth() const;

  void setFillTriangle(int fill_triangle);

  int getFillTriangle() const;

  void setCutClass(int cut_class);

  int getCutClass() const;

  void setWithCutSpace(int with_cut_space);

  int getWithCutSpace() const;

  void setEnclosureEndWidth(int enclosure_end_width);

  int getEnclosureEndWidth() const;

  void setEnclosureEndWithin(int enclosure_end_within);

  int getEnclosureEndWithin() const;

  void setEndPrlSpace(int end_prl_space);

  int getEndPrlSpace() const;

  void setEndPrl(int end_prl);

  int getEndPrl() const;

  void setEndToEndSpace(int end_to_end_space);

  int getEndToEndSpace() const;

  void setOneCutSpace(int one_cut_space);

  int getOneCutSpace() const;

  void setTwoCutSpace(int two_cut_space);

  int getTwoCutSpace() const;

  void setExtension(int extension);

  int getExtension() const;

  void setWrongDirExtension(int wrong_dir_extension);

  int getWrongDirExtension() const;

  void setOtherEndWidth(int other_end_width);

  int getOtherEndWidth() const;

  void setMaxLength(int max_length);

  int getMaxLength() const;

  void setMinLength(int min_length);

  int getMinLength() const;

  void setParSpace(int par_space);

  int getParSpace() const;

  void setParWithin(int par_within);

  int getParWithin() const;

  void setParPrl(int par_prl);

  int getParPrl() const;

  void setParMinLength(int par_min_length);

  int getParMinLength() const;

  void setEncloseDist(int enclose_dist);

  int getEncloseDist() const;

  void setCutToMetalSpace(int cut_to_metal_space);

  int getCutToMetalSpace() const;

  void setMinAdjLength(int min_adj_length);

  int getMinAdjLength() const;

  void setMinAdjLength1(int min_adj_length1);

  int getMinAdjLength1() const;

  void setMinAdjLength2(int min_adj_length2);

  int getMinAdjLength2() const;

  void setNotchLength(int notch_length);

  int getNotchLength() const;

  void setExactWidthValid(bool exact_width_valid);

  bool isExactWidthValid() const;

  void setWrongDirSpacingValid(bool wrong_dir_spacing_valid);

  bool isWrongDirSpacingValid() const;

  void setOppositeWidthValid(bool opposite_width_valid);

  bool isOppositeWidthValid() const;

  void setWithinValid(bool within_valid);

  bool isWithinValid() const;

  void setWrongDirWithinValid(bool wrong_dir_within_valid);

  bool isWrongDirWithinValid() const;

  void setSameMaskValid(bool same_mask_valid);

  bool isSameMaskValid() const;

  void setExceptExactWidthValid(bool except_exact_width_valid);

  bool isExceptExactWidthValid() const;

  void setFillConcaveCornerValid(bool fill_concave_corner_valid);

  bool isFillConcaveCornerValid() const;

  void setWithcutValid(bool withcut_valid);

  bool isWithcutValid() const;

  void setCutClassValid(bool cut_class_valid);

  bool isCutClassValid() const;

  void setWithCutAboveValid(bool with_cut_above_valid);

  bool isWithCutAboveValid() const;

  void setEnclosureEndValid(bool enclosure_end_valid);

  bool isEnclosureEndValid() const;

  void setEnclosureEndWithinValid(bool enclosure_end_within_valid);

  bool isEnclosureEndWithinValid() const;

  void setEndPrlSpacingValid(bool end_prl_spacing_valid);

  bool isEndPrlSpacingValid() const;

  void setPrlValid(bool prl_valid);

  bool isPrlValid() const;

  void setEndToEndValid(bool end_to_end_valid);

  bool isEndToEndValid() const;

  void setCutSpacesValid(bool cut_spaces_valid);

  bool isCutSpacesValid() const;

  void setExtensionValid(bool extension_valid);

  bool isExtensionValid() const;

  void setWrongDirExtensionValid(bool wrong_dir_extension_valid);

  bool isWrongDirExtensionValid() const;

  void setOtherEndWidthValid(bool other_end_width_valid);

  bool isOtherEndWidthValid() const;

  void setMaxLengthValid(bool max_length_valid);

  bool isMaxLengthValid() const;

  void setMinLengthValid(bool min_length_valid);

  bool isMinLengthValid() const;

  void setTwoSidesValid(bool two_sides_valid);

  bool isTwoSidesValid() const;

  void setEqualRectWidthValid(bool equal_rect_width_valid);

  bool isEqualRectWidthValid() const;

  void setParallelEdgeValid(bool parallel_edge_valid);

  bool isParallelEdgeValid() const;

  void setSubtractEolWidthValid(bool subtract_eol_width_valid);

  bool isSubtractEolWidthValid() const;

  void setParPrlValid(bool par_prl_valid);

  bool isParPrlValid() const;

  void setParMinLengthValid(bool par_min_length_valid);

  bool isParMinLengthValid() const;

  void setTwoEdgesValid(bool two_edges_valid);

  bool isTwoEdgesValid() const;

  void setSameMetalValid(bool same_metal_valid);

  bool isSameMetalValid() const;

  void setNonEolCornerOnlyValid(bool non_eol_corner_only_valid);

  bool isNonEolCornerOnlyValid() const;

  void setParallelSameMaskValid(bool parallel_same_mask_valid);

  bool isParallelSameMaskValid() const;

  void setEncloseCutValid(bool enclose_cut_valid);

  bool isEncloseCutValid() const;

  void setBelowValid(bool below_valid);

  bool isBelowValid() const;

  void setAboveValid(bool above_valid);

  bool isAboveValid() const;

  void setCutSpacingValid(bool cut_spacing_valid);

  bool isCutSpacingValid() const;

  void setAllCutsValid(bool all_cuts_valid);

  bool isAllCutsValid() const;

  void setToConcaveCornerValid(bool to_concave_corner_valid);

  bool isToConcaveCornerValid() const;

  void setMinAdjacentLengthValid(bool min_adjacent_length_valid);

  bool isMinAdjacentLengthValid() const;

  void setTwoMinAdjLengthValid(bool two_min_adj_length_valid);

  bool isTwoMinAdjLengthValid() const;

  void setToNotchLengthValid(bool to_notch_length_valid);

  bool isToNotchLengthValid() const;

  // User Code Begin dbTechLayerSpacingEolRule
  static dbTechLayerSpacingEolRule* create(dbTechLayer* layer);

  static dbTechLayerSpacingEolRule* getTechLayerSpacingEolRule(
      dbTechLayer* inly,
      uint32_t dbid);

  static void destroy(dbTechLayerSpacingEolRule* rule);
  // User Code End dbTechLayerSpacingEolRule
};

class dbTechLayerSpacingTablePrlRule : public dbObject
{
 public:
  void setEolWidth(int eol_width);

  int getEolWidth() const;

  void setWrongDirection(bool wrong_direction);

  bool isWrongDirection() const;

  void setSameMask(bool same_mask);

  bool isSameMask() const;

  void setExceeptEol(bool exceept_eol);

  bool isExceeptEol() const;

  // User Code Begin dbTechLayerSpacingTablePrlRule
  static dbTechLayerSpacingTablePrlRule* getTechLayerSpacingTablePrlRule(
      dbTechLayer* inly,
      uint32_t dbid);

  static dbTechLayerSpacingTablePrlRule* create(dbTechLayer* _layer);

  static void destroy(dbTechLayerSpacingTablePrlRule* rule);

  void setTable(const std::vector<int>& width_tbl,
                const std::vector<int>& length_tbl,
                const std::vector<std::vector<int>>& spacing_tbl,
                const std::map<uint32_t, std::pair<int, int>>& excluded_map);
  void getTable(std::vector<int>& width_tbl,
                std::vector<int>& length_tbl,
                std::vector<std::vector<int>>& spacing_tbl,
                std::map<uint32_t, std::pair<int, int>>& excluded_map);

  void setSpacingTableInfluence(
      const std::vector<std::tuple<int, int, int>>& influence_tbl);

  int getSpacing(int width, int length) const;

  bool hasExceptWithin(int width) const;

  std::pair<int, int> getExceptWithin(int width) const;

  // User Code End dbTechLayerSpacingTablePrlRule
};

class dbTechLayerTwoWiresForbiddenSpcRule : public dbObject
{
 public:
  void setMinSpacing(int min_spacing);

  int getMinSpacing() const;

  void setMaxSpacing(int max_spacing);

  int getMaxSpacing() const;

  void setMinSpanLength(int min_span_length);

  int getMinSpanLength() const;

  void setMaxSpanLength(int max_span_length);

  int getMaxSpanLength() const;

  void setPrl(int prl);

  int getPrl() const;

  void setMinExactSpanLength(bool min_exact_span_length);

  bool isMinExactSpanLength() const;

  void setMaxExactSpanLength(bool max_exact_span_length);

  bool isMaxExactSpanLength() const;

  // User Code Begin dbTechLayerTwoWiresForbiddenSpcRule
  static dbTechLayerTwoWiresForbiddenSpcRule* create(dbTechLayer* layer);

  static void destroy(dbTechLayerTwoWiresForbiddenSpcRule* rule);
  // User Code End dbTechLayerTwoWiresForbiddenSpcRule
};

class dbTechLayerVoltageSpacing : public dbObject
{
 public:
  void setTocutAbove(bool tocut_above);

  bool isTocutAbove() const;

  void setTocutBelow(bool tocut_below);

  bool isTocutBelow() const;

  // User Code Begin dbTechLayerVoltageSpacing
  const std::map<float, int>& getTable() const;
  void addEntry(float voltage, int spacing);

  static dbTechLayerVoltageSpacing* create(dbTechLayer* layer);

  static void destroy(dbTechLayerVoltageSpacing* rule);
  // User Code End dbTechLayerVoltageSpacing
};

class dbTechLayerWidthTableRule : public dbObject
{
 public:
  void setWrongDirection(bool wrong_direction);

  bool isWrongDirection() const;

  void setOrthogonal(bool orthogonal);

  bool isOrthogonal() const;

  // User Code Begin dbTechLayerWidthTableRule

  void addWidth(int width);

  std::vector<int> getWidthTable() const;

  static dbTechLayerWidthTableRule* create(dbTechLayer* layer);

  static dbTechLayerWidthTableRule* getTechLayerWidthTableRule(
      dbTechLayer* inly,
      uint32_t dbid);

  static void destroy(dbTechLayerWidthTableRule* rule);
  // User Code End dbTechLayerWidthTableRule
};

class dbTechLayerWrongDirSpacingRule : public dbObject
{
 public:
  void setWrongdirSpace(int wrongdir_space);

  int getWrongdirSpace() const;

  void setNoneolWidth(int noneol_width);

  int getNoneolWidth() const;

  void setLength(int length);

  int getLength() const;

  void setPrlLength(int prl_length);

  int getPrlLength() const;

  void setNoneolValid(bool noneol_valid);

  bool isNoneolValid() const;

  void setLengthValid(bool length_valid);

  bool isLengthValid() const;

  // User Code Begin dbTechLayerWrongDirSpacingRule
  static dbTechLayerWrongDirSpacingRule* create(dbTechLayer* layer);

  static dbTechLayerWrongDirSpacingRule* getTechLayerWrongDirSpacingRule(
      dbTechLayer* inly,
      uint32_t dbid);

  static void destroy(dbTechLayerWrongDirSpacingRule* rule);
  // User Code End dbTechLayerWrongDirSpacingRule
};

// Generator Code End ClassDefinition
///
/// dbProperty - Boolean property.
///
class dbBoolProperty : public dbProperty
{
 public:
  /// Get the value of this property.
  bool getValue();

  /// Set the value of this property.
  void setValue(bool value);

  /// Create a bool property. Returns nullptr if a property with the same name
  /// already exists.
  static dbBoolProperty* create(dbObject* object, const char* name, bool value);

  /// Find the named property of type bool. Returns nullptr if the property does
  /// not exist.
  static dbBoolProperty* find(dbObject* object, const char* name);
};

///
/// dbProperty - String property.
///
class dbStringProperty : public dbProperty
{
 public:
  /// Get the value of this property.
  std::string getValue();

  /// Set the value of this property.
  void setValue(const char* value);

  /// Create a string property. Returns nullptr if a property with the same name
  /// already exists.
  static dbStringProperty* create(dbObject* object,
                                  const char* name,
                                  const char* value);

  /// Find the named property of type string. Returns nullptr if the property
  /// does not exist.
  static dbStringProperty* find(dbObject* object, const char* name);
};

///
/// dbProperty - Int property.
///
class dbIntProperty : public dbProperty
{
 public:
  /// Get the value of this property.
  int getValue();

  /// Set the value of this property.
  void setValue(int value);

  /// Create a int property. Returns nullptr if a property with the same name
  /// already exists.
  static dbIntProperty* create(dbObject* object, const char* name, int value);

  /// Find the named property of type int. Returns nullptr if the property does
  /// not exist.
  static dbIntProperty* find(dbObject* object, const char* name);
};

///
/// dbProperty - Double property.
///
class dbDoubleProperty : public dbProperty
{
 public:
  /// Get the value of this property.
  double getValue();

  /// Set the value of this property.
  void setValue(double value);

  /// Create a double property. Returns nullptr if a property with the same name
  /// already exists.
  static dbDoubleProperty* create(dbObject* object,
                                  const char* name,
                                  double value);

  /// Find the named property of type double. Returns nullptr if the property
  /// does not exist.
  static dbDoubleProperty* find(dbObject* object, const char* name);
};

}  // namespace odb

// Overload std::less for these types
#include "odb/dbCompare.inc"  // IWYU pragma: export
