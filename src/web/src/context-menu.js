// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { latLngToDbu } from './coordinates.js';

// Mirrors kHighlightColors / kHighlightOverlayColors in the backend.
const HIGHLIGHT_COLORS = [
    { name: 'green',        hex: '#00ff00' },
    { name: 'yellow',       hex: '#ffff00' },
    { name: 'cyan',         hex: '#00ffff' },
    { name: 'magenta',      hex: '#ff00ff' },
    { name: 'red',          hex: '#ff0000' },
    { name: 'dark_green',   hex: '#008000' },
    { name: 'dark_magenta', hex: '#800080' },
    { name: 'blue',         hex: '#0000ff' },
    { name: 'orange',       hex: '#ffa500' },
    { name: 'purple',       hex: '#800080' },
    { name: 'lime',         hex: '#bfff00' },
    { name: 'teal',         hex: '#008080' },
    { name: 'pink',         hex: '#ffc0cb' },
    { name: 'brown',        hex: '#8b4513' },
    { name: 'indigo',       hex: '#4b0082' },
    { name: 'turquoise',    hex: '#40e0d0' },
];

export class ContextMenu {
    constructor(app) {
        this._app = app;
        this._el = document.createElement('div');
        this._el.className = 'context-menu';
        this._el.style.display = 'none';
        this._el.style.zIndex = '10000';
        document.body.appendChild(this._el);

        this._bindEvents();
    }

    _bindEvents() {
        // Hide menu when clicking (left or right) outside.
        document.addEventListener('mousedown', (e) => {
            if (!this._el.contains(e.target)) {
                this.hide();
            }
        });

        // Hide menu when map moves.
        if (this._app.map) {
            this._app.map.on('movestart', () => this.hide());
        }
    }

    show(e, latlng) {
        this._el.innerHTML = '';

        const menuItems = [
            { label: 'Add Label', action: () => this._showAddLabelDialog(latlng) },
            { type: 'separator' },
            { label: 'Select', subMenu: [
                { label: 'Connected Insts', action: () => this._sendContextAction('select_connected_insts') },
                { label: 'Output Nets', action: () => this._sendContextAction('select_connected_nets', { output: true, input: false }) },
                { label: 'Input Nets', action: () => this._sendContextAction('select_connected_nets', { output: false, input: true }) },
                { label: 'All Nets', action: () => this._sendContextAction('select_connected_nets', { output: true, input: true }) },
                { label: 'All buffer trees', action: () => this._sendContextAction('select_connected_buffer_trees') },
            ]},
            { label: 'Highlight', subMenu: [
                { label: 'Connected Insts', subMenu:
                    this._buildColorSubmenu('highlight_connected_insts') },
                { label: 'Output Nets', subMenu:
                    this._buildColorSubmenu('highlight_connected_nets', { output: true, input: false }) },
                { label: 'Input Nets', subMenu:
                    this._buildColorSubmenu('highlight_connected_nets', { output: false, input: true }) },
                { label: 'All Nets', subMenu:
                    this._buildColorSubmenu('highlight_connected_nets', { output: true, input: true }) },
                { label: 'All buffer trees', subMenu:
                    this._buildColorSubmenu('highlight_connected_buffer_trees') },
            ]},
            { label: 'Clear', subMenu: [
                { label: 'Selections', action: () => this._sendContextAction('clear_selections') },
                { label: 'Highlights', action: () => this._sendContextAction('clear_highlights') },
                { label: 'Rulers', action: () => { if (this._app.rulerManager) this._app.rulerManager.clearAllRulers(); } },
                { label: 'Labels', action: () => { if (this._app.labelManager) this._app.labelManager.clearAll(); } },
                { label: 'Focus nets', action: () => this._sendContextAction('clear_focus_nets') },
                { label: 'Route Guides', action: () => this._sendContextAction('clear_route_guides') },
                { label: 'Net Tracks', action: () => this._sendContextAction('clear_net_tracks') },
                { label: 'All', action: () => {
                    this._sendContextAction('clear_all');
                    if (this._app.rulerManager) this._app.rulerManager.clearAllRulers();
                    if (this._app.labelManager) this._app.labelManager.clearAll();
                }},
            ]},
            { label: 'Save', subMenu: [
                { label: 'Visible layout', action: () => {
                    const bounds = this._app.map.getBounds();
                    const sw = latLngToDbu(bounds.getSouthWest().lat, bounds.getSouthWest().lng, this._app.designScale, this._app.designMaxDXDY, this._app.designOriginX, this._app.designOriginY);
                    const ne = latLngToDbu(bounds.getNorthEast().lat, bounds.getNorthEast().lng, this._app.designScale, this._app.designMaxDXDY, this._app.designOriginX, this._app.designOriginY);
                    const bbox = [
                        Math.min(sw.dbuX, ne.dbuX),
                        Math.min(sw.dbuY, ne.dbuY),
                        Math.max(sw.dbuX, ne.dbuX),
                        Math.max(sw.dbuY, ne.dbuY),
                    ];
                    this._triggerImageDownload(true, bbox);
                }},
                { label: 'Entire layout', action: () => this._triggerImageDownload(false, null) },
            ]},
        ];

        this._buildMenu(menuItems, this._el);

        // Show first so we can measure, then clamp into the viewport so the
        // menu (and its tall submenus) never spills off the right/bottom edge.
        this._el.style.display = 'block';
        const margin = 4;
        const rect = this._el.getBoundingClientRect();
        const maxLeft = window.innerWidth - rect.width - margin;
        const maxTop = window.innerHeight - rect.height - margin;
        const left = Math.max(margin,
            Math.min(e.originalEvent.clientX, maxLeft));
        const top = Math.max(margin,
            Math.min(e.originalEvent.clientY, maxTop));
        this._el.style.left = left + 'px';
        this._el.style.top = top + 'px';
    }

