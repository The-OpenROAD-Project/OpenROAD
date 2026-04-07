// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { GoldenLayout, LayoutConfig } from 'https://esm.sh/golden-layout@2.6.0';
import { latLngToDbu } from './coordinates.js';
import { WebSocketManager } from './websocket-manager.js';
import { createWebSocketTileLayer } from './websocket-tile-layer.js';
import { TimingWidget } from './timing-widget.js';
import { ClockTreeWidget } from './clock-tree-widget.js';
import { ChartsWidget } from './charts-widget.js';
import { HierarchyBrowser } from './hierarchy-browser.js';
import { createInspectorPanel } from './inspector.js';
import { populateDisplayControls } from './display-controls.js';
import { createMenuBar } from './menu-bar.js';
import { RulerManager } from './ruler.js';
import { SchematicWidget } from './schematic-widget.js';
import { TclCompleter } from './tcl-completer.js';
import './theme.js';

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
        statusDiv.style.color = n > 20 ? 'var(--error)' : 'var(--fg-bright)';
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
    designOriginX: 0,    // bounds.xMin() in DBU (tile grid origin)
    designOriginY: 0,    // bounds.yMin() in DBU (tile grid origin)
    websocketManager: null,     // set after construction below
    goldenLayout: null,  // set after GL init below
    hasLiberty: false,
    techData: null,
    inspectorEl: null,
    tclOutputEl: null,
    highlightRect: null,
    hoverHighlightLayer: null,
    hoverHighlightPane: 'hover-highlight-pane',
    modulesLayer: null,
    pinsLayer: null,
    hierarchyBrowser: null,
    focusNets: new Set(),
    routeGuideNets: new Set(),
    visibleLayers: new Set(),
    heatMapData: null,
    activeHeatMap: '',
    heatMapLayer: null,
    heatMapLegendEl: null,
    renderHeatMapControls: null,
    rulerManager: null,
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
    pin_markers: true,
    blockages: true,
    // Blockages
    placement_blockages: true,
    routing_obstructions: true,
    // Rows (off by default, matching GUI)
    rows: false,
    // Tracks (off by default, matching GUI)
    tracks_pref: false,
    tracks_non_pref: false,
    // Module view
    module_view: false,
    // Debug
    debug: false,
};

const WebSocketTileLayer = createWebSocketTileLayer(visibility);
const BLANK_TILE
    = 'data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw==';

const HeatMapTileLayer = L.GridLayer.extend({
    initialize: function(websocketManager, appState, options) {
        this._websocketManager = websocketManager;
        this._appState = appState;
        L.GridLayer.prototype.initialize.call(this, options);
    },

    createTile: function(coords, done) {
        const tile = document.createElement('img');
        tile.alt = '';
        tile.setAttribute('role', 'presentation');
        tile._tileDone = false;
        tile.onload = () => {
            if (tile.src && tile.src.startsWith('blob:')) {
                URL.revokeObjectURL(tile.src);
            }
            if (!tile._tileDone) {
                tile._tileDone = true;
                done(null, tile);
            }
        };
        tile.onerror = () => {
            if (!tile._tileDone) {
                tile._tileDone = true;
                done(new Error('heat map tile load error'), tile);
            }
        };

        const active = this._appState.activeHeatMap;
        if (!active) {
            tile.src = BLANK_TILE;
            return tile;
        }

        this._websocketManager.request({
            type: 'heatmap_tile',
            name: active,
            z: coords.z,
            x: coords.x,
            y: coords.y,
        }).then(blob => {
            tile.src = URL.createObjectURL(blob);
        }).catch(() => {
            tile.src = BLANK_TILE;
        });

        return tile;
    },

    refreshTiles: function() {
        if (!this._map) return;
        for (const key in this._tiles) {
            const tileInfo = this._tiles[key];
            if (!tileInfo || !tileInfo.el) continue;
            const tile = tileInfo.el;
            const coords = tileInfo.coords;
            const active = this._appState.activeHeatMap;
            if (!active) {
                tile.src = BLANK_TILE;
                continue;
            }
            this._websocketManager.request({
                type: 'heatmap_tile',
                name: active,
                z: coords.z,
                x: coords.x,
                y: coords.y,
            }).then(blob => {
                if (tile.src && tile.src.startsWith('blob:')) {
                    URL.revokeObjectURL(tile.src);
                }
                tile.src = URL.createObjectURL(blob);
            }).catch(() => {
                tile.src = BLANK_TILE;
            });
        }
    },
});

