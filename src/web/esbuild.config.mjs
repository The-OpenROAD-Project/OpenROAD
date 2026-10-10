// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared esbuild settings for the viewer's bundles.  See //src/web:BUILD.

export default {
    // iife: one self-contained script whose top-level names stay out of the
    // page's global scope, however the page loads it.
    format: 'iife',
    platform: 'browser',
    target: ['es2022'],
    loader: {
        // leaflet.css and the golden-layout themes reach icons through relative
        // url(); inlined, the page needs no file besides the two blobs.
        '.png': 'dataurl',
        // schematic-widget.js hands the skin to onml.p(), which wants raw XML.
        '.svg': 'text',
    },
};
