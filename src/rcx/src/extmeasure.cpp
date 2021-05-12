///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#include <dbRtTree.h>

#include "dbUtil.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif
// #define CHECK_SAME_NET
//#define DEBUG_NET 208091
//#define MIN_FOR_LOOPS

namespace rcx {

using utl::RCX;

using odb::dbBlock;
using odb::dbBox;
using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbRtTree;
using odb::dbSet;
using odb::dbShape;
using odb::dbSigType;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbWire;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using odb::gs;
using odb::Rect;
using odb::SEQ;

bool extMeasure::getFirstShape(dbNet* net, dbShape& s) {
  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;
  dbWire* wire = net->getWire();

  bool status = false;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    pitr.getNextShape(pshape);
    s = pshape.shape;
    status = true;
    break;
  }
  return status;
}
bool extMeasure::parse_setLayer(Ath__parser* parser1, uint& layerNum,
                                bool print) {
  if (parser1->isKeyword(0, "(setLayer")) {
    if (print)
      parser1->printWords(stdout);

    layerNum = parser1->getInt(1);
    return true;
  } else if (parser1->isKeyword(0, "setLayer")) {
    if (print)
      parser1->printWords(stdout);

    layerNum = parser1->getInt(1);
    return true;
  }
  return false;
}
#ifdef OLD_READ_QCAP

int extMeasure::readQcap(extMain* extMain, const char* filename,
                         const char* design, const char* capFile,
                         bool skipBterms, dbDatabase* db) {
  bool debug = false;

  uint nm = 1000;

  dbChip* chip = dbChip::create(db);
  assert(chip);
  _block = dbBlock::create(chip, design, '/');
  assert(_block);
  _block->setBusDelimeters('[', ']');
  _block->setDefUnits(nm);

  _extMain = extMain;
  _extMain->_block = _block;
  _create_net_util.setBlock(_block);

  dbTech* tech = db->getTech();

  uint netCnt = 0;
  uint totWireCnt = 0;
  bool layerSectionFlag = false;

  Ath__parser parser1;
  parser1.addSeparator("\r");
  parser1.openFile((char*)filename);

  Ath__parser parserWord;
  parserWord.resetSeparator("=");

  Ath__parser parserParen;
  parserParen.resetSeparator("()");

  Ath__parser ambersandParser;
  ambersandParser.resetSeparator("&");

  while (parser1.parseNextLine() > 0) {
    if (parser1.isKeyword(0, "layer")) {
      if (!layerSectionFlag) {
        logger_->info(RCX, 55, "Reading layer section of file {}", filename);
        layerSectionFlag = true;
      }
      char* layerName = parser1.get(1);

      char* typeWord = parser1.get(4, "type=");
      if (typeWord == NULL)
        continue;
      parserWord.mkWords(typeWord);
      if (!parserWord.isKeyword(1, "interconnect"))
        continue;

      // parserWord.mkWords(parser1.get(4));
      // uint s= parserWord.getDouble(1);

      char* layerNumWord = parser1.get(4, "ID=");
      if (layerNumWord == NULL)
        continue;
      parserWord.mkWords(layerNumWord);
      uint layerNum = parserWord.getInt(1);

      _idTable[layerNum] = 0;  // reset mapping

      if (debug)
        parser1.printWords(stdout);

      dbTechLayer* techLayer = tech->findLayer(layerName);
      if (techLayer == NULL) {
        logger_->warn(
            RCX, 370,
            "Layer {} in line number {} in file {} has not beed defined in "
            "LEF file, will skip all attached geometries",
            layerName, parser1.getLineNum(), filename);
        continue;
      }

      dbTechLayerType type = techLayer->getType();

      if (type.getValue() != dbTechLayerType::ROUTING)
        continue;

      _idTable[layerNum] = techLayer->getRoutingLevel();

      logger_->info(
          RCX, 368,
          "Read layer name {} with number {} that corresponds to routing "
          "level {}",
          layerName, layerNum, _idTable[layerNum]);

      continue;
    }
    if (parser1.isKeyword(0, "net")) {
      char netName[256];
      strcpy(netName, parser1.get(1));

      char mainNetName[256];
      bool subNetFlag = false;
      ambersandParser.mkWords(netName);
      if (ambersandParser.getWordCnt() == 2) {
        subNetFlag = true;
        strcpy(mainNetName, ambersandParser.get(0));
      }

      if (debug)
        parser1.printWords(stdout);

      netCnt++;

      if (parser1.parseNextLine() > 0) {
        uint layerNum = 0;
        if (!parse_setLayer(&parser1, layerNum, debug))
          continue;  // empty nets, TODO warning

        dbRtTree rtTree;
        dbNet* net = NULL;

        uint wireCnt = 0;
        while (parser1.parseNextLine() > 0) {
          if (parse_setLayer(&parser1, layerNum, debug))
            continue;

          _ll[0] = Ath__double2int(parser1.getDouble(0) * nm);
          _ll[1] = Ath__double2int(parser1.getDouble(1) * nm);
          _ur[0] = Ath__double2int(parser1.getDouble(2) * nm);

          char* w3 = parser1.get(3);
          parserParen.mkWords(w3);
          _ur[1] = Ath__double2int(parserParen.getDouble(0) * nm);

          if (debug)
            parser1.printWords(stdout);

          uint level = _idTable[layerNum];
          if (level == 0) {
            logger_->info(RCX, 366,
                          "Skipping net {}, layer num {} not defined in LEF",
                          netName, layerNum);
          } else if (wireCnt == 0) {
            dbNet* mainNet = NULL;
            if (!subNetFlag) {
              net = _create_net_util.createNetSingleWire(
                  netName, _ll[0], _ll[1], _ur[0], _ur[1], level);
            } else {
              mainNet = _block->findNet(mainNetName);
              if (mainNet == NULL)
                net = _create_net_util.createNetSingleWire(
                    mainNetName, _ll[0], _ll[1], _ur[0], _ur[1], level);
              else
                net = _create_net_util.createNetSingleWire(
                    netName, _ll[0], _ll[1], _ur[0], _ur[1], level,
                    true /*skipBterms*/);
            }
            dbShape s;
            if ((net != NULL) && getFirstShape(net, s)) {
              if (debug) {
                logger_->info(RCX, 363, "\t\tCreated net {} : {} {}   {} {}",
                              net->getConstName(), s.xMin(), s.yMin(), s.xMax(),
                              s.yMax());
              }
              if (!subNetFlag) {
                dbWire* w1 = net->getWire();
                rtTree.decode(w1);
              } else {
                if (mainNet != NULL) {
                  dbRtTree m;
                  m.decode(mainNet->getWire());
                  dbRtTree s;
                  s.decode(net->getWire());
                  m.move(&s);
                  m.encode(mainNet->getWire());
                  dbNet::destroy(net);
                } else {
                  dbRtTree s;
                  s.decode(net->getWire());
                  s.encode(net->getWire());
                }
              }
            }
          } else if (wireCnt > 0) {
            char buff[1024];
            sprintf(buff, "%s__w%d_line%d", netName, wireCnt + 1,
                    parser1.getLineNum());

            dbNet* net1 = _create_net_util.createNetSingleWire(
                buff, _ll[0], _ll[1], _ur[0], _ur[1], level,
                true /*skipBterms*/);

            dbShape s;
            if ((net1 != NULL) && getFirstShape(net1, s)) {
              if (debug) {
                logger_->info(RCX, 256, "\t\tCreated net {} : {} {}   {} {}",
                              net1->getConstName(), s.xMin(), s.yMin(),
                              s.xMax(), s.yMax());
              }

              dbWire* wire = net1->getWire();
              dbRtTree T;
              T.decode(wire);
              rtTree.move(&T);
              dbNet::destroy(net1);
            }
          }
          uint n = strlen(w3);
          if (w3[n - 1] == ')') {
            if ((wireCnt > 0) && !subNetFlag) {
              rtTree.encode(net->getWire());
            }
            totWireCnt += wireCnt + 1;
            if (netCnt % 1000 == 0)
              logger_->info(RCX, 361, "Have read {} nets and {} wires", netCnt,
                            totWireCnt);

            break;
          }

          wireCnt++;
        }
        continue;
      }
    }
  }
  if (netCnt > 0) {
    dbBox* bb = _block->getBBox();
    Rect r(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
    _block->setDieArea(r);

    logger_->info(RCX, 51, "Have read {} nets from file {}", netCnt, filename);
  } else {
    logger_->warn(RCX, 70, "No nets were read from file {}", filename);
  }
  uint ccCnt = 0;
  int gndCnt = 0;
  if (capFile != NULL) {
    _extMain->_noModelRC = true;
    _extMain->_cornerCnt = 1;
    _extMain->_extDbCnt = 1;
    _block->setCornerCount(_extMain->_cornerCnt);
    _extMain->setupMapping(0);

    dbSet<dbNet> nets = _block->getNets();
    dbSet<dbNet>::iterator net_itr;
    dbNet* net;
    for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
      net = *net_itr;
      _extMain->makeNetRCsegs(net);
    }
    gndCnt = readCapFile(capFile, ccCnt);
  }
  logger_->info(RCX, 360, "Have created {} gnd caps and {} cc caps", gndCnt,
                ccCnt);

  return netCnt;
}
#endif
dbNet* extMeasure::createSingleWireNet(char* name, uint level, bool viaFlag,
                                       bool debug, bool skipVias,
                                       bool skipBterms) {
  if (viaFlag) {
    if (skipVias)
      return NULL;
    return NULL;
  }
  // wire

  dbNet* net = _create_net_util.createNetSingleWire(
      name, _ll[0], _ll[1], _ur[0], _ur[1], level, skipBterms);

  dbShape s;
  if ((net == NULL) || !getFirstShape(net, s)) {
    logger_->warn(RCX, 462, "\t\tCannot create wire: {} {}   {} {} for name {}",
                  s.xMin(), s.yMin(), s.xMax(), s.yMax(), name);
    return NULL;
  }
  if (debug) {
    logger_->info(RCX, 371, "\t\tCreated net {} : {} {}   {} {}",
                  net->getConstName(), s.xMin(), s.yMin(), s.xMax(), s.yMax());
  }
  return net;
}

int extMeasure::readQcap(extMain* extMain, const char* filename,
                         const char* design, const char* capFile,
                         bool skipBterms, dbDatabase* db) {
  bool skipVias = true;
  bool debug = false;

  uint nm = 1000;

  dbChip* chip = dbChip::create(db);
  assert(chip);
  _block = dbBlock::create(chip, design, '/');
  assert(_block);
  _block->setBusDelimeters('[', ']');
  _block->setDefUnits(nm);

  _extMain = extMain;
  _extMain->_block = _block;
  _create_net_util.setBlock(_block);

  dbTech* tech = db->getTech();

  bool viaTable[1000];
  int loHeightTable[1000];
  int hiHeightTable[1000];
  for (uint ii = 0; ii < 1000; ii++) {
    loHeightTable[ii] = 0;
    hiHeightTable[ii] = 0;
    viaTable[ii] = false;
  }

  uint netCnt = 0;
  uint totWireCnt = 0;
  bool layerSectionFlag = false;

  Ath__parser parser1;
  parser1.addSeparator("\r");
  parser1.openFile((char*)filename);

  Ath__parser parserWord;
  parserWord.resetSeparator("=");

  Ath__parser parserParen;
  parserParen.resetSeparator("()");

  Ath__parser ambersandParser;
  ambersandParser.resetSeparator("&");

  while (parser1.parseNextLine() > 0) {
    if (parser1.isKeyword(0, "layer")) {
      if (!layerSectionFlag) {
        logger_->info(RCX, 63, "Reading layer section of file {}", filename);
        layerSectionFlag = true;
      }
      char* layerName = parser1.get(1);

      char* typeWord = parser1.get(4, "type=");
      if (typeWord == NULL)
        continue;

      bool viaFlag = false;
      bool intercoonetFlag = false;

      parserWord.mkWords(typeWord);
      if (parserWord.isKeyword(1, "interconnect")) {
        intercoonetFlag = true;
      } else if (parserWord.isKeyword(1, "via")) {
        viaFlag = true;
      }
      char* layerNumWord = parser1.get(4, "ID=");

      if (layerNumWord == NULL) {
        logger_->warn(RCX, 461,
                      "Cannot read layer number for layer name {} at line: {}",
                      layerName, parser1.getLineNum());
        continue;
      }
      parserWord.mkWords(layerNumWord);
      uint layerNum = parserWord.getInt(1);

      _idTable[layerNum] = 0;  // reset mapping

      if (debug)
        parser1.printWords(stdout);

      int n1 = parser1.getInt(2, 1);
      int n2 = parser1.getInt(3, 1);

      if (intercoonetFlag) {
        dbTechLayer* techLayer = tech->findLayer(layerName);
        if (techLayer == NULL) {
          logger_->warn(
              RCX, 372,
              "Layer {} in line number {} in file {} has not beed defined in "
              "LEF file, will skip all attached geometries",
              layerName, parser1.getLineNum(), filename);
          continue;
        }

        dbTechLayerType type = techLayer->getType();

        if (type.getValue() != dbTechLayerType::ROUTING)
          continue;

        _idTable[layerNum] = techLayer->getRoutingLevel();

        logger_->info(RCX, 367,
                      "Read layer name {} with number {} that corresponds to "
                      "routing level {}",
                      layerName, layerNum, _idTable[layerNum]);

        loHeightTable[n1] = layerNum;
        hiHeightTable[n2] = layerNum;
      } else if (viaFlag) {
        int topLayer = loHeightTable[n2];
        int level1 = _idTable[topLayer];

        if (level1 > 0) {
          int botLayer = hiHeightTable[n1];
          int level2 = _idTable[botLayer];

          if (level2 > 0) {
            _idTable[layerNum] = level2;
            viaTable[layerNum] = true;
          }
        }
      }
      continue;
    }
    if (parser1.isKeyword(0, "net")) {
      char netName[256];
      strcpy(netName, parser1.get(1));

      char mainNetName[256];
      bool subNetFlag = false;
      ambersandParser.mkWords(netName);
      if (ambersandParser.getWordCnt() == 2) {
        subNetFlag = true;
        strcpy(mainNetName, ambersandParser.get(0));
      }

      if (debug)
        parser1.printWords(stdout);

      netCnt++;

      if (parser1.parseNextLine() > 0) {
        uint layerNum = 0;
        if (!parse_setLayer(&parser1, layerNum, debug))
          continue;  // empty nets, TODO warning

        dbRtTree rtTree;
        dbNet* net = NULL;

        uint wireCnt = 0;
        while (parser1.parseNextLine() > 0) {
          if (parse_setLayer(&parser1, layerNum, debug))
            continue;

          _ll[0] = Ath__double2int(parser1.getDouble(0) * nm);
          _ll[1] = Ath__double2int(parser1.getDouble(1) * nm);
          _ur[0] = Ath__double2int(parser1.getDouble(2) * nm);

          char* w3 = parser1.get(3);
          parserParen.mkWords(w3);
          _ur[1] = Ath__double2int(parserParen.getDouble(0) * nm);

          if (debug)
            parser1.printWords(stdout);

          uint level = _idTable[layerNum];
          if (level == 0) {
            logger_->info(RCX, 365,
                          "Skipping net {}, layer num {} not defined in LEF",
                          netName, layerNum);
          } else if (wireCnt == 0) {
            dbNet* mainNet = NULL;

            if (!subNetFlag) {
              // net= _create_net_util.createNetSingleWire(netName, _ll[0],
              // _ll[1], _ur[0], _ur[1], level);
              net = createSingleWireNet(netName, level, viaTable[layerNum],
                                        debug, skipVias);
              if (net != NULL) {
                dbWire* w1 = net->getWire();
                rtTree.decode(w1);
              }
            } else {
              mainNet = _block->findNet(mainNetName);
              if (mainNet == NULL) {
                // net= _create_net_util.createNetSingleWire(mainNetName,
                // _ll[0], _ll[1], _ur[0], _ur[1], level);
                net = createSingleWireNet(mainNetName, level,
                                          viaTable[layerNum], debug, skipVias);

                if (net != NULL) {
                  dbRtTree s;
                  s.decode(net->getWire());
                  s.encode(net->getWire());
                }
              } else {
                // net= _create_net_util.createNetSingleWire(netName, _ll[0],
                // _ll[1], _ur[0], _ur[1], level, true/*skipBterms*/);
                net = createSingleWireNet(netName, level, viaTable[layerNum],
                                          debug, skipVias, true /*skipBterms*/);
                if (net != NULL) {
                  dbRtTree m;
                  m.decode(mainNet->getWire());
                  dbRtTree s;
                  s.decode(net->getWire());
                  m.move(&s);
                  m.encode(mainNet->getWire());
                  dbNet::destroy(net);
                }
              }
            }
            /*
if (net!=NULL) {
                    if (!subNetFlag) {
                            dbWire *w1= net->getWire();
                            rtTree.decode(w1);
                    }
                    else {
                            if (mainNet!=NULL) {
                                    dbRtTree m;
                                    m.decode(mainNet->getWire());
                                    dbRtTree s;
                                    s.decode(net->getWire());
                                    m.move(&s);
                                    m.encode(mainNet->getWire());
                                    dbNet::destroy(net);
                            }
                            else {
                                    dbRtTree s;
                                    s.decode(net->getWire());
                                    s.encode(net->getWire());
                            }
                    }
            }
            */
          } else if (wireCnt > 0) {
            char buff[1024];
            sprintf(buff, "%s__w%d_line%d", netName, wireCnt + 1,
                    parser1.getLineNum());

            dbNet* net1 =
                createSingleWireNet(buff, level, viaTable[layerNum], debug,
                                    skipVias, true /*skipBterms*/);
            // dbNet *net1= _create_net_util.createNetSingleWire(buff,
            // _ll[0], _ll[1], _ur[0], _ur[1], level, true/*skipBterms*/);

            if (net1 != NULL) {
              dbWire* wire = net1->getWire();
              dbRtTree T;
              T.decode(wire);
              rtTree.move(&T);
              dbNet::destroy(net1);
            }
          }
          uint n = strlen(w3);
          if (w3[n - 1] == ')') {
            if ((wireCnt > 0) && !subNetFlag) {
              rtTree.encode(net->getWire());
            }
            totWireCnt += wireCnt + 1;
            if (netCnt % 1000 == 0)
              logger_->info(RCX, 362, "Have read {} nets and {} wires", netCnt,
                            totWireCnt);

            break;
          }

          wireCnt++;
        }
        continue;
      }
    }
  }
  if (netCnt > 0) {
    dbBox* bb = _block->getBBox();
    Rect r(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
    _block->setDieArea(r);

    logger_->info(RCX, 53, "Have read {} nets from file {}", netCnt, filename);
  } else {
    logger_->warn(RCX, 56, "No nets were read from file {}", filename);
  }
  uint ccCnt = 0;
  int gndCnt = 0;
  if (capFile != NULL) {
    _extMain->_noModelRC = true;
    _extMain->_cornerCnt = 1;
    _extMain->_extDbCnt = 1;
    _block->setCornerCount(_extMain->_cornerCnt);
    _extMain->setupMapping(0);

    dbSet<dbNet> nets = _block->getNets();
    dbSet<dbNet>::iterator net_itr;
    dbNet* net;
    for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
      net = *net_itr;
      _extMain->makeNetRCsegs(net);
    }
    gndCnt = readCapFile(capFile, ccCnt);
  }
  logger_->info(RCX, 71, "Have created {} gnd caps and {} cc caps", gndCnt,
                ccCnt);

  return netCnt;
}
int extMeasure::readAB(extMain* extMain, const char* filename,
                       const char* design, const char* capFile, bool skipBterms,
                       dbDatabase* db) {
  bool skipVias = true;
  bool debug = false;

  uint nm = 1000;

  dbChip* chip = dbChip::create(db);
  assert(chip);
  _block = dbBlock::create(chip, design, '/');
  assert(_block);
  _block->setBusDelimeters('[', ']');
  _block->setDefUnits(nm);

  _extMain = extMain;
  _extMain->_block = _block;
  _create_net_util.setBlock(_block);

  dbTech* tech = db->getTech();

  uint netCnt = 0;

  Ath__parser parser1;
  parser1.openFile((char*)filename);
  while (parser1.parseNextLine() > 0) {
    netCnt++;
    char* layerName = parser1.get(1);
    dbTechLayer* techLayer = tech->findLayer(layerName);
    if (techLayer == NULL) {
      logger_->warn(
          RCX, 369,
          "Layer {} in line number {} in file {} has not beed defined "
          "in LEF file, will skip all attached geometries",
          layerName, parser1.getLineNum(), filename);
      continue;
    }
    dbTechLayerType type = techLayer->getType();
    if (type.getValue() != dbTechLayerType::ROUTING)
      continue;

    char netName[256];
    strcpy(netName, parser1.get(0));

    _ll[0] = Ath__double2int(parser1.getDouble(2) * nm);
    _ll[1] = Ath__double2int(parser1.getDouble(3) * nm);
    _ur[0] = Ath__double2int(parser1.getDouble(4) * nm);
    _ur[1] = Ath__double2int(parser1.getDouble(5) * nm);

    uint level = techLayer->getRoutingLevel();
    dbNet* net =
        createSingleWireNet(netName, level, false, debug, skipVias, true);

    if (parser1.getWordCnt() > 6)
      net->setSigType(dbSigType::ANALOG);

    if (netCnt % 10000 == 0)
      logger_->info(RCX, 68, "Have read {} nets ", netCnt);
  }
  if (netCnt > 0) {
    dbBox* bb = _block->getBBox();
    Rect r(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
    _block->setDieArea(r);

    logger_->info(RCX, 54, "Have read {} nets from file {}", netCnt, filename);
  } else {
    logger_->warn(RCX, 57, "No nets were read from file {}", filename);
  }
  uint ccCnt = 0;
  int gndCnt = 0;
  if (capFile != NULL) {
    _extMain->_noModelRC = true;
    _extMain->_cornerCnt = 1;
    _extMain->_extDbCnt = 1;
    _block->setCornerCount(_extMain->_cornerCnt);
    _extMain->setupMapping(0);

    dbSet<dbNet> nets = _block->getNets();
    dbSet<dbNet>::iterator net_itr;
    dbNet* net;
    for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
      net = *net_itr;
      _extMain->makeNetRCsegs(net);
    }
    gndCnt = readCapFile(capFile, ccCnt);
  }
  logger_->info(RCX, 359, "Have created {} gnd caps and {} cc caps", gndCnt,
                ccCnt);

  return netCnt;
}

