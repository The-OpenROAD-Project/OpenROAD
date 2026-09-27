// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

// ─── Minimal Leaflet mock ────────────────────────────────────────────────────
function makeFakeLayer() {
    return {
        addTo() { return this; },
        addLayer() {},
        removeLayer() {},
        clearLayers() {},
        on() { return this; },
        getLatLng() { return { lat: 0, lng: 0 }; },
    };
}

globalThis.L = {
    layerGroup() { return makeFakeLayer(); },
    marker() { return makeFakeLayer(); },
    divIcon() { return {}; },
    DomEvent: { stopPropagation() {} },
};
globalThis.requestAnimationFrame = (cb) => setTimeout(cb, 0);

const { LabelManager } = await import('../../src/label-manager.js');

const flush = () => new Promise((r) => setTimeout(r, 0));

function makeManager(responses = {}, promptValue = 'note') {
    globalThis.window = { prompt: () => promptValue };
    const requests = [];
    const overlayRefreshes = { n: 0 };
    const app = {
        map: {
            createPane() { return { style: {} }; },
            getContainer() {
                return { style: {}, classList: { toggle() {} } };
            },
        },
        designScale: 1e-6,
        designMaxDXDY: 1000000,
        designOriginX: 0,
        designOriginY: 0,
        refreshOverlay() { overlayRefreshes.n++; },
        formatDbu(v) { return String(v); },
        inspectorEl: null,
        websocketManager: {
            request(msg) {
                requests.push(msg);
                return Promise.resolve(responses[msg.type] || {});
            },
        },
    };
    const visibility = { labels: true };
    const mgr = new LabelManager(app, visibility, () => {}, () => {});
    return { mgr, app, requests, overlayRefreshes };
}

