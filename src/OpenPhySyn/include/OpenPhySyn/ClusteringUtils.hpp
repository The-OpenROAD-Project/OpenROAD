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

#include <cmath>
#include <functional>
#include <iostream>
#include <unordered_set>
#include <vector>

namespace psn
{
class KCenterClustering
{
    template<class T>
    static float
    diameter(std::vector<T>&                   objects,
             const std::function<float(T, T)>& distance)
    {
        if (!objects.size())
        {
            return 0.0;
        }
        float max_dist = -1E30;
        for (T& p : objects)
        {
            for (T& q : objects)
            {
                float dist = distance(p, q);
                max_dist   = std::max(max_dist, dist);
            }
        }
        return max_dist;
    }

public:
    template<class T>
    static std::vector<T>
    cluster(std::vector<T>&                   superset,
            const std::function<float(T, T)>& distance, float cluster_threshold,
            int starting_index = 0)
    {
        if (!superset.size())
        {
            return std::vector<T>();
        }
        else if (superset.size() == 1)
        {
            return std::vector<T>({superset[0]});
        }
        T&    b_init   = superset[starting_index];
        float max_dist = -1E30;

        T b_hat_init;
        for (T& obj : superset)
        {
            float dist = distance(obj, b_init);
            if (dist > max_dist)
            {
                b_hat_init = obj;
                max_dist   = distance(obj, b_init);
            }
        }

        std::vector<T>        seeds;
        std::unordered_set<T> seeds_set;
        seeds.push_back(b_hat_init);
        seeds_set.insert(b_hat_init);

        float d          = diameter<T>(superset, distance);
        float s_diameter = diameter<T>(superset, distance);

        while ((d / s_diameter) > cluster_threshold)
        {
            max_dist = -1E30;
            T b_hat;
            for (T& b_hat_needle : superset)
            {
                if (!seeds_set.count(b_hat_needle))
                {
                    float min_dist = 1E30;
                    for (T& b : seeds)
                    {
                        float dist = distance(b, b_hat_needle);
                        if (dist < min_dist)
                        {
                            min_dist = dist;
                        }
                    }
                    if (min_dist > max_dist)
                    {
                        max_dist = min_dist;
                        b_hat    = b_hat_needle;
                    }
                }
            }
            d = max_dist;
            seeds.push_back(b_hat);
            seeds_set.insert(b_hat);
        }

        std::vector<std::vector<T>> clusters;
        std::vector<T>              centers;
        for (auto& seed : seeds)
        {
            clusters.push_back(std::vector<T>({seed}));
        }

        for (auto& b : superset)
        {
            if (!seeds_set.count(b))
            {
                float min_dist  = 1E30;
                float min_index = -1;
                for (size_t i = 0; i < clusters.size(); ++i)
                {
                    auto& cluster = clusters[i];
                    for (auto& w : cluster)
                    {
                        float dist = distance(b, w);
                        if (dist < min_dist)
                        {
                            min_index = i;
                            min_dist  = dist;
                        }
                    }
                }
                clusters[min_index].push_back(b);
                seeds_set.insert(b);
            }
        }

        for (auto& cluster : clusters)
        {
            if (!cluster.size())
            {
                continue;
            }
            else if (cluster.size() == 1)
            {
                centers.push_back(cluster[0]);
                continue;
            }
            float min_dist  = 1E30;
            float min_index = -1;
            for (size_t i = 0; i < cluster.size(); ++i)
            {
                max_dist = -1E30;
                for (T& hat : cluster)
                {
                    if (cluster[i] != hat)
                    {
                        float dist = distance(cluster[i], hat);
                        if (dist > max_dist)
                        {
                            max_dist = dist;
                        }
                    }
                }
                if (max_dist < min_dist)
                {
                    min_dist  = max_dist;
                    min_index = i;
                }
            }
            centers.push_back(cluster[min_index]);
        }

        return centers;
    }
};
} // namespace psn