dbRSeg* extMeasure::getRseg(const char* netname, const char* capMsg,
                            const char* tableEntryName) {
  dbNet* net = _block->findNet(netname);
  if (net == NULL) {
    logger_->warn(RCX, 74, "Cannot find net {} from the {} table entry {}",
                  netname, capMsg, tableEntryName);
    return NULL;
  }
  dbRSeg* r = getFirstDbRseg(net->getId());
  if (r == NULL) {
    logger_->warn(RCX, 460,
                  "Cannot find dbRseg for net {} from the {} table entry {}",
                  netname, capMsg, tableEntryName);
  }
  return r;
}
int extMeasure::readCapFile(const char* filename, uint& ccCnt) {
  uint totCnt = 0;
  double units = 1.0;
  Ath__parser parser;
  parser.openFile((char*)filename);
  parser.resetSeparator(" \t\n\r");

  Ath__parser parserWord;
  parserWord.resetSeparator("-");

  bool totalFlag = false;
  bool ccFlag = false;
  while (parser.parseNextLine() > 0) {
    parser.printWords(stdout);
    if (parser.isKeyword(0, "[pF]")) {
      units = 1000.0;
      continue;
    }
    if ((parser.isKeyword(0, "Cross-coupled")) ||
        (parser.isKeyword(0, "Cross-Coupled"))) {
      totalFlag = false;
      ccFlag = true;
      continue;
    }
    if (parser.isKeyword(0, "Total") || parser.isKeyword(1, "total")) {
      totalFlag = true;
      ccFlag = false;
      continue;
    }
    if (!((totalFlag || ccFlag) && (parser.getWordCnt() >= 4)))
      continue;

    uint indexCapword = 1;
    if (parser.getWordCnt() > 4)
      indexCapword = 2;
    double cap = parser.getDouble(indexCapword) * units;

    char* netname = parser.get(0);
    char bufNetName[512];
    strcpy(bufNetName, netname);

    if (ccFlag) {  // coupling cap
      parserWord.mkWords(netname);
      char* netname1 = parserWord.get(0);
      char* netname2 = parserWord.get(1);

      dbRSeg* rseg1 = getRseg(netname1, "Cross-Coupled", bufNetName);
      if (rseg1 == NULL)
        continue;
      dbRSeg* rseg2 = getRseg(netname2, "Cross-Coupled", bufNetName);
      if (rseg2 == NULL)
        continue;

      dbCCSeg* ccap = dbCCSeg::create(
          dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
          dbCapNode::getCapNode(_block, rseg2->getTargetNode()), true);
      ccap->addCapacitance(cap);

      logger_->info(RCX, 453, "Created coupling Cap {} for nets {} and {}", cap,
                    netname1, netname2);

      ccCnt++;
    } else if (totalFlag) {  // total cap
      dbRSeg* rseg1 = getRseg(netname, "Total", netname);
      if (rseg1 == NULL)
        continue;

      rseg1->setCapacitance(cap);

      logger_->info(RCX, 257, "Created gnd Cap {} for net {}", cap, netname);

      totCnt++;
    }
    continue;
  }
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;
  dbNet* net;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    net = *net_itr;

    dbRSeg* r = getFirstDbRseg(net->getId());
    if (r == NULL)
      continue;

    double totCap = r->getCapacitance();
    double totCC = net->getTotalCouplingCap();
    r->setCapacitance(totCap - totCC);
  }

  return totCnt;
}
void extMeasure::getMinWidth(dbTech* tech) {
  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator litr;
  dbTechLayer* layer;
  for (litr = layers.begin(); litr != layers.end(); ++litr) {
    layer = *litr;
    if (layer->getType() != dbTechLayerType::ROUTING)
      continue;

    uint level = layer->getRoutingLevel();
    uint pitch = layer->getPitch();
    uint minWidth = layer->getWidth();
    //		uint minSpacing= layer->getSpacing();
    //_minSpaceTable[level]= minSpacing;
    _minSpaceTable[level] = pitch - minWidth;
  }
}
void extMeasure::updateBox(uint w_layout, uint s_layout, int dir) {
  uint d = _dir;
  if (dir >= 0)
    d = dir;

  _ll[d] = _ur[d] + s_layout;
  _ur[d] = _ll[d] + w_layout;

  // printBox(stdout);
}
uint extMeasure::createNetSingleWire(char* dirName, uint idCnt, uint w_layout,
                                     uint s_layout, int dir) {
  if (w_layout == 0) {
    dbTechLayer* layer = _create_net_util.getRoutingLayer()[_met];
    w_layout = layer->getWidth();
  }
  if (s_layout == 0) {
    uint d = _dir;
    if (dir >= 0)
      d = dir;
    _ur[d] = _ll[d] + w_layout;
  } else {
    updateBox(w_layout, s_layout, dir);
  }

  //	uint w2= w_layout/2;
  int ll[2];
  int ur[2];
  ll[0] = _ll[0];
  ll[1] = _ll[1];
  ur[0] = _ur[0];
  ur[1] = _ur[1];

  /*
          if (_dir)
                  ll[0] -=w2;
          else
                  ll[1] -=w2;
  */

  //	ur[!_dir] = ur[!_dir] + _minWidth - w_layout;
  ur[!_dir] = ur[!_dir] - w_layout / 2;
  ll[!_dir] = ll[!_dir] + w_layout / 2;

  //	updateBox(w_layout, s_layout, dir);
  char left, right;
  _block->getBusDelimeters(left, right);

  char netName[1024];
  sprintf(netName, "%s%c%d%c", dirName, left, idCnt, right);
  if (_skip_delims)
    sprintf(netName, "%s_%d", dirName, idCnt);

  assert(_create_net_util.getBlock() == _block);
  dbNet* net = _create_net_util.createNetSingleWire(netName, ll[0], ll[1],
                                                    ur[0], ur[1], _met);

  dbBTerm* in1 = net->get1stBTerm();
  if (in1 != NULL) {
    in1->rename(net->getConstName());
    // fprintf(stdout, "M%d  %8d %8d   %8d %8d DX=%d DY=%d  %s\n",
    //  _met, ll[0], ll[1], ur[0], ur[1], ur[0]-ll[0], ur[1]-ll[1], netName);
  }

  uint netId = net->getId();
  addNew2dBox(net, ll, ur, _met, _dir, netId, false);

  _extMain->makeNetRCsegs(net);

  return netId;
}
uint extMeasure::createNetSingleWire_cntx(int met, char* dirName, uint idCnt,
                                          int d, int ll[2], int ur[2],
                                          int s_layout) {
  char netName[1024];

  sprintf(netName, "%s_cntxM%d_%d", dirName, met, idCnt);

  assert(_create_net_util.getBlock() == _block);
  dbNet* net = _create_net_util.createNetSingleWire(netName, ll[0], ll[1],
                                                    ur[0], ur[1], met);
  dbBTerm* in1 = net->get1stBTerm();
  if (in1 != NULL) {
    in1->rename(net->getConstName());
  }
  _extMain->makeNetRCsegs(net);

  return net->getId();
}
uint extMeasure::createDiagNetSingleWire(char* dirName, uint idCnt, int begin,
                                         int w_layout, int s_layout, int dir) {
  int ll[2], ur[2];
  ll[!_dir] = _ll[!_dir];
  ll[_dir] = begin;
  ur[!_dir] = _ur[!_dir];
  ur[_dir] = begin + w_layout;

  int met;
  if (_overMet > 0)
    met = _overMet;
  else if (_underMet > 0)
    met = _underMet;

  char left, right;
  _block->getBusDelimeters(left, right);

  char netName[1024];
  sprintf(netName, "%s%c%d%c", dirName, left, idCnt, right);
  if (_skip_delims)
    sprintf(netName, "%s_%d", dirName, idCnt);

  assert(_create_net_util.getBlock() == _block);
  dbNet* net = _create_net_util.createNetSingleWire(netName, ll[0], ll[1],
                                                    ur[0], ur[1], met);
  addNew2dBox(net, ll, ur, met, _dir, net->getId(), false);

  _extMain->makeNetRCsegs(net);

  return net->getId();
}
ext2dBox* extMeasure::addNew2dBox(dbNet* net, int* ll, int* ur, uint m, uint d,
                                  uint id, bool cntx) {
  ext2dBox* bb = _2dBoxPool->alloc();

  dbShape s;
  if ((net != NULL) && _extMain->getFirstShape(net, s)) {
    bb->_ll[0] = s.xMin();
    bb->_ll[1] = s.yMin();
    bb->_ur[0] = s.xMax();
    bb->_ur[1] = s.yMax();
  } else {
    bb->_ll[0] = ll[0];
    bb->_ll[1] = ll[1];
    bb->_ur[0] = ur[0];
    bb->_ur[1] = ur[1];
  }

  bb->_met = m;
  bb->_dir = d;
  bb->_id = id;
  bb->_map = 0;

  if (cntx)  // context net
    _2dBoxTable[1][m].add(bb);
  else  // main net
    _2dBoxTable[0][m].add(bb);

  return bb;
}
void ext2dBox::rotate() {
  int x = _ur[0];
  _ur[0] = _ur[1];
  _ur[1] = x;

  x = _ll[0];
  _ll[0] = _ll[1];
  _ll[1] = x;
  _dir = !_dir;
}
uint ext2dBox::length() {
  return _ur[_dir] - _ll[_dir];  // TEST !_dir
                                 //	return _ur[!_dir] - _ll[!dir];
}
uint ext2dBox::width() {
  return _ur[!_dir] - _ll[!_dir];  // TEST _dir
                                   //	return _ur[_dir] - _ll[_dir];
}
int ext2dBox::loX() { return _ll[0]; }
int ext2dBox::loY() { return _ll[1]; }
uint ext2dBox::id() { return _id; }
void ext2dBox::printGeoms3D(FILE* fp, double h, double t, int* orig) {
  fprintf(fp,
          "%3d %8d -- M%d D%d  %g %g  %g %g  L= %g W= %g  H= %g  TH= %g ORIG "
          "%g %g\n",
          _id, _map, _met, _dir, 0.001 * _ll[0], 0.001 * _ll[1], 0.001 * _ur[0],
          0.001 * _ur[1], 0.001 * length(), 0.001 * width(), h, t,
          0.001 * (_ll[0] - orig[0]), 0.001 * (_ll[1] - orig[1]));
}
void extMeasure::clean2dBoxTable(int met, bool cntx) {
  if (met <= 0)
    return;
  for (uint ii = 0; ii < _2dBoxTable[cntx][met].getCnt(); ii++) {
    ext2dBox* bb = _2dBoxTable[cntx][met].get(ii);
    _2dBoxPool->free(bb);
  }
  _2dBoxTable[cntx][met].resetCnt();
}
uint extMeasure::getBoxLength(uint ii, int met, bool cntx) {
  if (met <= 0)
    return 0;

  int cnt = _2dBoxTable[cntx][met].getCnt();
  if (cnt <= 0)
    return 0;

  ext2dBox* bb = _2dBoxTable[cntx][met].get(ii);

  //	return bb->length();
  return bb->width();
}
void extMeasure::getBox(int met, bool cntx, int& xlo, int& ylo, int& xhi,
                        int& yhi) {
  if (met <= 0)
    return;

  int cnt = _2dBoxTable[cntx][met].getCnt();
  if (cnt <= 0)
    return;

  ext2dBox* bbLo = _2dBoxTable[cntx][met].get(0);
  ext2dBox* bbHi = _2dBoxTable[cntx][met].get(cnt - 1);

  xlo = MIN(bbLo->loX(), bbHi->loX());
  ylo = MIN(bbLo->loY(), bbHi->loY());

  xhi = MAX(bbLo->_ur[0], bbHi->_ur[0]);
  yhi = MAX(bbLo->_ur[1], bbHi->_ur[1]);
}
void extMeasure::writeRaphaelPointXY(FILE* fp, double X, double Y) {
  fprintf(fp, "  %6.3f,%6.3f ; ", X, Y);
}