describe('LabelManager', () => {
    it('toggleLabelMode flips active state', () => {
        const { mgr } = makeManager();
        assert.equal(mgr.isActive(), false);
        mgr.toggleLabelMode();
        assert.equal(mgr.isActive(), true);
        mgr.cancelLabelMode();
        assert.equal(mgr.isActive(), false);
    });

    it('handleMapClick sends add_label with the clicked DBU point', async () => {
        const { mgr, requests, overlayRefreshes } = makeManager({
            add_label: { ok: true, name: 'L0',
                         labels: [{ name: 'L0', x: 1234, y: 5678, text: 'note' }] },
        });
        await mgr.handleMapClick(1234, 5678);
        await flush();
        const add = requests.find(r => r.type === 'add_label');
        assert.ok(add, 'add_label requested');
        assert.equal(add.x, 1234);
        assert.equal(add.y, 5678);
        assert.equal(add.text, 'note');
        // Refreshes from the add_label response — no second list_labels fetch.
        assert.equal(requests.filter(r => r.type === 'list_labels').length, 0);
        assert.equal(mgr._labels.length, 1);
        assert.ok(overlayRefreshes.n >= 1);
        // Placement mode is cleared after creating.
        assert.equal(mgr.isActive(), false);
    });

    it('handleMapClick does nothing when the prompt is cancelled', async () => {
        const { mgr, requests } = makeManager({}, null);
        await mgr.handleMapClick(10, 20);
        await flush();
        assert.equal(requests.filter(r => r.type === 'add_label').length, 0);
    });

    it('reload populates labels from list_labels', async () => {
        const { mgr } = makeManager({
            list_labels: { labels: [{ name: 'L0', x: 1, y: 2, text: 'a' }] },
        });
        await mgr.reload();
        assert.equal(mgr._labels.length, 1);
        assert.equal(mgr._labels[0].name, 'L0');
    });

    it('reload adopts the anchor list the server accepts', async () => {
        const { mgr } = makeManager({
            list_labels: {
                labels: [],
                anchors: ['center', 'top left', 'bottom right'],
            },
        });
        await mgr.reload();
        assert.deepEqual(mgr._anchors,
                         ['center', 'top left', 'bottom right']);
    });

    it('Anchor is a choice property built from the server list', async () => {
        let captured = null;
        globalThis.window = { prompt: () => 'note' };
        const app = {
            map: {
                createPane() { return { style: {} }; },
                getContainer() {
                    return { style: {}, classList: { toggle() {} } };
                },
            },
            designScale: 1e-6, designMaxDXDY: 1000000,
            designOriginX: 0, designOriginY: 0,
            refreshOverlay() {}, formatDbu(v) { return String(v); },
            inspectorEl: null,
            websocketManager: {
                request(msg) {
                    if (msg.type === 'list_labels') {
                        return Promise.resolve({
                            labels: [{ name: 'L0', x: 1, y: 2, text: 'a',
                                       anchor: 'top left' }],
                            anchors: ['center', 'top left'],
                        });
                    }
                    return Promise.resolve({});
                },
            },
        };
        const mgr = new LabelManager(
            app, { labels: true }, (d) => { captured = d; }, () => {});
        await mgr.reload();
        mgr._select('L0');
        const anchor = captured.properties.find(p => p.name === 'Anchor');
        assert.ok(anchor, 'Anchor property present');
        assert.equal(anchor.editable, true);
        // A list editor, so the Inspector renders a picker instead of a raw
        // text field that would accept an unusable value.
        assert.equal(anchor.editor.type, 'list');
        assert.deepEqual(anchor.editor.options, ['center', 'top left']);
        assert.equal(anchor.value, 'top left');
    });

    it('Font size edits the label size and is not unit-converted',
       async () => {
        let captured = null;
        globalThis.window = { prompt: () => 'note' };
        const requests = [];
        const app = {
            map: {
                createPane() { return { style: {} }; },
                getContainer() {
                    return { style: {}, classList: { toggle() {} } };
                },
            },
            designScale: 1e-6, designMaxDXDY: 1000000,
            designOriginX: 0, designOriginY: 0,
            refreshOverlay() {},
            // Whatever the display-unit switch is doing to x/y, it must not
            // reach the font size.
            formatDbu(v) { return `${v / 1000} um`; },
            inspectorEl: null,
            websocketManager: {
                request(msg) {
                    requests.push(msg);
                    if (msg.type === 'list_labels') {
                        return Promise.resolve({
                            labels: [{ name: 'L0', x: 1000, y: 2000, text: 'a',
                                       size: 14, anchor: 'center' }],
                            anchors: ['center'],
                        });
                    }
                    return Promise.resolve({});
                },
            },
        };
        const mgr = new LabelManager(
            app, { labels: true }, (d) => { captured = d; }, () => {});
        await mgr.reload();
        mgr._select('L0');

        const names = captured.properties.map(p => p.name);
        assert.ok(names.includes('Font size'), 'labelled "Font size"');
        assert.ok(!names.includes('Size'), 'not the ambiguous "Size"');

        const size = captured.properties.find(p => p.name === 'Font size');
        // Raw pixels: no unit suffix, unlike x and y.
        assert.equal(size.value, '14');
        assert.equal(captured.properties.find(p => p.name === 'x').value,
                     '1 um');

        captured.onPropertyChange('Font size', '20');
        await flush();
        const upd = requests.find(r => r.type === 'update_label');
        assert.ok(upd, 'update_label requested');
        assert.equal(upd.size, 20);
    });

    it('applyRemoteLabels adopts a pushed set and repaints', async () => {
        const { mgr, app, requests, overlayRefreshes } = makeManager();
        mgr.applyRemoteLabels([{ name: 'L9', x: 1, y: 2, text: 'from tcl' }]);
        assert.equal(mgr._labels.length, 1);
        assert.equal(mgr._labels[0].name, 'L9');
        assert.ok(overlayRefreshes.n >= 1, 'overlay repainted');
        // The push already carries the set; no round-trip needed.
        assert.equal(requests.filter(r => r.type === 'list_labels').length, 0);
    });

    it('applyRemoteLabels clears a selection whose label was removed',
       async () => {
        let inspected = 'unset';
        globalThis.window = { prompt: () => 'note' };
        const app = {
            map: {
                createPane() { return { style: {} }; },
                getContainer() {
                    return { style: {}, classList: { toggle() {} } };
                },
            },
            designScale: 1e-6, designMaxDXDY: 1000000,
            designOriginX: 0, designOriginY: 0,
            refreshOverlay() {}, formatDbu(v) { return String(v); },
            inspectorEl: null,
            websocketManager: {
                request(msg) {
                    if (msg.type === 'list_labels') {
                        return Promise.resolve({
                            labels: [{ name: 'L0', x: 1, y: 2, text: 'a',
                                       anchor: 'center' }],
                            anchors: ['center'],
                        });
                    }
                    return Promise.resolve({});
                },
            },
        };
        const mgr = new LabelManager(
            app, { labels: true }, (d) => { inspected = d; }, () => {});
        await mgr.reload();
        mgr._select('L0');
        assert.ok(inspected && inspected.name === 'L0');

        // Another client deleted it.  Leaving it selected would keep the
        // Inspector showing a label that no longer exists.
        mgr.applyRemoteLabels([]);
        assert.equal(mgr._selectedName, null);
        assert.equal(inspected, null);
    });

    it('applyRemoteLabels keeps a selection that survived the push',
       async () => {
        const { mgr } = makeManager();
        mgr._applyLabels([{ name: 'L0', x: 1, y: 2, text: 'a' }]);
        mgr._selectedName = 'L0';
        mgr.applyRemoteLabels([{ name: 'L0', x: 9, y: 9, text: 'moved' }]);
        assert.equal(mgr._selectedName, 'L0');
    });

    it('deleteLabel sends a delete_label request and refreshes', async () => {
        const { mgr, requests, overlayRefreshes } = makeManager();
        await mgr.deleteLabel('L0');
        await flush();
        const del = requests.find(r => r.type === 'delete_label');
        assert.ok(del);
        assert.equal(del.name, 'L0');
        assert.ok(overlayRefreshes.n >= 1);
    });

    it('clearAllLabels sends clear_labels', async () => {
        const { mgr, requests } = makeManager();
        await mgr.clearAllLabels();
        await flush();
        assert.ok(requests.some(r => r.type === 'clear_labels'));
    });

    it('_commitMove sends a single atomic update_label (no delete+add)',
       async () => {
        const { mgr, requests } = makeManager();
        const label = { name: 'L0', x: 1, y: 2, text: 'a', size: 0,
                        anchor: 'center', color: { r: 1, g: 2, b: 3, a: 255 } };
        await mgr._commitMove(label, 99, 88);
        await flush();
        const upd = requests.find(r => r.type === 'update_label');
        assert.ok(upd, 'update_label requested');
        assert.equal(upd.name, 'L0');
        assert.equal(upd.x, 99);
        assert.equal(upd.y, 88);
        assert.equal(upd.text, 'a');
        assert.equal(requests.filter(r => r.type === 'delete_label').length, 0);
        assert.equal(requests.filter(r => r.type === 'add_label').length, 0);
    });

    it('_edit sends a single atomic update_label with the patched fields',
       async () => {
        const { mgr, requests } = makeManager();
        const label = { name: 'L0', x: 5, y: 6, text: 'old', size: 0,
                        anchor: 'center', color: { r: 0, g: 0, b: 0, a: 255 } };
        await mgr._edit(label, { text: 'new', size: 14, anchor: 'top left' });
        await flush();
        const upd = requests.find(r => r.type === 'update_label');
        assert.ok(upd, 'update_label requested');
        assert.equal(upd.name, 'L0');
        assert.equal(upd.text, 'new');
        assert.equal(upd.size, 14);
        assert.equal(upd.anchor, 'top left');
        assert.equal(requests.filter(r => r.type === 'delete_label').length, 0);
    });
});
