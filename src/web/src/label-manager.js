// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// User text labels (2.12).  Unlike rulers, labels are stored SERVER-SIDE (in
// the shared TileGenerator) so they render into the overlay tiles and into
// save_image, matching the Qt GUI.  This client module provides the
// interactive layer the Qt GUI lacks: create-by-click, drag-to-reposition,
// and edit/delete via the Inspector.  Each label gets a small draggable DOM
// handle pinned over the view; the label text itself is drawn by the server
// on the overlay.

import { dbuToWorld, worldToDbu } from './coordinates.js';
import { beginSelection, onSelectionReset } from './ui-utils.js';

const HANDLES = 'label-handles';
// A press that moves further than this drags the handle instead of clicking.
const kDragTolerancePx = 3;

export class LabelManager {
    constructor(app, visibility, updateInspector, focusComponent) {
        this._app = app;
        this._visibility = visibility;
        this._updateInspector = updateInspector;
        this._focusComponent = focusComponent;

        this._labels = [];          // server snapshot (name/x/y/text/size/anchor/color)
        this._anchors = [];         // anchor names the server accepts
        this._mode = false;         // placement mode (next click creates a label)
        this._selectedName = null;
        this._placedFor = null;     // [scale, originX, originY] of the handles
        this._dragging = false;
        // Another object taking the selection unmarks the label.
        onSelectionReset(app, () => {
            if (this._selectedName !== null) {
                this._selectedName = null;
                this._renderHandles();
            }
        });
    }

    isActive() {
        return this._mode;
    }

    // Also re-places the handles if a resync moved the design's origin or
    // scale, but not under a drag, which would lose the dragged element.
    updateVisibility() {
        const app = this._app;
        const placed = this._placedFor;
        if (!this._dragging && (!placed || placed[0] !== app.designScale
                                || placed[1] !== app.designOriginX
                                || placed[2] !== app.designOriginY)) {
            this._renderHandles();
        }
        this._app.view?.setMarkerGroupVisible(HANDLES,
                                              !!this._visibility.labels);
    }

    toggleLabelMode() {
        this._mode = !this._mode;
        this._setCursor(this._mode);
    }

    cancelLabelMode() {
        this._mode = false;
        this._setCursor(false);
    }

    _setCursor(on) {
        this._app.view?.setCursorMode('label', on);
    }

    _wm() {
        return this._app.websocketManager;
    }

    // Adopt a server label snapshot and rebuild the handles.
    _applyLabels(labels) {
        this._labels = labels || [];
        this._renderHandles();
    }

    // Every label mutation response carries the full label list, so refresh
    // from it directly (no second list_labels round-trip) and repaint.
    _refresh(res) {
        this._applyLabels(res && res.labels);
        this._app.refreshOverlay();
    }

    // A labels_changed push: someone else (another browser, or Tcl) edited the
    // shared label set.  Same repaint as a local mutation, but drop the
    // selection if the label it pointed at is gone, or the Inspector would go
    // on showing a label that no longer exists.
    applyRemoteLabels(labels) {
        this._applyLabels(labels);
        if (this._selectedName
            && !this._labels.some(l => l.name === this._selectedName)) {
            this._selectedName = null;
            this._updateInspector(null);
            this._renderHandles();
        }
        this._app.refreshOverlay();
    }

    _updateLabel(name, fields) {
        return this._wm().request({ type: 'update_label', name, ...fields });
    }

    // Fetch the current server-side labels and (re)build the handles.  Used for
    // the initial load; mutations refresh from their own response.
    async reload() {
        const wm = this._wm();
        if (!wm) return;
        try {
            const res = await wm.request({ type: 'list_labels' });
            // Taken from the server so the Anchor picker offers exactly the
            // names add_label/update_label accept.
            this._anchors = res.anchors || [];
            this._applyLabels(res.labels);
        } catch (_) {
            this._applyLabels([]);
        }
    }

    // A click in placement mode: prompt for text and add a server-side label.
    async handleMapClick(dbuX, dbuY) {
        const text = window.prompt('Label text:');
        this._mode = false;
        this._setCursor(false);
        if (!text) return;
        try {
            const res = await this._wm().request({
                type: 'add_label', x: Math.round(dbuX), y: Math.round(dbuY),
                text,
            });
            this._refresh(res);
        } catch (err) {
            console.error('add_label failed:', err);
        }
    }

    _renderHandles() {
        const view = this._app.view;
        if (!view) return;
        view.clearMarkers(HANDLES);
        const app = this._app;
        if (!app.designScale) return;
        this._placedFor = [app.designScale, app.designOriginX,
                           app.designOriginY];

        for (const label of this._labels) {
            const el = document.createElement('div');
            el.className = 'label-handle'
                + (label.name === this._selectedName
                    ? ' label-handle-selected' : '');
            el.title = label.text;
            this._installDrag(el, label);
            view.addMarker(HANDLES, label.name,
                           dbuToWorld(label.x, label.y, app.designScale,
                                      app.designOriginX, app.designOriginY),
                           el, { center: true });
        }
    }