void extMeasure::writeBoxRaphael3D(FILE* fp, ext2dBox* bb, int* base_ll,
                                   int* base_ur, double y1, double th,
                                   double volt) {
  /* this function assumes the boxes bb, base_ll, and base_ur, are generated
   * from a vertical wire. Any box that is generated from a horizontal wire must
   * be rotat before passing into this function.
   */

  double len = ((double)bb->length()) / 1000;
  double width = ((double)bb->width()) / 1000;

  double middle = 0.001 * (base_ur[0] + base_ll[0]) * 0.5;
  double x;
  if (!bb->_dir) {
    x = len;
    len = width;
    width = x;
  }
  //	double x=len*0.5;
  x = 0.001 * bb->_ll[0] - middle;

  double l = 0.001 * (bb->_ll[1] - base_ll[1]);

  /*
  if (!bb->_dir)
          l = ((double)(bb->loX() - x1))/1000;
  else
          l = ((double)(bb->loY() - x1))/1000;
  */
  fprintf(fp, "POLY3D NAME= M%d_RC_%d_w%d; ", bb->_met, bb->_map, bb->_id);
  fprintf(fp, " COORD= ");
  writeRaphaelPointXY(fp, x, y1);
  writeRaphaelPointXY(fp, x + width, y1);
  writeRaphaelPointXY(fp, x + width, y1 + th);
  writeRaphaelPointXY(fp, x, y1 + th);

  fprintf(fp, " V1=0,0,%g; HEIGHT=%g;", l, len);
  fprintf(fp, " VOLT=%g ;\n", volt);
}
uint extMeasure::writeRaphael3D(FILE* fp, int met, bool cntx, double x1,
                                double y1, double th) {
  if (met <= 0 || !_3dFlag)
    return 0;

  uint cnt = 0;
  double l, width, len, x;
  for (uint ii = 0; ii < _2dBoxTable[cntx][met].getCnt(); ii++) {
    ext2dBox* bb = _2dBoxTable[cntx][met].get(ii);
    len = ((double)bb->length()) / 1000;
    width = ((double)bb->width()) / 1000;
    double tt;
    tt = len;
    len = width;
    width = tt;
    x = len * 0.5;
    if (!bb->_dir)
      l = ((double)(bb->loX() - x1)) / 1000;
    else
      l = ((double)(bb->loY() - x1)) / 1000;

    fprintf(fp, "POLY3D NAME= M%d__w0; ", met);
    fprintf(fp, " COORD= ");
    writeRaphaelPointXY(fp, -x, y1);
    writeRaphaelPointXY(fp, x, y1);
    writeRaphaelPointXY(fp, x, y1 + th);
    writeRaphaelPointXY(fp, -x, y1 + th);

    fprintf(fp, " V1=0,0,%g; HEIGHT=%g;", l, width);
    fprintf(fp, " VOLT=0 ;\n");
    cnt++;
  }
  return cnt;
}
uint extMeasure::writeDiagRaphael3D(FILE* fp, int met, bool cntx, double x1,
                                    double y1, double th) {
  if (met <= 0 || !_3dFlag)
    return 0;

  uint cnt = 0;
  double l, width, len;
  for (uint ii = 0; ii < _2dBoxTable[cntx][met].getCnt(); ii++) {
    ext2dBox* bb = _2dBoxTable[cntx][met].get(ii);
    width = ((double)bb->length()) / 1000;
    len = ((double)bb->width()) / 1000;
    if (!bb->_dir)
      l = ((double)(bb->loX() - x1)) / 1000;
    else
      l = ((double)(bb->loY() - x1)) / 1000;

    fprintf(fp, "POLY3D NAME= M%d__w%d; ", met, bb->id());
    fprintf(fp, " COORD= ");
    writeRaphaelPointXY(fp, l, y1);
    writeRaphaelPointXY(fp, l + width, y1);
    writeRaphaelPointXY(fp, l + width, y1 + th);
    writeRaphaelPointXY(fp, l, y1 + th);

    fprintf(fp, " V1=0,0,0; HEIGHT=%g;", len);
    fprintf(fp, " VOLT=0 ;\n");
    cnt++;
  }
  return cnt;
}
uint extMeasure::createContextNets(char* dirName, int bboxLL[2], int bboxUR[2],
                                   int met, double pitchMult) {
  if (met <= 0)
    return 0;

  // char left, right;
  // _block->getBusDelimeters(left, right);

  dbTechLayer* layer = _tech->findRoutingLayer(met);
  dbTechLayer* mlayer = _tech->findRoutingLayer(_met);
  uint minWidth = layer->getWidth();
  uint minSpace = layer->getSpacing();
  int pitch =
      Ath__double2int(1000 * ((minWidth + minSpace) * pitchMult) / 1000);

  int ll[2];
  int ur[2];

  uint offset = 0;
  /*
          if (met > _met)
                  offset= _minWidth+_minSpace;
          else
                  offset= 2*(minWidth+minSpace);
  */
  ll[_dir] = bboxLL[_dir] - offset;
  ur[_dir] = bboxUR[_dir] + offset;

  _ur[_dir] = ur[_dir];

  uint cnt = 1;

  uint not_dir = !_dir;
  int start = bboxLL[not_dir] + offset;
  //	int end= bboxUR[not_dir]-offset;
  int end = bboxUR[not_dir];
  for (int lenXY = (int)(start + minWidth); (int)(lenXY + minWidth) <= end;
       lenXY += pitch) {
    ll[not_dir] = lenXY;
    ur[not_dir] = lenXY + minWidth;

    char netName[1024];
    // sprintf(netName, "%s_m%d_cntxt%c%d%c", dirName, met, left, cnt++, right);
    sprintf(netName, "%s_m%d_cntxt_%d", dirName, met, cnt++);
    dbNet* net;
    assert(_create_net_util.getBlock() == _block);
    if (mlayer->getDirection() != dbTechLayerDir::HORIZONTAL)
      net = _create_net_util.createNetSingleWire(
          netName, ll[0], ll[1], ur[0], ur[1], met, dbTechLayerDir::HORIZONTAL,
          false);
    else
      net = _create_net_util.createNetSingleWire(netName, ll[0], ll[1], ur[0],
                                                 ur[1], met,
                                                 mlayer->getDirection(), false);
    //		net= _create_net_util.createNetSingleWire(netName, ll[0], ll[1],
    // ur[0], ur[1], met);

    addNew2dBox(net, ll, ur, met, not_dir, net->getId(), true);  // TEST not_dir
  }
  return cnt - 1;
}
dbRSeg* extMeasure::getFirstDbRseg(uint netId) {
  dbNet* net = dbNet::getNet(_block, netId);

  dbSet<dbRSeg> rSet = net->getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;

  //	double cap= 0.0;

  dbRSeg* rseg = NULL;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rseg = *rc_itr;
    break;
  }
  // rseg->setCapacitance(tot);

  return rseg;
}
double extMeasure::getCCfringe(uint lastNode, uint n, uint start, uint end) {
  double ccFr = 0.0;
  for (uint ii = start; ii <= end; ii++) {
    int d = n - ii;
    int u = n + ii;

    if (n + ii > lastNode)
      break;

    if (d > 0)
      ccFr += _capMatrix[d][n];

    ccFr += _capMatrix[n][u];
  }
  return ccFr;
}
double extMeasure::getCCfringe3D(uint lastNode, uint n, uint start, uint end) {
  double ccFr = 0.0;
  uint End;
  if (_diag)
    End = lastNode - n;
  else
    End = end;
  for (uint ii = start; ii <= End; ii++) {
    int d = n - ii;
    uint u = n + ii;
    /*
                    if (n+ii>lastNode)
                            break;
    */
    if (d > 0)
      ccFr += _capMatrix[1][d];
    if (u <= lastNode)
      ccFr += _capMatrix[1][u];
  }
  return ccFr;
}
void extMeasure::printBox(FILE* fp) {
  fprintf(fp, "( %8d %8d ) ( %8d %8d )\n", _ll[0], _ll[1], _ur[0], _ur[1]);
}
uint extMeasure::initWS_box(extMainOptions* opt, uint gridCnt) {
  dbTechLayer* layer = opt->_tech->findRoutingLayer(_met);
  _minWidth = layer->getWidth();
  _pitch = layer->getPitch();
  _minSpace = _pitch - _minWidth;
  _dir = layer->getDirection() == dbTechLayerDir::HORIZONTAL ? 1 : 0;

  uint patternSep = gridCnt * (_minWidth + _minSpace);

  _ll[0] = opt->_ur[0] + patternSep;
  _ll[1] = 0;

  _ur[_dir] = _ll[_dir];
  //	_ur[! _dir] = _ll[!_dir] + (opt->_len-_minWidth); // to agree with width
  // extension
  //    DF 620  _ur[! _dir] = _ll[!_dir] + opt->_len;
  _ur[!_dir] =
      _ll[!_dir] + opt->_len * _minWidth / 1000;  // _len is in nm per ext.ti

  return patternSep;
}
void extMeasure::updateForBench(extMainOptions* opt, extMain* extMain) {
  _benchFlag = true;
  _len = opt->_len;
  _wireCnt = opt->_wireCnt;
  _block = opt->_block;
  _tech = opt->_tech;
  _extMain = extMain;
  _3dFlag = opt->_3dFlag;
  _create_net_util.setBlock(_block, false);
  _dbunit = _block->getDbUnitsPerMicron();
}
uint extMeasure::defineBox(CoupleOptions& options) {
  _no_debug = false;
  _met = options[0];

  _len = options[3];
  _dist = options[4];
  _s_nm = options[4];

  int xy = options[5];
  _dir = options[6];

  _width = options[7];
  _w_nm = options[7];
  /*
          if (_dist>0) {
                  if (options[8]>_width) {
                          _width= options[8];
                          _w_nm= options[8];
                  }
  //		_width= (options[7]+options[8])/2;
  //		_w_nm= (options[7]+options[8])/2;rm O
          }
  */
  int base = options[9];
  // _dir= 1 horizontal
  // _dir= 0 vertical

  if (_dir == 0) {
    _ll[1] = xy;
    _ll[0] = base;
    _ur[1] = xy + _len;
    _ur[0] = base + _width;
  } else {
    _ll[0] = xy;
    _ll[1] = base;
    _ur[0] = xy + _len;
    _ur[1] = base + _width;
  }
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    _rc[ii]->_coupling = 0.0;
    _rc[ii]->_fringe = 0.0;
    _rc[ii]->_diag = 0.0;
    _rc[ii]->_res = 0.0;
    _rc[ii]->_sep = 0;
  }
  dbTechLayer* layer = _extMain->_tech->findRoutingLayer(_met);
  _minWidth = layer->getWidth();
#ifdef HI_ACC_1
  _toHi = (options[11] > 0) ? true : false;
#else
  _toHi = true;
#endif

  return _len;
}
void extMeasure::tableCopyP(Ath__array1D<int>* src, Ath__array1D<int>* dst) {
  for (uint ii = 0; ii < src->getCnt(); ii++)
    dst->add(src->get(ii));
}

void extMeasure::tableCopyP(Ath__array1D<SEQ*>* src, Ath__array1D<SEQ*>* dst) {
  for (uint ii = 0; ii < src->getCnt(); ii++)
    dst->add(src->get(ii));
}
void extMeasure::tableCopy(Ath__array1D<SEQ*>* src, Ath__array1D<SEQ*>* dst,
                           gs* pixelTable) {
  for (uint ii = 0; ii < src->getCnt(); ii++)
    copySeq(src->get(ii), dst, pixelTable);
}
void extMeasure::release(Ath__array1D<SEQ*>* seqTable, gs* pixelTable) {
  if (pixelTable == NULL)
    pixelTable = _pixelTable;

  for (uint ii = 0; ii < seqTable->getCnt(); ii++)
    pixelTable->release(seqTable->get(ii));

  seqTable->resetCnt();
}
/* DF 720
int extMeasure::calcDist(int *ll, int *ur)
{
        int d= ((_ur[_dir]+_ll[_dir]) - (ur[_dir]+ll[_dir]))/2;
        return d>=0 ? d : -d;
} */
int extMeasure::calcDist(int* ll, int* ur) {
  int d = ll[_dir] - _ur[_dir];
  if (d >= 0)
    return d;

  d = _ll[_dir] - ur[_dir];
  if (d >= 0)
    return d;
  /*
          d= ll[_dir] - _ll[_dir];
          if (d>0)
                  return d;

          d = _ll[_dir] - ll[_dir];
          if (d>0)
                  return d;
  */
  return 0;
}
SEQ* extMeasure::addSeq(int* ll, int* ur) {
  SEQ* s = _pixelTable->salloc();
  for (uint ii = 0; ii < 2; ii++) {
    s->_ll[ii] = ll[ii];
    s->_ur[ii] = ur[ii];
  }
  s->type = 0;
  return s;
}
void extMeasure::addSeq(int* ll, int* ur, Ath__array1D<SEQ*>* seqTable,
                        gs* pixelTable) {
  if (pixelTable == NULL)
    pixelTable = _pixelTable;

  SEQ* s = pixelTable->salloc();
  for (uint ii = 0; ii < 2; ii++) {
    s->_ll[ii] = ll[ii];
    s->_ur[ii] = ur[ii];
  }
  s->type = 0;
  if (seqTable != NULL)
    seqTable->add(s);
}
void extMeasure::addSeq(Ath__array1D<SEQ*>* seqTable, gs* pixelTable) {
  SEQ* s = pixelTable->salloc();
  for (uint ii = 0; ii < 2; ii++) {
    s->_ll[ii] = _ll[ii];
    s->_ur[ii] = _ur[ii];
  }
  s->type = 0;

  seqTable->add(s);
}

void extMeasure::copySeq(SEQ* t, Ath__array1D<SEQ*>* seqTable, gs* pixelTable) {
  SEQ* s = pixelTable->salloc();
  for (uint ii = 0; ii < 2; ii++) {
    s->_ll[ii] = t->_ll[ii];
    s->_ur[ii] = t->_ur[ii];
  }
  s->type = t->type;

  seqTable->add(s);
}
void extMeasure::copySeqUsingPool(SEQ* t, Ath__array1D<SEQ*>* seqTable) {
  SEQ* s = _seqPool->alloc();
  for (uint ii = 0; ii < 2; ii++) {
    s->_ll[ii] = t->_ll[ii];
    s->_ur[ii] = t->_ur[ii];
  }
  s->type = t->type;

  seqTable->add(s);
}

uint extMeasure::getOverUnderIndex()  // TO_TEST
{
  int n = _layerCnt - _met - 1;
  n *= _underMet - 1;
  n += _overMet - _met - 1;

  if ((n < 0) || (n >= (int)_layerCnt + 1)) {
    /*
                    fprintf(stdout, "getOverUnderIndex: out of range n= %d m=%d
       u= %d o= %d\n", n, _met, _underMet, _overMet);
    */
    logger_->info(RCX, 459,
                  "getOverUnderIndex: out of range n= {}   m={} u= {} o= {}", n,
                  _met, _underMet, _overMet);
  }

  return n;
}
extDistRC* extMeasure::getFringe(uint len, double* valTable) {
  //	m._met= met;
  //	m._width= width;
  //	m._underMet= 0;

  extDistRC* rcUnit = NULL;

  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    rcUnit = rcModel->getOverFringeRC(this);

    if (rcUnit == NULL)
      continue;

    valTable[ii] = rcUnit->getFringe() * len;
  }
  return rcUnit;
}
void extLenOU::addOverOrUnderLen(int met, bool over, uint len) {
  _overUnder = false;
  _under = false;
  if (over) {
    _overMet = -1;
    _underMet = met;
    _over = true;
  } else {
    _overMet = met;
    _underMet = -1;
    _over = false;
    _under = true;
  }
  _len = len;
}
void extLenOU::addOULen(int underMet, int overMet, uint len) {
  _overUnder = true;
  _under = false;
  _over = false;

  _overMet = overMet;
  _underMet = underMet;

  _len = len;
}
uint extMeasure::getLength(SEQ* s, int dir) {
  return s->_ur[dir] - s->_ll[dir];
}

uint extMeasure::blackCount(uint start, Ath__array1D<SEQ*>* resTable) {
  uint cnt = 0;
  for (uint jj = start; jj < resTable->getCnt(); jj++) {
    SEQ* s = resTable->get(jj);

    // pixelTable->show_seq(s);

    if (s->type > 0)  // Black
      cnt++;
  }
  return cnt;
}
extDistRC* extMeasure::computeOverFringe(uint overMet, uint overWidth, uint len,
                                         uint dist) {
  extDistRC* rcUnit = NULL;

  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    rcUnit = rcModel->_capOver[overMet]->getRC(_met, overWidth, dist);

    if (IsDebugNet())
      rcUnit->printDebugRC(_met, overMet, 0, _width, dist, len, logger_);

    if (rcUnit != NULL) {
      _rc[ii]->_fringe += rcUnit->_fringe * len;
      _rc[ii]->_res += rcUnit->_res * len;
    }
  }
  return rcUnit;
}
extDistRC* extMeasure::computeUnderFringe(uint underMet, uint underWidth,
                                          uint len, uint dist) {
  extDistRC* rcUnit = NULL;

  uint n = _met - underMet - 1;

  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    if (rcModel->_capUnder[underMet] == NULL)
      continue;

    rcUnit = rcModel->_capUnder[underMet]->getRC(n, underWidth, dist);
    if (IsDebugNet())
      rcUnit->printDebugRC(_met, 0, underMet, _width, dist, len, logger_);

    if (rcUnit != NULL) {
      _rc[ii]->_fringe += rcUnit->_fringe * len;
      _rc[ii]->_res += rcUnit->_res * len;
    }
  }
  return rcUnit;
}
void extMeasure::swap_coords(SEQ* s) {
  int xy = s->_ll[1];
  s->_ll[1] = s->_ll[0];
  s->_ll[0] = xy;

  xy = s->_ur[1];
  s->_ur[1] = s->_ur[0];
  s->_ur[0] = xy;
}
uint extMeasure::swap_coords(uint initCnt, uint endCnt,
                             Ath__array1D<SEQ*>* resTable) {
  for (uint ii = initCnt; ii < endCnt; ii++)
    swap_coords(resTable->get(ii));

  return endCnt - initCnt;
}

