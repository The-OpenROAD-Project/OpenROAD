// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { GoldenLayout, LayoutConfig } from 'https://esm.sh/golden-layout@2.6.0';
import { latLngToDbu } from './coordinates.js';
import { WebSocketManager } from './websocket-manager.js';
import { createWebSocketTileLayer, createOverlayTileLayer } from './websocket-tile-layer.js';
import { TimingWidget } from './timing-widget.js';
import { ClockTreeWidget } from './clock-tree-widget.js';
import { ChartsWidget } from './charts-widget.js';
import { HierarchyBrowser } from './hierarchy-browser.js';
import { createInspectorPanel } from './inspector.js';
import { isStaticMode } from './ui-utils.js';
import { populateDisplayControls } from './display-controls.js';
import { createMenuBar } from './menu-bar.js';
import { RulerManager } from './ruler.js';
import { SchematicWidget } from './schematic-widget.js';
import { DrcWidget } from './drc-widget.js';
import { TclCompleter } from './tcl-completer.js';
import { getCookie, setCookie, applyGLTheme } from './theme.js';
import { updateDocumentTitle } from './title.js';
import { ThreeDViewerWidget } from './3d-viewer-widget.js';

// ─── Status Indicator ───────────────────────────────────────────────────────

const statusDiv = document.getElementById('websocket-status');
let disconnectTimeout = null;
const DISCONNECT_DELAY_MS = 2000; // Show banner after 2 seconds of disconnection