function updateHeatMaps(data) {
    app.heatMapData = data;
    app.activeHeatMap = data.active || '';
    if (app.heatMapLayer) {
        if (app.activeHeatMap) {
            if (!app.map.hasLayer(app.heatMapLayer)) {
                app.heatMapLayer.addTo(app.map);
            }
        } else if (app.map.hasLayer(app.heatMapLayer)) {
            app.map.removeLayer(app.heatMapLayer);
        }
        app.heatMapLayer.refreshTiles();
    }
    if (app.renderHeatMapControls) {
        app.renderHeatMapControls(data);
    }
}
app.updateHeatMaps = updateHeatMaps;

function redrawAllLayers() {
    // Show/hide modules layer based on module_view visibility
    if (app.modulesLayer) {
        if (visibility.module_view && !app.map.hasLayer(app.modulesLayer)) {
            app.modulesLayer.addTo(app.map);
        } else if (!visibility.module_view && app.map.hasLayer(app.modulesLayer)) {
            app.map.removeLayer(app.modulesLayer);
        }
    }
    // Show/hide pin markers layer
    if (app.pinsLayer) {
        if (visibility.pin_markers && !app.map.hasLayer(app.pinsLayer)) {
            app.pinsLayer.addTo(app.map);
        } else if (!visibility.pin_markers && app.map.hasLayer(app.pinsLayer)) {
            app.map.removeLayer(app.pinsLayer);
        }
    }
    for (const layer of app.allLayers) {
        layer.refreshTiles();
    }
    if (app.heatMapLayer) {
        app.heatMapLayer.refreshTiles();
    }
}


