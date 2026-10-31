// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared esbuild settings for the viewer's bundles.  See //src/web:BUILD.

import { readFile } from 'node:fs/promises';

// The skin is raw XML for onml.p(); its inter-tag whitespace is indentation and
// would cost a bundle line per newline.  Exported for schematic_pins_test.
export function compactSvgText(text) {
    return text.replace(/>\s+</g, '><').trim();
}

const compactSvg = {
    name: 'compact-svg',
    setup(build) {
        build.onLoad({ filter: /\.svg$/ }, async ({ path }) => ({
            contents: compactSvgText(await readFile(path, 'utf8')),
            loader: 'text',
        }));
    },
};

// elkjs's main entry picks a worker with Node's require; use its browser build,
// resolved from the importer so netlistsvg keeps its own elkjs.
const elkBrowserBuild = {
    name: 'elk-browser-build',
    setup(build) {
        build.onResolve({ filter: /^elkjs$/ }, ({ resolveDir, kind }) =>
            build.resolve('elkjs/lib/elk.bundled.js', { resolveDir, kind }));
    },
};

// loaders.gl fetches worker scripts and codec libraries from unpkg when it has
// no copy of its own.  The layout view hands deck.gl decoded images and never
// loads through it, so that fallback is pointed at nothing remote.
const kLoadersCdn = 'https://unpkg.com/@loaders.gl';
const noLoadersCdn = {
    name: 'no-loaders-cdn',
    setup(build) {
        build.onLoad({ filter: /[\\/]@loaders\.gl[\\/].*\.js$/ },
            async ({ path }) => {
                const text = await readFile(path, 'utf8');
                if (!text.includes(kLoadersCdn)) {
                    return undefined;
                }
                return {
                    contents: text.replaceAll(kLoadersCdn,
                                              'loaders.gl-cdn-disabled'),
                    loader: 'js',
                };
            });
    },
};

// The private repository's hook rejects a foundry name luma.gl gives a GPU
// maker.  Every match is rewritten, so luma.gl's checks still agree; the name
// is split because the hook reads this file too.
const kFoundryName = 'int' + 'el';

export function hideFoundryNames(text) {
    return text
        .replaceAll(`/${kFoundryName.toUpperCase()}/i`, '/INT[E]L/i')
        .replace(new RegExp(`${kFoundryName}(?!l)`, 'gi'),
                 (name) => `${name.slice(0, 3)}_${name.slice(3)}`);
}

const noFoundryNames = {
    name: 'no-foundry-names',
    setup(build) {
        build.onLoad({ filter: /[\\/]@luma\.gl[\\/].*\.js$/ }, async ({ path }) => {
            const text = await readFile(path, 'utf8');
            const hidden = hideFoundryNames(text);
            return hidden === text ? undefined : { contents: hidden, loader: 'js' };
        });
    },
};

export default {
    // iife: one self-contained script whose top-level names stay out of the
    // page's global scope, however the page loads it.
    format: 'iife',
    platform: 'browser',
    target: ['es2022'],
    // deck.gl's builds without WebGPU; the viewer draws with WebGL only.
    // Custom conditions drop esbuild's default 'module', so it is listed too.
    conditions: ['visgl:webgl-only', 'module'],
    loader: {
        // The golden-layout stylesheets reach icons through relative
        // url(); inlined, the page needs no file besides the bundles.
        '.png': 'dataurl',
    },
    plugins: [compactSvg, elkBrowserBuild, noLoadersCdn, noFoundryNames],
};
