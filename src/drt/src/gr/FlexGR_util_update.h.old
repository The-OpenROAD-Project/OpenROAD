/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
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

#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <tuple>
#include <mutex>
#include <condition_variable>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"
#include "FlexGR_util.h"
#include "stt/SteinerTreeBuilder.h"

namespace drt {

__device__
int getIdx_device(int x, int y, int z, int x_dim, int y_dim, int z_dim);

__device__
bool getBit_device(uint64_t* cmap, unsigned idx, unsigned pos);

__device__
unsigned getBits_device(uint64_t* cmap, unsigned idx, unsigned pos, unsigned length);

__device__
void setBits_device(uint64_t* cmap, unsigned idx, unsigned pos, unsigned length, unsigned val);

__device__
void addToBits_device(uint64_t* cmap, unsigned idx, unsigned pos, unsigned length, unsigned val);

__device__
unsigned getRawDemand_device(uint64_t* cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir);

__device__
unsigned getRawSupply_device(uint64_t* cmap, 
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir);

__device__
bool hasBlock_device(uint64_t* cmap, 
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir);

__device__
void addRawDemand_device(
  uint64_t* cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta = 1);


__device__
float getCongCost_device(unsigned rawSupply, unsigned rawDemand);


__device__
unsigned getHistoryCost_device(
  uint64_t* cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z);

// Thread-safe queue wrapper
template <typename T>
class SafeQueue {
  private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cv;

  public:
    // Push task into the queue
    void push(const T& value) {
      std::lock_guard<std::mutex> lock(mtx);
      queue.push(value);
      cv.notify_one();
    }

    // Pop task from the queue
    bool pop(T& value) {
      std::unique_lock<std::mutex> lock(mtx);
      cv.wait(lock, [this] { return !queue.empty(); });
      value = queue.front();
      queue.pop();
      return true;
    }

    // Check if the queue is empty
    bool empty() {
      std::lock_guard<std::mutex> lock(mtx);
      return queue.empty();
    }
};



struct NetStruct {
  frNet* net = nullptr;
  int netId = -1;
  int nodeCntStart = -1;
  int numNodes = -1;

  std::vector<int> points; // Store all the steiner points in the net
  std::vector<std::pair<int, int> > vSegments;
  std::vector<std::pair<int, int> > hSegments;

  NetStruct() = default;

  NetStruct(frNet* _net, int _netId, int _nodeCntStart, int _numNodes) 
    : net(_net), netId(_netId), nodeCntStart(_nodeCntStart), numNodes(_numNodes) { }
 
};


// In the ripup and reroute stage, we need to keep tracking gcells with overflow
// and the nets passing through the grid
struct GridStruct {
  int xIdx = -1;
  int yIdx = -1;
  int zIdx = 0; // Default is 2D grid
  int idx = -1;  // 1D index  
  bool isOverflowH = false;
  bool isOverflowV = false;

  std::set<frNet*> nets; // All the nets passing through the grid
  
  GridStruct() = default;
  GridStruct(int _xIdx, int _yIdx, int _zIdx, int _idx) 
    : xIdx(_xIdx), yIdx(_yIdx), zIdx(_zIdx), idx(_idx) { }

};


struct IntPair {
  int start = -1;
  int end = -1;

  __host__ __device__
  IntPair() = default;

  __host__ __device__
  IntPair(int _start, int _end) : start(_start), end(_end) { }

  __host__ __device__
  int x() const { return start; }
  
  __host__ __device__
  int y() const { return end; }

  __host__ __device__
  void set(int _start, int _end) {
    start = _start;
    end = _end;
  }

};



} // namespace drt


