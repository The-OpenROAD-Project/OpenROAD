// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Data-model-driven checkbox tree (mirrors Qt's QStandardItemModel pattern).
// State lives in CheckboxTreeModel; DOM is synced from model after every
// change.
//
// Each row renders a visibility checkbox.  Rows that opt into selectability
// (via `selectable: true` on a leaf, or `addSelectable: true` on a group that
// then propagates to its descendants) also render a second checkbox in the
// selectability column.  When visibility is unchecked, the selectability
// checkbox is disabled — matching the Qt GUI's displayControls behavior.

import { CheckboxTreeModel } from './checkbox-tree-model.js';

export class VisTree {
    constructor(visibility, selectability, onChange) {
        this.visibility = visibility;
        this.selectability = selectability;
        this.onChange = onChange;
        const notify = () => {
            this._syncAll();
            this.onChange();
        };
        this.model = new CheckboxTreeModel(notify);
        this.selModel = new CheckboxTreeModel(notify);
    }

    // Add a tree from a declarative spec.
    // Leaf:  { key, label, selectable?: bool }
    // Group: { label, children: [...], visKey?, disabled?, addSelectable?: bool }
    // `addSelectable: true` on a group makes its descendant leaves selectable
    // unless they set `selectable: false` to opt out.  `selectable: true` on
    // a leaf opts in even without an ancestor `addSelectable`.
    add(spec) {
        this.model.addFromSpec(this._enrichSpec(spec, ''));
        this.selModel.addFromSpec(this._enrichSelSpec(spec, '', false));
        return this;
    }

    render(container) {
        for (const r of this.model.roots) container.appendChild(this._dom(r));
        this._syncAll();
    }

    // -- model helpers --

    // Compute a path-qualified model id.  Without the parent prefix, two
    // groups with the same label (e.g. top-level "Instances" and the
    // "Misc / Instances" subgroup) would collide in CheckboxTreeModel's
    // node map and make parent-level toggling pick the wrong subtree.
    _nodeId(spec, parentId) {
        const local = spec.key || spec.label;
        return parentId ? parentId + '/' + local : local;
    }

    // Convert user spec to visibility model spec.
    _enrichSpec(spec, parentId) {
        const id = this._nodeId(spec, parentId);
        const result = { id, data: spec };
        if (spec.children) {
            result.children = spec.children.map(c => this._enrichSpec(c, id));
        } else if (spec.key) {
            result.checked = !!this.visibility[spec.key];
        }
        if (spec.disabled != null) result.disabled = spec.disabled;
        return result;
    }

    // Convert user spec to selectability model spec.  Groups that don't have
    // any selectable descendant collapse to `hasCheckbox: false` so the
    // CheckboxTreeModel's tri-state propagation skips them naturally.
    _enrichSelSpec(spec, parentId, scope) {
        // A group can override the inherited "selectable scope" of its
        // subtree via `addSelectable: true/false`.
        const childScope = spec.addSelectable === true ? true
            : spec.addSelectable === false ? false
            : scope;
        const id = this._nodeId(spec, parentId);
        const node = { id, data: spec, hasCheckbox: false };
        if (spec.children) {
            node.children = spec.children.map(
                c => this._enrichSelSpec(c, id, childScope));
            // Group has a selectability checkbox iff at least one descendant
            // has one.
            const anyChild = (subtree) => subtree.some(
                c => c.hasCheckbox || (c.children && anyChild(c.children)));
            node.hasCheckbox = anyChild(node.children);
            if (node.hasCheckbox) {
                node.checked = true;
            }
        } else if (spec.key) {
            const isSelectable = spec.selectable === true
                || (spec.selectable !== false && scope);
            node.hasCheckbox = isSelectable;
            if (isSelectable) {
                node.checked = this.selectability[spec.key] !== false;
            }
        }
        return node;
    }

    _syncAll() {
        // Visibility column: refresh DOM and write back to `visibility`.
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
        // Selectability column: refresh DOM, write back to `selectability`,
        // and disable when the corresponding visibility node is unchecked.
        this.selModel.forEach(node => {
            if (!node.hasCheckbox) return;
            const spec = node.data;
            if (spec.key) this.selectability[spec.key] = node.checked;
            if (!node.selCb) return;
            node.selCb.checked = node.checked;
            node.selCb.indeterminate = node.indeterminate;
            const visNode = this.model.get(node.id);
            const visOff = visNode && !visNode.checked && !visNode.indeterminate;
            node.selCb.disabled = visOff;
            if (node.selLabel) {
                node.selLabel.classList.toggle('vis-sel-disabled', visOff);
            }
        });
        // Apply disabledBy: gray out nodes whose controlling key is off.
        this.model.forEach(node => {
            const spec = node.data;
            if (spec.disabledBy && node.el) {
                const disabled = !this.visibility[spec.disabledBy];
                node.el.classList.toggle('disabled', disabled);
            }
        });
    }

    // -- DOM --

    // Build a selectability checkbox for `node` (a visibility-model node),
    // wired to the parallel sel-model node with the same id.  Returns null
    // if this row doesn't expose selectability.
    _buildSelCheckbox(node) {
        const selNode = this.selModel.get(node.id);
        if (!selNode || !selNode.hasCheckbox) return null;
        const cb = document.createElement('input');
        cb.type = 'checkbox';
        cb.className = 'vis-sel-cb';
        cb.title = 'Selectable';
        cb.addEventListener('change', () => {
            this.selModel.check(selNode.id, cb.checked);
        });
        selNode.selCb = cb;
        return cb;
    }

    _dom(node) {
        const spec = node.data;

        const cb = document.createElement('input');
        cb.type = 'checkbox';
        cb.title = 'Visible';
        node.cb = cb;
        cb.addEventListener('change', () => this.model.check(node.id, cb.checked));

        const selCb = this._buildSelCheckbox(node);

        if (!node.children.length) {
            const label = document.createElement('label');
            label.className = 'vis-leaf';
            const spacer = document.createElement('span');
            spacer.className = 'vis-arrow';
            spacer.style.visibility = 'hidden';
            spacer.textContent = '▶';
            label.appendChild(spacer);
            label.appendChild(cb);
            if (selCb) {
                label.appendChild(selCb);
                const selNode = this.selModel.get(node.id);
                if (selNode) selNode.selLabel = label;
            } else {
                label.appendChild(this._selSpacer());
            }
            label.appendChild(document.createTextNode(spec.label));
            node.el = label;
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
        if (selCb) {
            header.appendChild(selCb);
            const selNode = this.selModel.get(node.id);
            if (selNode) selNode.selLabel = header;
        } else {
            header.appendChild(this._selSpacer());
        }
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

    // Empty placeholder that keeps the selectability column aligned for rows
    // that have no selectability checkbox.
    _selSpacer() {
        const span = document.createElement('span');
        span.className = 'vis-sel-cb vis-sel-spacer';
        return span;
    }
}