    hide() {
        this._el.style.display = 'none';
    }

    _buildMenu(items, container) {
        items.forEach(item => {
            if (item.type === 'separator') {
                const sep = document.createElement('div');
                sep.className = 'cm-separator';
                container.appendChild(sep);
                return;
            }

            const row = document.createElement('div');
            row.className = 'cm-item';
            if (item.swatch) {
                const sw = document.createElement('span');
                sw.className = 'cm-swatch';
                sw.style.background = item.swatch;
                row.appendChild(sw);
            }
            row.appendChild(document.createTextNode(item.label));

            if (item.subMenu) {
                row.classList.add('has-submenu');
                const subContainer = document.createElement('div');
                subContainer.className = 'cm-submenu';
                this._buildMenu(item.subMenu, subContainer);
                row.appendChild(subContainer);
                // Flip the submenu to the left edge when it would overflow the
                // viewport on the right (deep color submenus are 3 levels in).
                row.addEventListener('mouseenter', () => {
                    subContainer.classList.remove('flip-left');
                    const sub = subContainer.getBoundingClientRect();
                    if (sub.right > window.innerWidth) {
                        subContainer.classList.add('flip-left');
                    }
                });
            } else if (item.action) {
                row.addEventListener('click', (ev) => {
                    ev.stopPropagation();
                    this.hide();
                    try {
                        item.action();
                    } catch (err) {
                        console.error('Context menu item action threw:',
                                      item.label, err);
                    }
                });
            }

            container.appendChild(row);
        });
    }

    _triggerImageDownload(visible, bbox) {
        const filename = visible ? 'layout_visible.png' : 'layout_entire.png';
        const params = new URLSearchParams();
        params.set('type', visible ? 'visible' : 'entire');
        params.set('filename', filename);
        if (visible && bbox) {
            params.set('bbox', bbox.join(','));
        }
        const url = `/download/image?${params.toString()}`;
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.style.display = 'none';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
    }

    _buildColorSubmenu(action, baseArgs = {}) {
        return HIGHLIGHT_COLORS.map(({ name, hex }, idx) => ({
            label: name,
            swatch: hex,
            action: () => this._sendContextAction(action, { ...baseArgs, group: idx }),
        }));
    }

    _showAddLabelDialog(latlng) {
        const text = prompt('Enter label text:');
        if (text && this._app.labelManager) {
            this._app.labelManager.addLabelAt(text, 'yellow', 14, latlng);
        }
    }

    _sendContextAction(action, args = {}) {
        if (!this._app.websocketManager) {
            return;
        }
        this._app.websocketManager
            .request({ type: 'context_action', action, ...args })
            .then(() => {
                if (typeof this._app.redrawAllLayers === 'function') {
                    this._app.redrawAllLayers();
                }
            })
            .catch((err) => {
                console.error('Context action failed:', action, err);
            });
    }
}
