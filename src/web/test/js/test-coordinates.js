// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { dbuToLatLng, dbuRectToBounds, latLngToDbu } from '../../src/coordinates.js';

describe('dbuToLatLng', () => {
    const scale = 1e-6;
    const maxDXDY = 1000000;

    it('converts origin', () => {
        const [lat, lng] = dbuToLatLng(0, 0, scale, maxDXDY);
        assert.equal(lng, 0);
        assert.equal(lat, -maxDXDY * scale);
    });

    it('converts max corner', () => {
        const [lat, lng] = dbuToLatLng(maxDXDY, maxDXDY, scale, maxDXDY);
        assert.equal(lat, 0);
        assert.equal(lng, maxDXDY * scale);
    });

    it('converts mid-point', () => {
        const mid = maxDXDY / 2;
        const [lat, lng] = dbuToLatLng(mid, mid, scale, maxDXDY);
        assert.equal(lat, -mid * scale);
        assert.equal(lng, mid * scale);
    });
});

describe('dbuRectToBounds', () => {
    it('converts a rectangle', () => {
        const scale = 1e-6;
        const maxDXDY = 1000000;
        const [[lat1, lng1], [lat2, lng2]] = dbuRectToBounds(
            100, 200, 300, 400, scale, maxDXDY);
        assert.equal(lng1, 100 * scale);
        assert.equal(lng2, 300 * scale);
        assert.equal(lat1, (200 - maxDXDY) * scale);
        assert.equal(lat2, (400 - maxDXDY) * scale);
    });
});

describe('latLngToDbu', () => {
    it('converts back to dbu', () => {
        const scale = 1e-6;
        const maxDXDY = 1000000;
        const { dbuX, dbuY } = latLngToDbu(-0.5, 0.25, scale, maxDXDY);
        assert.equal(dbuX, Math.round(0.25 / scale));
        assert.equal(dbuY, Math.round(maxDXDY + (-0.5) / scale));
    });
});

describe('round-trip dbu ↔ latLng', () => {
    it('preserves coordinates', () => {
        const scale = 1e-6, maxDXDY = 500000;
        const [lat, lng] = dbuToLatLng(12345, 67890, scale, maxDXDY);
        const { dbuX, dbuY } = latLngToDbu(lat, lng, scale, maxDXDY);
        assert.equal(dbuX, 12345);
        assert.equal(dbuY, 67890);
    });

    it('preserves zero', () => {
        const scale = 1e-6, maxDXDY = 1000000;
        const [lat, lng] = dbuToLatLng(0, 0, scale, maxDXDY);
        const { dbuX, dbuY } = latLngToDbu(lat, lng, scale, maxDXDY);
        assert.equal(dbuX, 0);
        assert.equal(dbuY, 0);
    });

    it('preserves large values', () => {
        const scale = 1e-6, maxDXDY = 10000000;
        const [lat, lng] = dbuToLatLng(9999999, 9999999, scale, maxDXDY);
        const { dbuX, dbuY } = latLngToDbu(lat, lng, scale, maxDXDY);
        assert.equal(dbuX, 9999999);
        assert.equal(dbuY, 9999999);
    });
});
