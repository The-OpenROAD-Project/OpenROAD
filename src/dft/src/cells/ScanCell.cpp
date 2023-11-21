///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
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

#include "ScanCell.hh"

#include "ClockDomain.hh"

namespace dft {

ScanCell::ScanCell(const std::string& name,
                   std::unique_ptr<ClockDomain> clock_domain,
                   utl::Logger* logger)
    : name_(name), clock_domain_(std::move(clock_domain)), logger_(logger)
{
}

std::string_view ScanCell::getName() const
{
  return name_;
}

const ClockDomain& ScanCell::getClockDomain() const
{
  return *clock_domain_;
}

void ScanCell::Connect(const ScanLoad& load,
                       const ScanDriver& driver,
                       bool preserve) const
{
  std::visit(
      [this, &driver, preserve](auto&& load_term) {
        std::visit(
            [this, &load_term, preserve](auto&& driver_term) {
              this->Connect(load_term, driver_term, preserve);
            },
            driver.getValue());
      },
      load.getValue());
}

const char* ScanCell::GetTermName(odb::dbBTerm* term)
{
  return term->getConstName();
}

const char* ScanCell::GetTermName(odb::dbITerm* term)
{
  return term->getMTerm()->getConstName();
}

}  // namespace dft
