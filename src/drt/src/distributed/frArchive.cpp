// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "frArchive.h"

// explicit instantiation of class templates involved
namespace boost {
namespace archive {
template class basic_binary_oarchive<drt::frOArchive>;
template class basic_binary_iarchive<drt::frIArchive>;
template class binary_oarchive_impl<drt::frOArchive,
                                    std::ostream::char_type,
                                    std::ostream::traits_type>;
template class binary_iarchive_impl<drt::frIArchive,
                                    std::istream::char_type,
                                    std::istream::traits_type>;
template class detail::archive_serializer_map<drt::frOArchive>;
template class detail::archive_serializer_map<drt::frIArchive>;
}  // namespace archive
}  // namespace boost