function updateStatus() {
    const isConnected = app.websocketManager && app.websocketManager.isConnected;
    const pendingCount = app.websocketManager ? app.websocketManager.pendingCount : 0;
    
    if (!isConnected) {
        // After an intentional shutdown the "Server stopped" banner is
        // already showing — don't overwrite it with the generic message.
        if (app.websocketManager?._shutdown) {
            return;
        }
        // Only show banner after a delay to avoid flashing on page load
        if (!disconnectTimeout) {
            disconnectTimeout = setTimeout(() => {
                if (!app.websocketManager?.isConnected) {
                    statusDiv.innerHTML = '<div class="disconnected-banner">⚠ OpenROAD disconnected — retrying…</div>';
                    statusDiv.style.display = 'block';
                }
            }, DISCONNECT_DELAY_MS);
        }
    } else {
        // Connected - clear timeout and show pending indicator if needed
        if (disconnectTimeout) {
            clearTimeout(disconnectTimeout);
            disconnectTimeout = null;
        }
        
        if (pendingCount === 0) {
            statusDiv.style.display = 'none';
        } else {
            statusDiv.innerHTML = `<div class="pending-indicator">pending: ${pendingCount}</div>`;
            statusDiv.style.display = 'block';
            const color = pendingCount > 20 ? 'var(--error)' : 'var(--fg-bright)';
            statusDiv.querySelector('.pending-indicator').style.color = color;
        }
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
    // Raw tech-layer names (dbTechLayer::getName()) for the layers
    // currently visible.  Kept in sync with `visibleLayers` by
    // display-controls.js.  This is the wire-format that the backend
    // expects in `visible_layers`; `visibleLayers` itself holds the
    // hierarchical UI node IDs and must not leak into requests.
    visibleLayerNames: new Set(),
    // Set of chiplet `path`s currently visible.  Populated by
    // display-controls.js once techData.chiplets arrives; null means
    // "render every chiplet" (single-chip designs).
    visibleChiplets: null,
    useTrueZ: getCookie('or_use_true_z') === '1',
    showDbu: getCookie('or_show_dbu') === '1',
    selectableLayers: new Set(),
    heatMapData: null,
    activeHeatMap: '',
    heatMapLayer: null,
    heatMapLegendEl: null,
    renderHeatMapControls: null,
    rulerManager: null,
    getDbuPerMicron() {
        return this.techData?.dbu_per_micron || 1000;
    },
    // Format a DBU value as a display string, respecting the showDbu setting.
    // Mirrors Qt GUI's MainWindow::convertDBUToString.
    formatDbu(value, addUnits = false) {
        if (this.showDbu) return String(Math.round(value));
        const dbuPerUm = this.getDbuPerMicron();
        const precision = Math.ceil(Math.log10(dbuPerUm));
        const um = (value / dbuPerUm).toFixed(precision);
        return addUnits ? um + ' \u00b5m' : um;
    },
    // Format a distance (always positive) with auto-scaling units.
    formatDistance(dbuLength) {
        if (this.showDbu) return String(Math.round(dbuLength));
        const dbuPerUm = this.getDbuPerMicron();
        const um = dbuLength / dbuPerUm;
        if (um >= 1000) return (um / 1000).toFixed(3) + ' mm';
        if (um >= 1) return um.toFixed(3) + ' um';
        return (um * 1000).toFixed(1) + ' nm';
    },
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
    // Instance sub-shapes
    inst_names: true,
    inst_pins: true,
    inst_pin_names: true,
    // Shapes
    routing: true,
    routing_segments: true,
    routing_vias: true,
    special_nets: true,
    srouting_segments: true,
    srouting_vias: true,
    pins: true,
    pin_names: true,
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
    // Misc
    rulers: true,
    scale_bar: true,
    // Debug
    debug: false,
};

// Restore saved visibility state from a previous session.
try {
    const saved = getCookie('or_visibility');
    if (saved) {
        const parsed = JSON.parse(decodeURIComponent(saved));
        for (const [k, v] of Object.entries(parsed)) {
            visibility[k] = !!v;
        }
    }
} catch (_) {
    // Ignore malformed cookie.
}

// Selectability mirrors the Qt GUI's display-controls "selectable" column.
// Defaults to true (everything selectable), matching the Qt GUI.  Only
// categories that the Qt GUI exposes a selectable checkbox for are listed
// here; the server treats unspecified keys as selectable.
const selectability = {
    stdcells: true,
    macros: true,
    pad_input: true,
    pad_output: true,
    pad_inout: true,
    pad_power: true,
    pad_spacer: true,
    pad_areaio: true,
    pad_other: true,
    phys_fill: true,
    phys_endcap: true,
    phys_welltap: true,
    phys_tie: true,
    phys_antenna: true,
    phys_cover: true,
    phys_bump: true,
    phys_other: true,
    std_bufinv: true,
    std_bufinv_timing: true,
    std_clock_bufinv: true,
    std_clock_gate: true,
    std_level_shift: true,
    std_sequential: true,
    std_combinational: true,
    net_signal: true,
    net_power: true,
    net_ground: true,
    net_clock: true,
    net_reset: true,
    net_tieoff: true,
    net_scan: true,
    net_analog: true,
    pins: true,
    inst_pins: true,
    placement_blockages: true,
    routing_obstructions: true,
};

try {
    const saved = getCookie('or_selectability');
    if (saved) {
        const parsed = JSON.parse(decodeURIComponent(saved));
        for (const [k, v] of Object.entries(parsed)) {
            selectability[k] = !!v;
        }
    }
} catch (_) {
    // Ignore malformed cookie.
}

// `app` is forwarded so the tile layer can read app.visibleChiplets
// lazily on every request — the field is populated by display-controls
// once the server's tech metadata arrives.
const WebSocketTileLayer = createWebSocketTileLayer(
    visibility, app.visibleLayerNames, selectability, app.selectableLayers,
    app);
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

// Refresh only the highlight overlay layer (selection, hover, timing,
// DRC, route guides).  Much cheaper than redrawAllLayers because base
// geometry tiles are not re-rendered.
function refreshOverlay() {
    if (app.overlayLayer) {
        app.overlayLayer.refreshTiles();
    }
}
app.refreshOverlay = scheduleRefreshOverlay;

let _overlayRAF = null;
function scheduleRefreshOverlay() {
    if (_overlayRAF !== null) return;
    _overlayRAF = requestAnimationFrame(() => {
        _overlayRAF = null;
        refreshOverlay();
    });
}

function redrawAllLayers() {
    // Persist visibility and selectability state to cookies so they survive
    // page reloads.
    setCookie('or_visibility', encodeURIComponent(JSON.stringify(visibility)));
    setCookie('or_selectability',
              encodeURIComponent(JSON.stringify(selectability)));

    // Show/hide modules layer based on module_view visibility
    if (app.modulesLayer) {
        if (visibility.module_view && !app.map.hasLayer(app.modulesLayer)) {
            app.modulesLayer.addTo(app.map);
        } else if (!visibility.module_view && app.map.hasLayer(app.modulesLayer)) {
            app.map.removeLayer(app.modulesLayer);
        }
    }
    // Show/hide pin markers layer (controlled by Shapes > Pins)
    if (app.pinsLayer) {
        if (visibility.pins && !app.map.hasLayer(app.pinsLayer)) {
            app.pinsLayer.addTo(app.map);
        } else if (!visibility.pins && app.map.hasLayer(app.pinsLayer)) {
            app.map.removeLayer(app.pinsLayer);
        }
    }
    for (const layer of app.allLayers) {
        layer.refreshTiles();
    }
    if (app.heatMapLayer) {
        app.heatMapLayer.refreshTiles();
    }
    // Overlay layer must also refresh on structural changes (e.g. design
    // reload changes the coordinate space).
    refreshOverlay();
    // Update ruler and scale bar visibility.
    if (app.rulerManager) {
        app.rulerManager.updateVisibility();
    }
    if (app.updateScaleBar) {
        app.updateScaleBar();
    }
}

// Debounced wrapper: coalesces back-to-back server pushes (e.g.
// debug_refresh + debug_paused) into a single redrawAllLayers() call.
let _redrawRAF = null;
function scheduleRedrawAllLayers() {
    if (_redrawRAF !== null) return;
    _redrawRAF = requestAnimationFrame(() => {
        _redrawRAF = null;
        redrawAllLayers();
    });
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
        coordBar.textContent = `X: ${app.formatDbu(dbuX)}  Y: ${app.formatDbu(dbuY)}`;
    });
    app.map.on('mouseout', () => { app.lastMouseLatLng = null; });

    // Scale bar overlay (bottom-left, above coord bar).
    const scaleBar = document.createElement('div');
    scaleBar.id = 'scale-bar';
    mapDiv.appendChild(scaleBar);
    const scaleBarLine = document.createElement('div');
    scaleBarLine.className = 'scale-bar-line';
    scaleBar.appendChild(scaleBarLine);
    const scaleBarLabel = document.createElement('span');
    scaleBarLabel.className = 'scale-bar-label';
    scaleBar.appendChild(scaleBarLabel);

    // Round to the nearest 1/2/5 × 10^n value (e.g. 1, 2, 5, 10, 20, …).
    function niceRound(value) {
        const mag = Math.pow(10, Math.floor(Math.log10(value)));
        const residual = value / mag;
        if (residual < 1.5) return 1 * mag;
        if (residual < 3.5) return 2 * mag;
        if (residual < 7.5) return 5 * mag;
        return 10 * mag;
    }

    function updateScaleBar() {
        if (!app.designScale || !visibility.scale_bar) {
            scaleBar.style.display = 'none';
            return;
        }
        scaleBar.style.display = '';

        // Pixels per DBU at current zoom: designScale * 2^zoom.
        const zoom = app.map.getZoom();
        const pxPerDbu = app.designScale * Math.pow(2, zoom);

        // Target bar width: ~15% of the map container width.
        const containerWidth = app.map.getContainer().clientWidth || 400;
        const targetPx = containerWidth * 0.15;

        let barPx, label;
        if (app.showDbu) {
            const niceDbu = Math.max(1, niceRound(targetPx / pxPerDbu));
            barPx = Math.round(niceDbu * pxPerDbu);
            label = String(Math.round(niceDbu));
        } else {
            const dbuPerUm = app.techData?.dbu_per_micron || 1000;
            const pxPerUm = pxPerDbu * dbuPerUm;
            const niceUm = niceRound(targetPx / pxPerUm);

            barPx = Math.round(niceUm * pxPerUm);

            // Format with appropriate units.
            if (niceUm >= 1000) label = (niceUm / 1000) + ' mm';
            else if (niceUm >= 1) label = niceUm + ' \u00b5m';
            else if (niceUm >= 0.001) label = (niceUm * 1000) + ' nm';
            else label = (niceUm * 1e6) + ' pm';
        }

        scaleBarLine.style.width = barPx + 'px';
        scaleBarLabel.textContent = label;
    }
    app.map.on('zoomend moveend resize', updateScaleBar);
    app.updateScaleBar = updateScaleBar;

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

