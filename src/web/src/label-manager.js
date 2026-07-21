// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// User text labels (2.12).  Unlike rulers, labels are stored SERVER-SIDE (in
// the shared TileGenerator) so they render into the overlay tiles and into
// save_image, matching the Qt GUI.  This client module provides the
// interactive layer the Qt GUI lacks: create-by-click, drag-to-reposition,
// and edit/delete via the Inspector.  Each label gets a small draggable handle
// marker in a dedicated Leaflet pane; the label text itself is drawn by the
// server on the overlay.

import { dbuToLatLng, latLngToDbu } from './coordinates.js';

export class LabelManager {
    constructor(app, visibility, updateInspector, focusComponent) {
        this._app = app;
        this._visibility = visibility;
        this._updateInspector = updateInspector;
        this._focusComponent = focusComponent;

        this._labels = [];          // server snapshot (name/x/y/text/size/anchor/color)
        this._mode = false;         // placement mode (next click creates a label)
        this._selectedName = null;

        this._pane = null;
        this._handleGroup = null;
        this._initLayers();
    }

    _initLayers() {
        const map = this._app.map;
        if (!map) return;
        this._pane = map.createPane('label-pane');
        this._pane.style.zIndex = '690';
        this._pane.style.pointerEvents = 'auto';
        this._handleGroup = L.layerGroup([], { pane: 'label-pane' }).addTo(map);
    }

    isActive() {
        return this._mode;
    }

    updateVisibility() {
        if (this._pane) {
            this._pane.style.display = this._visibility.labels ? '' : 'none';
        }
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
        const c = this._app.map?.getContainer();
        if (c) c.style.cursor = on ? 'crosshair' : '';
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
        if (!this._handleGroup) return;
        this._handleGroup.clearLayers();
        const scale = this._app.designScale;
        if (!scale) return;
        const maxDXDY = this._app.designMaxDXDY;
        const ox = this._app.designOriginX;
        const oy = this._app.designOriginY;

        for (const label of this._labels) {
            const ll = dbuToLatLng(label.x, label.y, scale, maxDXDY, ox, oy);
            const selected = label.name === this._selectedName;
            const marker = L.marker(ll, {
                pane: 'label-pane',
                draggable: true,
                keyboard: false,
                icon: L.divIcon({
                    className: 'label-handle'
                        + (selected ? ' label-handle-selected' : ''),
                    html: '',
                    iconSize: [10, 10],
                }),
                title: label.text,
            });
            marker.on('click', (e) => {
                L.DomEvent.stopPropagation(e);
                this._select(label.name);
            });
            marker.on('dragend', (e) => {
                const p = e.target.getLatLng();
                const { dbuX, dbuY } = latLngToDbu(
                    p.lat, p.lng, scale, maxDXDY, ox, oy);
                this._commitMove(label, Math.round(dbuX), Math.round(dbuY));
            });
            this._handleGroup.addLayer(marker);
        }
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
        this._selectedName = name;
        this._renderHandles();
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
                { name: 'Size', value: String(label.size || 0), editable: true },
                { name: 'Anchor', value: label.anchor || 'center',
                  editable: true },
            ],
            onPropertyChange: (propName, newValue) => {
                const patch = { text: label.text, size: label.size || 0,
                                anchor: label.anchor || 'center' };
                if (propName === 'Text') patch.text = newValue;
                else if (propName === 'Size') patch.size = parseInt(newValue, 10) || 0;
                else if (propName === 'Anchor') patch.anchor = newValue.trim();
                this._edit(label, patch);
            },
        };
        this._updateInspector(data);
        this._focusComponent('Inspector');

        requestAnimationFrame(() => {
            const toolbar
                = this._app.inspectorEl?.querySelector('.inspector-toolbar');
            if (!toolbar) return;
            const btn = document.createElement('button');
            btn.className = 'inspector-btn';
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
