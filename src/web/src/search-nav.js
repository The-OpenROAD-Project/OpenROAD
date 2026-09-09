// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Search & navigation dialogs (issue #10619 table 2.8):
//  - Find: name/glob search by object type (mirrors Qt FindObjectDialog).
//  - Go to position: jump the view to x,y in microns (mirrors Qt GotoDialog).
// Both reuse the .modal-overlay/.modal-dialog pattern from menu-bar.js.

import { dbuToLatLng, dbuRectToBounds, latLngToDbu } from './coordinates.js';
import { applySelectionFlags } from './ui-utils.js';

// Build a modal shell and return handles.  Closes on backdrop click / Escape.
function buildModal(title, bodyHtml, okLabel) {
    const overlay = document.createElement('div');
    overlay.className = 'modal-overlay';
    overlay.innerHTML = `
        <div class="modal-dialog search-nav-dialog">
            <button class="modal-close" title="Close" aria-label="Close">&times;</button>
            <h3>${title}</h3>
            ${bodyHtml}
            <div class="modal-error" style="display:none"></div>
            <div class="modal-buttons">
                <button class="or-btn cancel">Cancel</button>
                <button class="or-btn or-btn-primary ok">${okLabel}</button>
            </div>
        </div>`;
    document.body.appendChild(overlay);

    const close = () => overlay.remove();
    overlay.addEventListener('click', (e) => { if (e.target === overlay) close(); });
    overlay.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
    overlay.querySelector('.cancel').addEventListener('click', close);
    overlay.querySelector('.modal-close').addEventListener('click', close);

    const errorDiv = overlay.querySelector('.modal-error');
    const showError = (msg, isInfo) => {
        errorDiv.textContent = msg;
        errorDiv.style.display = msg ? 'block' : 'none';
        errorDiv.classList.toggle('info', !!isInfo);
    };
    return { overlay, close, okBtn: overlay.querySelector('.ok'), showError };
}

// ── Find ──────────────────────────────────────────────────────────────────

const FIND_TYPES = [
    { label: 'Instance', value: 'inst' },
    { label: 'Net', value: 'net' },
    { label: 'Port', value: 'port' },
];

export function showFindDialog(app) {
    if (!app.designScale) {
        return;
    }
    const typeOptions = FIND_TYPES
        .map((t) => `<option value="${t.value}">${t.label}</option>`)
        .join('');
    const { overlay, close, okBtn, showError } = buildModal('Find Object', `
        <div class="sn-row">
            <label>Name</label>
            <input type="text" class="fb-path-input sn-pattern"
                   placeholder="name or glob (*, ?, [])">
        </div>
        <div class="sn-row">
            <label>Type</label>
            <select class="sn-type">${typeOptions}</select>
        </div>
        <label class="sn-check">
            <input type="checkbox" class="sn-case"> Match case
        </label>`, 'Find');

    const patternInput = overlay.querySelector('.sn-pattern');
    const typeSel = overlay.querySelector('.sn-type');
    const caseChk = overlay.querySelector('.sn-case');
    patternInput.focus();

    function doFind() {
        const pattern = patternInput.value.trim();
        if (!pattern) {
            showError('Enter a name or glob pattern.');
            return;
        }
        if (!app.websocketManager) {
            return;
        }
        showError('Searching…', true);
        app.websocketManager
            .request({
                type: 'find',
                obj_type: typeSel.value,
                pattern,
                match_case: caseChk.checked,
            })
            .then((resp) => {
                applySelectionFlags(app, resp);
                const count = resp && resp.count ? resp.count : 0;
                if (!count) {
                    showError('No objects found.');
                    return;
                }
                // Auto-zoom to the matched objects — an improvement over the Qt
                // Find dialog, which only selects without centering.
                if (Array.isArray(resp.bbox)) {
                    app.lastSelectionBounds = dbuRectToBounds(
                        resp.bbox[0], resp.bbox[1], resp.bbox[2], resp.bbox[3],
                        app.designScale, app.designMaxDXDY,
                        app.designOriginX, app.designOriginY);
                    app.map.fitBounds(app.lastSelectionBounds);
                }
                if (typeof app.updateInspector === 'function') {
                    app.updateInspector(resp);
                }
                if (typeof app.redrawAllLayers === 'function') {
                    app.redrawAllLayers();
                }
                const shown = resp.selection_count || count;
                showError(resp.truncated
                    ? `${count} found (showing first ${shown}).`
                    : `${count} found.`, true);
            })
            .catch((err) => showError('Find failed: ' + err));
    }

    okBtn.addEventListener('click', doFind);
    patternInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') { e.preventDefault(); doFind(); }
    });
    return { close };
}

// ── Go to position ──────────────────────────────────────────────────────────

export function showGotoDialog(app) {
    if (!app.designScale || !app.map) {
        return;
    }
    // Every field is read and written in whichever unit the "Show DBU"
    // setting (Qt's MainWindow::useDBU) is displaying, so the numbers here
    // match the ones the Inspector, the rulers and the scale bar are showing.
    const fmt = (dbu) => app.formatDbu(dbu);
    const parse = (str) => app.parseDbu(str);
    const unit = app.unitLabel();

    // Prefill X/Y with the current view center, so "Go to" with no edits is
    // a no-op.
    const c = app.map.getCenter();
    const centerDbu = latLngToDbu(c.lat, c.lng, app.designScale,
        app.designMaxDXDY, app.designOriginX, app.designOriginY);

    const { overlay, close, okBtn, showError } = buildModal('Go to Position', `
        <div class="sn-row"><label>X (${unit})</label>
            <input type="text" class="fb-path-input sn-x" value="${fmt(centerDbu.dbuX)}"></div>
        <div class="sn-row"><label>Y (${unit})</label>
            <input type="text" class="fb-path-input sn-y" value="${fmt(centerDbu.dbuY)}"></div>
        <div class="sn-row"><label>Size (${unit})</label>
            <input type="text" class="fb-path-input sn-size" placeholder="optional zoom window"></div>`,
        'Go to');

    const xIn = overlay.querySelector('.sn-x');
    const yIn = overlay.querySelector('.sn-y');
    const sizeIn = overlay.querySelector('.sn-size');
    xIn.focus();
    xIn.select();

    function doGoto() {
        const dbuX = parse(xIn.value);
        const dbuY = parse(yIn.value);
        if (dbuX === null || dbuY === null) {
            showError(`X and Y must be numbers (${unit}).`);
            return;
        }
        const latlng = dbuToLatLng(dbuX, dbuY, app.designScale,
            app.designMaxDXDY, app.designOriginX, app.designOriginY);

        // A size of 0 or less is not a window; fall through to the recentre.
        const sizeVal = sizeIn.value.trim();
        const sizeDbu = sizeVal ? parse(sizeVal) : null;
        if (sizeDbu !== null && sizeDbu > 0) {
            const half = sizeDbu / 2;
            app.map.fitBounds(dbuRectToBounds(
                dbuX - half, dbuY - half, dbuX + half, dbuY + half,
                app.designScale, app.designMaxDXDY,
                app.designOriginX, app.designOriginY));
        } else {
            app.map.setView(latlng, app.map.getZoom());
        }
        close();
    }

    okBtn.addEventListener('click', doGoto);
    for (const el of [xIn, yIn, sizeIn]) {
        el.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') { e.preventDefault(); doGoto(); }
        });
    }
    return { close };
}
