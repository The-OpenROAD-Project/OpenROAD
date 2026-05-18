# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

namespace eval syn {

proc elaborate { args } {
  syn::elaborate_cmd [join $args " "]
}

}