uint extMeasure::getOverlapSeq(uint met, SEQ* s, Ath__array1D<SEQ*>* resTable) {
  uint len1 = 0;

  if (!_rotatedGs) {
    len1 = _pixelTable->get_seq(s->_ll, s->_ur, _dir, met, resTable);
  } else {
    if (_dir > 0) {  // extracting horizontal segments
      len1 = _pixelTable->get_seq(s->_ll, s->_ur, _dir, met, resTable);
    } else {
      int sll[2];
      int sur[2];

      sll[0] = s->_ll[1];
      sll[1] = s->_ll[0];
      sur[0] = s->_ur[1];
      sur[1] = s->_ur[0];

      uint initCnt = resTable->getCnt();

      len1 = _pixelTable->get_seq(sll, sur, !_dir, met, resTable);

      swap_coords(initCnt, resTable->getCnt(), resTable);
    }
  }

  if ((len1 >= 0) && (len1 <= _len)) {
    return len1;
  } else {
#ifdef DEBUG_gs
    logger_->info(RCX, 454, "pixelTable gave len {}, bigger than expected {}",
                  len1, _len);
#endif
    return 0;
  }
}
uint extMeasure::getOverlapSeq(uint met, int* ll, int* ur,
                               Ath__array1D<SEQ*>* resTable) {
  uint len1 = 0;

  if (!_rotatedGs) {
    len1 = _pixelTable->get_seq(ll, ur, _dir, met, resTable);
  } else {
    if (_dir > 0) {  // extracting horizontal segments
      len1 = _pixelTable->get_seq(ll, ur, _dir, met, resTable);
    } else {
      int sll[2];
      int sur[2];

      sll[0] = ll[1];
      sll[1] = ll[0];
      sur[0] = ur[1];
      sur[1] = ur[0];

      uint initCnt = resTable->getCnt();

      len1 = _pixelTable->get_seq(sll, sur, !_dir, met, resTable);

      swap_coords(initCnt, resTable->getCnt(), resTable);
    }
  }

  if ((len1 >= 0) && (len1 <= _len)) {
    return len1;
  } else {
#ifdef DEBUG_gs
    logger_->info(RCX, 455, "pixelTable gave len {}, bigger than expected {}",
                  len1, _len);
#endif
    return 0;
  }
}

uint extMeasure::computeOverOrUnderSeq(Ath__array1D<SEQ*>* seqTable, uint met,
                                       Ath__array1D<SEQ*>* resTable,
                                       bool over) {
  uint len = 0;
  for (uint ii = 0; ii < seqTable->getCnt(); ii++) {
    SEQ* s = seqTable->get(ii);

    // pixelTable->show_seq(s);

    if (s->type > 0) {  // Black
      continue;
    }
    if ((s->_ll[0] < _ll[0]) || (s->_ll[1] < _ll[1]) || (s->_ur[0] > _ur[0]) ||
        (s->_ur[1] > _ur[1])) {
      // fprintf(stdout, "Out of Range result from gs for box (%d %d) (%d
      // %d)\n", 	_ll[0], _ll[1], _ur[0], _ur[1]);
      continue;
    }
    len += getOverlapSeq(met, s, resTable);

#ifdef FRINGE_UP_DOWN
    uint startIndex = resTable->getCnt();
    int maxDist = 1000;  // TO_TEST

    if (blackCount(startIndex, resTable) > 0) {
      for (uint jj = startIndex; jj < resTable->getCnt(); jj++) {
        SEQ* q = resTable->get(jj);

        if (q->type > 0)  // Black
          continue;

        int dist = getLength(q, !_dir);

        if (dist < 0)
          continue;  // TO_TEST
        if (dist > maxDist)
          continue;

        if (over)
          computeUnderFringe(met, _width, _width, dist);
        else
          computeOverFringe(met, _width, _width, dist);
      }
    }
#endif
  }
  if (len > _len)
    return 0;

  if (len <= 0)
    return 0;

#ifdef MIN_FOR_LOOPS
  extLenOU* ouLen = _lenOUPool->alloc();
  ouLen->addOverOrUnderLen(met, over, len);
  _lenOUtable->add(ouLen);
#else
  if (over)
    computeOverRC(len);
  else
    computeUnderRC(len);
#endif

  return len;
}
uint extMeasure::computeOUwith2planes(int* ll, int* ur,
                                      Ath__array1D<SEQ*>* resTable) {
  Ath__array1D<SEQ*> met1Table(16);

  bool over = true;
  uint met1 = _underMet;
  uint met2 = _overMet;
  if (_met - _underMet > _overMet - _met) {
    met2 = _underMet;
    met1 = _overMet;
    over = false;
  }
  getOverlapSeq(met1, ll, ur, &met1Table);

  uint len = 0;
  for (uint ii = 0; ii < met1Table.getCnt(); ii++) {
    SEQ* s = met1Table.get(ii);

    // pixelTable->show_seq(s);

    if (s->type == 0) {  // white
      resTable->add(s);
      continue;
    }
    /*
    if
    ((s->_ll[0]<_ll[0])||(s->_ll[1]<_ll[1])||(s->_ur[0]>_ur[0])||(s->_ur[1]>_ur[1]))
    {
            //fprintf(stdout, "Out of Range result from gs for box (%d %d) (%d
    %d)\n",
            //	_ll[0], _ll[1], _ur[0], _ur[1]);
            continue;
    }
    */
    len += getOverlapSeq(met2, s, resTable);
    _pixelTable->release(s);
  }
  return len;
}
void extMeasure::calcOU(uint len) {
#ifdef MIN_FOR_LOOPS
  extLenOU* ou = _lenOUPool->alloc();
  ou->addOULen(_underMet, _overMet, len);
  _lenOUtable->add(ou);
#else
  computeOverUnderRC(len);
#endif
}

uint extMeasure::computeOverUnder(int* ll, int* ur,
                                  Ath__array1D<SEQ*>* resTable) {
  uint ouLen = 0;

  if (_ouPixelTableIndexMap != NULL) {
    uint ou_plane = _ouPixelTableIndexMap[_underMet][_overMet];
    ouLen = _pixelTable->get_seq(ll, ur, _dir, ou_plane, resTable);
  } else {
    ouLen = computeOUwith2planes(ll, ur, resTable);
  }

  if ((ouLen < 0) || (ouLen > _len)) {
    //		fprintf(stdout, "pixelTable gave len %d, bigger than expected
    //%d\n", ouLen, _len);
    logger_->info(RCX, 456, "pixelTable gave len {}, bigger than expected {}",
                  ouLen, _len);
    return 0;
  }
  if (ouLen > 0)
    calcOU(ouLen);

  return ouLen;
}

uint extMeasure::computeOverOrUnderSeq(Ath__array1D<int>* seqTable, uint met,
                                       Ath__array1D<int>* resTable, bool over) {
  if (seqTable->getCnt() <= 0)
    return 0;

  uint len = 0;

  bool black = true;
  for (uint ii = 0; ii < seqTable->getCnt() - 1; ii++) {
    int xy1 = seqTable->get(ii);
    int xy2 = seqTable->get(ii + 1);

    black = !black;

    if (black)
      continue;

    if (xy1 == xy2)
      continue;

    uint len1 = mergeContextArray(_ccContextArray[met], _minSpaceTable[met],
                                  xy1, xy2, resTable);

    // if ((len1>=0)&&(len1<=_len))
    if (len1 >= 0)
      len += len1;
#ifdef DEBUG_gs
    else
      //			fprintf(stdout, "pixelTable gave len %d, bigger
      // than expected %d\n", len1, _len);
      logger_->info(RCX, 457, "pixelTable gave len {}, bigger than expected {}",
                    len1, _len);
#endif
  }
  if (len > _len)
    return 0;

  if (len <= 0)
    return 0;

  if (over)
    computeOverRC(len);
  else
    computeUnderRC(len);

  return len;
}

uint extMeasure::computeOverUnder(int xy1, int xy2,
                                  Ath__array1D<int>* resTable) {
  uint ouLen = intersectContextArray(xy1, xy2, _underMet, _overMet, resTable);

  if ((ouLen < 0) || (ouLen > _len)) {
    //		fprintf(stdout, "pixelTable gave len %d, bigger than expected
    //%d\n", ouLen, _len);
    logger_->info(RCX, 458, "pixelTable gave len {}, bigger than expected {}",
                  ouLen, _len);
    return 0;
  }
  if (ouLen > 0)
    computeOverUnderRC(ouLen);

  return ouLen;
}

uint extMeasure::mergeContextArray(Ath__array1D<int>* srcContext, int minS,
                                   Ath__array1D<int>* tgtContext) {
  tgtContext->resetCnt(0);
  uint ssize = srcContext->getCnt();
  if (ssize < 4)
    return 0;
  uint contextLength = 0;
  tgtContext->add(srcContext->get(0));
  int p1 = srcContext->get(1);
  int p2 = srcContext->get(2);
  int n1, n2;
  uint jj = 3;
  while (jj < ssize - 2) {
    n1 = srcContext->get(jj++);
    n2 = srcContext->get(jj++);
    if (n1 - p2 <= minS)
      p2 = n2;
    else {
      tgtContext->add(p1);
      tgtContext->add(p2);
      contextLength += p2 - p1;
      p1 = n1;
      p2 = n2;
    }
  }
  tgtContext->add(p1);
  tgtContext->add(p2);
  tgtContext->add(srcContext->get(ssize - 1));
  contextLength += p2 - p1;
  return contextLength;
}

uint extMeasure::mergeContextArray(Ath__array1D<int>* srcContext, int minS,
                                   int pmin, int pmax,
                                   Ath__array1D<int>* tgtContext) {
  tgtContext->resetCnt(0);
  uint ssize = srcContext->getCnt();
  if (ssize < 4)
    return 0;
  tgtContext->add(pmin);
  uint contextLength = 0;
  int p1, p2, n1, n2;
  uint jj;
  for (jj = 2; jj < ssize - 1; jj += 2)
    if (srcContext->get(jj) > pmin)
      break;
  if (jj >= ssize - 1) {
    tgtContext->add(pmax);
    return 0;
  }
  p1 = srcContext->get(jj - 1);
  if (p1 < pmin)
    p1 = pmin;
  p2 = srcContext->get(jj++);
  if (p2 > pmax)
    p2 = pmax;
  while (jj < ssize - 2) {
    n1 = srcContext->get(jj++);
    if (n1 >= pmax)
      break;
    n2 = srcContext->get(jj++);
    if (n2 > pmax)
      n2 = pmax;
    if (n1 - p2 <= minS)
      p2 = n2;
    else {
      tgtContext->add(p1);
      tgtContext->add(p2);
      contextLength += p2 - p1;
      p1 = n1;
      p2 = n2;
    }
  }
  tgtContext->add(p1);
  tgtContext->add(p2);
  contextLength += p2 - p1;
  tgtContext->add(pmax);
  return contextLength;
}

uint extMeasure::makeMergedContextArray(uint met, int minS) {
  return mergeContextArray(_ccContextArray[met], minS,
                           _ccMergedContextArray[met]);
}

uint extMeasure::makeMergedContextArray(uint met) {
  return mergeContextArray(_ccContextArray[met], _minSpaceTable[met],
                           _ccMergedContextArray[met]);
}

uint extMeasure::makeMergedContextArray(int pmin, int pmax, uint met,
                                        int minS) {
  return mergeContextArray(_ccContextArray[met], minS, pmin, pmax,
                           _ccMergedContextArray[met]);
}

uint extMeasure::makeMergedContextArray(int pmin, int pmax, uint met) {
  return mergeContextArray(_ccContextArray[met], _minSpaceTable[met], pmin,
                           pmax, _ccMergedContextArray[met]);
}

uint extMeasure::intersectContextArray(int pmin, int pmax, uint met1, uint met2,
                                       Ath__array1D<int>* tgtContext) {
  int minS1 = _minSpaceTable[met1];
  int minS2 = _minSpaceTable[met2];

  Ath__array1D<int> t1Context(1024);
  mergeContextArray(_ccContextArray[met1], minS1, pmin, pmax, &t1Context);
  Ath__array1D<int> t2Context(1024);
  mergeContextArray(_ccContextArray[met2], minS2, pmin, pmax, &t2Context);

  tgtContext->resetCnt(0);
  tgtContext->add(pmin);
  uint tsize1 = t1Context.getCnt();
  uint tsize2 = t2Context.getCnt();
  if (!tsize1 || !tsize2) {
    tgtContext->add(pmax);
    return 0;
  }
  uint readc1 = 1;
  uint readc2 = 1;
  uint jj1 = 1;
  uint jj2 = 1;
  uint icontextLength = 0;
  int p1min, p1max, p2min, p2max, ptmin, ptmax;
  while (1) {
    if (readc1) {
      if (jj1 + 2 >= tsize1)
        break;
      p1min = t1Context.get(jj1++);
      p1max = t1Context.get(jj1++);
      readc1 = 0;
    }
    if (readc2) {
      if (jj2 + 2 >= tsize2)
        break;
      p2min = t2Context.get(jj2++);
      p2max = t2Context.get(jj2++);
      readc2 = 0;
    }
    if (p1min >= p2max) {
      readc2 = 1;
      continue;
    }
    if (p2min >= p1max) {
      readc1 = 1;
      continue;
    }
    ptmin = p1min > p2min ? p1min : p2min;
    ptmax = p1max < p2max ? p1max : p2max;
    tgtContext->add(ptmin);
    tgtContext->add(ptmax);
    icontextLength += ptmax - ptmin;
    if (p1max > p2max)
      readc2 = 1;
    else if (p1max < p2max)
      readc1 = 1;
    else
      readc1 = readc2 = 1;
  }
  tgtContext->add(pmax);
  return icontextLength;
}

uint extMeasure::measureOverUnderCap() {
  int ll[2] = {_ll[0], _ll[1]};
  int ur[2] = {_ur[0], _ur[1]};
  ur[_dir] = ll[_dir];

  _tmpTable->resetCnt();
  _ouTable->resetCnt();
  _overTable->resetCnt();
  _underTable->resetCnt();

  uint ouLen = 0;
  uint underLen = 0;
  uint overLen = 0;

  if ((_met > 1) && (_met < (int)_layerCnt - 1)) {
    _underMet = _met - 1;
    _overMet = _met + 1;

    ouLen = computeOverUnder(ll, ur, _tmpTable);
  } else {
    addSeq(_tmpTable, _pixelTable);
    _underMet = _met;
    _overMet = _met;
  }
  uint totUnderLen = underLen;
  uint totOverLen = overLen;

  _underMet = _met;
  _overMet = _met;

  int remainderLength = _len - ouLen;

  while (remainderLength > 0) {
    _underMet--;
    _overMet++;

    underLen = 0;
    overLen = 0;
    if (_underMet > 0) {
      // cap over _underMet
      underLen = computeOverOrUnderSeq(_tmpTable, _underMet, _underTable, true);
      release(_tmpTable);

      if (_overMet < (int)_layerCnt) {
        // cap under _overMet
        overLen =
            computeOverOrUnderSeq(_underTable, _overMet, _overTable, false);
        release(_underTable);

        tableCopyP(_overTable, _tmpTable);
        _overTable->resetCnt();
        totOverLen += overLen;
      } else {
        tableCopyP(_underTable, _tmpTable);
        _underTable->resetCnt();
      }
      totUnderLen += underLen;
    } else if (_overMet < (int)_layerCnt) {
      overLen = computeOverOrUnderSeq(_tmpTable, _overMet, _overTable, false);
      release(_tmpTable);
      tableCopyP(_overTable, _tmpTable);
      _overTable->resetCnt();

      totOverLen += overLen;
    } else
      break;

    remainderLength -= (underLen + overLen);
  }
  release(_tmpTable);
  uint totLen = ouLen + totOverLen + totUnderLen;

  return totLen;
}
bool extMeasure::updateLengthAndExit(int& remainder, int& totCovered, int len) {
  if (len <= 0)
    return false;

  totCovered += len;
  remainder -= len;

  if (remainder <= 0)
    return true;

  return false;
}
int extMeasure::getDgPlaneAndTrackIndex(uint tgt_met, int trackDist,
                                        int& loTrack, int& hiTrack) {
  int n = tgt_met - *_dgContextBaseLvl + *_dgContextDepth;
  assert(n >= 0);
  if (n >= (int)*_dgContextPlanes)
    return -1;

  loTrack =
      _dgContextLowTrack[n] < -trackDist ? -trackDist : _dgContextLowTrack[n];
  hiTrack = _dgContextHiTrack[n] > trackDist ? trackDist : _dgContextHiTrack[n];

  return n;
}

// CJ doc notes
// suppose we do "ext ectract -cc_model 3", then
//   *_dgContextTracks is 7 (3*2 + 1), and *_dgContextTracks/2 is 3
//   _dgContextLowTrack[planeIndex] can be -3 ~ 0
//   _dgContextHiTrack[planeIndex] can be 0 ~ 3
// if we want to handle all Dg tracks in the plane, we can say:
//   int lowTrack = _dgContextLowTrack[planeIndex];
//   int hiTrack = _dgContextHiTrack[planeIndex];
// or if we want only from the lower 2 tracks to the higher 2, we can say:
//   int lowTrack = _dgContextLowTrack[planeIndex] < -2 ? -2 :
//   _dgContextLowTrack[planeIndex]; int hiTrack = _dgContextHiTrack[planeIndex]
//   > 2 ? 2 : _dgContextHiTrack[planeIndex];
//
void extMeasure::seq_release(Ath__array1D<SEQ*>* table) {
  for (uint jj = 0; jj < table->getCnt(); jj++) {
    SEQ* s = table->get(jj);
    _seqPool->free(s);
  }
  table->resetCnt();
}

