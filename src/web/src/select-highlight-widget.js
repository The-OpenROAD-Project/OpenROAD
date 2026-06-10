// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Select & Highlight Browser — mirrors the Qt GUI's SelectHighlightWindow.
// Two tables (Selected / Highlighted) backed by the server-side selection set
// and per-group highlight sets, with de-select / de-highlight / highlight /
// change-group / clear-all / zoom / inspect actions.

import { dbuRectToBounds } from './coordinates.js';

// Mirrors gui::Painter::kHighlightColors (16 groups).
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

export class SelectHighlightWidget {
    constructor(app) {
        this._app = app;
        this._build();
        const ws = app.websocketManager;
        if (ws && ws.readyPromise) {
            ws.readyPromise.then(() => this.update());
        }
    }

    _build() {
        const el = document.createElement('div');
        el.className = 'sh-widget';
        el.style.cssText =
            'display:flex;flex-direction:column;height:100%;overflow:hidden;'
            + 'font:12px sans-serif;color:var(--fg-primary);';

        this._selSection = this._makeSection('Selected', () => {
            this._action({ type: 'clear_selections' });
        });
        this._hltSection = this._makeSection('Highlighted', () => {
            this._action({ type: 'clear_highlights' });
        });

        el.appendChild(this._selSection.wrap);
        el.appendChild(this._hltSection.wrap);
        this.element = el;
    }

    // A titled, scrollable section with a "Clear All" button and a <tbody>.
    _makeSection(title, onClearAll) {
        const wrap = document.createElement('div');
        wrap.style.cssText
            = 'flex:1 1 50%;display:flex;flex-direction:column;'
            + 'overflow:hidden;border-bottom:1px solid var(--border-subtle);';

        const header = document.createElement('div');
        header.style.cssText
            = 'display:flex;align-items:center;justify-content:space-between;'
            + 'padding:3px 6px;background:var(--bg-header);'
            + 'border-bottom:1px solid var(--border-subtle);';
        const label = document.createElement('span');
        label.textContent = title;
        label.style.fontWeight = '600';
        const count = document.createElement('span');
        count.style.cssText = 'color:var(--fg-muted);margin-left:auto;'
            + 'margin-right:8px;font-size:11px;';
        const clearBtn = document.createElement('button');
        clearBtn.textContent = 'Clear All';
        clearBtn.style.cssText = 'font:11px sans-serif;cursor:pointer;';
        clearBtn.addEventListener('click', onClearAll);
        header.appendChild(label);
        header.appendChild(count);
        header.appendChild(clearBtn);

        const scroll = document.createElement('div');
        scroll.style.cssText = 'flex:1;overflow:auto;';
        const table = document.createElement('table');
        table.style.cssText = 'width:100%;border-collapse:collapse;';
        const tbody = document.createElement('tbody');
        table.appendChild(tbody);
        scroll.appendChild(table);

        wrap.appendChild(header);
        wrap.appendChild(scroll);
        return { wrap, tbody, count };
    }

    async update() {
        const ws = this._app.websocketManager;
        if (!ws) {
            return;
        }
        let data;
        try {
            data = await ws.request({ type: 'selection_list' });
        } catch (err) {
            return;
        }
        this._renderSelected(data.selected || []);
        this._renderHighlighted(data.highlighted || []);
    }

    _renderSelected(items) {
        const tbody = this._selSection.tbody;
        tbody.innerHTML = '';
        this._selSection.count.textContent = String(items.length);
        for (const it of items) {
            const tr = this._row(it, 'selected');
            const actions = this._actionCell([
                ['Highlight', (btn) => this._pickGroup(btn, (g) => {
                    this._action({
                        type: 'set_highlight_group', group: g, sel_ids: [it.id],
                    });
                })],
                ['Zoom', () => this._zoom(it.bbox)],
                ['✕', () => this._action({ type: 'deselect', ids: [it.id] }),
                    'De-Select'],
            ]);
            tr.appendChild(actions);
            tbody.appendChild(tr);
        }
    }

    _renderHighlighted(items) {
        const tbody = this._hltSection.tbody;
        tbody.innerHTML = '';
        this._hltSection.count.textContent = String(items.length);
        for (const it of items) {
            const tr = this._row(it, 'highlighted');
            // Group swatch cell.
            const gcell = document.createElement('td');
            gcell.style.cssText = 'padding:2px 4px;width:18px;';
            const sw = document.createElement('span');
            const color = HIGHLIGHT_COLORS[it.group % HIGHLIGHT_COLORS.length];
            sw.title = 'Group ' + (it.group + 1)
                + (color ? ' (' + color.name + ')' : '');
            sw.style.cssText = 'display:inline-block;width:11px;height:11px;'
                + 'border:1px solid var(--border-strong);vertical-align:middle;'
                + 'background:' + (color ? color.hex : '#888') + ';';
            gcell.appendChild(sw);
            tr.insertBefore(gcell, tr.firstChild);

            const actions = this._actionCell([
                ['Group', (btn) => this._pickGroup(btn, (g) => {
                    this._action({
                        type: 'change_highlight_group', group: g,
                        hl_ids: [it.id],
                    });
                })],
                ['Zoom', () => this._zoom(it.bbox)],
                ['✕', () => this._action({ type: 'dehighlight', ids: [it.id] }),
                    'De-Highlight'],
            ]);
            tr.appendChild(actions);
            tbody.appendChild(tr);
        }
    }

