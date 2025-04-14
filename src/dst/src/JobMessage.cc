// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "dst/JobMessage.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <sstream>
#include <string>

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