uint extMeasure::computeDiag(SEQ* s, uint targetMet, uint dir, uint planeIndex,
                             uint trackn, Ath__array1D<SEQ*>* residueSeq) {
  Ath__array1D<SEQ*>* dgContext = _dgContextArray[planeIndex][trackn];
  if (dgContext->getCnt() <= 1)
    return 0;

  Ath__array1D<SEQ*> overlapSeq(16);
  getDgOverlap(s, _dir, dgContext, &overlapSeq, residueSeq);

  uint len = 0;
  for (uint jj = 0; jj < overlapSeq.getCnt(); jj++) {
    SEQ* tgt = overlapSeq.get(jj);
    uint diagDist = calcDist(tgt->_ll, tgt->_ur);
    uint tgWidth = tgt->_ur[_dir] - tgt->_ll[_dir];
    uint len1 = getLength(tgt, !_dir);

    DebugDiagCoords(_met, targetMet, len1, diagDist, tgt->_ll, tgt->_ur);
    len += len1;
    bool skip_high_acc = true;
#ifdef HI_ACC_1
    bool verticalOverlap = false;
    if (_dist < 0 && !skip_high_acc) {
      if (diagDist <= _width && diagDist >= 0 && (int)_width < 10 * _minWidth &&
          _verticalDiag) {
        verticalCap(_rsegSrcId, tgt->type, len1, tgWidth, diagDist, targetMet);
        verticalOverlap = true;
      } else if (((int)tgWidth > 10 * _minWidth) &&
                 ((int)_width > 10 * _minWidth) && (tgWidth >= 2 * diagDist)) {
        areaCap(_rsegSrcId, tgt->type, len1, targetMet);
        verticalOverlap = true;
      } else if ((int)tgWidth > 2 * _minWidth && tgWidth >= 2 * diagDist) {
        calcDiagRC(_rsegSrcId, tgt->type, len1, 1000000, targetMet);
      }
      // TO_OPTIMIZE
      else if (_diagModel == 2) {
        if (_verticalDiag)
          verticalCap(_rsegSrcId, tgt->type, len1, tgWidth, diagDist,
                      targetMet);
        else
          calcDiagRC(_rsegSrcId, tgt->type, len1, tgWidth, diagDist, targetMet);
      }
      //			if (!verticalOverlap) {
      if (!verticalOverlap && _overMet > _met + 1) {
        addSeq(tgt->_ll, tgt->_ur, residueSeq);
      }

      continue;
    }
#endif
    if (_diagModel == 2)
      calcDiagRC(_rsegSrcId, tgt->type, len1, tgWidth, diagDist, targetMet);
    if (_diagModel == 1)
      calcDiagRC(_rsegSrcId, tgt->type, len1, diagDist, targetMet);
  }
  seq_release(&overlapSeq);
  return len;
}

int extMeasure::computeDiagOU(SEQ* s, uint trackMin, uint trackMax,
                              uint targetMet, Ath__array1D<SEQ*>* diagTable) {
#ifdef HI_ACC_1
  // int trackDist = _extMain->_couplingFlag;
  int trackDist = 2;
#else
  int trackDist = 3;
#endif
  int loTrack;
  int hiTrack;
  int planeIndex =
      getDgPlaneAndTrackIndex(targetMet, trackDist, loTrack, hiTrack);
  if (planeIndex < 0)
    return 0;

  uint len = 0;

  Ath__array1D<SEQ*> tmpTable(16);
  copySeqUsingPool(s, &tmpTable);

  Ath__array1D<SEQ*> residueTable(16);

  int trackTable[200];
  uint cnt = 0;
  for (int kk = (int)trackMin; kk <= (int)trackMax;
       kk++)  // skip overlapping track
  {
#ifdef HI_ACC_1
    if (kk <= _dgContextHiTrack[planeIndex])
#else
    if (kk < _dgContextHiTrack[planeIndex])
#endif
      trackTable[cnt++] = *_dgContextTracks / 2 + kk;

    if (!kk)
      continue;
    if (-kk >= _dgContextLowTrack[planeIndex])
      trackTable[cnt++] = *_dgContextTracks / 2 - kk;
  }
  // DIMITRIS -- MENTOR pattern 06
  if (cnt == 0)
    trackTable[cnt++] = *_dgContextTracks / 2;
  // DIMITRIS -- END

  for (uint ii = 0; ii < cnt; ii++) {
    int trackn = trackTable[ii];

#ifdef HI_ACC_1
    if (_dgContextArray[planeIndex][trackn]->getCnt() <= 1)
      continue;
#endif
    bool add_all_diag = false;
    if (!add_all_diag) {
      for (uint jj = 0; jj < tmpTable.getCnt(); jj++)
        len += computeDiag(tmpTable.get(jj), targetMet, _dir, planeIndex,
                           trackn, &residueTable);
    } else {
      len += computeDiag(s, targetMet, _dir, planeIndex, trackn, &residueTable);
    }

    seq_release(&tmpTable);
    tableCopyP(&residueTable, &tmpTable);
    residueTable.resetCnt();
  }
  // seq_release(&tmpTable);
  if (diagTable != NULL)
    tableCopyP(&tmpTable, diagTable);
  else
    seq_release(&tmpTable);
  return len;
}

