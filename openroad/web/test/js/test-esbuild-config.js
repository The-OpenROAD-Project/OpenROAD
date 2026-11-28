// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import config from '../../esbuild.config.mjs';

// Setting conditions at all drops esbuild's default 'module', which the
// bundles have always resolved with.
describe('conditions', () => {
    it("picks deck.gl's WebGL-only builds and keeps 'module'", () => {
        assert.ok(config.conditions.includes('visgl:webgl-only'));
        assert.ok(config.conditions.includes('module'));
    });
});
