// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { TimingWidget } from '../../src/timing-widget.js';

// Build an app stub that records every websocketManager.request call and lets
// each test control the response per message type, plus a mutable selection.
function createMockApp(responses = {}) {
    const requests = [];
    return {
        requests,
        selectedInstanceName: '',
        websocketManager: {
            request(msg) {
                requests.push(msg);
                const handler = responses[msg.type];
                return Promise.resolve(handler ? handler(msg) : {});
            },
        },
    };
}

// Drain the microtask queue so the async _applyCone body settles.
const flush = () => new Promise((r) => setTimeout(r, 0));

describe('TimingWidget timing cone', () => {
    it('builds a timing_cone request from the selected instance', async () => {
        const app = createMockApp({
            timing_cone: () => ({ node_count: 3, constrained: true,
                                  min_slack: -0.1, max_slack: 0.2,
                                  time_unit: 'ns', color_mode: 'slack' }),
        });
        app.selectedInstanceName = 'inst42';
        const widget = new TimingWidget(app, () => {}, () => {});

        widget._coneFanin.checked = true;
        widget._coneFanout.checked = false;
        widget._coneFaninDepth.value = '3';
        widget._coneFanoutDepth.value = '0';
        widget._coneColorMode.value = 'slack';

        await widget._applyCone();
        await flush();

        const req = app.requests.find(r => r.type === 'timing_cone');
        assert.ok(req, 'timing_cone was requested');
        assert.equal(req.fanin, true);
        assert.equal(req.fanout, false);
        assert.equal(req.fanin_depth, 3);
        assert.equal(req.fanout_depth, 0);
        assert.equal(req.color_mode, 'slack');
        assert.equal(req.inst_name, 'inst42');
        assert.equal(req.pin_name, undefined);
    });

    it('passes the depth color mode through', async () => {
        const app = createMockApp();
        app.selectedInstanceName = 'inst42';
        const widget = new TimingWidget(app, () => {}, () => {});
        widget._coneColorMode.value = 'depth';

        await widget._applyCone();
        await flush();

        const req = app.requests.find(r => r.type === 'timing_cone');
        assert.equal(req.color_mode, 'depth');
    });

    it('does nothing when neither fanin nor fanout is selected', async () => {
        const app = createMockApp();
        app.selectedInstanceName = 'inst42';
        const widget = new TimingWidget(app, () => {}, () => {});
        widget._coneFanin.checked = false;
        widget._coneFanout.checked = false;

        await widget._applyCone();
        await flush();

        assert.equal(app.requests.filter(r => r.type === 'timing_cone').length, 0);
        assert.match(widget._coneStatus.textContent, /fanin/);
    });

    it('reports when no instance is selected', async () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {}, () => {});

        await widget._applyCone();
        await flush();

        assert.equal(app.requests.filter(r => r.type === 'timing_cone').length, 0);
        assert.match(widget._coneStatus.textContent, /select an instance/i);
    });

    it('clear sends a clear request', () => {
        const app = createMockApp();
        const widget = new TimingWidget(app, () => {}, () => {});

        widget._clearCone();

        const req = app.requests.find(r => r.type === 'timing_cone');
        assert.ok(req);
        assert.equal(req.clear, true);
    });

    it('dispatches a schematic-sync event when enabled', async () => {
        const app = createMockApp();
        app.selectedInstanceName = 'inst42';
        const widget = new TimingWidget(app, () => {}, () => {});
        widget._coneSyncSchematic.checked = true;
        widget._coneFaninDepth.value = '2';

        let synced = null;
        document.addEventListener('openroad-cone-sync',
            (e) => { synced = e.detail; }, { once: true });

        await widget._applyCone();
        await flush();

        assert.ok(synced, 'sync event fired');
        assert.equal(synced.inst_name, 'inst42');
        assert.equal(synced.fanin_depth, 2);
    });
});