int extMeasure::compute_Diag_Over_Under(Ath__array1D<SEQ*>* seqTable,
                                        Ath__array1D<SEQ*>* resTable) {
  bool overUnder = true;
  int met1 = _underMet;
  int met2 = _overMet;

  if (_met - _underMet > _overMet - _met) {
    met2 = _underMet;
    met1 = _overMet;
    overUnder = false;
  }

  int totCovered = 0;
  for (uint ii = 0; ii < seqTable->getCnt(); ii++) {
    SEQ* s = seqTable->get(ii);

    if (s->type > 0)  // Black
      continue;

    uint len = s->_ur[!_dir] - s->_ll[!_dir];
    int remainder = len;

    if (_diagFlow) {
      if (_overMet < (int)_layerCnt)
        computeDiagOU(s, 0, 3, _overMet, NULL);
      // computeDiagOU(s, 0, 3, _overMet, _diagTable); // only diagonal, not
      // coupling
      // is additive to over/under
      // trying to capture single diagonal lines
    }

    addSeq(s->_ll, s->_ur, _diagTable);

    //		uint diagLen= 0;
    //		if (updateLengthAndExit(remainder, totCovered, diagLen))
    //			break;

    uint len1 = computeOverOrUnderSeq(_diagTable, met1, _underTable, overUnder);

    if (updateLengthAndExit(remainder, totCovered, len1))
      break;

    uint len2 = computeOverOrUnderSeq(_underTable, met2, resTable, !overUnder);

    if (updateLengthAndExit(remainder, totCovered, len2))
      break;

    release(_diagTable, _pixelTable);
    release(_underTable, _pixelTable);
  }
  release(_diagTable, _pixelTable);
  release(_underTable, _pixelTable);

  return totCovered;
}
int extMeasure::compute_Diag_OverOrUnder(Ath__array1D<SEQ*>* seqTable,
                                         bool over, uint met,
                                         Ath__array1D<SEQ*>* resTable) {
  int totCovered = 0;
  int diagTotLen = 0;
  for (uint ii = 0; ii < seqTable->getCnt(); ii++) {
    SEQ* s = seqTable->get(ii);

    if (s->type > 0)  // Black
      continue;

    uint len = s->_ur[!_dir] - s->_ll[!_dir];
    int remainder = len;

    //		uint diagLen= 0;

    if (_diagFlow) {
      if (!over) {
        computeDiagOU(s, 0, 3, met, NULL);
        // computeDiagOU(s, 0, 3, met, _diagTable); // only diagonal, not
        // coupling
        // is additive to over/under
        // trying to capture single diagonal lines

        //			if (updateLengthAndExit(remainder, totCovered,
        // diagLen)) 				break;
      }
    }
    addSeq(s->_ll, s->_ur, _diagTable);

    uint len1 = computeOverOrUnderSeq(_diagTable, met, resTable, over);

    if (updateLengthAndExit(remainder, totCovered, len1))
      break;

    release(_diagTable);
  }
  release(_diagTable);
  release(_underTable);

  return totCovered - diagTotLen;
}
uint extMeasure::measureUnderOnly(bool diagFlag) {
  int totCovered = 0;
  int remainderLen = _len;

  uint overLen = 0;
  _underMet = 0;
  for (_overMet = _met + 1; _overMet < (int)_layerCnt; _overMet++) {
    overLen =
        compute_Diag_OverOrUnder(_tmpSrcTable, false, _overMet, _tmpDstTable);

    if (updateLengthAndExit(remainderLen, totCovered, overLen))
      break;

    release(_tmpSrcTable);
    tableCopyP(_tmpDstTable, _tmpSrcTable);
    _tmpDstTable->resetCnt();
  }
  release(_tmpSrcTable);
  release(_tmpDstTable);

  return totCovered;
}
uint extMeasure::measureOverOnly(bool diagFlag) {
  int totCovered = 0;
  int remainder = _len;

  _overMet = -1;
  for (_underMet = _met - 1; _underMet > 0; _underMet--) {
    uint underLen =
        compute_Diag_OverOrUnder(_tmpSrcTable, true, _underMet, _tmpDstTable);

    if (updateLengthAndExit(remainder, totCovered, underLen))
      break;

    release(_tmpSrcTable);
    tableCopyP(_tmpDstTable, _tmpSrcTable);
    _tmpDstTable->resetCnt();
  }
  release(_tmpSrcTable);
  release(_tmpDstTable);

  return totCovered;
}
uint extMeasure::ouFlowStep(Ath__array1D<SEQ*>* overTable) {
  Ath__array1D<SEQ*> tmpTable(32);
  uint len = 0;
  for (uint ii = 0; ii < overTable->getCnt(); ii++) {
    SEQ* s = overTable->get(ii);

    /*		if (s->type==0) {
                            continue;
                    }
    */
    uint ouLen = getOverlapSeq(_underMet, s, &tmpTable);

    if (ouLen > 0) {
      calcOU(ouLen);
      len += ouLen;
    }
  }
  release(overTable);

  for (uint jj = 0; jj < tmpTable.getCnt(); jj++) {
    SEQ* s = tmpTable.get(jj);
    if (s->type == 0) {
      overTable->add(s);
      // s->type= 1;
      continue;
    }
    _pixelTable->release(s);
  }
  return len;
}
#ifdef DIAG_FIRST
int extMeasure::underFlowStep(Ath__array1D<SEQ*>* srcTable,
                              Ath__array1D<SEQ*>* overTable) {
  int totLen = 0;

  Ath__array1D<SEQ*> whiteTable(32);
  Ath__array1D<SEQ*> table1(32);
  Ath__array1D<SEQ*> diagTable(32);

  uint diagTotBeforeUnder = 0;
  for (uint ii = 0; ii < srcTable->getCnt(); ii++) {
    SEQ* s1 = srcTable->get(ii);

#ifdef HI_ACC_1
    if ((_overMet < _met + 3) && _diagFlow && _toHi) {
#else
    if ((_overMet < _met + 5) && _diagFlow && _toHi) {
#endif
      int diagLen = computeDiagOU(s1, 0, 1, _overMet, &diagTable);
      if (diagLen > 0) {
        diagTotBeforeUnder += diagLen;
        for (uint jj = 0; jj < diagTable.getCnt(); jj++) {
          SEQ* s2 = diagTable.get(jj);
          int oLen = getOverlapSeq(_overMet, s2, &table1);
          if (oLen > 0)
            totLen += oLen;
        }
        release(&diagTable);
        diagTable.resetCnt();
        //#ifndef HI_ACC_1
        continue;
        //#endif
      } else {
        release(&diagTable);
        diagTable.resetCnt();
      }
    }
#ifdef HI_ACC_1
    else if ((_overMet < _met + 5) && _diagFlow && _toHi) {
      computeDiagOU(s1, 0, 1, _overMet, NULL);
    }
#endif
    int oLen = getOverlapSeq(_overMet, s1, &table1);
    if (oLen > 0)
      totLen += oLen;
  }
  bool moreDiag =
      (_overMet < _met + 5) && _diagFlow && (diagTotBeforeUnder == 0) && _toHi;
#ifdef HI_ACC_1
  int trackDist = _extMain->_couplingFlag;
#else
  int trackDist = 3;
#endif
  for (uint jj = 0; jj < table1.getCnt(); jj++) {
    SEQ* s2 = table1.get(jj);

    if (s2->type == 0) {
      if (moreDiag)
        computeDiagOU(s2, 2, trackDist, _overMet, NULL);
      whiteTable.add(s2);
      continue;
    }
    overTable->add(s2);
    s2->type = 0;
  }
  release(srcTable);
  tableCopyP(&whiteTable, srcTable);

  return totLen;
}
#else
int extMeasure::underFlowStep(Ath__array1D<SEQ*>* srcTable,
                              Ath__array1D<SEQ*>* overTable) {
  int totLen = 0;

  Ath__array1D<SEQ*> whiteTable(32);
  Ath__array1D<SEQ*> table1(32);
  Ath__array1D<SEQ*> diagTable(32);

  for (uint ii = 0; ii < srcTable->getCnt(); ii++) {
    SEQ* s1 = srcTable->get(ii);
    int oLen = getOverlapSeq(_overMet, s1, &table1);
    if (oLen > 0)
      totLen += oLen;
  }
  for (uint jj = 0; jj < table1.getCnt(); jj++) {
    SEQ* s2 = table1.get(jj);

    if (s2->type == 0) {
      if (_diagFlow)
        _diagLen += computeDiagOU(s2, 0, 3, _overMet, NULL);
      whiteTable.add(s2);
      continue;
    }
    overTable->add(s2);
    s2->type = 0;
  }
  release(srcTable);
  tableCopyP(&whiteTable, srcTable);

  return totLen;
}
#endif
uint extMeasure::measureDiagFullOU() {
  // DEBUG_HERE

  _tmpSrcTable->resetCnt();
  _tmpDstTable->resetCnt();

  int ll[2] = {_ll[0], _ll[1]};
  int ur[2] = {_ur[0], _ur[1]};
  ur[_dir] = ll[_dir];
  //	ur[_dir]= (ll[_dir]+ur[_dir])/2;
  //	ll[_dir]= ur[_dir];

  addSeq(ll, ur, _tmpSrcTable);

#ifndef HI_ACC_1
  if (_met == 1)
    return measureUnderOnly(true);
#endif

  if (_met == (int)_layerCnt - 1)
    return measureOverOnly(false);

  _tmpTable->resetCnt();
  _ouTable->resetCnt();
  _overTable->resetCnt();
  _underTable->resetCnt();
  _diagTable->resetCnt();

  int totCovered = 0;
  int remainder = _len;

  // uint maxDist    = 10;
  uint maxDist = _extMain->_ccContextDepth;
  int upperLimit = _met + maxDist >= _layerCnt ? _layerCnt : _met + maxDist;
  int lowerLimit = _met - maxDist;
  if (lowerLimit < 0)
    lowerLimit = 0;

  //	uint totLen= 0;
  for (_overMet = _met + 1; _overMet < upperLimit; _overMet++) {
    int totUnderLen = underFlowStep(_tmpSrcTable, _overTable);
    if (totUnderLen <= 0) {
      release(_overTable);
      continue;
    }

    int underLen = totUnderLen;
    int totOUCovered = 0;
    bool skipUnderMetOverSub = false;
    for (_underMet = _met - 1; _underMet > lowerLimit; _underMet--) {
      uint overLen = ouFlowStep(_overTable);

      if (updateLengthAndExit(underLen, totOUCovered, overLen)) {
        skipUnderMetOverSub = true;
        release(_overTable);
        break;
      }
    }
    if (!skipUnderMetOverSub) {
      computeOverOrUnderSeq(_overTable, _overMet, _underTable, false);
      release(_underTable);
    }
    release(_overTable);
    if (updateLengthAndExit(remainder, totCovered, totUnderLen)) {
      release(_overTable);
      break;
    }
  }

  _overMet = -1;
  for (_underMet = _met - 1; _underMet > 0; _underMet--) {
    uint overLen =
        computeOverOrUnderSeq(_tmpSrcTable, _underMet, _tmpDstTable, true);

    if (updateLengthAndExit(remainder, totCovered, overLen))
      break;

    release(_tmpSrcTable);
    tableCopyP(_tmpDstTable, _tmpSrcTable);
    _tmpDstTable->resetCnt();
  }
  release(_tmpSrcTable);
  release(_tmpDstTable);

  return totCovered;
}

uint extMeasure::measureDiagOU(uint ouLevelLimit, uint diagLevelLimit) {
  return measureDiagFullOU();

  _tmpSrcTable->resetCnt();
  _tmpDstTable->resetCnt();

  int ll[2] = {_ll[0], _ll[1]};
  int ur[2] = {_ur[0], _ur[1]};
  ur[_dir] = ll[_dir];

  addSeq(ll, ur, _tmpSrcTable);

  if (_met == 1)
    return measureUnderOnly(true);

  if (_met == (int)_layerCnt - 1)
    return measureOverOnly(false);

  _tmpTable->resetCnt();
  _ouTable->resetCnt();
  _overTable->resetCnt();
  _underTable->resetCnt();
  _diagTable->resetCnt();

  int totCovered = 0;
  int remainder = _len;

  uint downDist = 1;
  uint upDist = 1;

  _underMet = _met;
  downDist = 0;
  _overMet = _met;
  upDist = 0;
  //	bool upDownFlag= false;
  while (true) {
    if (_underMet > 0) {
      _underMet--;
      downDist++;
    }

    if (_overMet < (int)_layerCnt) {
      _overMet++;
      upDist++;
    }
    if ((_underMet == 0) && (_overMet == (int)_layerCnt))
      break;

    if ((_underMet > 0) && (downDist <= ouLevelLimit) &&
        (upDist <= ouLevelLimit)) {
      uint ouLen = 0;

      for (uint ii = 0; ii < _tmpSrcTable->getCnt();
           ii++) {  // keep on adding inside loop
        SEQ* s = _tmpSrcTable->get(ii);

        if (s->type > 0)  // Black
          continue;

        ouLen += computeOverUnder(s->_ll, s->_ur, _ouTable);
      }
      release(_tmpSrcTable);
      if (updateLengthAndExit(remainder, totCovered, ouLen))
        break;
    } else {
      tableCopyP(_tmpSrcTable, _ouTable);
      _tmpSrcTable->resetCnt();
    }

    uint underLen = 0;
    uint overUnderLen = 0;

    if (_underMet > 0)
      overUnderLen = compute_Diag_Over_Under(_ouTable, _tmpDstTable);
    else
      underLen =
          compute_Diag_OverOrUnder(_ouTable, false, _overMet, _tmpDstTable);

    release(_ouTable);
    release(_tmpSrcTable);
    tableCopyP(_tmpDstTable, _tmpSrcTable);
    _tmpDstTable->resetCnt();

    if (updateLengthAndExit(remainder, totCovered, underLen + overUnderLen))
      break;
  }
  release(_tmpSrcTable);
  return totCovered;
}

void extMeasure::ccReportProgress() {
  uint repChunk = 1000000;
  if ((_totCCcnt > 0) && (_totCCcnt % repChunk == 0))
    logger_->info(RCX, 79, "Have processed {} CC caps, and stored {} CC caps",
                  _totCCcnt, _totBigCCcnt);
}
void extMeasure::printNet(dbRSeg* rseg, uint netId) {
  if (rseg == NULL)
    return;

  dbNet* net = rseg->getNet();

  if (netId == net->getId()) {
    _netId = netId;
    dbCapNode* cap2 = dbCapNode::getCapNode(_block, rseg->getTargetNode());
    uint node2 = cap2->getNode();
    uint shapeId2 = cap2->getShapeId();
  }
}
bool extMain::updateCoupCap(dbRSeg* rseg1, dbRSeg* rseg2, int jj, double v) {
  if (rseg1 != NULL && rseg2 != NULL) {
    dbCCSeg* ccap = dbCCSeg::create(
        dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
        dbCapNode::getCapNode(_block, rseg2->getTargetNode()), true);
    ccap->addCapacitance(v, jj);
    return true;
  }
  if (rseg1 != NULL)
    updateTotalCap(rseg1, v, jj);
  if (rseg2 != NULL)
    updateTotalCap(rseg2, v, jj);

  return false;
}
double extMain::calcFringe(extDistRC* rc, double deltaFr,
                           bool includeCoupling) {
  double ccCap = 0.0;
  if (includeCoupling)
    ccCap = rc->_coupling;

  double cap = rc->_fringe + ccCap - deltaFr;

  if (_gndcModify)
    cap *= _gndcFactor;

  return cap;
}
double extMain::updateTotalCap(dbRSeg* rseg, double cap, uint modelIndex) {
  if (rseg == NULL)
    return 0;

  int extDbIndex, sci, scDbIndex;
  extDbIndex = getProcessCornerDbIndex(modelIndex);
  double tot = rseg->getCapacitance(extDbIndex);
  tot += cap;
  if (_updateTotalCcnt >= 0) {
    if (_printFile == NULL)
      _printFile = fopen("updateCap.1", "w");
    _updateTotalCcnt++;
    fprintf(_printFile, "%d %d %g %g\n", _updateTotalCcnt, rseg->getId(), tot,
            cap);
  }

  rseg->setCapacitance(tot, extDbIndex);
  // return rseg->getCapacitance(extDbIndex);
  getScaledCornerDbIndex(modelIndex, sci, scDbIndex);
  if (sci == -1)
    return tot;
  getScaledGndC(sci, cap);
  double tots = rseg->getCapacitance(scDbIndex);
  tots += cap;
  rseg->setCapacitance(tots, scDbIndex);
  return tot;
}

void extDistRC::addRC(extDistRC* rcUnit, uint len, bool addCC) {
  if (rcUnit == NULL)
    return;

  _fringe += rcUnit->_fringe * len;

  if (addCC)  // dist based
    _coupling += rcUnit->_coupling * len;
}
double extMain::updateRes(dbRSeg* rseg, double res, uint model) {
  if (rseg == NULL)
    return 0;

  if (_eco && !rseg->getNet()->isWireAltered())
    return 0.0;

  if (_resModify)
    res *= _resFactor;

  if (_eco && !rseg->getNet()->isWireAltered())
    return 0.0;

  double tot = rseg->getResistance(model);
  tot += res;

  rseg->setResistance(tot, model);
  return rseg->getResistance(model);
}
bool extMeasure::isConnectedToBterm(dbRSeg* rseg1) {
  if (rseg1 == NULL)
    return false;

  dbCapNode* node1 = rseg1->getTargetCapNode();
  if (node1->isBTerm())
    return true;
  dbCapNode* node2 = rseg1->getSourceCapNode();
  if (node2->isBTerm())
    return true;

  return false;
}
bool extMeasure::isBtermConnection(dbRSeg* rseg1, dbRSeg* rseg2) {
  return _btermThreshold &&
         (isConnectedToBterm(rseg1) || isConnectedToBterm(rseg2)) &&
         (rseg1 != rseg2);
}

dbCCSeg* extMeasure::makeCcap(dbRSeg* rseg1, dbRSeg* rseg2, double ccCap) {
  if ((rseg1 != NULL) && (rseg2 != NULL) &&
      rseg1->getNet() != rseg2->getNet()) {  // signal nets

    _totCCcnt++;  // TO_TEST

    bool btermConnection = isBtermConnection(rseg1, rseg2);

    if ((ccCap >= _extMain->_coupleThreshold) || btermConnection) {
      _totBigCCcnt++;

      dbCapNode* node1 = rseg1->getTargetCapNode();
      dbCapNode* node2 = rseg2->getTargetCapNode();

      return dbCCSeg::create(node1, node2, true);
    } else {
      _totSmallCCcnt++;
      return NULL;
    }
  } else {
    return NULL;
  }
}
void extMeasure::addCCcap(dbCCSeg* ccap, double v, uint model) {
  double coupling = _ccModify ? v * _ccFactor : v;
  ccap->addCapacitance(coupling, model);
}
void extMeasure::addFringe(dbRSeg* rseg1, dbRSeg* rseg2, double frCap,
                           uint model) {
  if (_gndcModify)
    frCap *= _gndcFactor;

  if (rseg1 != NULL)
    _extMain->updateTotalCap(rseg1, frCap, model);

  if (rseg2 != NULL)
    _extMain->updateTotalCap(rseg2, frCap, model);
}

void extMeasure::calcDiagRC(int rsegId1, uint rsegId2, uint len, uint diagWidth,
                            uint diagDist, uint tgtMet) {
  double capTable[10];
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    capTable[ii] = len * getDiagUnderCC(rcModel, diagWidth, diagDist, tgtMet);
#ifdef DAVID_ACC_08_02
    _rc[ii]->_diag += capTable[ii];
    double ccTable[10];
    if (_dist > 0) {
      extDistRC* rc = getDiagUnderCC2(rcModel, diagWidth, diagDist, tgtMet);
      if (rc)
        ccTable[ii] = len * rc->_coupling;

      rc = rcModel->_capOver[_met]->getRC(0, _width, _dist);
      if (rc) {
        ccTable[ii] -= len * rc->_coupling;
        _rc[ii]->_coupling += ccTable[ii];
      }
    }
#endif
  }
  dbRSeg* rseg1 = NULL;
  dbRSeg* rseg2 = NULL;
  if (rsegId1 > 0)
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
  if (rsegId2 > 0)
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);

  dbCCSeg* ccCap = makeCcap(rseg1, rseg2, capTable[_minModelIndex]);

  for (uint model = 0; model < modelCnt; model++) {
    //		extMetRCTable* rcModel= _metRCTable.get(model);

    if (ccCap != NULL)
      addCCcap(ccCap, capTable[model], model);
    else
#ifdef HI_ACC_1
      addFringe(NULL, rseg2, capTable[model], model);
#else
      addFringe(rseg1, rseg2, capTable[model], model);
#endif
  }
}
void extMeasure::createCap(int rsegId1, uint rsegId2, double* capTable) {
  dbRSeg* rseg1 = NULL;
  dbRSeg* rseg2 = NULL;
  if (rsegId1 > 0)
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
  if (rsegId2 > 0)
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);

  dbCCSeg* ccCap = makeCcap(rseg1, rseg2, capTable[_minModelIndex]);

  uint modelCnt = _metRCTable.getCnt();
  for (uint model = 0; model < modelCnt; model++) {
    //		extMetRCTable* rcModel= _metRCTable.get(model);
    if (ccCap != NULL)
      addCCcap(ccCap, capTable[model], model);
    else
#ifdef HI_ACC_1
    {
      _rc[model]->_diag += capTable[model];
      addFringe(NULL, rseg2, capTable[model], model);
    }
#else
      addFringe(rseg1, rseg2, capTable[model], model);
#endif
  }
}
void extMeasure::areaCap(int rsegId1, uint rsegId2, uint len, uint tgtMet) {
  double capTable[10];
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    double area = 1.0 * _width;
    area *= len;
    extDistRC* rc = getUnderLastWidthDistRC(rcModel, tgtMet);
    capTable[ii] = 2 * area * rc->_fringe;  //_fringe always 1/2 of rulesGen

#ifdef HI_ACC_1
    dbRSeg* rseg2 = NULL;
    if (rsegId2 > 0)
      rseg2 = dbRSeg::getRSeg(_block, rsegId2);

    if (rseg2 != NULL) {
      uint met = _met;
      _met = tgtMet;
      extDistRC* area_rc = areaCapOverSub(ii, rcModel);
      _met = met;
      double areaCapOverSub = 2 * area * area_rc->_fringe;
      _extMain->updateTotalCap(rseg2, 0.0, 0.0, areaCapOverSub, ii);
    }
#endif

    // old way capTable[ii]= 2*area*getUnderRC(rcModel)->_fringe; //_fringe
    // always 1/2 of rulesGen
  }
  createCap(rsegId1, rsegId2, capTable);
}
extDistRC* extMeasure::areaCapOverSub(uint modelNum, extMetRCTable* rcModel) {
  if (rcModel == NULL)
    rcModel = _metRCTable.get(modelNum);

  extDistRC* rc = rcModel->getOverFringeRC(this);

  return rc;
}
bool extMeasure::verticalCap(int rsegId1, uint rsegId2, uint len, uint tgtWidth,
                             uint diagDist, uint tgtMet) {
  dbRSeg* rseg2 = NULL;
  if (rsegId2 > 0)
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);
  dbRSeg* rseg1 = NULL;
  if (rsegId1 > 0)
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);

  double capTable[10];
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    extDistRC* rc = getVerticalUnderRC(rcModel, diagDist, tgtWidth, tgtMet);
    if (rc == NULL)
      return false;

    //		double scale= 0.5*diagDist/tgtWidth;

    capTable[ii] = len * rc->_fringe;

    if ((rseg2 == NULL) && (rseg1 == NULL))
      continue;

    //		extDistRC *overSubFringe=
    //_metRCTable.get(ii)->_capOver[tgtMet]->getFringeRC(0, _width);
    extDistRC* overSubFringe =
        _metRCTable.get(ii)->_capOver[tgtMet]->getFringeRC(0, tgtWidth);
    if (overSubFringe == NULL)
      continue;
    double frCap = len * overSubFringe->_fringe;  // 02

    if (diagDist > tgtWidth) {
      //			double scale= 0.25*diagDist/_width;
      double scale = 0.25 * diagDist / tgtWidth;
      scale = 1.0 / scale;
      if (scale > 0.5)
        scale = 0.5;
      // if (tgtMet==_layerCnt-1)
      //	scale *= 0.5;

      frCap *= scale;
    }
    if (rseg2 != NULL)
      _extMain->updateTotalCap(rseg2, 0.0, 0.0, frCap, ii);
    if (rseg1 != NULL)
      _extMain->updateTotalCap(rseg1, 0.0, 0.0, 0.5 * frCap, ii);
  }
  createCap(rsegId1, rsegId2, capTable);
  return true;
}

void extMeasure::calcDiagRC(int rsegId1, uint rsegId2, uint len, uint dist,
                            uint tgtMet) {
  int DOUBLE_DIAG = 1;
  double capTable[10];
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

#ifdef HI_ACC_1
    if (dist != 1000000) {
      double cap = getDiagUnderCC(rcModel, dist, tgtMet);
      double diagCap = DOUBLE_DIAG * len * cap;
      capTable[ii] = diagCap;
      _rc[ii]->_diag += diagCap;

      const char* msg = "calcDiagRC";
      Debug_DiagValues(0.0, diagCap, msg);
    } else
      capTable[ii] = 2 * len * getUnderRC(rcModel)->_fringe;
#else
    capTable[ii] = len * getDiagUnderCC(rcModel, dist, tgtMet);
#endif
  }
  createCap(rsegId1, rsegId2, capTable);
}

void extMeasure::calcRC(dbRSeg* rseg1, dbRSeg* rseg2, uint totLenCovered) {
  bool btermConnection = isBtermConnection(rseg1, rseg2);

  int lenOverSub = _len - totLenCovered;
  //	int mUnder= _underMet; // will be replaced

  uint modelCnt = _metRCTable.getCnt();

  for (uint model = 0; model < modelCnt; model++) {
    extMetRCTable* rcModel = _metRCTable.get(model);

    if (totLenCovered > 0) {
      for (uint ii = 0; ii < _lenOUtable->getCnt(); ii++) {
        extLenOU* ou = _lenOUtable->get(ii);

        _overMet = ou->_overMet;
        _underMet = ou->_underMet;

        extDistRC* ouRC = NULL;

        if (ou->_over)
          ouRC = getOverRC(rcModel);
        else if (ou->_under)
          ouRC = getUnderRC(rcModel);
        else if (ou->_overUnder)
          ouRC = getOverUnderRC(rcModel);

        _rc[model]->addRC(ouRC, ou->_len, _dist > 0);
      }
    }

    if (_dist < 0) {  // dist is infinit
      ;               /*
extDistRC *rc1= _metRCTable.get(jj)->getOverFringeRC(this);
if (rc1!=NULL)
 deltaFr[jj]= rc1->getFringe() * totLenCovered;
*/
    } else {  // dist based

      _underMet = 0;

      extDistRC* rcOverSub = getOverRC(rcModel);
      if (lenOverSub > 0) {
        _rc[model]->addRC(rcOverSub, lenOverSub, _dist > 0);
      }
      double res = 0.0;
      if (rcOverSub != NULL)
        res = rcOverSub->_res * _len;

      double deltaFr = 0.0;
      double deltaRes = 0.0;
      extDistRC* rcMaxDist = rcModel->getOverFringeRC(this);

      if (rcMaxDist != NULL) {
        deltaFr = rcMaxDist->getFringe() * _len;
        deltaRes = rcMaxDist->_res * _len;
        res -= deltaRes;
      }

      if (rseg1 != NULL)
        _extMain->updateRes(rseg1, res, model);

      if (rseg2 != NULL)
        _extMain->updateRes(rseg2, res, model);

      dbCCSeg* ccap = NULL;
      bool includeCoupling = true;
      if ((rseg1 != NULL) && (rseg2 != NULL)) {  // signal nets

        _totCCcnt++;  // TO_TEST

        if ((_rc[_minModelIndex]->_coupling >= _extMain->_coupleThreshold) ||
            btermConnection) {
          //					dbNet* srcNet=
          // rseg1->getNet(); 					dbNet* tgtNet=
          // rseg2->getNet();

          // ccap= dbCCSeg::create(srcNet, rseg1->getTargetNode(),
          //		tgtNet, rseg2->getTargetNode(), true);

          ccap = dbCCSeg::create(
              dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
              dbCapNode::getCapNode(_block, rseg2->getTargetNode()), true);

          includeCoupling = false;
          _totBigCCcnt++;
        } else
          _totSmallCCcnt++;
      }
      extDistRC* finalRC = _rc[model];
      if (ccap != NULL) {
        double coupling =
            _ccModify ? finalRC->_coupling * _ccFactor : finalRC->_coupling;
        ccap->addCapacitance(coupling, model);
      }

      double frCap = _extMain->calcFringe(finalRC, deltaFr, includeCoupling);

      if (rseg1 != NULL)
        _extMain->updateTotalCap(rseg1, frCap, model);

      if (rseg2 != NULL)
        _extMain->updateTotalCap(rseg2, frCap, model);
    }
  }
  for (uint ii = 0; ii < _lenOUtable->getCnt(); ii++)
    _lenOUPool->free(_lenOUtable->get(ii));

  _lenOUtable->resetCnt();
}

