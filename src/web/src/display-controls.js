// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Display controls — layer checkboxes and visibility tree.

import { VisTree } from './vis-tree.js';

// Layer color palette (must match server-side palette in web.cpp)
const layerPalette = [
    [70, 130, 210],  // moderate blue
    [200, 50, 50],   // red
    [50, 180, 80],   // green
    [200, 160, 40],  // amber
    [160, 60, 200],  // purple
    [40, 190, 190],  // teal
    [220, 120, 50],  // orange
    [180, 70, 150],  // magenta
];

// Populate display controls with layer checkboxes and visibility tree.
export function populateDisplayControls(app, visibility, WSTileLayer,
                                         layersData, redrawAllLayers) {
    if (!app.displayControlsEl) return;
    app.displayControlsEl.innerHTML = '';
    app.allLayers = [];

    // Instance borders layer (always below routing layers)
    const instancesLayer = new WSTileLayer(app.wsManager, '_instances', {
        zIndex: 0,
    });
    instancesLayer.addTo(app.map);
    app.allLayers.push(instancesLayer);

    // --- Layers group ---
    const layerGroup = document.createElement('div');
    layerGroup.className = 'vis-group';

    const layerHeader = document.createElement('label');
    layerHeader.className = 'vis-group-header';
    const layerArrow = document.createElement('span');
    layerArrow.className = 'vis-arrow';
    layerArrow.textContent = '\u25BC';
    layerHeader.appendChild(layerArrow);
    const layerParentCb = document.createElement('input');
    layerParentCb.type = 'checkbox';
    layerParentCb.checked = true;
    layerHeader.appendChild(layerParentCb);
    layerHeader.appendChild(document.createTextNode('Layers'));
    layerGroup.appendChild(layerHeader);

    const layerChildren = document.createElement('div');
    layerChildren.className = 'vis-group-children';
    const layerCbs = [];

    layersData.layers.forEach((name, index) => {
        const layer = new WSTileLayer(app.wsManager, name, {
            opacity: 0.7,
            zIndex: index + 1
        });
        layer.addTo(app.map);
        app.allLayers.push(layer);

        const label = document.createElement('label');

        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.checked = true;
        checkbox.addEventListener('change', () => {
            if (checkbox.checked) {
                layer.addTo(app.map);
            } else {
                app.map.removeLayer(layer);
            }
        });
        label.appendChild(checkbox);
        layerCbs.push(checkbox);

        const colorSwatch = document.createElement('span');
        colorSwatch.className = 'layer-color';
        const c = layerPalette[index % layerPalette.length];
        colorSwatch.style.backgroundColor = `rgb(${c[0]},${c[1]},${c[2]})`;
        label.appendChild(colorSwatch);

        label.appendChild(document.createTextNode(name));
        layerChildren.appendChild(label);
    });
    layerGroup.appendChild(layerChildren);

    // Parent checkbox toggles all layers
    layerParentCb.addEventListener('change', () => {
        const checked = layerParentCb.checked;
        for (const cb of layerCbs) {
            cb.checked = checked;
            cb.dispatchEvent(new Event('change', { bubbles: true }));
        }
    });
    // Update parent state when children change
    layerChildren.addEventListener('change', () => {
        const allChecked = layerCbs.every(cb => cb.checked);
        const someChecked = layerCbs.some(cb => cb.checked);
        layerParentCb.checked = allChecked;
        layerParentCb.indeterminate = someChecked && !allChecked;
    });
    // Toggle collapse
    layerArrow.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        const collapsed = layerChildren.classList.toggle('collapsed');
        layerArrow.textContent = collapsed ? '\u25B6' : '\u25BC';
    });

    app.displayControlsEl.appendChild(layerGroup);

    // --- Visibility tree (Instances, Nets, Shapes, Debug) ---
    const visTree = new VisTree(visibility, redrawAllLayers);
    visTree.add({ label: 'Instances', children: [
        { label: 'Std Cells', visKey: 'stdcells', disabled: !app.hasLiberty, children: [
            { label: 'Bufs/Invs', children: [
                { key: 'std_bufinv_timing', label: 'Timing opt.' },
                { key: 'std_bufinv', label: 'Netlist' },
            ]},
            { key: 'std_combinational', label: 'Combinational' },
            { key: 'std_sequential', label: 'Sequential' },
            { label: 'Clock tree', children: [
                { key: 'std_clock_bufinv', label: 'Buffer/Inverter' },
                { key: 'std_clock_gate', label: 'Clock gate' },
            ]},
            { key: 'std_level_shift', label: 'Level shifter' },
        ]},
        { key: 'macros', label: 'Macros' },
        { label: 'Pads', children: [
            { key: 'pad_input', label: 'Input' },
            { key: 'pad_output', label: 'Output' },
            { key: 'pad_inout', label: 'Inout' },
            { key: 'pad_power', label: 'Power' },
            { key: 'pad_spacer', label: 'Spacer' },
            { key: 'pad_areaio', label: 'Area IO' },
            { key: 'pad_other', label: 'Other' },
        ]},
        { label: 'Physical', children: [
            { key: 'phys_fill', label: 'Fill' },
            { key: 'phys_endcap', label: 'Endcap' },
            { key: 'phys_welltap', label: 'Welltap' },
            { key: 'phys_tie', label: 'Tie Hi/Lo' },
            { key: 'phys_antenna', label: 'Antenna' },
            { key: 'phys_cover', label: 'Cover' },
            { key: 'phys_bump', label: 'Bump' },
            { key: 'phys_other', label: 'Other' },
        ]},
    ]});
    visTree.add({ label: 'Nets', children: [
        { key: 'net_signal', label: 'Signal' },
        { key: 'net_power', label: 'Power' },
        { key: 'net_ground', label: 'Ground' },
        { key: 'net_clock', label: 'Clock' },
        { key: 'net_reset', label: 'Reset' },
        { key: 'net_tieoff', label: 'Tie off' },
        { key: 'net_scan', label: 'Scan' },
        { key: 'net_analog', label: 'Analog' },
    ]});
    visTree.add({ label: 'Shapes', children: [
        { key: 'routing', label: 'Routing' },
        { key: 'special_nets', label: 'Special Nets' },
        { key: 'pins', label: 'Pins' },
        { key: 'blockages', label: 'Blockages' },
    ]});
    visTree.add({ key: 'debug', label: 'Debug tiles' });
    visTree.render(app.displayControlsEl);
}
