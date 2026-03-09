// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { GoldenLayout, LayoutConfig } from 'https://esm.sh/golden-layout@2.6.0';
import { latLngToDbu } from './coordinates.js';
import { WebSocketManager } from './websocket-manager.js';
import { createWebSocketTileLayer } from './websocket-tile-layer.js';
import { TimingWidget } from './timing-widget.js';
import { ClockTreeWidget } from './clock-tree-widget.js';
import { createInspectorPanel } from './inspector.js';
import { populateDisplayControls } from './display-controls.js';

// ─── Status Indicator ───────────────────────────────────────────────────────

const statusDiv = document.getElementById('websocket-status');

function updateStatus() {
    const n = app.websocketManager ? app.websocketManager.pending.size : 0;
    if (n === 0) {
        statusDiv.textContent = '';
        statusDiv.style.display = 'none';
    } else {
        statusDiv.textContent = `pending: ${n}`;
        statusDiv.style.display = '';
        statusDiv.style.color = n > 20 ? '#f88' : '#ff0';
    }
}

// ─── Component Factories ────────────────────────────────────────────────────

// Shared application state — replaces scattered module-level globals.
// Components receive this via closure now; when extracted to separate files
// they'll receive it as an explicit parameter.
const app = {
    map: null,
    fitBounds: null,
    displayControlsEl: null,
    allLayers: [],
    designScale: null,   // pixels-per-DBU for coordinate conversion
    designMaxDXDY: null, // max(width, height) in DBU for Y-axis mapping
    websocketManager: null,     // set after construction below
    goldenLayout: null,  // set after GL init below
    hasLiberty: false,
    inspectorEl: null,
    tclOutputEl: null,
    highlightRect: null,
    hoverRects: [],
};

const visibility = {
    stdcells: true,
    macros: true,
    // Pad sub-types
    pad_input: true,
    pad_output: true,
    pad_inout: true,
    pad_power: true,
    pad_spacer: true,
    pad_areaio: true,
    pad_other: true,
    // Physical sub-types
    phys_fill: false,
    phys_endcap: true,
    phys_welltap: true,
    phys_tie: true,
    phys_antenna: true,
    phys_cover: true,
    phys_bump: true,
    phys_other: true,
    // Std cell sub-types
    std_bufinv: true,
    std_bufinv_timing: true,
    std_clock_bufinv: true,
    std_clock_gate: true,
    std_level_shift: true,
    std_sequential: true,
    std_combinational: true,
    // Net sub-types
    net_signal: true,
    net_power: true,
    net_ground: true,
    net_clock: true,
    net_reset: true,
    net_tieoff: true,
    net_scan: true,
    net_analog: true,
    // Shapes
    routing: true,
    special_nets: true,
    pins: true,
    blockages: true,
    // Blockages
    placement_blockages: true,
    routing_obstructions: true,
    // Rows (off by default, matching GUI)
    rows: false,
    // Tracks (off by default, matching GUI)
    tracks_pref: false,
    tracks_non_pref: false,
    // Debug
    debug: false,
};

const WebSocketTileLayer = createWebSocketTileLayer(visibility);

function redrawAllLayers() {
    for (const layer of app.allLayers) {
        layer.refreshTiles();
    }
}


function createLayoutViewer(container) {
    const mapDiv = document.createElement('div');
    mapDiv.style.width = '100%';
    mapDiv.style.height = '100%';
    mapDiv.style.backgroundColor = '#111';
    container.element.appendChild(mapDiv);

    app.map = L.map(mapDiv, {
        crs: L.CRS.Simple,
        zoom: 1,
        zoomSnap: 0,
        fadeAnimation: false,
        attributionControl: false,
    });

    new ResizeObserver(() => {
        app.map.invalidateSize({ animate: false });
    }).observe(mapDiv);
}

function createDisplayControls(container) {
    const el = document.createElement('div');
    el.className = 'display-controls';
    el.innerHTML = '<div class="loading">Loading layers...</div>';
    container.element.appendChild(el);
    app.displayControlsEl = el;
}

function tclAppend(text, className) {
    if (!app.tclOutputEl) return;
    const span = document.createElement('span');
    if (className) span.className = className;
    span.textContent = text;
    app.tclOutputEl.appendChild(span);
    app.tclOutputEl.scrollTop = app.tclOutputEl.scrollHeight;
}

function createTclConsole(container) {
    const el = document.createElement('div');
    el.className = 'tcl-console';
    el.innerHTML =
        '<div class="tcl-output"></div>' +
        '<div class="tcl-input-row">' +
        '  <span class="tcl-prompt">%</span>' +
        '  <input class="tcl-input" type="text" placeholder="Enter Tcl command..." />' +
        '</div>';
    container.element.appendChild(el);

    app.tclOutputEl = el.querySelector('.tcl-output');
    const input = el.querySelector('.tcl-input');
    input.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            const cmd = input.value.trim();
            if (!cmd) return;
            tclAppend(`>>> ${cmd}\n`, 'tcl-cmd');
            input.value = '';
            app.websocketManager.request({ type: 'tcl_eval', cmd })
                .then(data => {
                    if (data.output) {
                        tclAppend(data.output,
                                  data.is_error ? 'tcl-error' : '');
                    }
                    if (data.result) {
                        tclAppend(data.result + '\n',
                                  data.is_error ? 'tcl-error' : '');
                    }
                })
                .catch(err => tclAppend(`Error: ${err}\n`, 'tcl-error'));
        }
    });
}

