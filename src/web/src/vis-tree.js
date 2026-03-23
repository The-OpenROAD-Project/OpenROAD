// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Data-model-driven checkbox tree (mirrors Qt's QStandardItemModel pattern).
// State lives in CheckboxTreeModel; DOM is synced from model after every change.

import { CheckboxTreeModel } from './checkbox-tree-model.js';

export class VisTree {
    constructor(visibility, onChange) {
        this.visibility = visibility;
        this.onChange = onChange;
        this.model = new CheckboxTreeModel(() => {
            this._syncAll();
            this.onChange();
        });
    }

    // Add a tree from a declarative spec.
    // Leaf:  { key, label }
    // Group: { label, children: [...], visKey?, disabled? }
    add(spec) {
        const enriched = this._enrichSpec(spec);
        this.model.addFromSpec(enriched);
        return this;
    }

    render(container) {
        for (const r of this.model.roots) container.appendChild(this._dom(r));
        this._syncAll();
    }

    // -- model helpers --

    // Convert user spec to model spec, initializing checked from visibility.
    _enrichSpec(spec) {
        const id = spec.key || spec.label;
        const result = { id, data: spec };
        if (spec.children) {
            result.children = spec.children.map(c => this._enrichSpec(c));
        } else if (spec.key) {
            result.checked = !!this.visibility[spec.key];
        }
        if (spec.disabled != null) result.disabled = spec.disabled;
        return result;
    }

    _syncAll() {
        this.model.forEach(node => {
            if (node.cb) {
                node.cb.checked = node.checked;
                node.cb.indeterminate = node.indeterminate;
            }
            const spec = node.data;
            if (spec.key) this.visibility[spec.key] = node.checked;
            if (spec.visKey) {
                this.visibility[spec.visKey] = node.checked || node.indeterminate;
            }
        });
    }

    // -- DOM --

    _dom(node) {
        const spec = node.data;

        const cb = document.createElement('input');
        cb.type = 'checkbox';
        node.cb = cb;
        cb.addEventListener('change', () => this.model.check(node.id, cb.checked));

        if (!node.children.length) {
            const label = document.createElement('label');
            const spacer = document.createElement('span');
            spacer.className = 'vis-arrow';
            spacer.style.visibility = 'hidden';
            spacer.textContent = '\u25B6';
            label.appendChild(spacer);
            label.appendChild(cb);
            label.appendChild(document.createTextNode(spec.label));
            return label;
        }

        const div = document.createElement('div');
        div.className = 'vis-group';

        const header = document.createElement('label');
        header.className = 'vis-group-header';
        const arrow = document.createElement('span');
        arrow.className = 'vis-arrow';
        arrow.textContent = '▶';
        header.appendChild(arrow);
        header.appendChild(cb);
        header.appendChild(document.createTextNode(spec.label));
        div.appendChild(header);

        const kids = document.createElement('div');
        kids.className = 'vis-group-children collapsed';
        if (spec.disabled) kids.classList.add('disabled');
        for (const c of node.children) kids.appendChild(this._dom(c));
        div.appendChild(kids);

        arrow.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            kids.classList.toggle('collapsed');
            arrow.textContent = kids.classList.contains('collapsed')
                ? '▶' : '▼';
        });

        return div;
    }
}