// Browser UX for `exit`/`quit` typed in the Tcl console. The browser
// override (web_serve.cpp tclExitHandler) sets exit_requested_, and
// Main.cc calls exit(EXIT_SUCCESS) once waitForStop() returns — so the
// whole OpenROAD process exits, not just the web session. (Compare
// `web_server -stop`, which only stops serving and arrives here as a
// broadcast `type: shutdown` handled below.)
// window.close() only succeeds when the tab was opened via JS (or via
// certain launcher integrations); when it fails we replace the page
// with a terminal overlay so the user knows OpenROAD exited and they
// can close the tab manually.
function handleServerShutdown() {
    // Idempotent: invoked from both the Tcl-eval response (`action: shutdown`)
    // and the broadcast push (`type: shutdown`); whichever arrives first wins.
    if (app._shutdownHandled) return;
    app._shutdownHandled = true;
    // Disable auto-reconnect and suppress the "disconnected" banner —
    // the disconnect is intentional.
    if (app.websocketManager) {
        app.websocketManager._shutdown = true;
        app.websocketManager.onPush = () => {};
    }
    const overlay = document.createElement('div');
    overlay.style.cssText =
        'position:fixed;inset:0;z-index:99999;background:#1e1e1e;color:#ddd;' +
        'display:flex;flex-direction:column;align-items:center;justify-content:center;' +
        'font-family:system-ui,sans-serif;font-size:16px;padding:24px;text-align:center;';
    overlay.innerHTML =
        '<div style="font-size:22px;margin-bottom:12px;">OpenROAD exited</div>' +
        '<div style="opacity:0.7;">You can close this tab.</div>';
    document.body.appendChild(overlay);
    // Hold the overlay visible long enough for the user to read it before
    // window.close() fires.  400 ms was below the perceptual threshold and
    // looked like the tab vanished instantly on `exit`.
    setTimeout(() => { try { window.close(); } catch (e) { /* ignore */ } }, 1500);
}