// ─── Inspector Panel ────────────────────────────────────────────────────────

const inspector = createInspectorPanel(app, redrawAllLayers);
const createInspector = inspector.createInspector;
const updateInspector = inspector.updateInspector;
const highlightBBox = inspector.highlightBBox;

function createBrowser(container) {
    createStubPanel(container, 'Hierarchy',
        'Design hierarchy browser.');
}

function createTimingWidget(container) {
    new TimingWidget(container, app, redrawAllLayers);
}

function createDRCWidget(container) {
    createStubPanel(container, 'DRC',
        'Design rule check violations viewer.');
}

function createClockWidget(container) {
    new ClockTreeWidget(container, app, redrawAllLayers);
}

function createChartsWidget(container) {
    createStubPanel(container, 'Charts',
        'Timing histograms and charts.');
}

function createHelpWidget(container) {
    const el = document.createElement('div');
    el.className = 'help-panel';
    el.innerHTML =
        '<h3>Keyboard Shortcuts</h3>' +
        '<table>' +
        '<tr><td><kbd>f</kbd></td><td>Fit design to viewport</td></tr>' +
        '<tr><td><kbd>scroll</kbd></td><td>Zoom in/out</td></tr>' +
        '<tr><td><kbd>drag</kbd></td><td>Pan the view</td></tr>' +
        '</table>';
    container.element.appendChild(el);
}

function createSelectHighlight(container) {
    createStubPanel(container, 'Selection',
        'Selection and highlight browser.');
}

function createStubPanel(container, title, description) {
    const el = document.createElement('div');
    el.className = 'stub-panel';
    el.innerHTML =
        `<div class="stub-title">${title}</div>` +
        `<div class="stub-desc">${description}</div>`;
    container.element.appendChild(el);
}

// ─── Layout Configuration ───────────────────────────────────────────────────

const defaultLayoutConfig = {
    root: {
        type: 'row',
        content: [
            {
                type: 'component',
                componentType: 'DisplayControls',
                title: 'Display Controls',
                width: 15,
            },
            {
                type: 'column',
                width: 55,
                content: [
                    {
                        type: 'component',
                        componentType: 'LayoutViewer',
                        title: 'Layout',
                        height: 70,
                        isClosable: false,
                    },
                    {
                        type: 'component',
                        componentType: 'TclConsole',
                        title: 'Tcl Console',
                        height: 30,
                    },
                ],
            },
            {
                type: 'stack',
                width: 30,
                content: [
                    {
                        type: 'component',
                        componentType: 'Inspector',
                        title: 'Inspector',
                    },
                    {
                        type: 'component',
                        componentType: 'Browser',
                        title: 'Hierarchy',
                    },
                    {
                        type: 'component',
                        componentType: 'TimingWidget',
                        title: 'Timing',
                    },
                    {
                        type: 'component',
                        componentType: 'DRCWidget',
                        title: 'DRC',
                    },
                    {
                        type: 'component',
                        componentType: 'ClockWidget',
                        title: 'Clock Tree',
                    },
                    {
                        type: 'component',
                        componentType: 'ChartsWidget',
                        title: 'Charts',
                    },
                    {
                        type: 'component',
                        componentType: 'HelpWidget',
                        title: 'Help',
                    },
                ],
            },
        ],
    },
};

// ─── Golden Layout Init ─────────────────────────────────────────────────────

app.goldenLayout = new GoldenLayout(document.getElementById('gl-container'));

app.goldenLayout.registerComponentFactoryFunction('LayoutViewer', createLayoutViewer);
app.goldenLayout.registerComponentFactoryFunction('DisplayControls', createDisplayControls);
app.goldenLayout.registerComponentFactoryFunction('TclConsole', createTclConsole);
app.goldenLayout.registerComponentFactoryFunction('Inspector', createInspector);
app.goldenLayout.registerComponentFactoryFunction('Browser', createBrowser);
app.goldenLayout.registerComponentFactoryFunction('TimingWidget', createTimingWidget);
app.goldenLayout.registerComponentFactoryFunction('DRCWidget', createDRCWidget);
app.goldenLayout.registerComponentFactoryFunction('ClockWidget', createClockWidget);
app.goldenLayout.registerComponentFactoryFunction('ChartsWidget', createChartsWidget);
app.goldenLayout.registerComponentFactoryFunction('HelpWidget', createHelpWidget);
app.goldenLayout.registerComponentFactoryFunction('SelectHighlight', createSelectHighlight);

