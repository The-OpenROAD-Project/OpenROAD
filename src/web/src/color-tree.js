// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared logic for the "color by owner" tree panels — the Hierarchy browser
// (dbModules) and the Clusters panel (dbGroups).  Both receive the same flat
// {id, parent_id, color, odb_id} node list from the server, present it as a
// collapsible tree, and send back one color per visible owner.
//
// The rule that matters here, and the reason this is one module instead of two
// copies: a COLLAPSED node paints its whole subtree, and where collapsed nodes
// nest, the highest one wins.  The server implements the same rule for
// `save_image -web` / `web_save_report` (computeEffectiveOwnerColors in
// hierarchy_report.cpp), so a second copy on this side is a silent way for the
// saved image to disagree with the viewer.

// Index a flat node list into {childrenMap, nodeMap} and a DFS row order.
// `rows` carries the tree depth each panel needs for indentation.
export function buildTreeIndex(nodes) {
    const childrenMap = new Map();
    const nodeMap = new Map();
    for (const n of nodes) {
        childrenMap.set(n.id, []);
        nodeMap.set(n.id, n);
    }
    for (const n of nodes) {
        if (n.parent_id >= 0 && childrenMap.has(n.parent_id)) {
            childrenMap.get(n.parent_id).push(n.id);
        }
    }
    const rows = [];
    const walk = (id, depth) => {
        rows.push({ id, depth });
        for (const childId of childrenMap.get(id) || []) {
            walk(childId, depth + 1);
        }
    };
    for (const root of nodes.filter(n => n.parent_id < 0)) {
        walk(root.id, 0);
    }
    return { childrenMap, nodeMap, rows };
}

// True when any ancestor of `node` is collapsed, i.e. the row is not rendered.
export function isHidden(node, nodeMap, collapsed) {
    let parentId = node.parent_id;
    while (parentId >= 0) {
        if (collapsed.has(parentId)) return true;
        const parent = nodeMap.get(parentId);
        if (!parent) break;
        parentId = parent.parent_id;
    }
    return false;
}

// Resolve `effectiveColor` for every entry of `state` (odb_id → {color,
// effectiveColor, ...}) from the current collapse set.
//
// `rows` must be in DFS order (parent before child), which buildTreeIndex
// guarantees — that is what lets the highest collapsed ancestor win without
// walking back up the tree per node.
export function computeEffectiveColors(rows, nodeMap, state, collapsed) {
    const byNodeId = new Map();
    // `inheriting` is what makes "the HIGHEST collapsed ancestor wins" hold
    // transitively: a node is painted by an ancestor either because its parent
    // is collapsed, or because its parent was itself being painted by one — an
    // expanded node nested inside a collapsed one must not hand its own color
    // back to its children.
    const inheriting = new Set();
    for (const row of rows) {
        const node = nodeMap.get(row.id);
        if (!node) continue;
        const st = state.get(node.odb_id);
        const parent = node.parent_id >= 0 ? nodeMap.get(node.parent_id) : null;
        const fromAncestor = parent
            && (collapsed.has(parent.id) || inheriting.has(parent.id));
        if (fromAncestor) inheriting.add(node.id);
        const effective = (fromAncestor ? byNodeId.get(parent.id) : null)
            || (st ? st.color : null);
        if (effective) byNodeId.set(node.id, effective);
        if (st) st.effectiveColor = effective || st.color;
    }
}

// The wire form both set_module_colors and set_group_colors take:
// "<odb id>:<r>,<g>,<b>,<a>;..." over the visible entries only.
//
// `only`, when given, is a Set of odb ids to restrict the map to — what the
// Clusters panel uses to isolate the selected cluster's subtree.  It filters
// *which* owners are sent, never which color each one gets: the effective
// colors are already resolved by computeEffectiveColors above, so an isolated
// map paints exactly the colors the swatches show.
export function serializeColorMap(state, alpha = 100, only = null) {
    const parts = [];
    for (const [odbId, st] of state) {
        if (!st.visible) continue;
        if (only && !only.has(odbId)) continue;
        const [r, g, b] = st.effectiveColor;
        parts.push(`${odbId}:${r},${g},${b},${alpha}`);
    }
    return parts.join(';');
}

export function fmtInt(v) {
    return v != null ? String(v) : '';
}

// Area comes over the wire in μm²; the panels show mm² once it gets large, or
// raw DBU² when the viewer is in DBU mode.
export function fmtArea(app, v) {
    if (v == null) return '';
    if (app.showDbu) {
        const dbu = app.getDbuPerMicron();
        return String(Math.round(v * dbu * dbu));
    }
    if (v >= 1e6) return (v / 1e6).toFixed(3) + ' mm²';
    return v.toFixed(3) + ' μm²';
}
