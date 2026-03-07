// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Data-model-driven checkbox tree (mirrors Qt's QStandardItemModel pattern).
// State lives in a plain JS tree; DOM is synced from model after every change.
// No event delegation, no bubbling — avoids the cascade bugs of the old approach.

export class VisTree {
    constructor(visibility, onChange) {
        this.visibility = visibility;
        this.onChange = onChange;
        this.roots = [];
    }

    // Add a tree from a declarative spec.
    // Leaf:  { key, label }
    // Group: { label, children: [...], visKey?, disabled? }
    add(spec) {
        this.roots.push(this._build(spec, null));
        return this;
    }

    render(container) {
        for (const r of this.roots) container.appendChild(this._dom(r));
        for (const r of this.roots) this._sync(r);
    }

    // -- model --

    _build(spec, parent) {
        const node = {
            label: spec.label, key: spec.key || null,
            visKey: spec.visKey || null, disabled: !!spec.disabled,
            parent, children: [], checked: true, indeterminate: false, cb: null,
        };
        if (spec.children) {
            for (const c of spec.children)
                node.children.push(this._build(c, node));
            this._computeParent(node);
        } else if (node.key) {
            node.checked = !!this.visibility[node.key];
        }
        return node;
    }

    _computeParent(node) {
        const all  = node.children.every(c => c.checked && !c.indeterminate);
        const none = node.children.every(c => !c.checked && !c.indeterminate);
        node.checked = all;
        node.indeterminate = !all && !none;
    }

    _setSubtree(node, checked) {
        node.checked = checked;
        node.indeterminate = false;
        for (const c of node.children) this._setSubtree(c, checked);
    }

    _onCheck(node) {
        node.checked = node.cb.checked;
        node.indeterminate = false;
        if (!node.disabled) {
            for (const c of node.children) this._setSubtree(c, node.checked);
        }
        for (let n = node.parent; n; n = n.parent) this._computeParent(n);
        for (const r of this.roots) this._sync(r);
        this.onChange();
    }

    _sync(node) {
        if (node.cb) {
            node.cb.checked = node.checked;
            node.cb.indeterminate = node.indeterminate;
        }
        if (node.key) this.visibility[node.key] = node.checked;
        if (node.visKey) this.visibility[node.visKey] = node.checked || node.indeterminate;
        for (const c of node.children) this._sync(c);
    }

    // -- DOM --

    _dom(node) {
        const cb = document.createElement('input');
        cb.type = 'checkbox';
        node.cb = cb;
        cb.addEventListener('change', () => this._onCheck(node));

        if (!node.children.length) {
            const label = document.createElement('label');
            label.appendChild(cb);
            label.appendChild(document.createTextNode(node.label));
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
        header.appendChild(document.createTextNode(node.label));
        div.appendChild(header);

        const kids = document.createElement('div');
        kids.className = 'vis-group-children collapsed';
        if (node.disabled) kids.classList.add('disabled');
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
