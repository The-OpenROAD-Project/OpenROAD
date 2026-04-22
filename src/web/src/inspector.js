// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Inspector panel — property tree, hover highlights, bbox display.

import { dbuToLatLng, dbuRectToBounds } from './coordinates.js';

// SVG icons — distinct shapes so they're easy to tell apart at a glance.
// Zoom to: magnifying glass with "+" (Material "zoom_in")
const ZOOM_TO_SVG =
    '<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/>' +
    '<path d="M12 10h-2v2H9v-2H7V9h2V7h1v2h2v1z"/>' +
    '</svg>';

// Back: left arrow (Material "arrow_back")
const BACK_SVG =
    '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z"/>' +
    '</svg>';

// Focus net: eye open (Material "visibility")
const FOCUS_SVG =
    '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>' +
    '</svg>';

// De-focus net: eye off (Material "visibility_off")
const DEFOCUS_SVG =
    '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46A11.8 11.8 0 0 0 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z"/>' +
    '</svg>';

// Show route guides: Material "alt_route"
const ROUTE_GUIDE_SVG =
    '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M9.78 11.16l-1.42 1.42a7.282 7.282 0 0 1-1.79-2.94l1.94-.49c.32.89.77 1.5 1.27 2.01zM11 6L7 2 3 6h3.02c.02.81.08 1.54.19 2.17l1.94-.49C8.08 7.2 8.03 6.63 8.02 6H11zm2 0h3c-.01.22-.03.47-.07.73l1.94.49c.09-.42.15-.87.17-1.22H21l-4-4-4 4zm1.56 8.42a7.282 7.282 0 0 1-1.79 2.94l1.42 1.42c.78-.87 1.4-1.85 1.79-2.88l-1.42-1.48zM19.98 18h-2.95a8.42 8.42 0 0 0 .34-1.54l-1.94-.49c-.11.63-.29 1.24-.54 1.82-.25.55-.56 1.05-.91 1.5L12.56 17.9c.46-.38.84-.82 1.15-1.29.15-.22.28-.46.39-.71H11v2h2.02c-.01.63-.07 1.23-.21 1.77l1.94.49C14.93 19.42 15.01 18.72 15.02 18H19l-4-4 .98.98z"/>' +
    '</svg>';

// Hide route guides: Material "alt_route" with strikethrough
const HIDE_ROUTE_GUIDE_SVG =
    '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M9.78 11.16l-1.42 1.42a7.282 7.282 0 0 1-1.79-2.94l1.94-.49c.32.89.77 1.5 1.27 2.01zM11 6L7 2 3 6h3.02c.02.81.08 1.54.19 2.17l1.94-.49C8.08 7.2 8.03 6.63 8.02 6H11zm2 0h3c-.01.22-.03.47-.07.73l1.94.49c.09-.42.15-.87.17-1.22H21l-4-4-4 4zm1.56 8.42a7.282 7.282 0 0 1-1.79 2.94l1.42 1.42c.78-.87 1.4-1.85 1.79-2.88l-1.42-1.48zM19.98 18h-2.95a8.42 8.42 0 0 0 .34-1.54l-1.94-.49c-.11.63-.29 1.24-.54 1.82-.25.55-.56 1.05-.91 1.5L12.56 17.9c.46-.38.84-.82 1.15-1.29.15-.22.28-.46.39-.71H11v2h2.02c-.01.63-.07 1.23-.21 1.77l1.94.49C14.93 19.42 15.01 18.72 15.02 18H19l-4-4 .98.98z" opacity="0.5"/>' +
    '<line x1="4" y1="20" x2="20" y2="4" stroke="currentColor" stroke-width="2"/>' +
    '</svg>';

// Clear focus: X (Material "close")
const CLEAR_FOCUS_SVG =
    '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/>' +
    '</svg>';