// Layout version — bump this to force a layout reset when components change.
const LAYOUT_VERSION = 2;

// Restore saved layout or use default
const savedLayout = localStorage.getItem('gl-layout');
const savedVersion = parseInt(localStorage.getItem('gl-layout-version'), 10);
if (savedLayout && savedVersion === LAYOUT_VERSION) {
    try {
        const resolved = JSON.parse(savedLayout);
        app.goldenLayout.loadLayout(LayoutConfig.fromResolved(resolved));
    } catch (e) {
        app.goldenLayout.loadLayout(defaultLayoutConfig);
    }
} else {
    app.goldenLayout.loadLayout(defaultLayoutConfig);
}
localStorage.setItem('gl-layout-version', LAYOUT_VERSION);

// Persist layout on changes (drag, resize, close, etc.)
app.goldenLayout.on('stateChanged', () => {
    localStorage.setItem('gl-layout', JSON.stringify(app.goldenLayout.saveLayout()));
});

// Handle window resize
window.addEventListener('resize', () => {
    app.goldenLayout.setSize(window.innerWidth, window.innerHeight);
});

// Focus a Golden Layout component tab by its componentType name.
function focusComponent(componentType) {
    function find(item) {
        if (item.isComponent && item.componentType === componentType) return item;
        if (item.contentItems) {
            for (const child of item.contentItems) {
                const found = find(child);
                if (found) return found;
            }
        }
        return null;
    }
    const item = find(app.goldenLayout.rootItem);
    if (item) item.focus();
}

// ─── WebSocket Init ─────────────────────────────────────────────────────────

const websocketUrl = `ws://${window.location.hostname || 'localhost'}:8080/ws`;
app.websocketManager = new WebSocketManager(websocketUrl, updateStatus);

app.websocketManager.readyPromise.then(async () => {
    try {
        const [techData, boundsData] = await Promise.all([
            app.websocketManager.request({ type: 'tech' }),
            app.websocketManager.request({ type: 'bounds' }),
        ]);
        app.hasLiberty = techData.has_liberty;

        // --- Set Bounds ---
        const designBounds = boundsData.bounds;

        const minY = designBounds[0][0];
        const minX = designBounds[0][1];
        const maxY = designBounds[1][0];
        const maxX = designBounds[1][1];

        const designWidth = maxX - minX;
        const designHeight = maxY - minY;
        const tileSize = 256;

        const scale = tileSize / Math.max(designWidth, designHeight);
        app.designScale = scale;
        app.designMaxDXDY = Math.max(designWidth, designHeight);

        app.fitBounds = [
            [-minY * scale, minX * scale],
            [-maxY * scale, maxX * scale]
        ];
        app.map.fitBounds(app.fitBounds);

        // Click-to-select: convert click position to DBU and query server
        app.map.on('click', (e) => {
            const { dbuX: dbu_x, dbuY: dbu_y } = latLngToDbu(
                e.latlng.lat, e.latlng.lng, app.designScale, app.designMaxDXDY);

            const vf = {};
            for (const [k, v] of Object.entries(visibility)) {
                vf[k] = v ? 1 : 0;
            }
            app.websocketManager.request({ type: 'select', dbu_x, dbu_y, zoom: app.map.getZoom(), ...vf })
                .then(data => {
                    console.log('Select response:', data, 'at dbu', dbu_x, dbu_y);
                    app.map.closePopup();
                    if (data.selected && data.selected.length > 0) {
                        const inst = data.selected[0];
                        L.popup()
                            .setLatLng(e.latlng)
                            .setContent(
                                `<strong>${inst.name}</strong><br>${inst.master}<br><small style="color:#888">(${dbu_x}, ${dbu_y})</small>`)
                            .openOn(app.map);
                        updateInspector(data);
                        focusComponent('Inspector');
                        // Highlight selected instance bbox
                        if (inst.bbox) {
                            highlightBBox(inst.bbox[0], inst.bbox[1],
                                          inst.bbox[2], inst.bbox[3]);
                        }
                    } else {
                        L.popup()
                            .setLatLng(e.latlng)
                            .setContent(
                                `<em>No instance at (${dbu_x}, ${dbu_y})</em>`)
                            .openOn(app.map);
                        updateInspector(null);
                        if (app.highlightRect) {
                            app.map.removeLayer(app.highlightRect);
                            app.highlightRect = null;
                        }
                    }
                    redrawAllLayers();
                })
                .catch(err => {
                    console.error('Select failed:', err);
                });
        });

        populateDisplayControls(app, visibility, WebSocketTileLayer,
                                techData, redrawAllLayers);
    } catch (err) {
        console.error('Failed to load initial data from server:', err);
    }
});

// ─── Keyboard Shortcuts ─────────────────────────────────────────────────────

document.addEventListener('keydown', (e) => {
    if (e.key === 'f' && app.fitBounds) {
        app.map.fitBounds(app.fitBounds);
    }
});
