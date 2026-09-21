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
//
// Row layout mirrors the Qt GUI's column order (displayControls.cpp): the
// name stretches on the left and the visibility / selectability checkboxes
// sit in fixed-width columns pinned to the right edge, under the icons of
// the panel header.  DOM order within a row is therefore name, visibility,
// selectability.

import { CheckboxTreeModel } from './checkbox-tree-model.js';
import { makeGroupHeader, attachGroupCollapse } from './ui-utils.js';

// Column header icons, matching the ones the Qt GUI puts in its QHeaderView
// (DisplayControlModel::headerData): Material "visibility" over the visibility
// column and Material "touch_app" over the selectability column.
const VISIBLE_ICON_SVG =
    '<svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>' +
    '</svg>';
const SELECT_ICON_SVG =
    '<svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M9 11.24V7.5a2.5 2.5 0 0 1 5 0v3.74c1.21-.81 2-2.18 2-3.74C16 5.01 13.99 3 11.5 3S7 5.01 7 7.5c0 1.56.79 2.93 2 3.74zm9.84 4.63l-4.54-2.26c-.17-.07-.35-.11-.54-.11H13v-6c0-.83-.67-1.5-1.5-1.5S10 6.67 10 7.5v10.74l-3.44-.72c-.37-.08-.76.04-1.03.31l-1.06 1.06 5.53 5.53c.28.28.66.44 1.06.44h6.79c.75 0 1.38-.55 1.49-1.29l.74-5.18c.1-.72-.27-1.42-.92-1.74z"/>' +
    '</svg>';

// Row name cell.  It stretches (CSS `flex: 1`) so the checkbox columns that
// follow it are pushed to the row's right edge.
export function makeNameSpan(text) {
    const span = document.createElement('span');
    span.className = 'vis-name';
    if (text != null) span.textContent = text;
    return span;
}

// Layout-only placeholder keeping the selectability column aligned on rows
// that have no selectability checkbox.
export function makeSelSpacer() {
    const span = document.createElement('span');
    span.className = 'vis-sel-cb vis-sel-spacer';
    return span;
}

// Header row for the display-controls panel: an empty stretching name cell
// followed by the visibility and selectability column icons, so each icon
// lines up with the checkbox column below it.
export function makeColumnHeader() {
    const header = document.createElement('div');
    header.className = 'display-controls-header';
    header.appendChild(makeNameSpan(''));
    for (const [svg, title] of [[VISIBLE_ICON_SVG, 'Visible'],
                                [SELECT_ICON_SVG, 'Selectable']]) {
        const cell = document.createElement('span');
        cell.className = 'vis-header-icon';
        cell.title = title;
        cell.innerHTML = svg;
        header.appendChild(cell);
    }
    return header;
}

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
        cb.className = 'vis-cb';
        cb.title = 'Visible';
        node.cb = cb;
        cb.addEventListener('change', () => this.model.check(node.id, cb.checked));

        const selCb = this._buildSelCheckbox(node);

        // Append the checkbox columns last so they land at the row's right
        // edge; the name span between them and the arrow takes up the slack.
        const appendColumns = (row) => {
            row.appendChild(cb);
            if (selCb) {
                row.appendChild(selCb);
                const selNode = this.selModel.get(node.id);
                if (selNode) selNode.selLabel = row;
            } else {
                row.appendChild(makeSelSpacer());
            }
        };

        if (!node.children.length) {
            // A <div>, not a <label>, for the same reason group headers are
            // (see makeGroupHeader): a <label> wrapping the visibility
            // checkbox activates it for a click anywhere in the row — the
            // name, the indent spacer, the row's own padding — so a click
            // that merely missed the 13px box flipped the layer.  Only the
            // checkboxes toggle state now, matching the Qt GUI's tree.
            const row = document.createElement('div');
            row.className = 'vis-leaf';
            const spacer = document.createElement('span');
            spacer.className = 'vis-arrow';
            spacer.style.visibility = 'hidden';
            spacer.textContent = '▶';
            row.appendChild(spacer);
            row.appendChild(makeNameSpan(spec.label));
            appendColumns(row);
            node.el = row;
            return row;
        }

        const div = document.createElement('div');
        div.className = 'vis-group';

        const { header, arrow, name } = makeGroupHeader();
        name.textContent = spec.label;
        appendColumns(header);
        div.appendChild(header);

        const kids = document.createElement('div');
        kids.className = 'vis-group-children';
        if (spec.disabled) kids.classList.add('disabled');
        for (const c of node.children) kids.appendChild(this._dom(c));
        div.appendChild(kids);

        attachGroupCollapse(header, arrow, kids, true);

        return div;
    }
}
