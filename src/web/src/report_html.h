// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <string_view>

namespace web {

// What index.html holds between its saved-report markers, which a saved
// report copies into its own head; "" unless both markers are there.
std::string reportHead(std::string_view index_html);

// JSON to write inside a <script>: each "<", only ever within its strings,
// becomes \u003c, which reads the same and cannot end the block.
std::string scriptSafeJson(std::string_view json);

// Text for the inside of an HTML comment: splits the sequences that would
// open or close one early.
std::string htmlCommentSafe(std::string text);

}  // namespace web
