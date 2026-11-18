// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Pure-model checkbox tree with tri-state propagation.
// No DOM dependency — consumers handle their own rendering.
//
// Nodes with hasCheckbox:false are structural: they are traversed through
// but skipped during tri-state computation.

export class CheckboxTreeModel {
    constructor(onChange) {
        this.onChange = onChange;
        this.roots = [];
        this._nodeMap = new Map();
    }

    // Build from a declarative spec.
    // Spec: { id, label?, checked?, hasCheckbox?, data?, children?: [...] }
    // Returns the root node.
    addFromSpec(spec, parent = null) {
        const node = this._makeNode(spec.id, parent, spec);
        node.checked = spec.checked !== false;
        if (spec.children) {
            for (const child of spec.children) {
                node.children.push(this.addFromSpec(child, node));
            }
            if (node.hasCheckbox) {
                this._computeParent(node);
            }
        }
        if (!parent) {
            this.roots.push(node);
        }
        return node;
    }

    // Build from a flat list of { id, parentId, hasCheckbox, checked, data }.
    // Returns array of root nodes.
    buildFromNodes(flatNodes) {
        this._nodeMap.clear();
        this.roots = [];

        for (const item of flatNodes) {
            this._makeNode(item.id, null, item);
        }

        for (const item of flatNodes) {
            const node = this._nodeMap.get(item.id);
            if (item.parentId != null && this._nodeMap.has(item.parentId)) {
                const parent = this._nodeMap.get(item.parentId);
                node.parent = parent;
                parent.children.push(node);
            } else {
                this.roots.push(node);
            }
        }

        // Compute parent states bottom-up.
        for (let i = flatNodes.length - 1; i >= 0; i--) {
            const node = this._nodeMap.get(flatNodes[i].id);
            if (node.hasCheckbox && node.children.length > 0) {
                this._computeParent(node);
            }
        }

        return this.roots;
    }

    get(id) {
        return this._nodeMap.get(id);
    }

    isVisible(id) {
        const node = this.get(id);
        return node ? node.checked : false;
    }

    // Handle a user check/uncheck.  Propagates down then up.
    check(id, checked) {
        const node = this._nodeMap.get(id);
        if (!node) return;
        node.checked = checked;
        node.indeterminate = false;
        for (const c of node.children) {
            this._setSubtree(c, checked);
        }
        for (let n = node.parent; n; n = n.parent) {
            if (n.hasCheckbox) {
                this._computeParent(n);
            }
        }
        this.onChange();
    }

    // Same down+up propagation as check(), but does NOT fire onChange.
    // The caller takes responsibility for any side-effects that
    // onChange would normally drive (DOM sync, cookie write, redraw).
    // Used to apply a cascade originated in a sibling tree without
    // triggering this tree's own mirror-back logic.
    cascadeQuiet(id, checked) {
        const node = this._nodeMap.get(id);
        if (!node) return;
        node.checked = checked;
        node.indeterminate = false;
        for (const c of node.children) {
            this._setSubtree(c, checked);
        }
        for (let n = node.parent; n; n = n.parent) {
            if (n.hasCheckbox) {
                this._computeParent(n);
            }
        }
    }

    // Bulk check/uncheck.  Single onChange call at the end.
    // idToChecked: object { id: boolean, ... }
    checkSet(idToChecked) {
        for (const [id, checked] of Object.entries(idToChecked)) {
            const node = this._nodeMap.get(id);
            if (!node) continue;
            node.checked = checked;
            node.indeterminate = false;
        }
        this._recomputeAllParents();
        this.onChange();
    }

    // Mirror tri-state from an external source — used to keep a sibling
    // tree (e.g. the Chiplets panel) in sync with this one without
    // forcing partial selections into a binary state.  idToState maps
    // id -> { checked, indeterminate }.  Unlike check()/checkSet(), no
    // propagation is performed: the caller is expected to supply states
    // already aggregated from the authoritative source.  Nodes absent
    // from the map keep their previous state; any descendants of a
    // mirrored parent are NOT cascaded.
    mirrorStates(idToState) {
        this._applyMirror(idToState);
        this.onChange();
    }

    // Same as mirrorStates but does not fire onChange.  Use when the
    // mirror is only updating the UI tri-state of a sibling tree and
    // the heavier side-effects of onChange (e.g. visibleChiplets
    // recompute + redrawAllLayers) would be redundant.
    mirrorStatesQuiet(idToState) {
        this._applyMirror(idToState);
    }

    _applyMirror(idToState) {
        for (const [id, state] of Object.entries(idToState)) {
            const node = this._nodeMap.get(id);
            if (!node) continue;
            node.checked = !!state.checked;
            node.indeterminate = !!state.indeterminate;
        }
    }

    // DFS iteration over every node.
    forEach(fn) {
        const walk = (node) => {
            fn(node);
            for (const c of node.children) walk(c);
        };
        for (const r of this.roots) walk(r);
    }

    // -- internal --

    _makeNode(id, parent, spec) {
        const node = {
            id,
            parent,
            children: [],
            hasCheckbox: spec.hasCheckbox !== false,
            checked: spec.checked !== false,
            indeterminate: false,
            data: spec.data != null ? spec.data : spec,
        };
        this._nodeMap.set(id, node);
        return node;
    }

    // Gather the nearest checkbox-bearing descendants of a node.
    // Non-checkbox children are traversed through.
    _checkboxDescendants(node) {
        const result = [];
        for (const child of node.children) {
            if (child.hasCheckbox) {
                result.push(child);
            } else {
                result.push(...this._checkboxDescendants(child));
            }
        }
        return result;
    }

    _computeParent(node) {
        const leaves = this._checkboxDescendants(node);
        if (leaves.length === 0) return;
        const all  = leaves.every(c => c.checked && !c.indeterminate);
        const none = leaves.every(c => !c.checked && !c.indeterminate);
        node.checked = all;
        node.indeterminate = !all && !none;
    }

    _setSubtree(node, checked) {
        if (node.hasCheckbox) {
            node.checked = checked;
            node.indeterminate = false;
        }
        for (const c of node.children) {
            this._setSubtree(c, checked);
        }
    }

    _recomputeAllParents() {
        const recompute = (node) => {
            for (const c of node.children) recompute(c);
            if (node.hasCheckbox && node.children.length > 0) {
                this._computeParent(node);
            }
        };
        for (const r of this.roots) recompute(r);
    }
}