    // Build the common <tr> with Object (clickable → inspect) + Type cells.
    _row(it, kind) {
        const tr = document.createElement('tr');
        tr.style.borderBottom = '1px solid var(--border-subtle)';

        const nameCell = document.createElement('td');
        nameCell.textContent = it.name;
        nameCell.title = it.name;
        nameCell.style.cssText = 'padding:2px 6px;cursor:pointer;'
            + 'color:var(--accent-link);max-width:160px;overflow:hidden;'
            + 'text-overflow:ellipsis;white-space:nowrap;';
        nameCell.addEventListener('click', () => this._inspect(kind, it.id));

        const typeCell = document.createElement('td');
        typeCell.textContent = it.type || '';
        typeCell.style.cssText = 'padding:2px 6px;color:var(--fg-secondary);';

        tr.appendChild(nameCell);
        tr.appendChild(typeCell);
        return tr;
    }

    _actionCell(buttons) {
        const td = document.createElement('td');
        td.style.cssText = 'padding:2px 4px;text-align:right;white-space:nowrap;';
        for (const [text, handler, title] of buttons) {
            const b = document.createElement('button');
            b.textContent = text;
            if (title) {
                b.title = title;
            }
            b.style.cssText = 'font:11px sans-serif;margin-left:4px;'
                + 'cursor:pointer;';
            b.addEventListener('click', () => handler(b));
            td.appendChild(b);
        }
        return td;
    }

    // Small popup with the 16 group swatches; calls onPick(groupIndex).
    _pickGroup(anchor, onPick) {
        this._closePicker();
        const pop = document.createElement('div');
        pop.style.cssText = 'position:fixed;z-index:10001;'
            + 'background:var(--bg-context);border:1px solid var(--border-strong);'
            + 'border-radius:4px;padding:4px;display:grid;'
            + 'grid-template-columns:repeat(4,16px);gap:3px;'
            + 'box-shadow:0 2px 8px var(--shadow);';
        HIGHLIGHT_COLORS.forEach((c, idx) => {
            const sw = document.createElement('div');
            sw.title = (idx + 1) + ': ' + c.name;
            sw.style.cssText = 'width:16px;height:16px;cursor:pointer;'
                + 'border:1px solid var(--border-strong);background:' + c.hex + ';';
            sw.addEventListener('click', (e) => {
                e.stopPropagation();
                this._closePicker();
                onPick(idx);
            });
            pop.appendChild(sw);
        });
        const rect = anchor.getBoundingClientRect();
        pop.style.left = Math.min(rect.left, window.innerWidth - 90) + 'px';
        pop.style.top = (rect.bottom + 2) + 'px';
        document.body.appendChild(pop);
        this._picker = pop;
        // Close on next outside mousedown.
        this._pickerClose = (e) => {
            if (!pop.contains(e.target)) {
                this._closePicker();
            }
        };
        setTimeout(() => document.addEventListener(
            'mousedown', this._pickerClose), 0);
    }

    _closePicker() {
        if (this._picker) {
            this._picker.remove();
            this._picker = null;
            document.removeEventListener('mousedown', this._pickerClose);
        }
    }

    _zoom(bbox) {
        const app = this._app;
        if (!bbox || !app.map || !app.designScale) {
            return;
        }
        const [x1, y1, x2, y2] = bbox;
        app.map.fitBounds(
            dbuRectToBounds(x1, y1, x2, y2, app.designScale,
                            app.designMaxDXDY, app.designOriginX,
                            app.designOriginY),
            { maxZoom: app.map.getZoom() });
    }

    _inspect(kind, id) {
        const ws = this._app.websocketManager;
        if (!ws) {
            return;
        }
        ws.request({ type: 'browser_inspect', kind, id })
            .then((data) => {
                if (this._app.updateInspector) {
                    this._app.updateInspector(data);
                }
            })
            .catch((err) => console.error('browser_inspect failed:', err));
    }

    // Send a mutating request, then refresh overlay + re-fetch the lists.
    _action(msg) {
        const ws = this._app.websocketManager;
        if (!ws) {
            return Promise.resolve();
        }
        return ws.request(msg)
            .then(() => {
                if (this._app.refreshOverlay) {
                    this._app.refreshOverlay();
                }
                this.update();
            })
            .catch((err) => console.error('selection action failed:',
                                          msg.type, err));
    }
}
