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

#include "dst/JobMessage.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <sstream>

#include "dst/BalancerJobDescription.h"

using namespace dst;

template <class Archive>
inline bool is_loading(const Archive& ar)
{
  return std::is_same<typename Archive::is_loading, boost::mpl::true_>::value;
}

template <class Archive>
void JobMessage::serialize(Archive& ar, const unsigned int version)
{
  (ar) & msg_type_;
  (ar) & job_type_;
  (ar) & desc_;
  if (!is_loading(ar)) {
    std::string eop = EOP;
    (ar) & eop;
  }
}

bool JobMessage::serializeMsg(SerializeType type,
                              JobMessage& msg,
                              std::string& str)
{
  if (type == WRITE) {
    try {
      std::ostringstream oarchive_stream;
      boost::archive::text_oarchive archive(oarchive_stream);
      archive << msg;
      str = oarchive_stream.str();
    } catch (const boost::archive::archive_exception& e) {
      return false;
    }
  } else {
    try {
      std::istringstream iarchive_stream(str);
      boost::archive::text_iarchive archive(iarchive_stream);
      archive >> msg;
    } catch (const boost::archive::archive_exception& e) {
      return false;
    }
  }
  return true;
}