export function createInspectorPanel(app, redrawAllLayers) {
    let lastInspectData = null;
    let pendingInspectId = null;
    const kMinHoverBoxPixels = 10;

    function showLoading() {
        if (!app.inspectorEl) return;
        app.inspectorEl.innerHTML = '';
        const loading = document.createElement('div');
        loading.className = 'stub-panel';
        loading.innerHTML =
            '<div class="stub-title">Loading\u2026</div>' +
            '<div class="stub-desc">Fetching properties from server.</div>';
        app.inspectorEl.appendChild(loading);
    }

    async function toggleFocusNet(name) {
        const action = app.focusNets.has(name) ? 'remove' : 'add';
        if (action === 'add') app.focusNets.add(name);
        else app.focusNets.delete(name);
        try {
            await app.websocketManager.request({
                type: 'set_focus_nets', action, net_name: name,
            });
        } catch (err) {
            console.error('set_focus_nets failed:', err);
        }
        redrawAllLayers();
        if (lastInspectData) updateInspector(lastInspectData);
    }

    async function clearFocusNets() {
        app.focusNets.clear();
        try {
            await app.websocketManager.request({
                type: 'set_focus_nets', action: 'clear', net_name: '',
            });
        } catch (err) {
            console.error('set_focus_nets clear failed:', err);
        }
        redrawAllLayers();
        if (lastInspectData) updateInspector(lastInspectData);
    }

    async function toggleRouteGuides(name) {
        const action = app.routeGuideNets.has(name) ? 'remove' : 'add';
        if (action === 'add') app.routeGuideNets.add(name);
        else app.routeGuideNets.delete(name);
        try {
            await app.websocketManager.request({
                type: 'set_route_guides', action, net_name: name,
            });
        } catch (err) {
            console.error('set_route_guides failed:', err);
        }
        redrawAllLayers();
        if (lastInspectData) updateInspector(lastInspectData);
    }

    function clearClientHoverHighlight() {
        if (app.hoverHighlightLayer) {
            app.map.removeLayer(app.hoverHighlightLayer);
            app.hoverHighlightLayer = null;
        }
    }

    function clearHoverHighlight() {
        clearClientHoverHighlight();
        app.websocketManager.request({ type: 'hover', select_id: -1 })
            .then(() => {})
            .catch(() => {});
    }

    function boundsWithMinimumScreenSize(x1, y1, x2, y2) {
        const baseBounds = L.latLngBounds(
            dbuRectToBounds(x1, y1, x2, y2, app.designScale, app.designMaxDXDY, app.designOriginX, app.designOriginY)
        );
        if (!app.map) {
            return baseBounds;
        }

        const southWest = baseBounds.getSouthWest();
        const northEast = baseBounds.getNorthEast();
        const swPoint = app.map.latLngToContainerPoint(southWest);
        const nePoint = app.map.latLngToContainerPoint(northEast);

        const minX = Math.min(swPoint.x, nePoint.x);
        const maxX = Math.max(swPoint.x, nePoint.x);
        const minY = Math.min(swPoint.y, nePoint.y);
        const maxY = Math.max(swPoint.y, nePoint.y);

        let expandedMinX = minX;
        let expandedMaxX = maxX;
        let expandedMinY = minY;
        let expandedMaxY = maxY;

        if ((maxX - minX) < kMinHoverBoxPixels) {
            const pad = (kMinHoverBoxPixels - (maxX - minX)) / 2;
            expandedMinX -= pad;
            expandedMaxX += pad;
        }
        if ((maxY - minY) < kMinHoverBoxPixels) {
            const pad = (kMinHoverBoxPixels - (maxY - minY)) / 2;
            expandedMinY -= pad;
            expandedMaxY += pad;
        }

        const topLeft = app.map.containerPointToLatLng(L.point(expandedMinX, expandedMinY));
        const bottomRight = app.map.containerPointToLatLng(L.point(expandedMaxX, expandedMaxY));
        return L.latLngBounds(topLeft, bottomRight);
    }

    function renderHoverRects(rects) {
        clearClientHoverHighlight();
        if (!app.map || !app.designScale || !rects || rects.length === 0) {
            return;
        }

        app.hoverHighlightLayer = L.layerGroup(
            rects.map(([x1, y1, x2, y2]) => L.rectangle(boundsWithMinimumScreenSize(x1, y1, x2, y2), {
                color: '#ffe85c',
                weight: 3,
                fill: true,
                fillColor: '#fff27a',
                fillOpacity: 0.14,
                opacity: 1,
                interactive: false,
                className: 'hover-highlight',
                pane: app.hoverHighlightPane,
            }))
        ).addTo(app.map);
    }

    function makeClickable(el, selectId) {
        el.classList.add('inspector-link');
        el.addEventListener('click', (e) => {
            e.stopPropagation();
            navigateInspector(selectId);
        });
        el.addEventListener('mouseenter', () => {
            app.websocketManager.request({ type: 'hover', select_id: selectId })
                .then(data => {
                    renderHoverRects(data.rects || []);
                    redrawAllLayers();
                })
                .catch(() => {});
        });
        el.addEventListener('mouseleave', () => {
            clearHoverHighlight();
            redrawAllLayers();
        });
    }

    function navigateInspector(selectId) {
        // Cancel previous in-flight inspect request
        if (pendingInspectId !== null) {
            app.websocketManager.cancel(pendingInspectId);
            pendingInspectId = null;
        }

        // Show loading state immediately
        showLoading();

        const promise = app.websocketManager.request(
            { type: 'inspect', select_id: selectId });
        pendingInspectId = promise.requestId;

        promise
            .then(data => {
                pendingInspectId = null;
                if (data.error) {
                    console.error('Inspect error:', data.error);
                    return;
                }
                updateInspector(data);

                app.map.closePopup();
                clearClientHoverHighlight();
                if (app.highlightRect) {
                    app.map.removeLayer(app.highlightRect);
                    app.highlightRect = null;
                }
                if (data.bbox && app.map && app.designScale) {
                    const [x1, y1, x2, y2] = data.bbox;
                    // For non-instance objects, show dashed bbox outline
                    // (instances get the yellow tile-based highlight instead)
                    if (data.type !== 'Inst') {
                        highlightBBox(x1, y1, x2, y2);
                    }
                }
                // Redraw tiles to update instance highlight
                redrawAllLayers();
            })
            .catch(err => {
                pendingInspectId = null;
                console.error('Inspect failed:', err);
            });
    }

    function navigateBack() {
        if (pendingInspectId !== null) {
            app.websocketManager.cancel(pendingInspectId);
            pendingInspectId = null;
        }

        showLoading();

        const promise = app.websocketManager.request({ type: 'inspect_back' });
        pendingInspectId = promise.requestId;

        promise
            .then(data => {
                pendingInspectId = null;
                if (data.error) {
                    console.error('Inspect back error:', data.error);
                    return;
                }
                updateInspector(data);

                app.map.closePopup();
                clearClientHoverHighlight();
                if (app.highlightRect) {
                    app.map.removeLayer(app.highlightRect);
                    app.highlightRect = null;
                }
                if (data.bbox && app.map && app.designScale && data.type !== 'Inst') {
                    const [x1, y1, x2, y2] = data.bbox;
                    highlightBBox(x1, y1, x2, y2);
                }
                redrawAllLayers();
            })
            .catch(err => {
                pendingInspectId = null;
                console.error('Inspect back failed:', err);
            });
    }

    function highlightBBox(x1, y1, x2, y2) {
        if (app.highlightRect) {
            app.map.removeLayer(app.highlightRect);
        }
        const bounds = dbuRectToBounds(x1, y1, x2, y2, app.designScale, app.designMaxDXDY, app.designOriginX, app.designOriginY);
        app.highlightRect = L.rectangle(bounds, {
            color: '#ff0', weight: 2, fill: false, dashArray: '6,4',
        }).addTo(app.map);
    }

    function renderProperty(prop) {
        // Group with children (PropertyList or SelectionSet)
        if (prop.children) {
            const group = document.createElement('div');
            group.className = 'inspector-group';

            const header = document.createElement('div');
            header.className = 'inspector-group-header';
            const arrow = document.createElement('span');
            arrow.className = 'vis-arrow';
            arrow.textContent = '▶';
            const nameSpan = document.createElement('span');
            nameSpan.className = 'inspector-prop-name';
            nameSpan.textContent = prop.name;
            const countSpan = document.createElement('span');
            countSpan.className = 'inspector-count';
            countSpan.textContent = `(${prop.children.length})`;
            header.appendChild(arrow);
            header.appendChild(nameSpan);
            header.appendChild(countSpan);
            group.appendChild(header);

            const kids = document.createElement('div');
            const autoExpand = prop.children.length < 10;
            kids.className = 'inspector-group-children' + (autoExpand ? '' : ' collapsed');
            arrow.textContent = autoExpand ? '▼' : '▶';
            for (const child of prop.children) {
                kids.appendChild(renderProperty(child));
            }
            group.appendChild(kids);

            arrow.addEventListener('click', () => {
                kids.classList.toggle('collapsed');
                arrow.textContent = kids.classList.contains('collapsed')
                    ? '▶' : '▼';
            });
            header.addEventListener('click', () => {
                kids.classList.toggle('collapsed');
                arrow.textContent = kids.classList.contains('collapsed')
                    ? '▶' : '▼';
            });

            return group;
        }

        // Leaf property: name + value
        const row = document.createElement('div');
        row.className = 'inspector-prop';
        const nameEl = document.createElement('span');
        nameEl.className = 'inspector-prop-name';
        nameEl.textContent = prop.name || '';
        const valEl = document.createElement('span');
        valEl.className = 'inspector-prop-value';
        valEl.textContent = prop.value || '';
        row.appendChild(nameEl);
        row.appendChild(valEl);

        // For single-target rows like SelectionSet entries, make the whole row
        // interactive so hover is easy to hit.
        if (prop.name_select_id !== undefined && prop.value_select_id === undefined) {
            makeClickable(row, prop.name_select_id);
            nameEl.classList.add('inspector-link');
        } else if (prop.name_select_id !== undefined) {
            makeClickable(nameEl, prop.name_select_id);
        }
        if (prop.value_select_id !== undefined) {
            makeClickable(valEl, prop.value_select_id);
        }

        return row;
    }

    function zoomToBBox(bbox) {
        if (!bbox || !app.map || !app.designScale) return;
        const [x1, y1, x2, y2] = bbox;
        app.map.fitBounds(dbuRectToBounds(x1, y1, x2, y2, app.designScale, app.designMaxDXDY, app.designOriginX, app.designOriginY),
                          { padding: [20, 20] });
    }

    function updateInspector(data) {
        if (!app.inspectorEl) return;
        lastInspectData = data;
        app.inspectorEl.innerHTML = '';

        if (!data || !data.properties || data.properties.length === 0) {
            const placeholder = document.createElement('div');
            placeholder.className = 'stub-panel';
            placeholder.innerHTML =
                '<div class="stub-title">Inspector</div>' +
                '<div class="stub-desc">Select an object in the layout to inspect its properties.</div>';
            app.inspectorEl.appendChild(placeholder);
            return;
        }

        // Toolbar with action buttons
        if (data) {
            const toolbar = document.createElement('div');
            toolbar.className = 'inspector-toolbar';

            const backBtn = document.createElement('button');
            backBtn.className = 'inspector-btn';
            backBtn.title = 'Back';
            backBtn.innerHTML = BACK_SVG;
            backBtn.disabled = !data.can_navigate_back;
            backBtn.addEventListener('click', () => navigateBack());
            toolbar.appendChild(backBtn);

            if (data.bbox) {
                const zoomBtn = document.createElement('button');
                zoomBtn.className = 'inspector-btn';
                zoomBtn.title = 'Zoom to';
                zoomBtn.innerHTML = ZOOM_TO_SVG;
                zoomBtn.addEventListener('click', () => zoomToBBox(data.bbox));
                toolbar.appendChild(zoomBtn);
            }

            // Focus/De-focus button for Net objects
            if (data.type === 'Net' && data.name) {
                const isFocused = app.focusNets.has(data.name);
                const focusBtn = document.createElement('button');
                focusBtn.className = 'inspector-btn';
                focusBtn.title = isFocused ? 'De-focus net' : 'Focus net';
                focusBtn.innerHTML = isFocused ? DEFOCUS_SVG : FOCUS_SVG;
                focusBtn.addEventListener('click', () => toggleFocusNet(data.name));
                toolbar.appendChild(focusBtn);
            }

            // Show/Hide route guides button for Net objects with guides
            if (data.type === 'Net' && data.name && data.has_guides) {
                const isShowing = app.routeGuideNets.has(data.name);
                const guideBtn = document.createElement('button');
                guideBtn.className = 'inspector-btn';
                guideBtn.title = isShowing ? 'Hide route guides' : 'Show route guides';
                guideBtn.innerHTML = isShowing ? HIDE_ROUTE_GUIDE_SVG : ROUTE_GUIDE_SVG;
                guideBtn.addEventListener('click', () => toggleRouteGuides(data.name));
                toolbar.appendChild(guideBtn);
            }

            // Clear all focus nets button
            if (app.focusNets.size > 0) {
                const clearBtn = document.createElement('button');
                clearBtn.className = 'inspector-btn';
                clearBtn.title = 'Clear focus nets';
                clearBtn.innerHTML = CLEAR_FOCUS_SVG;
                clearBtn.addEventListener('click', () => clearFocusNets());
                toolbar.appendChild(clearBtn);
            }

            app.inspectorEl.appendChild(toolbar);
        }

        for (const prop of data.properties) {
            app.inspectorEl.appendChild(renderProperty(prop));
        }
    }

    function createInspector(container) {
        const el = document.createElement('div');
        el.className = 'inspector';
        app.inspectorEl = el;
        container.element.appendChild(el);

        // Show placeholder initially
        updateInspector(null);
    }

    return { createInspector, updateInspector, highlightBBox };
}