void extMeasure::OverSubRC(dbRSeg* rseg1, dbRSeg* rseg2, int ouCovered,
                           int diagCovered, int srcCovered) {
  int res_lenOverSub = _len - ouCovered;  // 0228
  res_lenOverSub = 0;                     // 0315 -- new calc
  bool SCALING_RES = false;

  int DIAG_SUB_DIVIDER = 1;

  double SUB_MULT_CAP =
      1.0;  // Open ended resitance should account by 1/4 -- 11/15

  double SUB_MULT_RES = 1.0;
  if (SCALING_RES) {
    double dist_track = 0.0;
    SUB_MULT_RES = ScaleResbyTrack(true, dist_track);
    res_lenOverSub = _len;
  }
  int lenOverSub = _len - ouCovered;
  if (lenOverSub < 0)
    lenOverSub = 0;

  bool rvia1 = rseg1 != NULL && isVia(rseg1->getId());

  if (!((lenOverSub > 0) || (res_lenOverSub > 0))) {
    return;
  }

  _underMet = 0;
  for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
    extDistRC* rc = _metRCTable.get(jj)->getOverFringeRC(this);
    if (rc == NULL)
      continue;
    double cap = 0;
    double tot = 0;
    if (lenOverSub > 0) {
      cap = SUB_MULT_CAP * rc->getFringe() * lenOverSub;
      tot = _extMain->updateTotalCap(rseg1, cap, jj);
    }
    double res = 0;
    if (!_extMain->_lef_res && !rvia1) {
      if (res_lenOverSub > 0) {
        extDistRC* rc0 = _metRCTable.get(jj)->getOverFringeRC(this, 0);
        extDistRC* rc_last =
            _metRCTable.get(jj)->getOverFringeRC_last(_met, _width);
        double delta0 = rc0->_res - rc_last->_res;
        if (delta0 < 0)
          delta0 = -delta0;
        if (delta0 < 0.000001)
          SUB_MULT_RES = 1.0;
        // if (lenOverSub>0) {
        res = rc->getRes() * res_lenOverSub;
        res *= SUB_MULT_RES;
        _extMain->updateRes(rseg1, res, jj);
      }
    }
    const char* msg = "OverSubRC (No Neighbor)";
    OverSubDebug(rc, lenOverSub, res_lenOverSub, res, cap, msg);
  }
}

/**
 * ScaleResbyTrack scales the original calculation by
 * a coefficient depending on how many track(s) away the
 * nearest neighbors to the wire of interest.
 *
 * To scale the calculated resistance to be closer to
 * golden resistance from commercial tool.
 *
 * @return the scaling coefficient for the resistance.
 * @currently not used
 */
double extMeasure::ScaleResbyTrack(bool openEnded, double& dist_track) {
  dist_track = 0.0;

  bool SKIP_SCALING = false;
  if (SKIP_SCALING)
    return 1;

  // Dividers: 1, 1.2, 2, 3 respectively for tracks: 1, 2, >=3  --- assumption:
  // extRules is 1/2 of total res
  double SUB_MULT_RES = 1;
  if (openEnded) {
    SUB_MULT_RES = 0.4;
    return SUB_MULT_RES;
  }
  if (_extMain->_minDistTable[_met] > 0 && !SKIP_SCALING) {
    dist_track = _dist / _extMain->_minDistTable[_met];
    if (dist_track >= 3)
      SUB_MULT_RES = 2.0 / (1 + 4);
    else if (dist_track > 1 && dist_track <= 2)
      SUB_MULT_RES = 1;
    else if (dist_track > 2 && dist_track <= 4)
      SUB_MULT_RES = 2.0 / (1 + 3);
  }
  return SUB_MULT_RES;
}

void extMeasure::OverSubRC_dist(dbRSeg* rseg1, dbRSeg* rseg2, int ouCovered,
                                int diagCovered, int srcCovered) {
  double SUB_MULT = 1.0;
  double dist_track = 0.0;
  double SUB_MULT_RES = ScaleResbyTrack(false, dist_track);
  double res_lenOverSub = _len;
  res_lenOverSub = 0;  // 0315 -- new calc
  // ----------------------------------------- DF Scaling - 022821
  int lenOverSub = _len - ouCovered;

  int lenOverSub_bot = _len - srcCovered;
  if (lenOverSub_bot > 0)
    lenOverSub += lenOverSub_bot;

  // bool rvia1= rseg1!=NULL && extMain::getShapeProperty_rc(rseg1->getNet(),
  // rseg1->getId())>0; bool rvia2= rseg2!=NULL &&
  // extMain::getShapeProperty_rc(rseg2->getNet(), rseg2->getId())>0;
  bool rvia1 = rseg1 != NULL && isVia(rseg1->getId());
  bool rvia2 = rseg2 != NULL && isVia(rseg2->getId());

  if (!((lenOverSub > 0) || (res_lenOverSub > 0))) {
    return;
  }
  _underMet = 0;
  for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
    extMetRCTable* rcModel = _metRCTable.get(jj);
    extDistRC* rc = getOverRC(rcModel);

    extDistRC* rc_last = rcModel->getOverFringeRC_last(_met, _width);
    double delta = rc->_res - rc_last->_res;
    if (delta < 0)
      delta = -delta;

    extDistRC* rc0 = rcModel->getOverFringeRC(this, 0);
    double delta0 = rc0->_res - rc_last->_res;
    if (delta0 < 0)
      delta0 = -delta0;

    SUB_MULT_RES = 1.0;

    if (rc == NULL)
      continue;

    double res = rc->getRes() * res_lenOverSub;
    res *= SUB_MULT_RES;

    if (!_extMain->_lef_res) {
      if (!rvia1)
        _extMain->updateRes(rseg1, res, jj);
      if (!rvia2)
        _extMain->updateRes(rseg2, res, jj);
    }

    double tot = 0;
    double fr = 0;
    double cc = 0;
    if (lenOverSub > 0) {
      if (_sameNetFlag) {  // TO OPTIMIZE
        fr = SUB_MULT * rc->getFringe() * lenOverSub;
        tot = _extMain->updateTotalCap(rseg1, fr, jj);
      } else {
        double fr = SUB_MULT * rc->getFringe() * lenOverSub;
        tot = _extMain->updateTotalCap(rseg1, fr, jj);
        _extMain->updateTotalCap(rseg2, fr, jj);

        if (_dist > 0) {  // dist based
          cc = SUB_MULT * rc->getCoupling() * lenOverSub;
          _extMain->updateCoupCap(rseg1, rseg2, jj, cc);
          tot += cc;
        }
      }
    }
    const char* msg = "OverSubRC_dist (With Neighbor)";
    OverSubDebug(rc, lenOverSub, lenOverSub, res, fr + cc, msg);
  }
}

int extMeasure::computeAndStoreRC(dbRSeg* rseg1, dbRSeg* rseg2,
                                  int srcCovered) {
  bool DEBUG1 = false;
  if (DEBUG1) {
    segInfo("SRC", _netSrcId, _rsegSrcId);
    segInfo("DST", _netTgtId, _rsegTgtId);
  }
  // Copy from computeAndStoreRC_720
  bool SUBTRACT_DIAG = false;
  bool no_ou = true;
  bool USE_DB_UBITS = false;
  if (rseg1 == NULL && rseg2 == NULL)
    return 0;

  bool traceFlag = false;
  bool watchFlag = IsDebugNet();

  rcSegInfo();
  if (IsDebugNet())
    debugPrint(logger_, RCX, "debug_net", 1,
               "measureRC:"
               "C"
               "\t[BEGIN-OUD] ----- OverUnder/Diagonal RC ----- BEGIN");

  uint modelCnt = _metRCTable.getCnt();
  int totLenCovered = 0;
  _lenOUtable->resetCnt();
  if (_extMain->_usingMetalPlanes && (_extMain->_geoThickTable == NULL)) {
    _diagLen = 0;
    if (_extMain->_ccContextDepth > 0) {
      if (!_diagFlow)
        totLenCovered = measureOverUnderCap();
      else {
        totLenCovered = measureDiagOU(1, 2);
      }
    }
  }
  ouCovered_debug(totLenCovered);

  if (USE_DB_UBITS) {
    totLenCovered = _extMain->GetDBcoords2(totLenCovered);
    _len = _extMain->GetDBcoords2(_len);
    _diagLen = _extMain->GetDBcoords2(_diagLen);
  }
  int lenOverSub = _len - totLenCovered;

  if (_diagLen > 0 && SUBTRACT_DIAG)
    lenOverSub -= _diagLen;

  if (lenOverSub < 0)
    lenOverSub = 0;

  double deltaFr[10];
  double deltaRes[10];
  for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
    deltaFr[jj] = 0.0;
    deltaRes[jj] = 0.0;
  }
  double SUB_MULT = 1.0;
  bool COMPUTE_OVER_SUB = true;

  // Case where the geometric search returns no neighbor found
  // _dist is infinit
  if (_dist < 0) {
    if (totLenCovered < 0)
      totLenCovered = 0;

    _underMet = 0;

    _no_debug = true;
    _no_debug = false;

    bool rvia1 = isVia(rseg1->getId());
    for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
      bool ou = false;
      _rc[jj]->_res = 0;  // DF 022821 : Res non context based

      if (_rc[jj]->_fringe > 0) {
        ou = true;
        double tot = _extMain->updateTotalCap(rseg1, _rc[jj]->_fringe, jj);
      }
      if (ou && IsDebugNet())
        _rc[jj]->printDebugRC_values("OverUnder Total Open");
    }
    if (IsDebugNet())
      debugPrint(logger_, RCX, "debug_net", 1,
                 "measureRC:"
                 "C",
                 "\t[END-OUD] ----- OverUnder/Diagonal ----- END");
    rcSegInfo();

    if (COMPUTE_OVER_SUB) {
      OverSubRC(rseg1, NULL, totLenCovered, _diagLen, _len);
      return totLenCovered;
    }
  } else {  // dist based

    _underMet = 0;

    // getFringe(_len, deltaFr);
    // _no_debug= true;
    //	computeR(_len, deltaRes);
    // _no_debug= false;
    //	_extMain->updateTotalRes(rseg1, rseg2, this, deltaRes, modelCnt);

    bool rvia1 = rseg1 != NULL && isVia(rseg1->getId());
    bool rvia2 = rseg2 != NULL && isVia(rseg2->getId());

    for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
      bool ou = false;
      double totR1 = 0;
      double totR2 = 0;
      /* Res NOT context dependent DF: 022821
      if (!_extMain->_lef_res && _rc[jj]->_res > 0) {
        if (!rvia1) {
          ou = true;
          totR1 = _extMain->updateRes(rseg1, _rc[jj]->_res, jj);
        }
        if (!rvia2) {
          ou = true;
          totR2 = _extMain->updateRes(rseg2, _rc[jj]->_res, jj);
        }
      } */
      double tot1 = 0;
      double tot2 = 0;
      if (_rc[jj]->_fringe > 0) {
        ou = true;
        tot1 = _extMain->updateTotalCap(rseg1, _rc[jj]->_fringe, jj);
        tot2 = _extMain->updateTotalCap(rseg2, _rc[jj]->_fringe, jj);
      }
      if (_rc[jj]->_coupling > 0) {
        ou = true;
        _extMain->updateCoupCap(rseg1, rseg2, jj, _rc[jj]->_coupling);
      }
      tot1 += _rc[jj]->_coupling;
      tot2 += _rc[jj]->_coupling;
      if (ou && IsDebugNet())
        _rc[jj]->printDebugRC_values("OverUnder Total Dist");
    }
    rcSegInfo();
    if (IsDebugNet())
      debugPrint(logger_, RCX, "debug_net", 1,
                 "measureRC:"
                 "C"
                 "\t[END-OUD] ------ OverUnder/Diagonal RC ------ END");

    if (COMPUTE_OVER_SUB) {
      // OverSubRC(rseg1, NULL, totLenCovered, _diagLen, srcCovered);
      OverSubRC_dist(rseg1, rseg2, totLenCovered, _diagLen, _len);
      return totLenCovered;
    }
  }
  return totLenCovered;
}

void extMeasure::measureRC(CoupleOptions& options) {
  // return;

  _totSegCnt++;

  int rsegId1 = options[1];  // dbRSeg id for SRC segment
  int rsegId2 = options[2];  // dbRSeg id for Target segment

  //  	if ((rsegId1<0)&&(rsegId2<0)) // power nets
  //  		return;

  _rsegSrcId = rsegId1;
  _rsegTgtId = rsegId2;

  defineBox(options);

  if (_extMain->_lefRC)
    return;

//_netId= 0;
#ifdef DEBUG_NET
  uint debugNetId = DEBUG_NET;
#endif

  dbRSeg* rseg1 = NULL;
  dbNet* srcNet = NULL;
  uint netId1 = 0;
  if (rsegId1 > 0) {
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
    srcNet = rseg1->getNet();
    netId1 = srcNet->getId();
#ifdef DEBUG_NET
    printNet(rseg1, debugNetId);
#endif
  }
  // fprintf(stdout, "net: %d %s\n", netId1, srcNet->getConstName());
  _netSrcId = netId1;

  dbRSeg* rseg2 = NULL;
  dbNet* tgtNet = NULL;
  uint netId2 = 0;
  if (rsegId2 > 0) {
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);
    tgtNet = rseg2->getNet();
    netId2 = tgtNet->getId();
#ifdef DEBUG_NET
    printNet(rseg2, debugNetId);
#endif
  }
#ifdef HI_ACC_1
  _sameNetFlag = (srcNet == tgtNet);
#endif

  _netTgtId = netId2;

  _totSignalSegCnt++;

  if (_met >= (int)_layerCnt)  // TO_TEST
    return;
  //	if (netId1==netId2)
  //		return;

  if (_extMain->_measureRcCnt >= 0) {
    if (_extMain->_printFile == NULL)
      _extMain->_printFile = fopen("measureRC.1", "w");
    fprintf(_extMain->_printFile,
            "%d met= %d  len= %d  dist= %d r1= %d r2= %d\n", _totSignalSegCnt,
            _met, _len, _dist, rsegId1, rsegId2);
  }
  if (_extMain->_geoThickTable != NULL) {
    double diff = 0.0;

    if ((_extMain->_geoThickTable[_met] != NULL) &&
        !_extMain->_geoThickTable[_met]
             ->getThicknessDiff(_ll[0], _ll[1], _width, diff)) {
      _metRCTable.set(0, _extMain->getRCmodel(0)->getMetRCTable(0));
    } else {
      uint n = _extMain->getRCmodel(0)->findBiggestDatarateIndex(diff);
      n = _extMain->getRCmodel(0)->findClosestDataRate(n, diff);
      _metRCTable.set(0, _extMain->getRCmodel(0)->getMetRCTable(n));
    }

    /*
                    double thRef= 0.5;
                    double nomThick= 0.0;
                    double thickDiff=
       _extMain->_geoThickTable[_met]->getThicknessDiff(_ll[0], _ll[1], _width,
       nomThick);

                    double th= 0.001*nomThick*(1+thickDiff);

                    double diff= (th-thRef)/thRef;
                    if (diff==0.0) {
                            _metRCTable.set(1,
       _extMain->getRCmodel(0)->getMetRCTable(0));
                    }
                    else {
                            uint n=
       _extMain->getRCmodel(0)->findBiggestDatarateIndex(diff);
                            _metRCTable.set(1,
       _extMain->getRCmodel(0)->getMetRCTable(n));
                    }
                    // set 2nd model with
    */
  }
  //	uint modelCnt= _metRCTable.getCnt();
  _verticalDiag = _currentModel->getVerticalDiagFlag();
  int prevCovered = options[20];
  prevCovered = 0;

  // -------------------------------- db units -------------
  bool USE_DB_UNITS = false;
  if (USE_DB_UNITS) {
    if (_dist > 0)
      _dist = _extMain->GetDBcoords2(_dist);
    _width = _extMain->GetDBcoords2(_width);
  }
  // if (_dist>0)
  //  _dist= 64;

  _netId = _extMain->_debug_net_id;

  DebugStart();

  int totCovered = computeAndStoreRC(rseg1, rseg2, prevCovered);

  bool rvia1 = rseg1 != NULL && isVia(rseg1->getId());
  if (!rvia1 && rseg1 != NULL) {
    for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
      _rc[jj]->_res = 0;
    }
    bool DEBUG1 = _netId == 604801;
    if (IsDebugNet()) {
      const char* netName = "";
      if (_netId > 0) {
        dbNet* net = dbNet::getNet(_block, _netId);
        netName = net->getConstName();
      }
      fprintf(stdout,
              " ---------------------------------------------------------------"
              "------------------\n");
      fprintf(stdout, "     %7d %7d %7d %7d    M%d  D%d  L%d   N%d N%d %s\n",
              _ll[0], _ur[0], _ll[1], _ur[1], _met, _dist, _len, _netSrcId,
              _netTgtId, netName);
      fprintf(stdout,
              " ---------------------------------------------------------------"
              "------------------\n");
    }
    uint track_num = options[18];
    double deltaRes[10];
    for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
      deltaRes[jj] = 0.0;
    }
    // if (_dist>0)

    SEQ* s = addSeq(_ll, _ur);
    int len_covered = computeResDist(s, 1, 4, _met, NULL);
    int len_down_not_coupled = _len - len_covered;

    int maxDist = getMaxDist(_met, 0);
    if (_dist > 0 && len_down_not_coupled > 0) {
      calcRes(rseg1->getId(), len_down_not_coupled, 0, _dist, _met);
      len_covered += len_down_not_coupled;
    }
    if (len_covered > 0) {
      calcRes0(deltaRes, _met, len_covered);
    }
    // calcRes(_rsegSrcId, len_down_not_coupled, _dist, 0, _met);

    for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
      double totR1 = _rc[jj]->_res;
      if (totR1 > 0) {
        totR1 -= deltaRes[jj];
        double totalSegCap = 0;
        if (totR1 != 0.0)
          totalSegCap = _extMain->updateRes(rseg1, totR1, jj);
      }
    }
  }
  if (IsDebugNet())
    debugPrint(logger_, RCX, "debug_net", 1,
               "[END-DistRC:C]"
               "\tDistRC:C"
               " ----- measureRC: ----- END\n");
  options[20] = totCovered;

  // ccReportProgress();
}

