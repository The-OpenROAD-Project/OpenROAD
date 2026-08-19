# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

or_integration_tests(
  "web"
  PASSFAIL_TESTS
    cpp_tests
)

# The report bundler strips module syntax with regexes; this pins the shapes.
add_test(
  NAME web_embed_report_assets
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/embed_report_assets_test.py
)

# The CSP names the inline scripts by hash; a hash over anything but the served
# bytes blocks them with no build-time failure.
add_test(
  NAME web_embed_web_assets
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/embed_web_assets_test.py
)

# The fetch empties --dest and short-circuits on a stamp; both are quiet when
# wrong.  Needs no network.
add_test(
  NAME web_fetch_packages
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/fetch_packages_test.py
)

# The asset lists exist once per build system; this keeps them equal.
add_test(
  NAME web_asset_list_parity
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/asset_list_parity.py
)

add_subdirectory(cpp)