function createTclConsole(container) {
    const el = document.createElement('div');
    el.className = 'tcl-console';
    el.innerHTML =
        '<div class="tcl-output"></div>' +
        '<div class="tcl-input-row">' +
        '  <span class="tcl-prompt">%</span>' +
        '  <input class="tcl-input" type="text" placeholder="Enter Tcl command..." spellcheck="false" autocomplete="off" autocapitalize="none" autocorrect="off"/>' +
        '</div>';
    container.element.appendChild(el);

    app.tclOutputEl = el.querySelector('.tcl-output');

    if (isStaticMode(app)) {
        el.querySelector('.tcl-input-row').style.display = 'none';
        const notice = document.createElement('span');
        notice.className = 'tcl-static-notice';
        notice.setAttribute('role', 'status');
        notice.setAttribute('aria-live', 'polite');
        notice.textContent = 'Tcl console is not available in saved reports.';
        app.tclOutputEl.appendChild(notice);
        return;
    }

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
            // Log output produced while the command runs streams in
            // separately as {"type":"log",...} push messages (handled
            // below in the onPush dispatch).  The eval response only
            // carries the Tcl return value plus shutdown signaling.
            app.websocketManager.request({ type: 'tcl_eval', cmd })
                .then(data => {
                    if (data.result) {
                        tclAppend(data.result + '\n',
                                  data.is_error ? 'tcl-error' : '');
                    }
                    if (data.action === 'shutdown') {
                        handleServerShutdown();
                    }
                    if (!data.is_error && app.drcWidget) {
                        app.drcWidget.refresh();
                    }
                })
                .catch(err => tclAppend(`Error: ${err}\n`, 'tcl-error'));
        }
    });
}