int extMeasure::computeAndStoreRC_720(dbRSeg* rseg1, dbRSeg* rseg2,
                                      int srcCovered) {
  bool SUBTRACT_DIAG = false;
  bool no_ou = true;
  bool USE_DB_UBITS = false;
  if (rseg1 == NULL && rseg2 == NULL)
    return 0;

  bool traceFlag = false;
  bool watchFlag = IsDebugNet();
  _netId = _extMain->_debug_net_id;
  if (_netId > 0)
    traceFlag = printTraceNet("\nBEGIN", true, NULL, 0, 0);

  uint modelCnt = _metRCTable.getCnt();
  int totLenCovered = 0;
  _lenOUtable->resetCnt();
  if (_extMain->_usingMetalPlanes && (_extMain->_geoThickTable == NULL)) {
    _diagLen = 0;
    if (_extMain->_ccContextDepth > 0) {
      if (!_diagFlow)
        totLenCovered = measureOverUnderCap();
      else
        totLenCovered = measureDiagOU(1, 2);
    }
  }
  totLenCovered += srcCovered;

#ifdef MIN_FOR_LOOPS

  calcRC(rseg1, rseg2, totLenCovered);
  return;
#endif

  double deltaFr[10];
  double deltaRes[10];
  for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
    deltaFr[jj] = 0.0;
    deltaRes[jj] = 0.0;
  }
  if (USE_DB_UBITS) {
    totLenCovered = _extMain->GetDBcoords2(totLenCovered);
    _len = _extMain->GetDBcoords2(_len);
    _diagLen = _extMain->GetDBcoords2(_diagLen);
  }
  int lenOverSub = _len - totLenCovered;

  if (_diagLen > 0 && SUBTRACT_DIAG)
    lenOverSub -= _diagLen;

  if (lenOverSub < 0)
    lenOverSub = 0;

  if (traceFlag) {
    debugPrint(logger_, RCX, "debug_net", 2,
               "Trace:"
               "C"
               "            OU {}  SUB {}  DIAG {}  PREV_COVERED {}",
               totLenCovered, lenOverSub, _diagLen, srcCovered);
    printNetCaps();
  }
  //	printTraceNet("OU", false, NULL, lenOverSub, totLenCovered);

  //	int mUnder= _underMet; // will be replaced

  double SUB_MULT = 1.0;
  bool COMPUTE_OVER_SUB = true;
  if (_dist < 0) {  // dist is infinit
    if (totLenCovered < 0)
      totLenCovered = 0;

    computeR(_len, deltaRes);
    _extMain->updateTotalRes(rseg1, rseg2, this, deltaRes, modelCnt);

    if (totLenCovered > 0) {
      _underMet = 0;
      for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
        extDistRC* rc = _metRCTable.get(jj)->getOverFringeRC(this);
        if (rc != NULL) {
          deltaFr[jj] = SUB_MULT * rc->getFringe() * totLenCovered;
          deltaRes[jj] = rc->getRes() * totLenCovered;
        }
      }
      // computeR(_len, deltaRes);
      // _extMain->updateTotalRes(rseg1, rseg2, this, deltaRes, modelCnt);

      if (rseg1 != NULL)
#ifdef HI_ACC_1
        _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, false, false);
#else
        _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, false);
#endif
      if (rseg2 != NULL)
        _extMain->updateTotalCap(rseg2, this, deltaFr, modelCnt, false);

      if (traceFlag)
        printTraceNet("0D", false, NULL, lenOverSub, totLenCovered);
    }
  } else {  // dist based

    if (lenOverSub > 0) {
      _underMet = 0;
      computeOverRC(lenOverSub);
    }

    // deltaFr[jj]= getFringe(m._met, m._width, jj) * m._len; TO_TEST
    // ---------------------------------- TO DEBUG ------------
    getFringe(_len, deltaFr);
    // ---------------------------------- TO DEBUG ------------

    computeR(_len, deltaRes);
    _extMain->updateTotalRes(rseg1, rseg2, this, deltaRes, modelCnt);

    if ((rseg1 != NULL) && (rseg2 != NULL)) {  // signal nets

      bool btermConnection = isBtermConnection(rseg1, rseg2);

      _totCCcnt++;  // TO_TEST
#ifdef HI_ACC_2
      if (rseg1->getNet() == rseg2->getNet()) {  // same signal net
#ifdef HI_ACC_1
        _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, false, true);
#else
        _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, false);
#endif
        _extMain->updateTotalCap(rseg2, this, deltaFr, modelCnt, false);

        if (traceFlag)
          printTraceNet("AC2", false, NULL, 0, 0);

        return totLenCovered;
      }
#endif

      if ((_rc[_minModelIndex]->_coupling < _extMain->_coupleThreshold) &&
          !btermConnection) {  // TO_TEST

#ifdef HI_ACC_1
        _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, true, true);
#else
        _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, true);
#endif
        _extMain->updateTotalCap(rseg2, this, deltaFr, modelCnt, true);

        if (traceFlag)
          printTraceNet("cC", false);

        _totSmallCCcnt++;

        return totLenCovered;
      }
      _totBigCCcnt++;

      //			dbNet* srcNet= rseg1->getNet();
      //			dbNet* tgtNet= rseg2->getNet();

      dbCCSeg* ccap = dbCCSeg::create(
          dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
          dbCapNode::getCapNode(_block, rseg2->getTargetNode()), true);

      double cap;
      int extDbIndex, sci, scDbIndex;
      for (uint jj = 0; jj < modelCnt; jj++) {
        cap = _ccModify ? _rc[jj]->_coupling * _ccFactor : _rc[jj]->_coupling;
        extDbIndex = _extMain->getProcessCornerDbIndex(jj);
        ccap->addCapacitance(cap, extDbIndex);
        _extMain->getScaledCornerDbIndex(jj, sci, scDbIndex);
        if (sci != -1) {
          _extMain->getScaledCC(sci, cap);
          ccap->addCapacitance(cap, scDbIndex);
        }
        int net1 = rseg1->getNet()->getId();
        int net2 = rseg2->getNet()->getId();
        if (_netId == net1 || _netId == net2) {
          debugPrint(logger_, RCX, "debug_net", 2,
                     "Trace:"
                     "C"
                     "\taddCapacitance-CC:  {}-{} {}-{} {}",
                     net1, rseg1->getId(), net2, rseg2->getId(), cap);
        }
      }
      // --------------------------- to test it was include_coupling= false
      bool include_coupling = false;
// updateCCCap(rseg1, rseg2, m._rc->_coupling);
#ifdef HI_ACC_1
      _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, include_coupling,
                               true);
#else
      _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, false);
#endif
      _extMain->updateTotalCap(rseg2, this, deltaFr, modelCnt, false);

      if (traceFlag)
        printTraceNet("CC", false, ccap);

    } else if (rseg1 != NULL) {
#ifdef HI_ACC_1
      _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, true, true);
#else
      _extMain->updateTotalCap(rseg1, this, deltaFr, modelCnt, true);
#endif
      if (traceFlag)
        printTraceNet("GN", false);
    } else if (rseg2 != NULL) {
      _extMain->updateTotalCap(rseg2, this, deltaFr, modelCnt, true);
      if (traceFlag)
        printTraceNet("GN", false);
    }
  }
  return totLenCovered;
}

void extMeasure::getDgOverlap(SEQ* sseq, uint dir,
                              Ath__array1D<SEQ*>* dgContext,
                              Ath__array1D<SEQ*>* overlapSeq,
                              Ath__array1D<SEQ*>* residueSeq) {
  int idx = dgContext->get(0)->_ll[0];
  uint lp = dir ? 0 : 1;  // x : y
  uint wp = dir ? 1 : 0;  // y : x
  SEQ* rseq;
  SEQ* tseq;
  SEQ* wseq;
  int covered = sseq->_ll[lp];

#ifdef HI_ACC_1
  if (idx == dgContext->getCnt()) {
    rseq = _seqPool->alloc();
    rseq->_ll[wp] = sseq->_ll[wp];
    rseq->_ur[wp] = sseq->_ur[wp];
    rseq->_ll[lp] = sseq->_ll[lp];
    rseq->_ur[lp] = sseq->_ur[lp];
    residueSeq->add(rseq);
    return;
  }
#endif

  dbRSeg* srseg = NULL;
  if (_rsegSrcId > 0)
    srseg = dbRSeg::getRSeg(_block, _rsegSrcId);
  for (; idx < (int)dgContext->getCnt(); idx++) {
    tseq = dgContext->get(idx);
    if (tseq->_ur[lp] <= covered)
      continue;

    if (tseq->_ll[lp] >= sseq->_ur[lp]) {
      rseq = _seqPool->alloc();
      rseq->_ll[wp] = sseq->_ll[wp];
      rseq->_ur[wp] = sseq->_ur[wp];
      rseq->_ll[lp] = covered;
      rseq->_ur[lp] = sseq->_ur[lp];
      assert(rseq->_ur[lp] >= rseq->_ll[lp]);
      residueSeq->add(rseq);
      break;
    }
#ifdef CHECK_SAME_NET
    dbRSeg* trseg = NULL;
    if (tseq->type > 0)
      trseg = dbRSeg::getRSeg(_block, tseq->type);
    if ((trseg != NULL) && (srseg != NULL) &&
        (trseg->getNet() == srseg->getNet())) {
      if ((tseq->_ur[lp] >= sseq->_ur[lp]) ||
          (idx == (int)dgContext->getCnt() - 1)) {
        rseq = _seqPool->alloc();
        rseq->_ll[wp] = sseq->_ll[wp];
        rseq->_ur[wp] = sseq->_ur[wp];
        rseq->_ll[lp] = covered;
        rseq->_ur[lp] = sseq->_ur[lp];
        assert(rseq->_ur[lp] >= rseq->_ll[lp]);
        residueSeq->add(rseq);
        break;
      } else
        continue;
    }
#endif
    wseq = _seqPool->alloc();
    wseq->type = tseq->type;
    wseq->_ll[wp] = tseq->_ll[wp];
    wseq->_ur[wp] = tseq->_ur[wp];
    if (tseq->_ur[lp] <= sseq->_ur[lp])
      wseq->_ur[lp] = tseq->_ur[lp];
    else
      wseq->_ur[lp] = sseq->_ur[lp];
    if (tseq->_ll[lp] <= covered)
      wseq->_ll[lp] = covered;
    else {
      wseq->_ll[lp] = tseq->_ll[lp];
      rseq = _seqPool->alloc();
      rseq->_ll[wp] = sseq->_ll[wp];
      rseq->_ur[wp] = sseq->_ur[wp];
      rseq->_ll[lp] = covered;
      rseq->_ur[lp] = tseq->_ll[lp];
      assert(rseq->_ur[lp] >= rseq->_ll[lp]);
      residueSeq->add(rseq);
    }
    assert(wseq->_ur[lp] >= wseq->_ll[lp]);
    overlapSeq->add(wseq);
    covered = wseq->_ur[lp];
    if (tseq->_ur[lp] >= sseq->_ur[lp])
      break;
    if (idx == (int)dgContext->getCnt() - 1 && covered < sseq->_ur[lp]) {
      rseq = _seqPool->alloc();
      rseq->_ll[wp] = sseq->_ll[wp];
      rseq->_ur[wp] = sseq->_ur[wp];
      rseq->_ll[lp] = covered;
      rseq->_ur[lp] = sseq->_ur[lp];
      assert(rseq->_ur[lp] >= rseq->_ll[lp]);
      residueSeq->add(rseq);
    }
  }
  dgContext->get(0)->_ll[0] = idx;
}

void extMeasure::getDgOverlap(CoupleOptions& options) {
  int ttttprintOverlap = 1;
  int srcseqcnt = 0;
  if (ttttprintOverlap && !_dgContextFile) {
    _dgContextFile = fopen("overlapSeq.1", "w");
    fprintf(_dgContextFile, "wire overlapping context:\n");
    srcseqcnt = 0;
  }
  uint met = -options[0];
  srcseqcnt++;
  SEQ* seq = _seqPool->alloc();
  SEQ* pseq;
  int dir = options[6];
  uint xidx = 0;
  uint yidx = 1;
  uint lidx, bidx;
  lidx = dir == 1 ? xidx : yidx;
  bidx = dir == 1 ? yidx : xidx;
  seq->_ll[lidx] = options[1];
  seq->_ll[bidx] = options[3];
  seq->_ur[lidx] = options[2];
  seq->_ur[bidx] = options[4];
  if (ttttprintOverlap)
    fprintf(_dgContextFile,
            "\nSource Seq %d:ll_0=%d ll_1=%d ur_0=%d ur_1=%d met=%d dir=%d\n",
            srcseqcnt, seq->_ll[0], seq->_ll[1], seq->_ur[0], seq->_ur[1], met,
            dir);
  Ath__array1D<SEQ*> overlapSeq(16);
  Ath__array1D<SEQ*> residueSeq(16);
  Ath__array1D<SEQ*>* dgContext = NULL;
  for (int jj = 1; jj <= *_dgContextHiLvl; jj++) {
    int gridn = *_dgContextDepth + jj;
    for (int kk = 0; kk <= _dgContextHiTrack[gridn]; kk++) {
      int trackn = *_dgContextTracks / 2 + kk;
      dgContext = _dgContextArray[gridn][trackn];
      overlapSeq.resetCnt();
      residueSeq.resetCnt();
      getDgOverlap(seq, dir, dgContext, &overlapSeq, &residueSeq);
      if (!ttttprintOverlap)
        continue;

      for (uint ss = 0; ss < overlapSeq.getCnt(); ss++) {
        pseq = overlapSeq.get(ss);
        fprintf(
            _dgContextFile,
            "\n    overlap %d:ll_0=%d ll_1=%d ur_0=%d ur_1=%d met=%d trk=%d\n",
            ss, pseq->_ll[0], pseq->_ll[1], pseq->_ur[0], pseq->_ur[1],
            jj + met, kk + _dgContextBaseTrack[gridn]);
      }
      for (uint ss1 = 0; ss1 < residueSeq.getCnt(); ss1++) {
        if (ss1 == 0 && overlapSeq.getCnt() == 0)
          fprintf(_dgContextFile, "\n");
        pseq = residueSeq.get(ss1);
        fprintf(
            _dgContextFile,
            "    residue %d:ll_0=%d ll_1=%d ur_0=%d ur_1=%d met=%d trk=%d\n",
            ss1, pseq->_ll[0], pseq->_ll[1], pseq->_ur[0], pseq->_ur[1],
            jj + met, kk + _dgContextBaseTrack[gridn]);
      }
    }
  }
}

void extMeasure::initTargetSeq() {
  Ath__array1D<SEQ*>* dgContext = NULL;
  SEQ* seq;
  for (int jj = 1; jj <= *_dgContextHiLvl; jj++) {
    int gridn = *_dgContextDepth + jj;
    for (int kk = _dgContextLowTrack[gridn]; kk <= _dgContextHiTrack[gridn];
         kk++) {
      int trackn = *_dgContextTracks / 2 + kk;
      dgContext = _dgContextArray[gridn][trackn];
      seq = dgContext->get(0);
      seq->_ll[0] = 1;
    }
  }
}

void extMeasure::printDgContext() {
  if (_dgContextFile == NULL)
    //_dgContextFile=stdout;
    return;

  _dgContextCnt++;
  fprintf(_dgContextFile, "diagonalContext %d: baseLevel %d\n", _dgContextCnt,
          *_dgContextBaseLvl);
  Ath__array1D<SEQ*>* dgContext = NULL;
  SEQ* seq = NULL;
  for (int jj = *_dgContextLowLvl; jj <= *_dgContextHiLvl; jj++) {
    int gridn = *_dgContextDepth + jj;
    fprintf(_dgContextFile, "  level %d, plane %d, baseTrack %d\n",
            *_dgContextBaseLvl + jj, gridn, _dgContextBaseTrack[gridn]);
    int lowTrack = _dgContextLowTrack[gridn];
    int hiTrack = _dgContextHiTrack[gridn];
    for (int kk = lowTrack; kk <= hiTrack; kk++) {
      int trackn = *_dgContextTracks / 2 + kk;
      dgContext = _dgContextArray[gridn][trackn];
      fprintf(_dgContextFile, "    track %d (%d), %d seqs\n",
              _dgContextBaseTrack[gridn] + kk, trackn, dgContext->getCnt());
      for (uint ii = 0; ii < dgContext->getCnt(); ii++) {
        seq = dgContext->get(ii);
        fprintf(_dgContextFile,
                "      seq %d: ll_0=%d ll_1=%d ur_0=%d ur_1=%d rseg=%d\n", ii,
                seq->_ll[0], seq->_ll[1], seq->_ur[0], seq->_ur[1], seq->type);
      }
    }
  }
}

}  // namespace rcx
