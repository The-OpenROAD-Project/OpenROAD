// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { GoldenLayout, LayoutConfig } from 'https://esm.sh/golden-layout@2.6.0';
import { latLngToDbu, dbuToLatLng, dbuRectToBounds } from './coordinates.js';
import { WebSocketManager } from './websocket-manager.js';
import {
    createWebSocketTileLayer,
    createOverlayTileLayer,
    currentDpr,
    floorClampZoom,
} from './websocket-tile-layer.js';
import { createMergedTileLayer } from './merged-tile-layer.js';
import { installDeviceGridSnapping } from './device-pixels.js';
import {
    tileSizeCss, useStaticTileSize, withDeviceExactTileSize,
    watchDevicePixelRatio, tileSizeFields,
} from './tile-request.js';
import { TimingWidget } from './timing-widget.js';
import { ClockTreeWidget } from './clock-tree-widget.js';
import { ChartsWidget } from './charts-widget.js';
import { HierarchyBrowser } from './hierarchy-browser.js';
import { createInspectorPanel } from './inspector.js';
import { SelectionBrowser } from './selection-browser.js';
import { applySelectionFlags, beginSelection, boundsEqual, buildMapOptions,
         buildVisibilityFlags, computeBoundsTransforms, computeScaleBar,
         decorateTabIcons, formatDbu, formatDistance, isCurrentSelection,
         isStaticMode, maxUsefulZoom, parseDbu, rafCoalesce, showToast,
         unitLabel }
    from './ui-utils.js';
import { populateDisplayControls } from './display-controls.js';
import { createMenuBar } from './menu-bar.js';
import { createToolbar } from './toolbar.js';
import { showGlobalConnectDialog, showInsertBufferDialog } from './edit-dialogs.js';
import { RulerManager } from './ruler.js';
import { LabelManager } from './label-manager.js';
import { SchematicWidget } from './schematic-widget.js';
import { DrcWidget } from './drc-widget.js';
import { TclCompleter } from './tcl-completer.js';
import { getCookie, setCookie, applyGLTheme, persistTheme } from './theme.js';
import { serializeDisplayState, applyDisplayStateEntries } from './display-state.js';
import { updateDocumentTitle } from './title.js';
import { ThreeDViewerWidget } from './3d-viewer-widget.js';
import { ContextMenu } from './context-menu.js';
import { showFindDialog, showGotoDialog } from './search-nav.js';
import { captureLayout } from './capture.js';

// ─── Status Indicator ───────────────────────────────────────────────────────

const statusDiv = document.getElementById('websocket-status');
let disconnectTimeout = null;
const DISCONNECT_DELAY_MS = 2000; // Show banner after 2 seconds of disconnection
// Outstanding tile requests are normal while panning; this many at once means
// the server is not keeping up, which is worth a number rather than just the
// progress line.
const PENDING_BACKLOG = 20;

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
        if (disconnectTimeout) {
            clearTimeout(disconnectTimeout);
            disconnectTimeout = null;
        }
        // Ordinary tile traffic is the progress line's job.  The count only
        // earns space on screen once the queue is long enough to be the
        // explanation for a viewer that feels stuck.
        if (pendingCount > PENDING_BACKLOG) {
            statusDiv.innerHTML = '<div class="or-hud or-hud-top-right '
                + `pending-indicator">pending: ${pendingCount}</div>`;
            statusDiv.style.display = 'block';
        } else {
            statusDiv.style.display = 'none';
        }
    }
    setTileProgress(pendingCount);
}

// Show the tile-request line while requests are outstanding.  A no-op when the
// Layout panel is closed, which is the only place the line lives.
function setTileProgress(pendingCount) {
    const el = app.tileProgressEl;
    if (!el) return;
    el.classList.toggle('active', pendingCount > 0);
    // The count is worth keeping for anyone diagnosing a slow server, just not
    // worth a chip on screen.
    el.title = pendingCount > 0 ? `${pendingCount} tile requests pending` : '';
}

// ─── Component Factories ────────────────────────────────────────────────────