// ─── Inspector Panel ────────────────────────────────────────────────────────

const inspector = createInspectorPanel(app, redrawAllLayers, scheduleRefreshOverlay);
const createInspector = inspector.createInspector;
const updateInspector = inspector.updateInspector;
const highlightBBox = inspector.highlightBBox;
const pulseHighlight = inspector.pulseHighlight;
app.updateInspector = updateInspector;
app.navigateInspector = inspector.navigateInspector;
app.refreshInspector = inspector.refreshInspector;

function createBrowser(container) {
    new HierarchyBrowser(container, app, redrawAllLayers);
}

function createTimingWidget(container) {
    app.timingWidget = new TimingWidget(app, redrawAllLayers, scheduleRefreshOverlay);
    container.element.appendChild(app.timingWidget.element);
}

function createDRCWidget(container) {
    app.drcWidget = new DrcWidget(app, redrawAllLayers, scheduleRefreshOverlay);
    container.element.appendChild(app.drcWidget.element);
}

function createClockWidget(container) {
    app.clockTreeWidget = new ClockTreeWidget(container, app, redrawAllLayers, scheduleRefreshOverlay);
}

function createChartsWidget(container) {
    app.chartsWidget = new ChartsWidget(app, redrawAllLayers);
    container.element.appendChild(app.chartsWidget.element);
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

function create3DViewerWidget(container) {
    app.threeDViewerWidget = new ThreeDViewerWidget(container, app);
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
                            {
                                type: 'component',
                                componentType: '3DViewer',
                                title: '3D Viewer',
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
app.goldenLayout.registerComponentFactoryFunction('3DViewer', create3DViewerWidget);
app.goldenLayout.registerComponentFactoryFunction('HelpWidget', createHelpWidget);
app.goldenLayout.registerComponentFactoryFunction('SelectHighlight', createSelectHighlight);

// Layout version — bump this to force a layout reset when components change.
const LAYOUT_VERSION = 3;

// ─── WebSocket Init ─────────────────────────────────────────────────────────
// Must be created before loadLayout so that components (e.g. SchematicWidget)
// constructed during layout initialisation can access app.websocketManager.

const staticCache = window.__STATIC_CACHE__ || null;
if (staticCache) {
    app.websocketManager = WebSocketManager.fromCache(staticCache, updateStatus);
} else {
    const websocketUrl = `ws://${window.location.host || 'localhost:8080'}/ws`;
    app.websocketManager = new WebSocketManager(websocketUrl, updateStatus);
}

// Check initial connection status
updateStatus();

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
    '3DViewer': '3D Viewer',
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
    applyGLTheme(next);
    setCookie('or_theme', next);
    // Also write to localStorage for standalone file:// reports.
    if (typeof localStorage !== 'undefined') {
        localStorage.setItem('or_theme', next);
    }
    // Re-render canvas-based widgets that read theme colors.
    if (app.chartsWidget) app.chartsWidget.render();
    if (app.clockTreeWidget) app.clockTreeWidget.render();
};

app.toggleShowDbu = function() {
    app.showDbu = !app.showDbu;
    setCookie('or_show_dbu', app.showDbu ? '1' : '0');
    // Re-render rulers so their labels update.
    if (app.rulerManager) app.rulerManager._rerenderAll();
    // Re-render hierarchy browser if present.
    if (app.hierarchyBrowser) app.hierarchyBrowser._render();
    // Update scale bar.
    if (app.updateScaleBar) app.updateScaleBar();
    // Re-request inspector properties with new formatting.
    if (app.refreshInspector) app.refreshInspector();
};

// ─── Menu Bar ────────────────────────────────────────────────────────────────

createMenuBar(app);

// Debug-graphics pause affordance: appended lazily when the first
// debug_paused push arrives.  Clicking "Continue" tells the server to
// release the placer thread.
function ensureDebugContinueButton() {
    let btn = document.getElementById('debug-continue-btn');
    if (btn) return btn;
    btn = document.createElement('button');
    btn.id = 'debug-continue-btn';
    btn.className = 'debug-continue-btn';
    btn.textContent = 'Continue';
    btn.title = 'Advance the debugger (gpl, cts, ...)';
    btn.addEventListener('click', () => {
        // Fire-and-forget; server's broadcast tells us when the placer
        // actually resumed.
        app.websocketManager.request({ type: 'debug_continue' })
            .catch(() => {});
    });
    document.body.appendChild(btn);
    return btn;
}

// Handle server-push notifications (e.g. search indices ready)
app.websocketManager.onPush = (msg) => {
    if (msg.type === 'refresh') {
        document.getElementById('loading-overlay').style.display = 'none';
        redrawAllLayers();
    } else if (msg.type === 'drcUpdated') {
        if (app._drcUpdateTimeout) {
            clearTimeout(app._drcUpdateTimeout);
        }
        app._drcUpdateTimeout = setTimeout(() => {
            if (app.drcWidget) {
                app.drcWidget.refresh();
            }
        }, 500);
    } else if (msg.type === 'debug_paused') {
        ensureDebugContinueButton().style.display = 'block';
        // Refetch tiles so the user sees the current paused state.
        // Use the debounced version so that a debug_refresh arriving
        // in the same event-loop turn is coalesced (avoids 2x tiles).
        scheduleRedrawAllLayers();
        // Fetch debug charts (e.g. GPL HPWL vs iteration).
        if (app.chartsWidget) {
            app.websocketManager.request({ type: 'debug_charts' })
                .then(data => app.chartsWidget.setDebugCharts(data.charts || []))
                .catch(() => {});
        }
    } else if (msg.type === 'debug_resumed') {
        const btn = document.getElementById('debug-continue-btn');
        if (btn) btn.style.display = 'none';
    } else if (msg.type === 'debug_refresh') {
        // Instance positions changed — clear the stale Leaflet highlight
        // outline (the tile-based highlight updates automatically).
        if (app.highlightRect) {
            app.map.removeLayer(app.highlightRect);
            app.highlightRect = null;
        }
        scheduleRedrawAllLayers();
    } else if (msg.type === 'log') {
        // Logger output from the main Tcl thread (e.g. global_placement).
        // The text already contains \n between lines from the batch; strip
        // any trailing newline to avoid a blank line at the end.
        let text = msg.text;
        if (text.endsWith('\n')) text = text.slice(0, -1);
        if (text) tclAppend(text + '\n', '');
    } else if (msg.type === 'shutdown') {
        // Server is stopping intentionally (web_server -stop).
        // Disable auto-reconnect and show a clear message. Note that
        // when the user typed `exit`/`quit` in the browser, the eval
        // response's `action: shutdown` already ran handleServerShutdown
        // (which set _shutdown and replaced onPush with a no-op), so
        // this branch only runs in the external-stop case.
        app.websocketManager._shutdown = true;
        statusDiv.innerHTML = '<div class="disconnected-banner">Server stopped</div>';
        statusDiv.style.display = 'block';
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
        updateDocumentTitle(techData.block_name);

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

            if (staticCache) {
                // Lock to the pre-rendered tile zoom level and fit.
                const cacheZoom = staticCache.zoom;
                app.map.setMinZoom(cacheZoom);
                app.map.setMaxZoom(cacheZoom);
                app.map.fitBounds(app.fitBounds);
                app.map.scrollWheelZoom.disable();
                app.map.touchZoom.disable();
                app.map.boxZoom.disable();
                app.map.doubleClickZoom.disable();

                // Path highlight overlay image.
                app.pathOverlay = L.imageOverlay('', app.fitBounds, {
                    opacity: 1, interactive: false, zIndex: 1000,
                });
                staticCache.setPathOverlay = (src) => {
                    if (src) {
                        app.pathOverlay.setUrl(src);
                        app.pathOverlay.addTo(app.map);
                    } else {
                        app.map.removeLayer(app.pathOverlay);
                    }
                };
            }
        }

        // Click-to-select: convert click position to DBU and query server
        if (staticCache) {
            // Hide loading overlay — shapes are always ready in static mode.
            document.getElementById('loading-overlay').style.display = 'none';
        }
        if (!staticCache) app.map.on('click', (e) => {
            if (!app.designScale) return;
            if (app.rulerManager && app.rulerManager.isActive()) return;
            const { dbuX: dbu_x, dbuY: dbu_y } = latLngToDbu(
                e.latlng.lat, e.latlng.lng, app.designScale, app.designMaxDXDY,
                app.designOriginX, app.designOriginY);

            const vf = {};
            for (const [k, v] of Object.entries(visibility)) {
                vf[k] = !!v;
            }
            // Selectability is sent with `s_` prefix to mirror the flat
            // visibility key scheme; the server parses both columns.
            for (const [k, v] of Object.entries(selectability)) {
                vf['s_' + k] = !!v;
            }
            const selectRequest = {
                type: 'select',
                dbu_x,
                dbu_y,
                zoom: Math.round(app.map.getZoom()),
                visible_layers: [...app.visibleLayerNames],
                selectable_layers: [...app.selectableLayers],
                use_dbu: app.showDbu,
                ...vf,
            };
            if (e.originalEvent && e.originalEvent.shiftKey) {
                selectRequest.add_to_selection = true;
            }
            if (app.visibleChiplets instanceof Set) {
                selectRequest.visible_chiplets = [...app.visibleChiplets];
            }
            app.websocketManager.request(selectRequest)
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
                            pulseHighlight(inst.bbox);
                        }
                    } else if (!selectRequest.add_to_selection) {
                        // Shift+click on empty space preserves the existing
                        // multi-selection on the server, so leave the
                        // inspector/navigation controls and highlight intact.
                        updateInspector(null);
                        if (app.highlightRect) {
                            app.map.removeLayer(app.highlightRect);
                            app.highlightRect = null;
                        }
                    }
                    refreshOverlay();
                })
                .catch(err => {
                    console.error('Select failed:', err);
                });
        });

        // ─── Right-click rubber-band zoom ──────────────────────────────
        if (!staticCache) {
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

        populateDisplayControls(app, visibility, selectability,
                                WebSocketTileLayer,
                                techData, redrawAllLayers, HeatMapTileLayer);

        // Create the highlight overlay layer — sits above all base/metal
        // layers but below the heatmap.  Only carries selection, hover,
        // timing, DRC, and route-guide shapes on a transparent background,
        // so base tiles stay cached when highlights change.
        // Skip in static mode: there is no WebSocket server to serve
        // overlay_tile requests.
        if (!app.overlayLayer && !staticCache) {
            const OverlayTileLayer = createOverlayTileLayer(visibility, app);
            app.overlayLayer = new OverlayTileLayer(app.websocketManager, {
                zIndex: app.allLayers.length + 5,
                opacity: 1,
            });
            app.overlayLayer.addTo(app.map);
        }

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
