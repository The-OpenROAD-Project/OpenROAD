// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Canvas right-click context menu.  Mirrors the Qt GUI LayoutViewer
// Select / Highlight / View / Save / Clear submenus; the backend replicates
// MainWindow::selectHighlightConnected{Insts,Nets,BufferTrees} and the Save /
// Clear actions.
//
// Highlight offers all 16 groups (colors), inline, on every item — a superset
// of the Qt right-click menu, which only exposes colors under "All buffer
// trees" and wires just 4 of them.

import { applySelectionFlags } from './ui-utils.js';

// Mirrors gui::Painter::kHighlightColors in the backend (index == group).
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

// Shared targets for the "Select →" and "Highlight →" submenus.  `suffix` is
// appended to `select_` / `highlight_`; `needsNet` items require a selected
// net, the rest require a selected instance.
const CONNECTED_ITEMS = [
    { label: 'Connected Insts', suffix: 'connected_insts', needsNet: true },
    { label: 'Output Nets', suffix: 'connected_nets', args: { output: true, input: false } },
    { label: 'Input Nets', suffix: 'connected_nets', args: { output: false, input: true } },
    { label: 'All Nets', suffix: 'connected_nets', args: { output: true, input: true } },
    { label: 'All buffer trees', suffix: 'connected_buffer_trees' },
    // Web-only superset of the Qt behavior: also walk inverter repeater chains.
    { label: 'All buffer trees (incl. inverters)', suffix: 'connected_buffer_trees',
      args: { include_inverters: true } },
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
        // Hide the menu when clicking (left or right) outside of it.
        document.addEventListener('mousedown', (e) => {
            if (!this._el.contains(e.target)) {
                this.hide();
            }
        });

        // Hide the menu when the map moves.
        if (this._app.map) {
            this._app.map.on('movestart', () => this.hide());
        }
    }

    show(e) {
        this._el.innerHTML = '';

        // Enable items by selection type, mirroring the Qt GUI's
        // updateContextMenuItems(): "Connected Insts" needs a selected net;
        // the net/buffer-tree items need a selected instance.
        const hasInst = !!this._app.selHasInst;
        const hasNet = !!this._app.selHasNet;
        const noSel = !(hasInst || hasNet);
        // Select and Highlight share the same targets and enable rules; only
        // the leaf differs (action vs. color submenu).  Build both from one
        // descriptor so they can't drift.
        const connectedSubmenu = (leaf) => CONNECTED_ITEMS.map((it) => ({
            label: it.label,
            disabled: it.needsNet ? !hasNet : !hasInst,
            ...leaf(it.suffix, it.args || {}),
        }));
        const menuItems = [
            { label: 'Select', disabled: noSel,
              subMenu: connectedSubmenu((suffix, args) => ({
                  action: () => this._sendContextAction('select_' + suffix, args),
              })) },
            { label: 'Highlight', disabled: noSel,
              subMenu: connectedSubmenu((suffix, args) => ({
                  subMenu: this._buildColorSubmenu('highlight_' + suffix, args),
              })) },
            { label: 'Save', subMenu: [
                { label: 'Visible layout',
                  action: () => this._app.captureLayout?.({ entire: false }) },
                { label: 'Entire layout',
                  action: () => this._app.captureLayout?.({ entire: true }) },
            ]},
            { label: 'Clear', subMenu: [
                { label: 'Selections', action: () => this._sendContextAction('clear_selections') },
                { label: 'Highlights', action: () => this._sendContextAction('clear_highlights') },
                { label: 'Rulers', action: () => { this._app.rulerManager?.clearAllRulers(); } },
                { label: 'Focus nets', action: () => this._sendContextAction('clear_focus_nets') },
                { label: 'Route Guides', action: () => this._sendContextAction('clear_route_guides') },
                { label: 'All', action: () => {
                    this._sendContextAction('clear_all');
                    this._app.rulerManager?.clearAllRulers();
                }},
            ]},
        ];

        this._buildMenu(menuItems, this._el);

        // Show first so we can measure, then clamp into the viewport so the
        // menu (and its submenus) never spills off the right/bottom edge.
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

    // Build a 16-color submenu for a highlight action; each entry sends the
    // group index (0-15) matching gui::Painter::kHighlightColors.
    _buildColorSubmenu(action, baseArgs = {}) {
        return HIGHLIGHT_COLORS.map(({ name, hex }, idx) => ({
            label: name,
            swatch: hex,
            action: () => this._sendContextAction(action, { ...baseArgs, group: idx }),
        }));
    }

    _buildMenu(items, container) {
        items.forEach(item => {
            const row = document.createElement('div');
            row.className = 'cm-item';
            if (item.swatch) {
                const sw = document.createElement('span');
                sw.className = 'cm-swatch';
                sw.style.background = item.swatch;
                row.appendChild(sw);
            }
            row.appendChild(document.createTextNode(item.label));

            if (item.disabled) {
                // Greyed-out entry: no action, no submenu expansion.
                row.classList.add('disabled');
                if (item.subMenu) {
                    row.classList.add('has-submenu');
                }
                container.appendChild(row);
                return;
            }

            if (item.subMenu) {
                row.classList.add('has-submenu');
                const subContainer = document.createElement('div');
                subContainer.className = 'cm-submenu';
                this._buildMenu(item.subMenu, subContainer);
                row.appendChild(subContainer);
                // Flip the submenu to the left when it would overflow the
                // viewport on the right.
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

    _sendContextAction(action, args = {}) {
        if (!this._app.websocketManager) {
            return;
        }
        this._app.websocketManager
            .request({ type: 'context_action', action, ...args })
            .then((resp) => {
                // Keep the client-side selection flags in sync so the menu can
                // enable/disable items by type on the next open.
                applySelectionFlags(this._app, resp);
                // Improvement over Qt (which is silent): tell the user when the
                // action found nothing, usually because the wrong object type
                // was selected (Connected Insts wants a net; the others want an
                // instance).
                if (resp && resp.connected_count === 0) {
                    this._toast('Nothing connected — select an '
                                + 'instance (or a net for "Connected Insts") first.');
                }
                if (typeof this._app.redrawAllLayers === 'function') {
                    this._app.redrawAllLayers();
                }
            })
            .catch((err) => {
                console.error('Context action failed:', action, err);
            });
    }

    // Minimal transient message shown near the top of the map container.
    _toast(msg) {
        const host = (this._app.map && this._app.map.getContainer)
            ? this._app.map.getContainer()
            : document.body;
        const el = document.createElement('div');
        el.className = 'cm-toast';
        el.textContent = msg;
        host.appendChild(el);
        setTimeout(() => { el.classList.add('cm-toast-hide'); }, 1800);
        setTimeout(() => { el.remove(); }, 2200);
    }
}