// Shared application state — replaces scattered module-level globals.
// Components receive this via closure now; when extracted to separate files
// they'll receive it as an explicit parameter.
const app = {
    map: null,
    fitBounds: null,
    lastSelectionBounds: null,  // Leaflet bounds of the last selected object
    selHasInst: false,          // selection contains any instance
    selHasNet: false,           // selection contains any net
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
    accessPointsLayer: null,
    regionsLayer: null,
    mfgGridLayer: null,
    gcellGridLayer: null,
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
    // Per-layer fill pattern, keyed by raw tech-layer name → int matching the
    // server's FillPattern enum (1 = solid). Populated/persisted by
    // display-controls.js and read lazily by websocket-tile-layer.js.
    layerPatterns: {},
    useTrueZ: getCookie('or_use_true_z') === '1',
    showDbu: getCookie('or_show_dbu') === '1',
    // Default style for NEW rulers (2.12): 'euclidian' | 'manhattan'.
    rulerStyle: getCookie('or_ruler_style') === 'manhattan'
        ? 'manhattan' : 'euclidian',
    labelManager: null,
    selectableLayers: new Set(),
    heatMapData: null,
    activeHeatMap: '',
    heatMapLayer: null,
    heatMapLegendEl: null,
    renderHeatMapControls: null,
    rulerManager: null,
    // Bumped by beginSelection() whenever a panel takes over the selection;
    // see ui-utils.js.  Panels register their reset callbacks in
    // `selectionResetters` via onSelectionReset().
    selectionToken: 0,
    selectionResetters: [],
    getDbuPerMicron() {
        return this.techData?.dbu_per_micron || 1000;
    },
    // The display-unit state the ui-utils formatters below are driven by.
    unitOpts() {
        return { showDbu: this.showDbu, dbuPerMicron: this.getDbuPerMicron() };
    },
    formatDbu(value, addUnits = false) {
        return formatDbu(value, this.unitOpts(), addUnits);
    },
    parseDbu(str) {
        return parseDbu(str, this.unitOpts());
    },
    formatDistance(dbuLength) {
        return formatDistance(dbuLength, this.unitOpts());
    },
    unitLabel() {
        return unitLabel(this.unitOpts());
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
    // Access points (dbAccessPoint X markers) — off by default, matching GUI
    access_points: false,
    // Region (dbRegion) boundaries — on by default, matching GUI
    regions: true,
    // Manufacturing-grid dots — off by default, matching GUI
    mfg_grid: false,
    // GCell grid lines — off by default, matching GUI
    gcell_grid: false,
    // Flywires only (selected nets as straight driver->sink lines) —
    // off by default, matching GUI
    flywires_only: false,
    blockages: true,
    // Blockages
    placement_blockages: true,
    routing_obstructions: true,
    // Metal fill (dbFill) — off by default, matching GUI
    fills: false,
    // Rows (off by default, matching GUI)
    rows: false,
    // Tracks (off by default, matching GUI)
    tracks_pref: false,
    tracks_non_pref: false,
    // Module view
    module_view: false,
    // Misc
    detailed: false,
    rulers: true,
    labels: true,
    scale_bar: true,
    // Route guides of focused nets — off by default, matching GUI
    focused_nets_guides: false,
    // Highlight of the current selection — on by default, matching GUI
    highlight_selected: true,
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

// Expose the live visibility/selectability so the context menu "Save" can
// serialize the same payload the tile requests use (visibility-aware export).
app.visibility = visibility;
app.selectability = selectability;

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

// ─── Tile grouping ──────────────────────────────────────────────────────────
//
// One pane per tech layer per chiplet puts a multi-die design at ~97 panes and
// ~582 MB of decoded tile images, past the browser's ~458 MB ceiling, where
// Chrome discards decodes and the discarded regions paint white until something
// forces a full invalidation.  Merging the routing layers into N canvas panes,
// N derived from a memory budget, bounds the total regardless of how many layers
// or chiplets the design has.
//
//   ?mergetiles=0        legacy one-pane-per-layer, for A/B comparison
//   ?tilebudget=<MB>     override the budget (default 350 MB)
//   ?mergegroups=<N>     pin N directly, bypassing the budget
(function configureTileMerging() {
    let params = null;
    try {
        params = new URLSearchParams(window.location.search);
    } catch (_) {
        params = null;
    }
    const param = (name) => (params ? params.get(name) : null);

    app.mergeTiles = param('mergetiles') !== '0';
    const budgetMB = Number(param('tilebudget'));
    if (Number.isFinite(budgetMB) && budgetMB > 0) {
        app.tileBudgetBytes = Math.round(budgetMB * 1024 * 1024);
    }
    const groups = Number(param('mergegroups'));
    if (Number.isFinite(groups) && groups > 0) {
        app.mergeGroupCount = Math.floor(groups);
    }
    // display-controls reads dpr through app so it does not have to import a
    // layer module just to size the memory budget.
    app.tileDpr = currentDpr;
    app.MergedTileLayer = createMergedTileLayer({
        visibility,
        selectability,
        visibleLayers: app.visibleLayerNames,
        selectableLayers: app.selectableLayers,
        app,
    }, { dpr: currentDpr });
})();
const BLANK_TILE
    = 'data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw==';

const HeatMapTileLayer = L.GridLayer.extend({
    initialize: function(websocketManager, appState, options) {
        this._websocketManager = websocketManager;
        this._appState = appState;
        // Same grid as the layer tiles it is drawn over.
        L.GridLayer.prototype.initialize.call(
            this, withDeviceExactTileSize(options));
    },

    // Upscale-only display, same as the layout tile layer: the map rests on
    // integer zoom so heatmap tiles show 1:1 with no fractional rescaling.
    _clampZoom: function(zoom) {
        return L.GridLayer.prototype._clampZoom.call(
            this, floorClampZoom(this, zoom));
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
            // Sized like the layer tiles beneath it; without this the heat map
            // is a 256 px image stretched over crisp layers on any HiDPI
            // display.
            ...tileSizeFields(currentDpr(), this.getTileSize().x),
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
                ...tileSizeFields(currentDpr(), this.getTileSize().x),
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
// Every selection/highlight mutation refreshes the overlay, so this single
// hook keeps the selection browser in sync (it debounces and skips fetches
// while hidden).
const scheduleRefreshOverlay = rafCoalesce(() => {
    refreshOverlay();
    if (app.selectionBrowser) app.selectionBrowser.scheduleRefresh();
});
app.refreshOverlay = scheduleRefreshOverlay;

function redrawAllLayers() {
    // Persist visibility and selectability state to cookies so they survive
    // page reloads.
    setCookie('or_visibility', encodeURIComponent(JSON.stringify(visibility)));
    setCookie('or_selectability',
              encodeURIComponent(JSON.stringify(selectability)));
    // Keep the server's saved-state snapshot current for save_display_controls.
    scheduleSyncDisplayState();

    // Show/hide the toggleable pseudo-layer tile layers.
    const toggleableLayers = [
        [app.modulesLayer, visibility.module_view],   // Module view
        [app.pinsLayer, visibility.pins],             // Shapes > Pins
        [app.accessPointsLayer, visibility.access_points],
        [app.regionsLayer, visibility.regions],
        [app.mfgGridLayer, visibility.mfg_grid],
        [app.gcellGridLayer, visibility.gcell_grid],
    ];
    for (const [layer, visible] of toggleableLayers) {
        if (!layer) continue;
        if (visible && !app.map.hasLayer(layer)) {
            layer.addTo(app.map);
        } else if (!visible && app.map.hasLayer(layer)) {
            app.map.removeLayer(layer);
        }
    }
    for (const layer of app.allLayers) {
        layer.refreshTiles();
    }
    if (app.heatMapLayer) {
        app.heatMapLayer.refreshTiles();
    }
    // Keep the client-side selection outline in sync with the "Highlight
    // selected" toggle: detach it when off, re-attach it when back on (the
    // rectangle object is preserved so the current selection survives an
    // off→on round trip; the server-side highlight is gated by the overlay
    // refresh below).  An in-flight selection pulse is cancelled on off.
    if (app.highlightRect) {
        const showHighlight = visibility.highlight_selected !== false;
        if (!showHighlight && app.map.hasLayer(app.highlightRect)) {
            app.map.removeLayer(app.highlightRect);
        } else if (showHighlight && !app.map.hasLayer(app.highlightRect)) {
            app.highlightRect.addTo(app.map);
        }
    }
    if (!visibility.highlight_selected && app.clearSelectionPulse) {
        app.clearSelectionPulse();
    }
    // Overlay layer must also refresh on structural changes (e.g. design
    // reload changes the coordinate space).
    refreshOverlay();
    // Update ruler and scale bar visibility.
    if (app.rulerManager) {
        app.rulerManager.updateVisibility();
    }
    if (app.labelManager) {
        app.labelManager.updateVisibility();
    }
    if (app.updateScaleBar) {
        app.updateScaleBar();
    }
}

// Debounced wrapper: coalesces back-to-back server pushes (e.g.
// debug_refresh + debug_paused) into a single redrawAllLayers() call.
const scheduleRedrawAllLayers = rafCoalesce(redrawAllLayers);

// Push the current display state to the server (coalesced via rAF) so the
// Tcl save_display_controls command has an up-to-date snapshot to write.
const scheduleSyncDisplayState = rafCoalesce(() => {
    if (!app.websocketManager || isStaticMode(app)) return;
    app.websocketManager.request({
        type: 'set_display_state',
        state: serializeDisplayState(),
    }).catch(() => { /* server offline / not ready — ignore */ });
});
app.syncDisplayState = scheduleSyncDisplayState;

// Apply a saved display state (from restore_display_controls) by writing the
// entries back and reloading, so the well-tested init path rebuilds the panel
// consistently.  The current camera is preserved across the reload, and
// sessionStorage survives it — which is why the split store works here.
function applyDisplayState(state) {
    if (!state || typeof state !== 'object') return;
    const entries = state.entries;
    if (!entries || typeof entries !== 'object') return;
    applyDisplayStateEntries(entries);
    // Preserve the current view so restoring controls doesn't move the map.
    try {
        if (app.map && typeof sessionStorage !== 'undefined') {
            const c = app.map.getCenter();
            sessionStorage.setItem('or_restore_view',
                JSON.stringify({ lat: c.lat, lng: c.lng,
                                 zoom: app.map.getZoom() }));
        }
    } catch (_) { /* ignore */ }
    window.location.reload();
}

// Expose so the context menu can refresh base + overlay tiles after a
// server-side context_action (e.g. Select → Connected).
app.redrawAllLayers = redrawAllLayers;

// Expose the WYSIWYG capture so the context menu "Save" can grab the scene.
app.captureLayout = (opts = {}) => captureLayout(app, opts);

function createLayoutViewer(container) {
    const mapDiv = document.createElement('div');
    mapDiv.className = 'layout-viewer';
    mapDiv.style.width = '100%';
    mapDiv.style.height = '100%';
    mapDiv.style.backgroundColor = 'var(--bg-map)';
    container.element.appendChild(mapDiv);

    const heatMapLegend = document.createElement('div');
    heatMapLegend.className = 'or-hud or-hud-bottom-right heatmap-map-legend hidden';
    mapDiv.appendChild(heatMapLegend);
    app.heatMapLegendEl = heatMapLegend;

    // Tile requests in flight, as a line along the top of the canvas.  Held on
    // app because updateStatus runs from the socket callbacks, which know
    // nothing about this panel -- and the panel can be closed, so every write
    // to it goes through setTileProgress below.
    const progress = document.createElement('div');
    progress.className = 'or-progress';
    mapDiv.appendChild(progress);
    app.tileProgressEl = progress;

    // Closing the panel discards this DOM, so drop the references to it.  Both
    // readers null-check (setTileProgress here, updateHeatMapLegend in
    // display-controls.js), and createLayoutViewer sets them again if the panel
    // is reopened.
    //
    // app.map is deliberately left alone.  Calling map.remove() and nulling it
    // would be the matching cleanup, but ~140 call sites across eleven modules
    // reach app.map, many of them unguarded -- the View menu's zoom items, the
    // inspector's zoom-to, the rulers, the display controls -- so nulling it
    // turns a detached-but-harmless map into a dozen ways to throw while the
    // panel is closed.  Guarding those is a lifecycle change, not a styling
    // one.
    container.on('destroy', () => {
        app.tileProgressEl = null;
        app.heatMapLegendEl = null;
    });

    app.map = L.map(mapDiv, buildMapOptions());
    // On a fractional dpr, Leaflet's whole-CSS-pixel placement leaves tile
    // boundaries mid-device-pixel and they show as dark hairlines; this nudges
    // each tile container back onto the grid after every move.
    installDeviceGridSnapping(app.map);
    // Tiles are rasterized for the ratio in force when they were requested and
    // are never revisited on their own, so a window moved to another monitor —
    // or a browser zoom change — leaves every tile stretched from the old ratio
    // into the new box until something forces a refresh. Nothing did.
    watchDevicePixelRatio(() => redrawAllLayers());
    const hoverPane = app.map.createPane(app.hoverHighlightPane);
    hoverPane.style.zIndex = '650';
    hoverPane.style.pointerEvents = 'none';

    // "Fit" button below the default zoom (+/-) control, top-left.
    const FitControl = L.Control.extend({
        onAdd: function() {
            const c = L.DomUtil.create(
                'div', 'leaflet-bar leaflet-control leaflet-control-fit');
            const a = L.DomUtil.create('a', '', c);
            a.href = '#';
            a.title = 'Fit';
            a.setAttribute('role', 'button');
            a.setAttribute('aria-label', 'Fit');
            a.textContent = '⤢';
            L.DomEvent.on(a, 'click', L.DomEvent.stop)
                .on(a, 'click', () => {
                    if (app.fitBounds) app.map.fitBounds(app.fitBounds);
                });
            return c;
        },
    });
    new FitControl({ position: 'topleft' }).addTo(app.map);

    new ResizeObserver(() => {
        app.map.invalidateSize({ animate: false });
    }).observe(mapDiv);

    // Scale bar and coordinate readout share one HUD at the bottom-left of
    // the viewer.  The scale bar is a display option and hides on its own, so
    // the divider beside it goes when it does, and the HUD itself disappears
    // when neither readout has anything to show -- otherwise an empty chip
    // sits on the layout before the pointer has ever entered it.
    const hud = document.createElement('div');
    hud.className = 'or-hud or-hud-bottom-left hidden';
    const scaleBar = document.createElement('div');
    scaleBar.id = 'scale-bar';
    const hudDivider = document.createElement('span');
    hudDivider.className = 'or-hud-divider';
    const coordBar = document.createElement('div');
    coordBar.id = 'coord-bar';
    hud.appendChild(scaleBar);
    hud.appendChild(hudDivider);
    hud.appendChild(coordBar);
    mapDiv.appendChild(hud);

    function syncHud() {
        // Content, not just the display property: the scale bar starts out
        // display:'' but empty -- updateScaleBar has not run yet -- and an
        // empty div would otherwise count as a visible readout.
        const scale = scaleBar.style.display !== 'none'
            && scaleBar.childElementCount > 0;
        const coord = coordBar.textContent !== '';
        hudDivider.style.display = (scale && coord) ? '' : 'none';
        hud.classList.toggle('hidden', !scale && !coord);
    }
    syncHud();

    app.map.on('mousemove', (e) => {
        app.lastMouseLatLng = e.latlng;
        if (!app.designScale) return;
        const { dbuX, dbuY } = latLngToDbu(
            e.latlng.lat, e.latlng.lng, app.designScale, app.designMaxDXDY,
            app.designOriginX, app.designOriginY);
        coordBar.textContent = `X: ${app.formatDbu(dbuX)}  Y: ${app.formatDbu(dbuY)}`;
        syncHud();
    });
    app.map.on('mouseout', (e) => {
        app.lastMouseLatLng = null;
        // Leaflet raises mouseout for a move onto anything inside the
        // container too -- a zoom control, a marker, the HUD itself -- and the
        // readout should hold its value for those.  Only a move that actually
        // leaves the viewer clears it.
        const to = e.originalEvent && e.originalEvent.relatedTarget;
        if (to && mapDiv.contains(to)) {
            return;
        }
        coordBar.textContent = '';
        syncHud();
    });

    const SB_H = 22;       // svg height
    const SB_BAR_H = 8;    // bracket height
    const SB_PAD = 22;     // horizontal room for end labels overflowing the bar

    function renderScaleBar(barPx, label, segments) {
        const top = 2;
        const base = top + SB_BAR_H;
        const x0 = SB_PAD;
        const x1 = SB_PAD + barPx;
        const labelY = SB_H - 2;
        const wide = barPx >= 40;  // hide "0"/ticks on very short bars
        const parts = [];
        // Bracket: |_| (left/right verticals + baseline, open top).
        parts.push(`<path d="M${x0} ${top} V${base} H${x1} V${top}" `
            + `fill="none" stroke="currentColor" stroke-width="2"/>`);
        // Interior ticks (half height), only when the bar is wide enough.
        if (wide && segments > 1) {
            for (let i = 1; i < segments; i++) {
                const tx = x0 + (barPx * i) / segments;
                parts.push(`<line x1="${tx}" y1="${base - SB_BAR_H / 2}" `
                    + `x2="${tx}" y2="${base}" stroke="currentColor" stroke-width="1"/>`);
            }
        }
        // Labels: "0" at the left end, the total at the right end.
        if (wide) {
            parts.push(`<text x="${x0}" y="${labelY}" text-anchor="middle" `
                + `class="scale-bar-text">0</text>`);
        }
        parts.push(`<text x="${x1}" y="${labelY}" text-anchor="middle" `
            + `class="scale-bar-text">${label}</text>`);
        scaleBar.innerHTML =
            `<svg width="${x1 + SB_PAD}" height="${SB_H}">${parts.join('')}</svg>`;
    }

    function updateScaleBar() {
        if (!app.designScale || !visibility.scale_bar) {
            scaleBar.style.display = 'none';
            syncHud();
            return;
        }
        // Pixels per DBU at current zoom: designScale * 2^zoom.
        const pxPerDbu = app.designScale * Math.pow(2, app.map.getZoom());
        // Target bar width: ~15% of the map container width.
        const containerWidth = app.map.getContainer().clientWidth || 400;
        const sb = computeScaleBar({
            targetPx: containerWidth * 0.15,
            pxPerDbu,
            dbuPerMicron: app.techData?.dbu_per_micron,
            showDbu: app.showDbu,
        });
        if (!sb) {
            scaleBar.style.display = 'none';
            syncHud();
            return;
        }
        scaleBar.style.display = '';
        renderScaleBar(sb.barPx, sb.label, sb.segments);
        syncHud();
    }

    // Coalesce updates during continuous zoom gestures so the bar tracks
    // the animation instead of only snapping at gesture end.  Pan doesn't
    // change the bar (geometry depends only on zoom and container width),
    // so 'move'/'moveend' are deliberately not listened to.
    const scheduleUpdateScaleBar = rafCoalesce(updateScaleBar);
    app.map.on('zoom zoomend resize', scheduleUpdateScaleBar);
    app.updateScaleBar = updateScaleBar;

    app.rulerManager = new RulerManager(app, visibility, updateInspector, focusComponent);
    app.labelManager = new LabelManager(app, visibility, updateInspector, focusComponent);
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

// Expose visibility so the inspector can honor the "Highlight selected"
// toggle when drawing its client-side selection outline/pulse.
app.visibility = visibility;
const inspector = createInspectorPanel(app, redrawAllLayers, scheduleRefreshOverlay);
const createInspector = inspector.createInspector;
const updateInspector = inspector.updateInspector;
const highlightBBox = inspector.highlightBBox;
const pulseHighlight = inspector.pulseHighlight;
app.updateInspector = updateInspector;
app.navigateInspector = inspector.navigateInspector;
app.refreshInspector = inspector.refreshInspector;
app.animateSelection = inspector.animateSelection;
app.stopSelectionAnimation = inspector.stopSelectionAnimation;

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
        '<tr><td><kbd>Shift+click</kbd></td><td>Add object to selection</td></tr>' +
        '<tr><td><kbd>Ctrl/Cmd+click</kbd></td><td>Select object and its connected nets</td></tr>' +
        '<tr><td><kbd>k</kbd></td><td>Toggle ruler mode</td></tr>' +
        '<tr><td><kbd>Shift+K</kbd></td><td>Clear all rulers</td></tr>' +
        '<tr><td><kbd>Escape</kbd></td><td>Cancel ruler (when building)</td></tr>' +
        '</table>';
    container.element.appendChild(el);
}

function createSelectHighlight(container) {
    app.selectionBrowser
        = new SelectionBrowser(container, app, scheduleRefreshOverlay);
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

const kHeaderHeight = 28;

const defaultLayoutConfig = {
    dimensions: { headerHeight: kHeaderHeight },
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
                        componentType: 'SelectHighlight',
                        title: 'Select Highlight',
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
// v4: SelectHighlight (selection browser) added to the default layout.
const LAYOUT_VERSION = 4;

// ─── WebSocket Init ─────────────────────────────────────────────────────────
// Must be created before loadLayout so that components (e.g. SchematicWidget)
// constructed during layout initialisation can access app.websocketManager.

const staticCache = window.__STATIC_CACHE__ || null;
if (staticCache) {
    // Before any layer or the map scale is built: a report's tiles are baked at
    // a fixed size and cannot be re-rendered to fit a different box.
    useStaticTileSize();
    app.websocketManager = WebSocketManager.fromCache(staticCache, updateStatus);
} else {
    const websocketUrl = `ws://${window.location.host || 'localhost:8080'}/ws`;
    app.websocketManager = new WebSocketManager(websocketUrl, updateStatus);
    // On reconnect the server may have been restarted (possibly with a
    // different design) — resync the coordinate transforms; a bounds
    // change here reloads through the boot path.
    app.websocketManager.onReconnected = () => {
        resyncBounds(null, { reloadOnChange: true }).catch(() => {});
    };
}

// Check initial connection status
updateStatus();

// Restore saved layout or use default
const savedLayout = localStorage.getItem('gl-layout');
const savedVersion = parseInt(localStorage.getItem('gl-layout-version'), 10);
if (savedLayout && savedVersion === LAYOUT_VERSION) {
    try {
        const resolved = JSON.parse(savedLayout);
        const restored = LayoutConfig.fromResolved(resolved);
        restored.dimensions = {
            ...restored.dimensions, headerHeight: kHeaderHeight,
        };
        app.goldenLayout.loadLayout(restored);
    } catch (e) {
        app.goldenLayout.loadLayout(defaultLayoutConfig);
    }
} else {
    app.goldenLayout.loadLayout(defaultLayoutConfig);
}
localStorage.setItem('gl-layout-version', LAYOUT_VERSION);
addTabIcons();

// Add the per-panel tab icons and, if any were added, make Golden Layout
// measure the headers again -- it chose which tabs fit before the icons
// widened them, so without this the tabs that no longer fit overlap the
// stack's controls instead of moving into the overflow dropdown.
function addTabIcons() {
    if (decorateTabIcons() > 0) {
        app.goldenLayout.updateSize();
    }
}

// Persist layout on changes (drag, resize, close, etc.)
app.goldenLayout.on('stateChanged', () => {
    localStorage.setItem('gl-layout', JSON.stringify(app.goldenLayout.saveLayout()));
    addTabIcons();
});

// Resize GoldenLayout to the space #gl-container actually has.  The container
// is flex-sized (see style.css), so its height also moves when the toolbar
// appears or collapses -- and GoldenLayout does not track its container on its
// own, so every cause has to come through here.  Measuring the element beats
// subtracting the chrome's height from window.innerHeight: that arithmetic has
// to be kept in step with the chrome, and missing the toolbar pushed the
// bottom of the panels (the Tcl console's prompt) off screen.
function syncLayoutSize() {
    const el = document.getElementById('gl-container');
    app.goldenLayout.setSize(el.clientWidth, el.clientHeight);
}
app.syncLayoutSize = syncLayoutSize;

window.addEventListener('resize', syncLayoutSize);

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
    persistTheme(next);
    // Re-render canvas-based widgets that read theme colors.
    if (app.chartsWidget) app.chartsWidget.render();
    if (app.clockTreeWidget) app.clockTreeWidget.render();
    scheduleSyncDisplayState();
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
    scheduleSyncDisplayState();
};

// Toggle the default style used for NEW rulers (2.12); existing rulers keep
// their per-ruler "Euclidian" flag.  Persisted like other preferences.
app.toggleRulerStyle = function() {
    app.rulerStyle = app.rulerStyle === 'manhattan' ? 'euclidian' : 'manhattan';
    setCookie('or_ruler_style', app.rulerStyle);
};

// ─── Menu Bar & Toolbar ──────────────────────────────────────────────────────

// Shared entry point for running a Tcl script from a custom menu item or
// toolbar button: optionally echo the command to the console (like the Qt
// GUI's -echo), run it, and surface the result/errors in the console.
app.echoTcl = (cmd) => tclAppend(`>>> ${cmd}\n`, 'tcl-cmd');
app.runTclScript = (script, echo) => {
    if (!script) return Promise.resolve();
    if (echo) app.echoTcl(script);
    return app.websocketManager.request({ type: 'tcl_eval', cmd: script })
        .then(data => {
            if (data && data.result) {
                tclAppend(data.result + '\n',
                          data.is_error ? 'tcl-error' : '');
            }
            return data;
        })
        .catch(err => { tclAppend(`Error: ${err}\n`, 'tcl-error'); });
};

// Apply a custom-UI registry snapshot (from the custom_ui request on connect
// or a live server push) and re-render the menu bar and toolbar.
function applyCustomUi(data) {
    if (!data) return;
    app.customMenu = data.menu || [];
    app.customToolbar = data.toolbar || [];
    if (app.rebuildMenuBar) app.rebuildMenuBar();
    if (app.rebuildToolbar) app.rebuildToolbar();
    // The toolbar may have just appeared or collapsed, changing how much room
    // is left for the panels.
    syncLayoutSize();
}

app.customMenu = [];
app.customToolbar = [];

// Editing-utility dialogs (Global Connect / Insert Buffer). Exposed so the
// Tools menu and the inspector's per-net action button can open them; both
// need scheduleRedrawAllLayers to refresh the layout after a DB edit.
app.scheduleRedrawAllLayers = scheduleRedrawAllLayers;
app.showGlobalConnectDialog = () => showGlobalConnectDialog(app);
app.showInsertBufferDialog = (netName) => showInsertBufferDialog(app, netName);

createMenuBar(app);
createToolbar(app);

// Canvas right-click context menu ("Select →" connected objects).
app.contextMenu = new ContextMenu(app);

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

// ─── Bounds / coordinate-transform sync ─────────────────────────────────────
// The server's getBounds() is dynamic: DB edits (moving an instance
// outside the block bbox, deleting an edge instance) change the tile
// georeference.  The transforms below are applied at boot and re-synced
// whenever the server reports different bounds — otherwise every later
// click and highlight lands offset from the re-rendered tiles.

function applyBounds(designBounds) {
    // The map's whole coordinate system is defined in units of one tile, so
    // this must be the size the layers actually use.
    const t = computeBoundsTransforms(designBounds, tileSizeCss());
    if (!t) return false;
    app.currentBounds = designBounds;
    app.designScale = t.scale;
    app.designMaxDXDY = t.maxDXDY;
    app.designOriginX = t.originX;
    app.designOriginY = t.originY;
    app.fitBounds = t.fitBounds;
    // Bound the zoom before the first fit: the tile layers are L.GridLayer,
    // which contributes no maxZoom, so the map would otherwise let the user
    // zoom until the tile math degenerates.  Set per design, since the cap
    // depends on this design's scale.
    app.map.setMaxZoom(maxUsefulZoom(t.scale));
    return true;
}

async function resyncBounds(inlineBounds, { reloadOnChange = false } = {}) {
    // Returns true when it already redrew the layers (bounds changed), so
    // callers can avoid a second redundant redraw.
    if (isStaticMode(app) || !app.map) return false;
    let designBounds = inlineBounds;
    if (!designBounds) {
        try {
            const data = await app.websocketManager.request({ type: 'bounds' });
            designBounds = data.bounds;
        } catch (err) {
            return false;
        }
    }
    if (!designBounds || boundsEqual(app.currentBounds, designBounds)) {
        return false;
    }

    if (reloadOnChange) {
        // After a reconnect, different bounds usually mean a different
        // design — rebuild everything through the well-tested boot path.
        window.location.reload();
        return true;
    }

    // Keep the DBU point at the center of the view where it is.
    const hadDesign = !!app.designScale;
    const center = hadDesign
        ? latLngToDbu(app.map.getCenter().lat, app.map.getCenter().lng,
                      app.designScale, app.designMaxDXDY,
                      app.designOriginX, app.designOriginY)
        : null;
    if (!applyBounds(designBounds)) return false;
    if (center) {
        const ll = dbuToLatLng(center.dbuX, center.dbuY, app.designScale,
                               app.designMaxDXDY, app.designOriginX,
                               app.designOriginY);
        app.map.setView(ll, app.map.getZoom(), { animate: false });
    }
    // Client-side rectangles hold latlngs from the old transforms.
    if (app.highlightRect) {
        app.map.removeLayer(app.highlightRect);
        app.highlightRect = null;
    }
    redrawAllLayers();
    return true;
}

// Handle server-push notifications (e.g. search indices ready)
app.websocketManager.onPush = (msg) => {
    if (msg.type === 'refresh') {
        document.getElementById('loading-overlay').style.display = 'none';
        // An edit may have changed the design bounds (and with them the
        // tile georeference); resync transforms before/along the redraw.
        // resyncBounds already redraws when the bounds changed, so only
        // redraw here when it didn't (same bounds, edited geometry).
        resyncBounds(msg.bounds)
            .then((redrew) => { if (!redrew) redrawAllLayers(); })
            .catch(() => redrawAllLayers());
        // The design may have been edited by another session's
        // set_property; refresh the inspected object's properties.
        if (app.refreshInspector) app.refreshInspector();
    } else if (msg.type === 'selection_invalidated') {
        // A design object was destroyed (trigger_action or Tcl); the
        // server dropped this session's selection state.  Clear the
        // inspector and stale highlights.
        if (app.updateInspector) app.updateInspector(null);
        if (app.stopSelectionAnimation) app.stopSelectionAnimation();
        if (app.highlightRect && app.map) {
            app.map.removeLayer(app.highlightRect);
            app.highlightRect = null;
        }
        scheduleRefreshOverlay();
    } else if (msg.type === 'labels_changed') {
        // Labels live server-side and are shared, so another client's edit
        // (or a Tcl add_label) changes what this one should be drawing.
        // The push carries the new set, so adopt it without a round-trip.
        if (app.labelManager) {
            app.labelManager.applyRemoteLabels(msg.labels);
        }
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
    } else if (msg.type === 'restore_display_state') {
        // restore_display_controls (Tcl) broadcast a saved state — apply it.
        applyDisplayState(msg.state);
    } else if (msg.type === 'custom_ui') {
        // Tcl-registered menu items / toolbar buttons changed (e.g. a
        // create_toolbar_button typed in any client's console). Re-render.
        applyCustomUi(msg);
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

        // Fetch any Tcl-registered custom menu items / toolbar buttons so
        // they survive a page reload and appear for late-connecting clients.
        app.websocketManager.request({ type: 'custom_ui' })
            .then(applyCustomUi)
            .catch(() => {});

        // --- Set Bounds ---
        const designBounds = boundsData.bounds;

        // No design loaded — skip map setup, let user open a DB via menu.
        const hasDesign = applyBounds(designBounds);
        if (hasDesign) {
            // Load any server-side text labels (2.12) now that applyBounds
            // has set the coordinate transform, so their handles can be
            // placed.
            if (app.labelManager) app.labelManager.reload();

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

            // Restore the pre-reload camera saved by applyDisplayState so
            // restore_display_controls doesn't move the view.
            try {
                const raw = sessionStorage.getItem('or_restore_view');
                if (raw) {
                    sessionStorage.removeItem('or_restore_view');
                    const v = JSON.parse(raw);
                    if (v && isFinite(v.lat) && isFinite(v.lng)
                        && isFinite(v.zoom)) {
                        app.map.setView([v.lat, v.lng], v.zoom);
                    }
                }
            } catch (_) { /* ignore */ }
        }

        // Click-to-select: convert click position to DBU and query server
        if (staticCache) {
            // Hide loading overlay — shapes are always ready in static mode.
            document.getElementById('loading-overlay').style.display = 'none';
        }
        // Shared select-at-point logic used by left-click and right-click.
        // Returns the server response (or null).  `focusInspector` switches the
        // panel to the Inspector (left-click); right-click keeps the current
        // panel and only updates the selection so the context menu can reflect
        // the object under the cursor.
        function selectAtLatLng(latlng, opts = {}) {
            const { addToSelection = false, focusInspector = true,
                    context = false, showConnectivity = false } = opts;
            if (!app.designScale) return Promise.resolve(null);
            const { dbuX: dbu_x, dbuY: dbu_y } = latLngToDbu(
                latlng.lat, latlng.lng, app.designScale, app.designMaxDXDY,
                app.designOriginX, app.designOriginY);

            // In label-placement mode, a click creates a text annotation
            // instead of selecting (2.12).
            if (app.labelManager && app.labelManager.isActive()) {
                app.labelManager.handleMapClick(dbu_x, dbu_y);
                return;
            }

            const vf = buildVisibilityFlags(visibility, selectability);
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
            if (addToSelection) {
                selectRequest.add_to_selection = true;
            }
            if (context) {
                selectRequest.context = true;
            }
            // Ctrl+click: Qt parity (selectHighlightConnectedNets) — also
            // pull the clicked instance's SIGNAL nets into the selection.
            if (showConnectivity) {
                selectRequest.show_connectivity = true;
            }
            if (app.visibleChiplets instanceof Set) {
                selectRequest.visible_chiplets = [...app.visibleChiplets];
            }
            const token = beginSelection(app);
            return app.websocketManager.request(selectRequest)
                .then(data => {
                    // A newer selection (another click, a layer row, the
                    // Inspector) has already replaced this one on the server;
                    // this response describes a selection that no longer
                    // exists, so it must not drive the Inspector.
                    if (!isCurrentSelection(app, token)) return;
                    console.log('Select response:', data, 'at dbu', dbu_x, dbu_y);
                    app.map.closePopup();
                    // Type flags so the context menu can enable items by type.
                    applySelectionFlags(app, data);
                    if (data.selected && data.selected.length > 0) {
                        const inst = data.selected[0];
                        if (inst.type === 'Inst') {
                            app.selectedInstanceName = inst.name;
                            if (app.schematicWidget) {
                                app.schematicWidget.refresh();
                            }
                        }
                        updateInspector(data);
                        if (focusInspector) focusComponent('Inspector');
                        // Outline the object the Inspector is showing, using
                        // ITS bbox — `data.bbox` — not `selected[0].bbox`.
                        // For a net the two are different rects: selected[]
                        // carries the hit-test bbox (dbNet::getTermBBox,
                        // terminals only) while data.bbox is the descriptor's
                        // full extent (wire ∪ terminals).  On a power net whose
                        // straps span the design they differ by ~2x, so the box
                        // landed nowhere near the net the Inspector was
                        // describing.  selected[0] is also the wrong OBJECT
                        // whenever the server cycled `pick` through overlapping
                        // hits.  Instances are unaffected either way: their
                        // hit-test bbox IS the descriptor bbox.
                        if (data.bbox) {
                            highlightBBox(data.bbox[0], data.bbox[1],
                                          data.bbox[2], data.bbox[3], data.type);
                            pulseHighlight(data.bbox, data.type);
                            // Remember bounds for "Zoom to Selection".
                            app.lastSelectionBounds = dbuRectToBounds(
                                data.bbox[0], data.bbox[1],
                                data.bbox[2], data.bbox[3],
                                app.designScale, app.designMaxDXDY,
                                app.designOriginX, app.designOriginY);
                        }
                        if (selectRequest.show_connectivity
                            && data.connected_added > 0) {
                            showToast(`Selected ${data.connected_added} `
                                      + 'connected net'
                                      + (data.connected_added > 1 ? 's' : ''));
                        }
                    } else if (!addToSelection && !context) {
                        // Empty left-click: clear inspector/highlight.  An empty
                        // right-click (context) keeps the current selection.
                        updateInspector(null);
                        if (app.highlightRect) {
                            app.map.removeLayer(app.highlightRect);
                            app.highlightRect = null;
                        }
                        app.lastSelectionBounds = null;
                    }
                    refreshOverlay();
                    return data;
                })
                .catch(err => {
                    console.error('Select failed:', err);
                    return null;
                });
        }
        app.selectAtLatLng = selectAtLatLng;

        if (!staticCache) app.map.on('click', (e) => {
            if (app.rulerManager && app.rulerManager.isActive()) return;
            const ev = e.originalEvent;
            selectAtLatLng(e.latlng, {
                addToSelection: !!(ev && ev.shiftKey),
                // Accept Cmd+click too: on macOS Ctrl+click is the
                // context-menu gesture, so metaKey is the reachable modifier.
                showConnectivity: !!(ev && (ev.ctrlKey || ev.metaKey)),
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

                const rect = container.getBoundingClientRect();

                if (wasShowing) {
                    // Drag — rubber-band zoom.
                    const p1 = app.map.containerPointToLatLng([
                        start.x - rect.left, start.y - rect.top]);
                    const p2 = app.map.containerPointToLatLng([
                        e.clientX - rect.left, e.clientY - rect.top]);
                    app.map.fitBounds([
                        [Math.min(p1.lat, p2.lat), Math.min(p1.lng, p2.lng)],
                        [Math.max(p1.lat, p2.lat), Math.max(p1.lng, p2.lng)],
                    ]);
                    return;
                }

                // Pure click — select the object under the cursor, then show
                // the context menu contextualized on it (Qt-like).  Only over
                // the map and not while a ruler is being placed.
                const overMap = e.clientX >= rect.left && e.clientX <= rect.right
                             && e.clientY >= rect.top  && e.clientY <= rect.bottom;
                if (!overMap) return;
                if (app.rulerManager && app.rulerManager.isActive()) return;

                const latlng = app.map.containerPointToLatLng([
                    e.clientX - rect.left, e.clientY - rect.top]);
                app.selectAtLatLng(latlng, { context: true, focusInspector: false })
                    .then(() => app.contextMenu.show({ originalEvent: e }));
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

        // Seed the server's display-state cache with the cookie-restored
        // state, so save_display_controls works before any interaction
        // (otherwise it would warn or write a previous session's state).
        scheduleSyncDisplayState();
    } catch (err) {
        console.error('Failed to load initial data from server:', err);
    }
});

// ─── Timing cone → schematic sync ───────────────────────────────────────────
// Single global listener (registered once here, not per SchematicWidget) that
// forwards the timing widget's cone-sync event to the current schematic widget.
document.addEventListener('openroad-cone-sync', (e) => {
    app.schematicWidget?.syncCone(e.detail || {});
});

// ─── Keyboard Shortcuts ─────────────────────────────────────────────────────

document.addEventListener('keydown', (e) => {
    // Ignore shortcuts when typing in an input field
    const tag = e.target.tagName;
    if (tag === 'INPUT' || tag === 'TEXTAREA' || e.target.isContentEditable) return;

    const key = e.key.toLowerCase();
    if (key === 'escape' && app.rulerManager && app.rulerManager.isActive()) {
        app.rulerManager.cancelRulerBuild();
    } else if (key === 'escape' && app.labelManager
               && app.labelManager.isActive()) {
        app.labelManager.cancelLabelMode();
    } else if (key === 'k' && !e.shiftKey && !e.ctrlKey && !e.metaKey) {
        if (app.rulerManager) app.rulerManager.toggleRulerMode();
    } else if (key === 'k' && e.shiftKey && !e.ctrlKey && !e.metaKey) {
        if (app.rulerManager) app.rulerManager.clearAllRulers();
    } else if (key === 'l' && !e.shiftKey && !e.ctrlKey && !e.metaKey) {
        if (app.labelManager) app.labelManager.toggleLabelMode();
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
    } else if (key === 'f' && (e.ctrlKey || e.metaKey)) {
        // Both dialog shortcuts must preventDefault.  The dialog focuses and
        // select()s its first field synchronously, so this keystroke's own
        // default action would then be delivered to that field and replace
        // the prefilled value with the shortcut's own letter.  Ctrl/Cmd+F
        // additionally has the browser's find bar to suppress.
        e.preventDefault();
        if (app.designScale) showFindDialog(app);
    } else if (key === 'g' && e.shiftKey && !e.ctrlKey && !e.metaKey) {
        e.preventDefault();  // see above
        if (app.designScale) showGotoDialog(app);
    } else if (key === 't' && !e.shiftKey && !e.ctrlKey && !e.metaKey) {
        app.toggleTheme();
    }
}, true);
