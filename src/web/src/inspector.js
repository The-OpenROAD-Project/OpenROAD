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

// Clear focus: X (Material "close")
const CLEAR_FOCUS_SVG =
    '<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">' +
    '<path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/>' +
    '</svg>';

export function createInspectorPanel(app, redrawAllLayers) {
    let lastInspectData = null;

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

    function clearHoverHighlight() {
        for (const r of app.hoverRects) {
            app.map.removeLayer(r);
        }
        app.hoverRects = [];
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
                    if (data.rects && app.map && app.designScale) {
                        clearHoverHighlight();
                        const minPx = 20;
                        const zoom = app.map.getZoom();
                        const scale = Math.pow(2, zoom);
                        const minDbu = minPx / (app.designScale * scale);
                        for (const [x1, y1, x2, y2] of data.rects) {
                            const cx = (x1 + x2) / 2;
                            const cy = (y1 + y2) / 2;
                            const hw = Math.max((x2 - x1) / 2, minDbu / 2);
                            const hh = Math.max((y2 - y1) / 2, minDbu / 2);
                            const bounds = dbuRectToBounds(
                                cx - hw, cy - hh, cx + hw, cy + hh,
                                app.designScale, app.designMaxDXDY);
                            app.hoverRects.push(L.rectangle(bounds, {
                                className: 'hover-highlight',
                                color: '#ff0', weight: 3, fill: true,
                                fillColor: '#ff0', fillOpacity: 0.2,
                            }).addTo(app.map));
                        }
                    }
                })
                .catch(() => {});
        });
        el.addEventListener('mouseleave', () => {
            clearHoverHighlight();
        });
    }

    function navigateInspector(selectId) {
        app.websocketManager.request({ type: 'inspect', select_id: selectId })
            .then(data => {
                if (data.error) {
                    console.error('Inspect error:', data.error);
                    return;
                }
                updateInspector(data);

                // Clear hover highlight from the link we just clicked
                clearHoverHighlight();

                // Show popup and highlight on the map
                app.map.closePopup();
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
                    // Show a popup at the center of the bbox
                    const center = dbuToLatLng((x1 + x2) / 2, (y1 + y2) / 2,
                                               app.designScale, app.designMaxDXDY);
                    L.popup()
                        .setLatLng(center)
                        .setContent(
                            `<strong>${data.name}</strong><br>` +
                            `<small style="color:#888">${data.type}</small>`)
                        .openOn(app.map);
                }
                // Redraw tiles to update instance highlight
                redrawAllLayers();
            })
            .catch(err => console.error('Inspect failed:', err));
    }

    function highlightBBox(x1, y1, x2, y2) {
        if (app.highlightRect) {
            app.map.removeLayer(app.highlightRect);
        }
        const bounds = dbuRectToBounds(x1, y1, x2, y2, app.designScale, app.designMaxDXDY);
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

        // Make name/value clickable if they have a select_id
        if (prop.name_select_id !== undefined) {
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
        app.map.fitBounds(dbuRectToBounds(x1, y1, x2, y2, app.designScale, app.designMaxDXDY),
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
        if (data.bbox) {
            const toolbar = document.createElement('div');
            toolbar.className = 'inspector-toolbar';
            const zoomBtn = document.createElement('button');
            zoomBtn.className = 'inspector-btn';
            zoomBtn.title = 'Zoom to';
            zoomBtn.innerHTML = ZOOM_TO_SVG;
            zoomBtn.addEventListener('click', () => zoomToBBox(data.bbox));
            toolbar.appendChild(zoomBtn);

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
