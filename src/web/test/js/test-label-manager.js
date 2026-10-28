// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

import './setup-dom.js';
import { makeFakeView } from './fake-view.js';

globalThis.requestAnimationFrame = (cb) => setTimeout(cb, 0);

const { LabelManager } = await import('../../src/label-manager.js');
const { beginSelection, onSelectionReset } = await import('../../src/ui-utils.js');

const flush = () => new Promise((r) => setTimeout(r, 0));

function makeManager(responses = {}, promptValue = 'note') {
    window.prompt = () => promptValue;
    const requests = [];
    const overlayRefreshes = { n: 0 };
    const app = {
        view: makeFakeView(),
        designScale: 1e-6,
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
        window.prompt = () => 'note';
        const app = {
            view: makeFakeView(),
            designScale: 1e-6,
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
        window.prompt = () => 'note';
        const requests = [];
        const app = {
            view: makeFakeView(),
            designScale: 1e-6,
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
        window.prompt = () => 'note';
        const app = {
            view: makeFakeView(),
            designScale: 1e-6,
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

// ─── Handles on the view ─────────────────────────────────────────────────────

function pointer(type, x, y, extra = {}) {
    const e = new window.MouseEvent(type, { bubbles: true, cancelable: true,
                                            clientX: x, clientY: y,
                                            button: 0, ...extra });
    e.pointerId = 1;
    return e;
}

async function makeHandleManager(labels) {
    const ctx = makeManager({ list_labels: { labels } });
    // 256 world units over 1e6 DBU: the fake view shows the whole square.
    ctx.app.designScale = 256 / 1e6;
    ctx.app.view.getContainer().getBoundingClientRect = () => ({
        left: 0, top: 0, width: 1000, height: 800 });
    await ctx.mgr.reload();
    return ctx;
}

describe('LabelManager handles', () => {
    it('pins a centred handle at each label position', async () => {
        const { app } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a' }]);
        const [h] = app.view.markersIn('label-handles');
        assert.deepEqual(h.world, [128, 64]);
        assert.equal(h.center, true);
        assert.ok(h.el.classList.contains('label-handle'));
        assert.equal(h.el.title, 'a');
    });

    it('a press and release without moving selects the label', async () => {
        const { mgr, app } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a' }]);
        const { el } = app.view.markersIn('label-handles')[0];
        el.dispatchEvent(pointer('pointerdown', 500, 464));
        el.dispatchEvent(pointer('pointerup', 501, 465));
        assert.equal(mgr._selectedName, 'L0');
        const sel = app.view.markersIn('label-handles')[0].el;
        assert.ok(sel.classList.contains('label-handle-selected'));
    });

    it('a drag moves the handle and commits one update_label', async () => {
        const { app, requests } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a', size: 0 }]);
        const { el } = app.view.markersIn('label-handles')[0];
        // The handle sits at screen (500, 464); drag it 64 px right.
        el.dispatchEvent(pointer('pointerdown', 500, 464));
        el.dispatchEvent(pointer('pointermove', 564, 464));
        assert.deepEqual(app.view.markersIn('label-handles')[0].world,
                         [192, 64]);
        el.dispatchEvent(pointer('pointerup', 564, 464));
        await flush();
        const upd = requests.filter((r) => r.type === 'update_label');
        assert.equal(upd.length, 1);
        assert.equal(upd[0].x, 750000);
        assert.equal(upd[0].y, 250000);
    });

    it('a press on a handle does not reach the view as a click', async () => {
        const { app } = await makeHandleManager([
            { name: 'L0', x: 0, y: 0, text: 'a' }]);
        const { el } = app.view.markersIn('label-handles')[0];
        let reached = false;
        app.view.getContainer().addEventListener('pointerdown',
                                                 () => { reached = true; });
        el.dispatchEvent(pointer('pointerdown', 0, 0));
        assert.equal(reached, false);
    });

    // resyncBounds can change the design's scale and origin, then redraws.
    it('re-places the handles when the visibility is updated', async () => {
        const { mgr, app } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a' }]);
        app.designScale = 512 / 1e6;
        mgr.updateVisibility();
        assert.deepEqual(app.view.markersIn('label-handles')[0].world, [256, 128]);
    });

    it('unmarks the label when another object takes the selection', async () => {
        const { mgr, app } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a' }]);
        mgr._select('L0');
        assert.ok(app.view.markersIn('label-handles')[0].el
            .classList.contains('label-handle-selected'));
        beginSelection(app);
        assert.equal(mgr._selectedName, null);
        assert.ok(!app.view.markersIn('label-handles')[0].el
            .classList.contains('label-handle-selected'));
    });

    it('takes the selection from other panels when selected', async () => {
        const { mgr, app } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a' }]);
        let resets = 0;
        onSelectionReset(app, () => resets++);
        mgr._select('L0');
        assert.equal(resets, 1);
        assert.equal(mgr._selectedName, 'L0');
    });

    it('draws the handles once when selected again', async () => {
        const { mgr } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a' }]);
        mgr._select('L0');
        let renders = 0;
        const render = mgr._renderHandles.bind(mgr);
        mgr._renderHandles = () => { renders++; render(); };
        mgr._select('L0');
        assert.equal(renders, 1);
    });

    it('shows the label in the Inspector again without the selection',
       async () => {
        const { mgr, app } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a' }]);
        mgr._select('L0');
        const shown = [];
        let focused = 0;
        mgr._updateInspector = (d) => shown.push(d.name);
        mgr._focusComponent = () => focused++;
        const token = app.selectionToken;
        mgr.refreshInspector('L0');
        assert.deepEqual(shown, ['L0']);
        assert.equal(focused, 0);
        assert.equal(app.selectionToken, token);
    });

    it('leaves the handles alone when nothing moved', async () => {
        const { mgr, app } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a' }]);
        const { el } = app.view.markersIn('label-handles')[0];
        mgr.updateVisibility();
        assert.equal(app.view.markersIn('label-handles')[0].el, el);
    });

    it('keeps the dragged handle through a resync', async () => {
        const { mgr, app } = await makeHandleManager([
            { name: 'L0', x: 500000, y: 250000, text: 'a', size: 0 }]);
        const { el } = app.view.markersIn('label-handles')[0];
        el.dispatchEvent(pointer('pointerdown', 500, 464));
        el.dispatchEvent(pointer('pointermove', 564, 464));
        app.designScale = 512 / 1e6;
        mgr.updateVisibility();
        assert.equal(app.view.markersIn('label-handles')[0].el, el);
    });

    it('follows the Labels visibility', async () => {
        const { mgr, app } = await makeHandleManager([
            { name: 'L0', x: 0, y: 0, text: 'a' }]);
        mgr._visibility.labels = false;
        mgr.updateVisibility();
        assert.equal(app.view.markers.get('label-handles').visible, false);
    });

    it('placement mode holds the crosshair cursor', () => {
        const { mgr, app } = makeManager();
        mgr.toggleLabelMode();
        assert.ok(app.view.cursorModes.has('label'));
        mgr.cancelLabelMode();
        assert.ok(!app.view.cursorModes.has('label'));
    });
});
