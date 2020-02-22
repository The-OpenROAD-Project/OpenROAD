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

#include <OpenPhySyn/DesignSettings.hpp>
#include <OpenPhySyn/PsnGlobal.hpp>
#include <OpenPhySyn/PsnLogger.hpp>

namespace psn
{

DesignSettings::DesignSettings(float max_area, float res_per_micron,
                               float cap_per_micron)
{
    setMaxArea(max_area);
    setResistancePerMicron(res_per_micron);
    setCapacitancePerMicron(cap_per_micron);
}

float
DesignSettings::maxArea() const
{
    if (max_area_ == 0)
    {
        PSN_LOG_WARN("Max area is 0 or not set.");
    }
    return max_area_;
}
float
DesignSettings::resistancePerMicron() const
{
    if (res_per_micron_ == 0)
    {
        PSN_LOG_WARN("Resistance per micron is 0 or not set, use_set_wire_rc.");
    }
    return res_per_micron_;
}
float
DesignSettings::capacitancePerMicron() const
{
    if (cap_per_micron_ == 0)
    {
        PSN_LOG_WARN(
            "Capacitance per micron is 0 or not set, use set_wire_rc.");
    }
    return cap_per_micron_;
}

DesignSettings*
DesignSettings::setMaxArea(float max_area)
{
    max_area_ = max_area;
    return this;
}
DesignSettings*
DesignSettings::setResistancePerMicron(float res_per_micron)
{
    res_per_micron_ = res_per_micron;
    return this;
}
DesignSettings*
DesignSettings::setCapacitancePerMicron(float cap_per_micron)
{
    cap_per_micron_ = cap_per_micron;
    return this;
}
} // namespace psn
