// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared esbuild settings for the viewer's bundles.  See //src/web:BUILD.

export default {
    // iife, not esm: the saved report inlines the bundle inside a plain
    // <script> block, where a module's imports would have nothing to resolve
    // against.
    format: 'iife',
    platform: 'browser',
    target: ['es2022'],
    loader: {
        // leaflet.css and the golden-layout themes reach their icons through
        // relative url(); inlining them keeps the whole page to two files and
        // means no icon is ever fetched from a CDN (issue #11065).
        '.png': 'dataurl',
        // schematic-widget.js hands the skin to onml.p(), which wants raw XML.
        '.svg': 'text',
    },
};
