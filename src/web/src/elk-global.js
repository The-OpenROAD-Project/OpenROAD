// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// netlistsvg's prebuilt bundle reads ELK off the global scope as it loads.  A
// module of its own, since imports are hoisted above a module's statements.

import ELK from 'elkjs/lib/elk.bundled.js';

window.ELK = ELK;