    // Press and release selects the label; a drag moves it.
    _installDrag(el, label) {
        const view = this._app.view;
        const toWorld = (e) => {
            const r = view.getContainer().getBoundingClientRect();
            return view.unproject([e.clientX - r.left, e.clientY - r.top]);
        };
        let press = null;
        el.addEventListener('pointerdown', (e) => {
            if (e.button !== 0) return;
            e.stopPropagation();
            e.preventDefault();
            el.setPointerCapture?.(e.pointerId);
            press = { x: e.clientX, y: e.clientY, moved: false };
            this._dragging = true;
        });
        el.addEventListener('pointermove', (e) => {
            if (!press) return;
            if (Math.abs(e.clientX - press.x) > kDragTolerancePx
                || Math.abs(e.clientY - press.y) > kDragTolerancePx) {
                press.moved = true;
            }
            if (press.moved) {
                view.moveMarker(HANDLES, label.name, toWorld(e));
            }
        });
        const finish = (e, commit) => {
            if (!press) return;
            const p = press;
            press = null;
            this._dragging = false;
            el.releasePointerCapture?.(e.pointerId);
            if (!commit) {
                this._renderHandles();
            } else if (!p.moved) {
                this._select(label.name);
            } else {
                const app = this._app;
                const [wx, wy] = toWorld(e);
                const { dbuX, dbuY } = worldToDbu(
                    wx, wy, app.designScale,
                    app.designOriginX, app.designOriginY);
                this._commitMove(label, Math.round(dbuX), Math.round(dbuY));
            }
        };
        el.addEventListener('pointerup', (e) => finish(e, true));
        el.addEventListener('pointercancel', (e) => finish(e, false));
        // The view's own click and double-click handling skips markers.
        el.addEventListener('click', (e) => e.stopPropagation());
    }

    // Move the label to a new position with a single atomic update (no
    // delete+add, so a failure mid-way can never drop the label).
    async _commitMove(label, x, y) {
        try {
            const res = await this._updateLabel(label.name, {
                x, y, text: label.text, size: label.size || 0,
                anchor: label.anchor || 'center', color: label.color,
            });
            this._refresh(res);
        } catch (err) {
            console.error('move label failed:', err);
        }
    }

    _select(name) {
        // Our own mark goes first, so the reset that unmarks every other panel
        // does not redraw the handles a second time.
        this._selectedName = null;
        beginSelection(this._app);
        this._selectedName = name;
        this._renderHandles();
        this._inspect(name, { focus: true });
    }

    // Shows the label in the Inspector again without taking the selection.
    refreshInspector(name) {
        this._inspect(name, { focus: false });
    }

    _inspect(name, { focus }) {
        const label = this._labels.find(l => l.name === name);
        if (!label) return;
        const fmt = (dbu) => this._app.formatDbu(dbu, true);

        const data = {
            type: 'Label',
            name: label.name,
            bbox: [label.x, label.y, label.x, label.y],
            properties: [
                { name: 'Name', value: label.name },
                { name: 'Text', value: label.text, editable: true },
                { name: 'x', value: fmt(label.x) },
                { name: 'y', value: fmt(label.y) },
                // "Font size", not "Size": it is a font height in pixels, not
                // a distance in the design, so it does not follow the µm/DBU
                // display-unit switch the way x and y do.
                { name: 'Font size', value: String(label.size || 0),
                  editable: true },
                { name: 'Anchor', value: label.anchor || 'center',
                  editable: true,
                  editor: { type: 'list', options: this._anchors } },
            ],
            onPropertyChange: (propName, newValue) => {
                const patch = { text: label.text, size: label.size || 0,
                                anchor: label.anchor || 'center' };
                if (propName === 'Text') patch.text = newValue;
                else if (propName === 'Font size') patch.size = parseInt(newValue, 10) || 0;
                else if (propName === 'Anchor') patch.anchor = newValue.trim();
                this._edit(label, patch);
            },
        };
        this._updateInspector(data);
        if (focus) {
            this._focusComponent('Inspector');
        }

        requestAnimationFrame(() => {
            const toolbar
                = this._app.inspectorEl?.querySelector('.inspector-toolbar');
            if (!toolbar) return;
            // Avoid stacking duplicate delete buttons across re-selections.
            if (toolbar.querySelector('.delete-label-btn')) return;
            const btn = document.createElement('button');
            btn.className = 'or-btn or-btn-icon delete-label-btn';
            btn.title = 'Delete label';
            btn.innerHTML =
                '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">'
                + '<path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/>'
                + '</svg>';
            btn.addEventListener('click', () => this.deleteLabel(name));
            toolbar.appendChild(btn);
        });
    }

    // Apply an edit atomically, keeping the same name/position.
    async _edit(label, patch) {
        try {
            const res = await this._updateLabel(label.name, {
                x: label.x, y: label.y, text: patch.text, size: patch.size,
                anchor: patch.anchor, color: label.color,
            });
            this._refresh(res);
            this._select(label.name);
        } catch (err) {
            console.error('edit label failed:', err);
        }
    }

    async deleteLabel(name) {
        try {
            const res = await this._wm().request({ type: 'delete_label', name });
            if (this._selectedName === name) {
                this._selectedName = null;
                this._updateInspector(null);
            }
            this._refresh(res);
        } catch (err) {
            console.error('delete_label failed:', err);
        }
    }

    async clearAllLabels() {
        try {
            const res = await this._wm().request({ type: 'clear_labels' });
            this._selectedName = null;
            this._refresh(res);
        } catch (err) {
            console.error('clear_labels failed:', err);
        }
    }
}
