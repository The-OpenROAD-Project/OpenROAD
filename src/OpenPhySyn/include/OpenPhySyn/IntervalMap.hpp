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

#pragma once

#include <iterator>
#include <limits>
#include <map>

// IntervalMap can be used to map an interval/range to an arbitrary value
// Useful for for caching cells associated with numerical ranges.
template<typename Key, typename Value>
class IntervalMap
{
private:
    std::map<Key, Value> map_;

public:
    IntervalMap(Value const& v)
    {
        map_.insert(map_.begin(),
                    std::make_pair(std::numeric_limits<Key>::min(), v));
    }

    void
    assign(Key const& begin, Key const& end, Value const& val)
    {

        auto lower_bound_begin = map_.lower_bound(begin);
        auto delete_begin      = lower_bound_begin;
        auto search_begin      = lower_bound_begin->first;
        if (--lower_bound_begin != map_.end())
        {
            search_begin = lower_bound_begin->first;
        }

        auto lower_bound_end = map_.lower_bound(end);
        auto upper_bound_end = map_.upper_bound(end);
        if (lower_bound_end == upper_bound_end)
        {
            --lower_bound_end;
        }
        auto  delete_end = lower_bound_end;
        Value tail       = delete_end->second;
        auto  search_end = lower_bound_end->first;

        map_.erase(delete_begin, ++delete_end);
        map_.insert(std::begin(map_), std::make_pair(begin, val));
        map_.insert(std::begin(map_), std::make_pair(end, tail));

        auto  it_start = map_.find(search_begin);
        Value prev     = it_start->second;
        it_start++;
        auto it_end = map_.find(search_end);
        it_end++;
        while (it_start != it_end)
        {
            if (it_start->second == prev)
            {
                auto to_del = it_start;
                it_start++;
                map_.erase(to_del);
            }
            else
            {
                prev = it_start->second;
                it_start++;
            }
        }
    }

    Value& operator[](Key const& key)
    {
        return (--map_.upper_bound(key))->second;
    }
};