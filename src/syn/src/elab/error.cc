// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The only elab/ TU that pulls utl/Logger.h (and transitively spdlog's
// bundled fmt). The slang-using TUs (backend_builder.cc, driver.cc) avoid
// the include because slang brings a different fmt version, and mixing the
// two in one TU causes inline-namespace collisions on fmt::vNN.
//
// Hosts the two pieces that genuinely need the full utl::Logger definition:
//   - slang_frontend::reportError  (calls Logger::error)
//   - syn::elaborateText           (default-constructs a utl::Logger)

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "driver.h"
#include "syn/ir/Graph.h"
#include "utl/Logger.h"

namespace syn {

// Defined in driver.cc.
std::unique_ptr<Graph> elaborateImpl(utl::Logger* logger,
                                     const std::vector<std::string>& args,
                                     sta::dbSta* sta,
                                     std::optional<std::string> buffer);

std::unique_ptr<Graph> elaborateText(const std::string& source,
                                     const std::vector<std::string>& args)
{
  utl::Logger logger;
  return elaborateImpl(&logger, args, nullptr, source);
}

}  // namespace syn

namespace slang_frontend {

void reportError(utl::Logger* logger, int code, std::string_view message)
{
  logger->error(utl::SYN, code, "{}", message);
}

}  // namespace slang_frontend
