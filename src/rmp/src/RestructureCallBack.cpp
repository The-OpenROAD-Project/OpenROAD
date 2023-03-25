/* Authors: Osama */
/*
 * Copyright (c) 2021, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "RestructureCallBack.h"

#include <stdio.h>
#include <stdlib.h>

#include "RestructureJobDescription.h"
#include "dst/Distributed.h"
#include "ord/OpenRoad.hh"
#include "rmp/Restructure.h"
#include "utl/Logger.h"

using namespace rmp;

void RestructureCallBack::onRestructureJobReceived(dst::JobMessage& msg,
                                                   dst::socket& sock)
{
  if (msg.getJobType() != dst::JobMessage::RESTRUCTURE)
    return;

  RestructureJobDescription* desc
      = static_cast<RestructureJobDescription*>(msg.getJobDescription());
  auto db = rest_->db_;
  if (db->getChip()) {
    auto newDb = odb::dbDatabase::create();
    FILE* stream = fopen(desc->getODBPath().c_str(), "r");
    if (stream == nullptr) {
      return;
    }
    newDb->read(stream);
    fclose(stream);
    for (auto inst : db->getChip()->getBlock()->getInsts()) {
      if (inst->isFixed())
        continue;
      for (auto iterm : inst->getITerms()) {
        auto net = iterm->getNet();
        if (net == nullptr)
          continue;
        iterm->disconnect();
        if (net->getITerms().size() == 0 && net->getBTerms().size() == 0
            && !net->isSpecial())
          odb::dbNet::destroy(net);
      }
      odb::dbInst::destroy(inst);
    }
    auto block = db->getChip()->getBlock();
    for (auto newInst : newDb->getChip()->getBlock()->getInsts()) {
      if (block->findInst(newInst->getName().c_str()))
        continue;
      auto master = db->findMaster(newInst->getMaster()->getName().c_str());
      auto inst
          = odb::dbInst::create(block, master, newInst->getName().c_str());
      inst->setOrient(newInst->getOrient());
      int x, y;
      newInst->getOrigin(x, y);
      inst->setOrigin(x, y);
      inst->setPlacementStatus(newInst->getPlacementStatus());
      inst->setSourceType(newInst->getSourceType());
      for (auto iterm : inst->getITerms()) {
        auto newNet = newInst->findITerm(iterm->getMTerm()->getName().c_str())
                          ->getNet();
        if (newNet) {
          auto netName
              = newInst->findITerm(iterm->getMTerm()->getName().c_str())
                    ->getNet()
                    ->getName();
          auto net = block->findNet(netName.c_str());
          if (net == nullptr) {
            net = odb::dbNet::create(block, newNet->getName().c_str());
            net->setSigType(newNet->getSigType());
          }
          iterm->connect(net);
          if (net->isSpecial())
            iterm->setSpecial();
        }
      }
    }

  } else {
    for (auto file : desc->getLibFiles()) {
      Tcl_Eval(ord::OpenRoad::openRoad()->tclInterp(),
               fmt::format("read_liberty {}", file).c_str());
    }
    ord::OpenRoad::openRoad()->readDb(desc->getODBPath().c_str());
    rest_->block_ = db->getChip()->getBlock();
    Tcl_Eval(ord::OpenRoad::openRoad()->tclInterp(),
             fmt::format("read_sdc {}", desc->getSDCPath()).c_str());
  }
  rest_->resizer_->setAllRC(desc->getWireSignalRes(),
                            desc->getWireSignalCap(),
                            desc->getWireClockRes(),
                            desc->getWireClockCap());
  rest_->resizer_->estimateWireParasitics();
  rest_->lib_file_names_ = desc->getLibFiles();
  rest_->locell_ = desc->getLoCell();
  rest_->loport_ = desc->getLoPort();
  rest_->hicell_ = desc->getHiCell();
  rest_->hiport_ = desc->getHiPort();
  rest_->work_dir_name_ = desc->getWorkDirName();
  rest_->post_abc_script_ = desc->getPostABCScript();
  rest_->path_insts_.clear();
  auto block = rest_->block_;
  for (auto id : desc->getReplaceableInstsIds()) {
    rest_->path_insts_.insert(block->findInst(id.c_str()));
  }
  int numInstances = -1;
  int levelGain = -1;
  float delay = -1.0;
  std::string blif_path = desc->getBlifPath();
  rest_->runABCJob(desc->getMode(),
                   desc->getIterations(),
                   numInstances,
                   levelGain,
                   delay,
                   blif_path);
  auto dbFileName = fmt::format("{}{}_{}.db",
                                desc->getWorkDirName(),
                                desc->getMode(),
                                desc->getIterations());
  ord::OpenRoad::openRoad()->writeDb(dbFileName.c_str());
  dst::JobMessage resultMsg(dst::JobMessage::SUCCESS);
  auto uResultDesc = std::make_unique<RestructureJobDescription>();
  uResultDesc->setMode(desc->getMode());
  uResultDesc->setDelay(delay);
  uResultDesc->setNumInstances(numInstances);
  uResultDesc->setLevelGain(levelGain);
  uResultDesc->setBlifPath(blif_path);
  uResultDesc->setODBPath(dbFileName);
  resultMsg.setJobDescription(std::move(uResultDesc));

  dist_->sendResult(resultMsg, sock);
  sock.close();
}