function createLayoutViewer(container) {
    const mapDiv = document.createElement('div');
    mapDiv.className = 'layout-viewer';
    mapDiv.style.width = '100%';
    mapDiv.style.height = '100%';
    mapDiv.style.backgroundColor = 'var(--bg-map)';
    container.element.appendChild(mapDiv);

    const heatMapLegend = document.createElement('div');
    heatMapLegend.className = 'heatmap-map-legend hidden';
    mapDiv.appendChild(heatMapLegend);
    app.heatMapLegendEl = heatMapLegend;

    app.map = L.map(mapDiv, {
        crs: L.CRS.Simple,
        zoom: 1,
        zoomSnap: 0,
        fadeAnimation: false,
        attributionControl: false,
    });
    const hoverPane = app.map.createPane(app.hoverHighlightPane);
    hoverPane.style.zIndex = '650';
    hoverPane.style.pointerEvents = 'none';

    new ResizeObserver(() => {
        app.map.invalidateSize({ animate: false });
    }).observe(mapDiv);

    // Coordinate readout overlay (bottom-left of the layout viewer).
    const coordBar = document.createElement('div');
    coordBar.id = 'coord-bar';
    mapDiv.appendChild(coordBar);

    app.map.on('mousemove', (e) => {
        app.lastMouseLatLng = e.latlng;
        if (!app.designScale) return;
        const { dbuX, dbuY } = latLngToDbu(
            e.latlng.lat, e.latlng.lng, app.designScale, app.designMaxDXDY,
            app.designOriginX, app.designOriginY);
        const dbuPerUm = app.techData?.dbu_per_micron || 1000;
        const precision = Math.ceil(Math.log10(dbuPerUm));
        const xUm = (dbuX / dbuPerUm).toFixed(precision);
        const yUm = (dbuY / dbuPerUm).toFixed(precision);
        coordBar.textContent = `X: ${xUm}  Y: ${yUm}`;
    });
    app.map.on('mouseout', () => { app.lastMouseLatLng = null; });

    app.rulerManager = new RulerManager(app, visibility, updateInspector, focusComponent);
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
    const completer = new TclCompleter(input, app.websocketManager);

    input.addEventListener('keydown', (e) => {
        // Let completer handle first (Tab, arrow keys, Enter-when-popup-visible)
        if (completer.handleKeyDown(e)) return;

        if (e.key === 'Enter') {
            const cmd = input.value.trim();
            if (!cmd) return;
            tclAppend(`>>> ${cmd}\n`, 'tcl-cmd');
            completer.addToHistory(cmd);
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
app.updateInspector = updateInspector;

function createBrowser(container) {
    new HierarchyBrowser(container, app, redrawAllLayers);
}

function createTimingWidget(container) {
    app.timingWidget = new TimingWidget(container, app, redrawAllLayers);
}

function createDRCWidget(container) {
    createStubPanel(container, 'DRC',
        'Design rule check violations viewer.');
}

function createClockWidget(container) {
    app.clockTreeWidget = new ClockTreeWidget(container, app, redrawAllLayers);
}

function createChartsWidget(container) {
    app.chartsWidget = new ChartsWidget(container, app, redrawAllLayers);
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
        '<tr><td><kbd>right-drag</kbd></td><td>Rubber-band zoom</td></tr>' +
        '<tr><td><kbd>k</kbd></td><td>Toggle ruler mode</td></tr>' +
        '<tr><td><kbd>Shift+K</kbd></td><td>Clear all rulers</td></tr>' +
        '<tr><td><kbd>Escape</kbd></td><td>Cancel ruler (when building)</td></tr>' +
        '</table>';
    container.element.appendChild(el);
}

function createSelectHighlight(container) {
    createStubPanel(container, 'Selection',
        'Selection and highlight browser.');
}

function createSchematicWidget(container) {
    new SchematicWidget(container, app);
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
                        type: 'stack',
                        height: 70,
                        content: [
                            {
                                type: 'component',
                                componentType: 'LayoutViewer',
                                title: 'Layout',
                                isClosable: false,
                            },
                            {
                                type: 'component',
                                componentType: 'SchematicWidget',
                                title: 'Schematic',
                            },
                        ],
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
app.goldenLayout.registerComponentFactoryFunction('SchematicWidget', createSchematicWidget);
app.goldenLayout.registerComponentFactoryFunction('HelpWidget', createHelpWidget);
app.goldenLayout.registerComponentFactoryFunction('SelectHighlight', createSelectHighlight);

// Layout version — bump this to force a layout reset when components change.
const LAYOUT_VERSION = 3;

// ─── WebSocket Init ─────────────────────────────────────────────────────────
// Must be created before loadLayout so that components (e.g. SchematicWidget)
// constructed during layout initialisation can access app.websocketManager.

const websocketUrl = `ws://${window.location.host || 'localhost:8080'}/ws`;
app.websocketManager = new WebSocketManager(websocketUrl, updateStatus);

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
    const menuBarHeight = document.getElementById('menu-bar').offsetHeight;
    app.goldenLayout.setSize(window.innerWidth, window.innerHeight - menuBarHeight);
});

// componentType → display title (must match defaultLayoutConfig).
const componentTitles = {
    LayoutViewer: 'Layout',
    DisplayControls: 'Display Controls',
    TclConsole: 'Tcl Console',
    Inspector: 'Inspector',
    Browser: 'Hierarchy',
    TimingWidget: 'Timing',
    DRCWidget: 'DRC',
    ClockWidget: 'Clock Tree',
    ChartsWidget: 'Charts',
    SchematicWidget: 'Schematic',
    HelpWidget: 'Help',
    SelectHighlight: 'Select Highlight',
};

// Focus a Golden Layout component tab, or re-create it if it was closed.
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
    if (item) {
        item.focus();
    } else {
        const title = componentTitles[componentType] || componentType;
        app.goldenLayout.addComponent(componentType, undefined, title);
    }
}

app.focusComponent = focusComponent;

app.toggleTheme = function() {
    const next = document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark';
    document.documentElement.dataset.theme = next;
    localStorage.setItem('theme', next);
    // Re-render canvas-based widgets that read theme colors.
    if (app.chartsWidget) app.chartsWidget.render();
    if (app.clockTreeWidget) app.clockTreeWidget.render();
};

// ─── Menu Bar ────────────────────────────────────────────────────────────────

createMenuBar(app);

// Handle server-push notifications (e.g. search indices ready)
app.websocketManager.onPush = (msg) => {
    if (msg.type === 'refresh') {
        document.getElementById('loading-overlay').style.display = 'none';
        redrawAllLayers();
    }
};

app.websocketManager.readyPromise.then(async () => {
    try {
        const [techData, boundsData, heatMapData] = await Promise.all([
            app.websocketManager.request({ type: 'tech' }),
            app.websocketManager.request({ type: 'bounds' }),
            app.websocketManager.request({ type: 'heatmaps' }),
        ]);
        app.hasLiberty = techData.has_liberty;
        app.techData = techData;

        // --- Set Bounds ---
        const designBounds = boundsData.bounds;

        const minY = designBounds[0][0];
        const minX = designBounds[0][1];
        const maxY = designBounds[1][0];
        const maxX = designBounds[1][1];

        const designWidth = maxX - minX;
        const designHeight = maxY - minY;

        // No design loaded — skip map setup, let user open a DB via menu.
        const hasDesign = designWidth > 0 && designHeight > 0;
        if (hasDesign) {
            const tileSize = 256;
            const maxDXDY = Math.max(designWidth, designHeight);
            const scale = tileSize / maxDXDY;
            app.designScale = scale;
            app.designMaxDXDY = maxDXDY;
            app.designOriginX = minX;
            app.designOriginY = minY;

            app.fitBounds = [
                [-maxDXDY * scale, 0],
                [(designHeight - maxDXDY) * scale, designWidth * scale]
            ];
            app.map.fitBounds(app.fitBounds);
        }

        // Click-to-select: convert click position to DBU and query server
        app.map.on('click', (e) => {
            if (!app.designScale) return;
            if (app.rulerManager && app.rulerManager.isActive()) return;
            const { dbuX: dbu_x, dbuY: dbu_y } = latLngToDbu(
                e.latlng.lat, e.latlng.lng, app.designScale, app.designMaxDXDY,
                app.designOriginX, app.designOriginY);

            const vf = {};
            for (const [k, v] of Object.entries(visibility)) {
                vf[k] = v ? 1 : 0;
            }
            app.websocketManager.request({ type: 'select', dbu_x, dbu_y, zoom: app.map.getZoom(), visible_layers: [...app.visibleLayers], ...vf })
                .then(data => {
                    console.log('Select response:', data, 'at dbu', dbu_x, dbu_y);
                    app.map.closePopup();
                    if (data.selected && data.selected.length > 0) {
                        const inst = data.selected[0];
                        if (inst.type === 'Inst') {
                            app.selectedInstanceName = inst.name;
                            if (app.schematicWidget) {
                                app.schematicWidget.refresh();
                            }
                        }
                        updateInspector(data);
                        focusComponent('Inspector');
                        // Highlight selected instance bbox
                        if (inst.bbox) {
                            highlightBBox(inst.bbox[0], inst.bbox[1],
                                          inst.bbox[2], inst.bbox[3]);
                        }
                    } else {
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

        // ─── Right-click rubber-band zoom ──────────────────────────────
        {
            const container = app.map.getContainer();
            let rbStart = null;   // {x, y} in client coords
            let rbDiv = null;     // overlay element

            container.addEventListener('contextmenu', (e) => {
                e.preventDefault();
            });

            container.addEventListener('mousedown', (e) => {
                if (e.button !== 2) return;
                rbStart = { x: e.clientX, y: e.clientY };
                app.map.dragging.disable();
            });

            window.addEventListener('mousemove', (e) => {
                if (!rbStart) return;
                const dx = e.clientX - rbStart.x;
                const dy = e.clientY - rbStart.y;
                if (!rbDiv && Math.abs(dx) >= 4 && Math.abs(dy) >= 4) {
                    rbDiv = document.createElement('div');
                    rbDiv.className = 'rubber-band';
                    document.body.appendChild(rbDiv);
                }
                if (rbDiv) {
                    const left = Math.min(rbStart.x, e.clientX);
                    const top = Math.min(rbStart.y, e.clientY);
                    rbDiv.style.left = left + 'px';
                    rbDiv.style.top = top + 'px';
                    rbDiv.style.width = Math.abs(dx) + 'px';
                    rbDiv.style.height = Math.abs(dy) + 'px';
                }
            });

            window.addEventListener('mouseup', (e) => {
                if (!rbStart) return;
                const wasShowing = !!rbDiv;
                if (rbDiv) {
                    rbDiv.remove();
                    rbDiv = null;
                }
                const start = rbStart;
                rbStart = null;
                app.map.dragging.enable();

                if (!wasShowing) return;

                // Convert the two screen corners to lat/lng and zoom
                const rect = container.getBoundingClientRect();
                const p1 = app.map.containerPointToLatLng([
                    start.x - rect.left, start.y - rect.top]);
                const p2 = app.map.containerPointToLatLng([
                    e.clientX - rect.left, e.clientY - rect.top]);
                app.map.fitBounds([
                    [Math.min(p1.lat, p2.lat), Math.min(p1.lng, p2.lng)],
                    [Math.max(p1.lat, p2.lat), Math.max(p1.lng, p2.lng)],
                ]);
            });
        }

        populateDisplayControls(app, visibility, WebSocketTileLayer,
                                techData, redrawAllLayers, HeatMapTileLayer);
        updateHeatMaps(heatMapData);

        // Only show the loading overlay if a design is loaded but shapes
        // aren't ready yet.  On browser reload (without server restart),
        // shapes are already built so we skip the overlay.
        if (hasDesign && !boundsData.shapes_ready) {
            document.getElementById('loading-overlay').style.display = 'flex';
        }
    } catch (err) {
        console.error('Failed to load initial data from server:', err);
    }
});

// ─── Keyboard Shortcuts ─────────────────────────────────────────────────────

document.addEventListener('keydown', (e) => {
    // Ignore shortcuts when typing in an input field
    const tag = e.target.tagName;
    if (tag === 'INPUT' || tag === 'TEXTAREA' || e.target.isContentEditable) return;

    const key = e.key.toLowerCase();
    if (key === 'escape' && app.rulerManager && app.rulerManager.isActive()) {
        app.rulerManager.cancelRulerBuild();
    } else if (key === 'k' && !e.shiftKey && !e.ctrlKey && !e.metaKey) {
        if (app.rulerManager) app.rulerManager.toggleRulerMode();
    } else if (key === 'k' && e.shiftKey && !e.ctrlKey && !e.metaKey) {
        if (app.rulerManager) app.rulerManager.clearAllRulers();
    } else if (key === 'f' && !e.ctrlKey && !e.metaKey && app.fitBounds) {
        app.map.fitBounds(app.fitBounds);
    } else if (key === 'z' && !e.shiftKey && !e.ctrlKey && app.map) {
        if (app.lastMouseLatLng) {
            app.map.setZoomAround(app.lastMouseLatLng, app.map.getZoom() + 1);
        } else {
            app.map.zoomIn();
        }
    } else if (key === 'z' && e.shiftKey && !e.ctrlKey && app.map) {
        if (app.lastMouseLatLng) {
            app.map.setZoomAround(app.lastMouseLatLng, app.map.getZoom() - 1);
        } else {
            app.map.zoomOut();
        }
    } else if (key === 't' && !e.shiftKey && !e.ctrlKey && !e.metaKey) {
        app.toggleTheme();
    }
}, true);
