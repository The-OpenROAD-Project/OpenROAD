// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//
// Import Liberty cell definitions as blackbox modules into a slang Compilation.
//
#pragma once

namespace slang {
class SourceManager;
namespace ast {
class Compilation;
}
}  // namespace slang

namespace sta {
class dbSta;
}

namespace syn {

void importBlackboxesFromLiberty(slang::SourceManager& mgr,
                                 slang::ast::Compilation& target,
                                 sta::dbSta* sta);

}  // namespace syn
