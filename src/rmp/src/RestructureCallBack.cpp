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
  rest_->setTieLoPort(desc->getLoCell(), desc->getLoPort());
  rest_->setTieHiPort(desc->getHiCell(), desc->getHiPort());
  for (auto lib_file : desc->getLibFiles()) {
    rest_->addLibFile(lib_file);
  }
  rest_->setWorkDirName(desc->getWorkDirName());
  rest_->setPostABCScript(desc->getPostABCScript());
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

  dst::JobMessage resultMsg(dst::JobMessage::SUCCESS);
  auto uResultDesc = std::make_unique<RestructureJobDescription>();
  uResultDesc->setMode(desc->getMode());
  uResultDesc->setDelay(delay);
  uResultDesc->setNumInstances(numInstances);
  uResultDesc->setLevelGain(levelGain);
  uResultDesc->setBlifPath(blif_path);
  resultMsg.setJobDescription(std::move(uResultDesc));

  dist_->sendResult(resultMsg, sock);
  sock.close();
}
