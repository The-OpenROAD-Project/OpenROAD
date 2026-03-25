// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Drop-in replacement for WebSocketManager that serves pre-embedded data.
// Used in static HTML mode where all data is inlined at build time.
//
// Implements the same request(msg) → Promise interface that all widgets use,
// so zero widget code changes are needed.

export class StaticDataManager {
    constructor(data) {
        this._data = data;
        this.readyPromise = Promise.resolve();
        this.pending = new Map(); // match WebSocketManager interface
    }

    // Stub responses for request types not included in the payload
    static _stubs = {
        heatmaps: { active: '', heatmaps: [] },
        module_hierarchy: { modules: [] },
        clock_tree: { clocks: [] },
        select: { selected: [] },
        timing_highlight: { ok: true },
    };

    // Same interface as WebSocketManager.request(msg) → Promise<data>
    request(msg) {
        const key = this._resolveKey(msg);
        const result = this._data[key];
        if (result !== undefined) {
            return Promise.resolve(structuredClone(result));
        }
        const stub = StaticDataManager._stubs[msg.type];
        if (stub !== undefined) {
            return Promise.resolve(structuredClone(stub));
        }
        // Unsupported in static mode (tiles, tcl_eval, highlights, etc.)
        return Promise.reject(
            new Error(`Not available in static mode: ${msg.type}`));
    }

    _resolveKey(msg) {
        if (msg.type === 'timing_report') {
            return msg.is_setup ? 'timing_report_setup' : 'timing_report_hold';
        }
        return msg.type;
    }
}
