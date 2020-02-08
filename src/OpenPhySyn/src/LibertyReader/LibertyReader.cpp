// BSD 3-Clause License

// Copyright (c) 2019, SCALE Lab, Brown University
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.

// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.

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
#include "LibertyReader.hpp"
#include <OpenPhySyn/Sta/DatabaseStaNetwork.hpp>
#include <OpenSTA/liberty/LeakagePower.hh>
#include <OpenSTA/liberty/Liberty.hh>
#include <OpenSTA/liberty/LibertyBuilder.hh>
#include <OpenSTA/liberty/LibertyReader.hh>
#include <OpenSTA/liberty/LibertyReaderPvt.hh>
#include <OpenSTA/network/Network.hh>
#include <OpenSTA/util/Error.hh>
#include "PsnException/FileException.hpp"
#include "PsnException/ParseLibertyException.hpp"
namespace psn
{

LibertyReader::LibertyReader(sta::DatabaseSta* sta) : sta_(sta)
{
}

Liberty*
LibertyReader::read(const char* path, bool infer_latches)
{
    try
    {
        Liberty* liberty = sta_->readLiberty(
            path, sta_->cmdCorner(), sta::MinMaxAll::all(), infer_latches);
        return liberty;
    }
    catch (sta::StaException& e)
    {
        throw ParseLibertyException(e.what());
    }
}

} // namespace psn